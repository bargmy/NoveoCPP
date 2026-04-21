#include "MessageItemWidget.h"

#include <QApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMediaContent>
#include <QMediaPlayer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QPixmapCache>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QShowEvent>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QVideoWidget>
#include <functional>

namespace {
QString resolveAttachmentType(const FileAttachment& file, const QString& fallbackUrl)
{
    QString type = file.type.trimmed().toLower();
    if (!type.isEmpty()) {
        return type;
    }
    const QString target = file.name.isEmpty() ? fallbackUrl : file.name;
    const QString lower = target.toLower();
    if (lower.endsWith(".png") || lower.endsWith(".jpg") || lower.endsWith(".jpeg") || lower.endsWith(".gif") || lower.endsWith(".webp")) {
        return QStringLiteral("image/*");
    }
    if (lower.endsWith(".mp4") || lower.endsWith(".webm") || lower.endsWith(".mov") || lower.endsWith(".mkv")) {
        return QStringLiteral("video/*");
    }
    if (lower.endsWith(".mp3") || lower.endsWith(".wav") || lower.endsWith(".ogg") || lower.endsWith(".m4a") || lower.endsWith(".aac")) {
        return QStringLiteral("audio/*");
    }
    return QStringLiteral("application/octet-stream");
}

QNetworkAccessManager* sharedNetworkManager()
{
    static QPointer<QNetworkAccessManager> manager;
    if (!manager) {
        manager = new QNetworkAccessManager(qApp);
    }
    return manager;
}
}

MessageItemWidget::MessageItemWidget(const Message& message,
                                     const QString& senderName,
                                     const QString& senderAvatarUrl,
                                     const QPixmap& senderAvatar,
                                     const QString& resolvedFileUrl,
                                     const QString& replySender,
                                     const QString& replyPreviewText,
                                     const QString& currentUserId,
                                     const QString& chatType,
                                     bool isChannelOwner,
                                     bool darkMode,
                                     QWidget* parent)
    : QWidget(parent),
      m_message(message),
      m_senderName(senderName),
      m_senderAvatarUrl(senderAvatarUrl),
      m_fileUrl(resolvedFileUrl),
      m_replySender(replySender),
      m_replyPreviewText(replyPreviewText),
      m_currentUserId(currentUserId),
      m_chatType(chatType),
      m_isChannelOwner(isChannelOwner),
      m_darkMode(darkMode)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setMouseTracking(true);

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(14, 3, 14, 3);
    root->setSpacing(0);

    const bool isMe = (m_message.senderId == m_currentUserId);
    if (isMe) {
        root->addStretch();
    } else {
        m_avatarLabel = new QLabel(this);
        m_avatarLabel->setFixedSize(34, 34);
        m_avatarLabel->setAlignment(Qt::AlignCenter);
        setSenderAvatar(senderAvatar);
        root->addWidget(m_avatarLabel, 0, Qt::AlignLeft | Qt::AlignBottom);
        root->addSpacing(8);
    }

    m_bubble = new QFrame(this);
    m_bubble->setObjectName(QStringLiteral("messageBubbleFrame"));
    m_bubble->setMouseTracking(true);
    m_bubble->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    m_bubble->setMaximumWidth(620);

    auto* bubbleLayout = new QVBoxLayout(m_bubble);
    bubbleLayout->setContentsMargins(12, 9, 12, 9);
    bubbleLayout->setSpacing(7);

    if (!isMe && m_chatType != QStringLiteral("private") && !m_senderName.isEmpty()) {
        m_senderLabel = new QLabel(m_senderName, m_bubble);
        bubbleLayout->addWidget(m_senderLabel);
    }

    if (!m_message.forwardedInfo.isNull()) {
        m_forwardedLabel = new QLabel(QStringLiteral("Forwarded message"), m_bubble);
        bubbleLayout->addWidget(m_forwardedLabel);
    }

    if (!m_message.replyToId.isEmpty()) {
        QString preview = m_replyPreviewText;
        if (preview.isEmpty()) {
            preview = QStringLiteral("[Original message]");
        }
        if (preview.size() > 70) {
            preview = preview.left(67) + QStringLiteral("...");
        }
        const QString sender = m_replySender.isEmpty() ? QStringLiteral("Unknown") : m_replySender;
        m_replyAnchorButton = new QPushButton(QStringLiteral("%1: %2").arg(sender, preview), m_bubble);
        m_replyAnchorButton->setFlat(true);
        m_replyAnchorButton->setCursor(Qt::PointingHandCursor);
        connect(m_replyAnchorButton, &QPushButton::clicked, this, [this]() {
            emit replyAnchorClicked(m_message.replyToId);
        });
        bubbleLayout->addWidget(m_replyAnchorButton);
    }

    if (!m_message.file.isNull() && !m_fileUrl.isEmpty()) {
        renderAttachmentContent(bubbleLayout);
    }

    renderTextContent(bubbleLayout);
    renderActionRow(bubbleLayout);

    m_metaLabel = new QLabel(m_bubble);
    m_metaLabel->setAlignment(Qt::AlignRight);
    m_metaLabel->setStyleSheet("font-size: 11px; color: #6b7280;");
    bubbleLayout->addWidget(m_metaLabel);
    rebuildMetaText();

    root->addWidget(m_bubble);
    if (!isMe) {
        root->addStretch();
    }

    applyBubbleStyle();
    QTimer::singleShot(0, this, [this]() { maybeLoadImagePreview(); });
}

QString MessageItemWidget::messageId() const
{
    return m_message.messageId;
}

void MessageItemWidget::setMessageStatus(MessageStatus status)
{
    m_message.status = status;
    rebuildMetaText();
}

void MessageItemWidget::setEditedAt(qint64 editedAt)
{
    m_message.editedAt = editedAt;
    rebuildMetaText();
}

void MessageItemWidget::setMessageText(const QString& text)
{
    m_message.text = text;
    if (m_textLabel) {
        m_textLabel->setText(text);
    }
}

void MessageItemWidget::setHighlighted(bool highlighted)
{
    m_isHighlighted = highlighted;
    applyBubbleStyle();
}

void MessageItemWidget::setTheme(bool darkMode)
{
    m_darkMode = darkMode;
    applyBubbleStyle();
}

bool MessageItemWidget::representsAudioUrl(const QString& url) const
{
    const QString clean = url.trimmed();
    if (clean.isEmpty() || m_fileUrl.isEmpty()) {
        return false;
    }
    return QUrl(clean).adjusted(QUrl::RemoveFragment | QUrl::RemoveQuery) ==
           QUrl(m_fileUrl).adjusted(QUrl::RemoveFragment | QUrl::RemoveQuery);
}

void MessageItemWidget::setAudioPlaying(bool playing)
{
    if (!m_audioButton) {
        return;
    }
    m_audioButton->setText(playing ? QStringLiteral("Pause") : QStringLiteral("Play"));
}

bool MessageItemWidget::representsSenderAvatarUrl(const QString& url) const
{
    if (m_senderAvatarUrl.isEmpty() || url.isEmpty()) {
        return false;
    }
    return QUrl(m_senderAvatarUrl).adjusted(QUrl::RemoveFragment | QUrl::RemoveQuery) ==
           QUrl(url).adjusted(QUrl::RemoveFragment | QUrl::RemoveQuery);
}

void MessageItemWidget::setSenderAvatar(const QPixmap& avatar)
{
    if (!m_avatarLabel) {
        return;
    }
    if (avatar.isNull()) {
        m_avatarLabel->clear();
        return;
    }
    m_avatarLabel->setPixmap(avatar.scaled(m_avatarLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

QString MessageItemWidget::inferFileType() const
{
    return resolveAttachmentType(m_message.file, m_fileUrl);
}

QString MessageItemWidget::fileDisplayName() const
{
    if (!m_message.file.name.isEmpty()) {
        return m_message.file.name;
    }
    const QUrl url(m_fileUrl);
    if (url.isValid()) {
        const QString path = url.path();
        if (!path.isEmpty()) {
            return QFileInfo(path).fileName();
        }
    }
    return QStringLiteral("Attachment");
}

QString MessageItemWidget::formatTimestamp() const
{
    QDateTime dt;
    dt.setSecsSinceEpoch(m_message.timestamp);
    return dt.toString("hh:mm AP");
}

QString MessageItemWidget::statusGlyph() const
{
    if (m_message.senderId != m_currentUserId) {
        return QString();
    }
    switch (m_message.status) {
    case MessageStatus::Pending:
        return QStringLiteral(" o");
    case MessageStatus::Sent:
        return QStringLiteral(" v");
    case MessageStatus::Seen:
        return QStringLiteral(" vv");
    }
    return QString();
}

void MessageItemWidget::rebuildMetaText()
{
    if (!m_metaLabel) {
        return;
    }
    QString text = formatTimestamp();
    if (m_message.editedAt > 0) {
        text += QStringLiteral(" edited");
    }
    text += statusGlyph();
    m_metaLabel->setText(text);
}

void MessageItemWidget::applyBubbleStyle()
{
    const bool isMe = (m_message.senderId == m_currentUserId);
    QString bg;
    QString fg;
    QString border;
    if (m_darkMode) {
        bg = isMe ? QStringLiteral("#1d3f5f") : QStringLiteral("#272a31");
        fg = QStringLiteral("#f3f4f6");
        border = m_isHighlighted ? QStringLiteral("#facc15") : QStringLiteral("#3a3f48");
    } else {
        bg = isMe ? QStringLiteral("#e7f0ff") : QStringLiteral("#ffffff");
        fg = QStringLiteral("#0f172a");
        border = m_isHighlighted ? QStringLiteral("#f59e0b") : QStringLiteral("#dbe0ea");
    }

    m_bubble->setStyleSheet(QString(
        "QFrame#messageBubbleFrame {"
        " border: 1px solid %1;"
        " border-radius: 14px;"
        " background: %2;"
        " color: %3;"
        "}"
    ).arg(border, bg, fg));

    applyControlStyles();
}

void MessageItemWidget::applyControlStyles()
{
    const QString textColor = m_darkMode ? QStringLiteral("#f3f4f6") : QStringLiteral("#0f172a");
    const QString muted = m_darkMode ? QStringLiteral("#9aa4b2") : QStringLiteral("#64748b");
    const QString actionColor = m_darkMode ? QStringLiteral("#93c5fd") : QStringLiteral("#2563eb");
    const QString actionHover = m_darkMode ? QStringLiteral("rgba(96, 165, 250, 0.15)") : QStringLiteral("#eff6ff");
    const QString replyBg = m_darkMode ? QStringLiteral("rgba(255,255,255,0.06)") : QStringLiteral("rgba(15,23,42,0.03)");
    const QString replyHover = m_darkMode ? QStringLiteral("rgba(255,255,255,0.12)") : QStringLiteral("rgba(15,23,42,0.08)");
    const QString attachmentBg = m_darkMode ? QStringLiteral("#1f2430") : QStringLiteral("#f8fafc");
    const QString attachmentBorder = m_darkMode ? QStringLiteral("#475569") : QStringLiteral("#d5dbe5");

    if (m_senderLabel) {
        m_senderLabel->setStyleSheet(QStringLiteral("font-size: 12px; font-weight: 700; letter-spacing: 0.2px; color: %1;")
                                         .arg(m_darkMode ? QStringLiteral("#93c5fd") : QStringLiteral("#2563eb")));
    }
    if (m_forwardedLabel) {
        m_forwardedLabel->setStyleSheet(QStringLiteral("font-size: 11px; font-style: italic; color: %1;").arg(muted));
    }
    if (m_textLabel) {
        m_textLabel->setStyleSheet(QStringLiteral("font-size: 13px; line-height: 1.35; color: %1;").arg(textColor));
    }
    if (m_metaLabel) {
        m_metaLabel->setStyleSheet(QStringLiteral("font-size: 11px; color: %1;").arg(muted));
    }
    if (m_replyAnchorButton) {
        m_replyAnchorButton->setStyleSheet(QString(
            "QPushButton {"
            " text-align: left;"
            " border: none;"
            " border-left: 2px solid %1;"
            " padding: 4px 6px;"
            " color: %2;"
            " background: %3;"
            "}"
            "QPushButton:hover { background: %4; }")
                                               .arg(actionColor, textColor, replyBg, replyHover));
    }

    if (m_imageButton) {
        m_imageButton->setStyleSheet(QString(
            "QToolButton { border: 1px solid %1; border-radius: 8px; background: %2; color: %3; }"
            "QToolButton:hover { border-color: %4; background: %5; }")
                                         .arg(attachmentBorder, attachmentBg, textColor, actionColor, actionHover));
    }

    const QString plainActionStyle = QString(
        "QPushButton { border: 1px solid %1; border-radius: 8px; padding: 4px 8px; background: %2; color: %3; }"
        "QPushButton:hover { border-color: %4; background: %5; }")
                                         .arg(attachmentBorder, attachmentBg, textColor, actionColor, actionHover);
    if (m_audioButton) {
        m_audioButton->setStyleSheet(plainActionStyle);
    }
    if (m_fileButton) {
        m_fileButton->setStyleSheet(plainActionStyle);
    }
    if (m_videoPlayButton) {
        m_videoPlayButton->setStyleSheet(plainActionStyle);
    }
    if (m_videoOpenButton) {
        m_videoOpenButton->setStyleSheet(plainActionStyle);
    }

    const auto actionButtons = m_bubble->findChildren<QPushButton*>(QStringLiteral("messageActionButton"));
    for (QPushButton* actionBtn : actionButtons) {
        if (!actionBtn) {
            continue;
        }
        actionBtn->setStyleSheet(QString(
            "QPushButton { border: none; border-radius: 6px; padding: 3px 7px; font-size: 11px; font-weight: 600; color: %1; background: transparent; }"
            "QPushButton:hover { color: %2; background: %3; }")
                                     .arg(muted, actionColor, actionHover));
    }
}

void MessageItemWidget::renderTextContent(QVBoxLayout* bubbleLayout)
{
    if (!shouldRenderTextContent()) {
        return;
    }
    m_textLabel = new QLabel(m_message.text, m_bubble);
    m_textLabel->setWordWrap(true);
    m_textLabel->setTextFormat(Qt::PlainText);
    m_textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_textLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    m_textLabel->setMinimumWidth(0);
    bubbleLayout->addWidget(m_textLabel);
}

void MessageItemWidget::renderAttachmentContent(QVBoxLayout* bubbleLayout)
{
    const QString type = inferFileType();
    const QString name = fileDisplayName();

    if (type.startsWith(QStringLiteral("image/"))) {
        m_imageButton = new QToolButton(m_bubble);
        m_imageButton->setText(QStringLiteral("Loading preview..."));
        m_imageButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
        m_imageButton->setIconSize(QSize(320, 230));
        m_imageButton->setMinimumSize(220, 150);
        m_imageButton->setMaximumSize(360, 260);
        m_imageButton->setCursor(Qt::PointingHandCursor);
        connect(m_imageButton, &QToolButton::clicked, this, [this]() {
            emit openMediaRequested(QStringLiteral("image"), m_fileUrl);
        });
        bubbleLayout->addWidget(m_imageButton, 0, Qt::AlignLeft);
        const QString cacheKey = QStringLiteral("noveo_msg_image_preview::%1").arg(m_fileUrl);
        QPixmap cached;
        if (QPixmapCache::find(cacheKey, &cached)) {
            m_imageButton->setIcon(QIcon(cached));
            m_imageButton->setText(QString());
            m_imagePreviewLoaded = true;
        }
        return;
    }

    if (type.startsWith(QStringLiteral("video/"))) {
        m_videoWidget = new QVideoWidget(m_bubble);
        m_videoWidget->setMinimumSize(280, 160);
        m_videoWidget->setMaximumSize(320, 220);
        bubbleLayout->addWidget(m_videoWidget, 0, Qt::AlignLeft);

        auto* controls = new QHBoxLayout();
        m_videoPlayButton = new QPushButton(QStringLiteral("Play"), m_bubble);
        m_videoOpenButton = new QPushButton(QStringLiteral("Open Viewer"), m_bubble);
        m_videoPlayButton->setCursor(Qt::PointingHandCursor);
        m_videoOpenButton->setCursor(Qt::PointingHandCursor);
        controls->addWidget(m_videoPlayButton);
        controls->addWidget(m_videoOpenButton);
        controls->addStretch();
        bubbleLayout->addLayout(controls);

        connect(m_videoPlayButton, &QPushButton::clicked, this, [this]() {
            ensureVideoPlayer();
            if (!m_videoPlayer) {
                return;
            }
            if (m_videoPlayer->state() == QMediaPlayer::PlayingState) {
                m_videoPlayer->pause();
                m_videoPlayButton->setText(QStringLiteral("Play"));
            } else {
                m_videoPlayer->play();
                m_videoPlayButton->setText(QStringLiteral("Pause"));
            }
        });
        connect(m_videoOpenButton, &QPushButton::clicked, this, [this]() {
            emit openMediaRequested(QStringLiteral("video"), m_fileUrl);
        });
        return;
    }

    if (type.startsWith(QStringLiteral("audio/"))) {
        auto* row = new QHBoxLayout();
        m_audioButton = new QPushButton(QStringLiteral("Play"), m_bubble);
        m_audioButton->setCursor(Qt::PointingHandCursor);
        auto* label = new QLabel(name, m_bubble);
        label->setWordWrap(true);
        row->addWidget(m_audioButton);
        row->addWidget(label, 1);
        bubbleLayout->addLayout(row);

        connect(m_audioButton, &QPushButton::clicked, this, [this, name]() {
            emit playAudioRequested(m_fileUrl, name, this);
        });
        return;
    }

    m_fileButton = new QPushButton(QStringLiteral("Open %1").arg(name), m_bubble);
    m_fileButton->setCursor(Qt::PointingHandCursor);
    connect(m_fileButton, &QPushButton::clicked, this, [this]() {
        emit openFileRequested(m_fileUrl);
    });
    bubbleLayout->addWidget(m_fileButton, 0, Qt::AlignLeft);
}

void MessageItemWidget::renderActionRow(QVBoxLayout* bubbleLayout)
{
    Q_UNUSED(bubbleLayout);
    // Inline message action buttons are intentionally hidden.
    // Actions remain available via message context menu.
}

void MessageItemWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    maybeLoadImagePreview();
}

bool MessageItemWidget::shouldRenderTextContent() const
{
    const QString trimmed = m_message.text.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }
    if (m_message.file.isNull()) {
        return true;
    }

    const QString lower = trimmed.toLower();
    const QString attachmentName = m_message.file.name.trimmed();
    if (!attachmentName.isEmpty()) {
        if (trimmed.compare(attachmentName, Qt::CaseInsensitive) == 0 ||
            trimmed.compare(QStringLiteral("[%1]").arg(attachmentName), Qt::CaseInsensitive) == 0) {
            return false;
        }
    }
    if (lower.startsWith(QStringLiteral("http://")) || lower.startsWith(QStringLiteral("https://"))) {
        return false;
    }
    if (lower.startsWith(QStringLiteral("[file:")) ||
        lower == QStringLiteral("[image]") ||
        lower.startsWith(QStringLiteral("[video]")) ||
        lower.startsWith(QStringLiteral("[audio]")) ||
        lower == QStringLiteral("[message]")) {
        return false;
    }

    return true;
}

void MessageItemWidget::maybeLoadImagePreview()
{
    if (!m_imageButton || m_imagePreviewLoaded || m_imagePreviewRequested || m_fileUrl.isEmpty() || !isVisible()) {
        return;
    }

    const QString name = fileDisplayName();
    const QString cacheKey = QStringLiteral("noveo_msg_image_preview::%1").arg(m_fileUrl);
    QPixmap cached;
    if (QPixmapCache::find(cacheKey, &cached)) {
        m_imageButton->setIcon(QIcon(cached));
        m_imageButton->setText(QString());
        m_imagePreviewLoaded = true;
        return;
    }

    m_imagePreviewRequested = true;
    const QUrl imageUrl(m_fileUrl);
    if (imageUrl.isLocalFile() || imageUrl.scheme().isEmpty()) {
        QPixmap px(imageUrl.isLocalFile() ? imageUrl.toLocalFile() : m_fileUrl);
        m_imagePreviewRequested = false;
        if (px.isNull()) {
            m_imageButton->setText(name);
            m_imagePreviewLoaded = true;
            return;
        }
        const QPixmap scaled = px.scaled(QSize(320, 230), Qt::KeepAspectRatio, Qt::FastTransformation);
        QPixmapCache::insert(cacheKey, scaled);
        m_imageButton->setIcon(QIcon(scaled));
        m_imageButton->setText(QString());
        m_imagePreviewLoaded = true;
        return;
    }

    QNetworkReply* reply = sharedNetworkManager()->get(QNetworkRequest(imageUrl));
    connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
    QPointer<MessageItemWidget> self(this);
    connect(reply, &QNetworkReply::finished, this, [self, reply, cacheKey, name]() {
        if (!self) {
            return;
        }
        self->m_imagePreviewRequested = false;
        if (!self->m_imageButton) {
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            self->m_imageButton->setText(name);
            self->m_imagePreviewLoaded = true;
            return;
        }
        QPixmap px;
        if (!px.loadFromData(reply->readAll())) {
            self->m_imageButton->setText(name);
            self->m_imagePreviewLoaded = true;
            return;
        }
        const QPixmap scaled = px.scaled(QSize(320, 230), Qt::KeepAspectRatio, Qt::FastTransformation);
        QPixmapCache::insert(cacheKey, scaled);
        self->m_imageButton->setIcon(QIcon(scaled));
        self->m_imageButton->setText(QString());
        self->m_imagePreviewLoaded = true;
    });
}

void MessageItemWidget::ensureVideoPlayer()
{
    if (m_videoPlayerReady || !m_videoWidget) {
        return;
    }
    if (!m_videoPlayer) {
        m_videoPlayer = new QMediaPlayer(this);
    }
    m_videoPlayer->setVideoOutput(m_videoWidget);
    m_videoPlayer->setMedia(QMediaContent(QUrl(m_fileUrl)));
    m_videoPlayerReady = true;
}
