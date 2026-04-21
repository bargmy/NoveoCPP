#include "MainWindow.h"
#include "AppConfig.h"
#include "ChatSettingsDialog.h"
#include "MediaViewerDialog.h"
#include "MessageItemWidget.h"
#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QListWidget>
#include <QStackedWidget>
#include <QSizePolicy>
#include <QScrollArea>
#include <QFrame>
#include <QSettings>
#include <QTimer>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QRegularExpression>
#include <QCheckBox>
#include <QApplication>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QDateTime>
#include <QScrollBar> 
#include <QSet>
#include <QNetworkRequest>
#include <QPainterPath>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QUuid>
#include <QMenu>
#include <QDebug>
#include <algorithm>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QAction>
#include <QCloseEvent>
#include <QStatusBar>
#include <QInputDialog>
#include <QFileDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonParseError>
#include <QDesktopServices>
#include <QSignalBlocker>
#include <QUrl>
#include <QShortcut>
#include <QCoreApplication>
#include <QClipboard>
#include <QMimeData>
#include <QBuffer>
#include <QImage>
#include <QMediaPlayer>
#include <QMediaContent>
#include <QSlider>
#include <QVariant>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QEvent>
#include <QLayoutItem>

const int AvatarUrlRole = Qt::UserRole + 10;
const int FileUrlRole = Qt::UserRole + 12;
const int FileTypeRole = Qt::UserRole + 13;
const QString API_BASE_URL = AppConfig::apiBaseUrl();

namespace {
QJsonObject parsePossiblyNestedContentObject(const QString& text)
{
    QString raw = text.trimmed();
    for (int depth = 0; depth < 3 && !raw.isEmpty(); ++depth) {
        QJsonParseError error;
        const QJsonDocument parsed = QJsonDocument::fromJson(raw.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError) {
            return QJsonObject();
        }
        if (parsed.isObject()) {
            return parsed.object();
        }
        if (parsed.isArray() || parsed.isNull()) {
            return QJsonObject();
        }
        const QVariant variant = parsed.toVariant();
        if (variant.type() == QVariant::String) {
            const QString nested = variant.toString().trimmed();
            if (nested == raw) {
                return QJsonObject();
            }
            raw = nested;
            continue;
        }
        return QJsonObject();
    }
    return QJsonObject();
}

FileAttachment parseFileAttachmentValue(const QJsonValue& value)
{
    FileAttachment file;
    if (value.isObject()) {
        const QJsonObject obj = value.toObject();
        file.url = obj.value(QStringLiteral("url")).toString();
        file.name = obj.value(QStringLiteral("name")).toString();
        file.type = obj.value(QStringLiteral("type")).toString();
        file.size = static_cast<qint64>(obj.value(QStringLiteral("size")).toDouble());
    } else if (value.isString()) {
        file.url = value.toString();
    }
    return file;
}

QString extractRenderableMessageText(const Message& msg, FileAttachment* effectiveFile = nullptr)
{
    FileAttachment resolvedFile = msg.file;
    QString text = msg.text.trimmed();
    const QJsonObject parsedContent = parsePossiblyNestedContentObject(text);
    if (!parsedContent.isEmpty() &&
        (parsedContent.contains(QStringLiteral("text")) ||
         parsedContent.contains(QStringLiteral("file")) ||
         parsedContent.contains(QStringLiteral("theme")))) {
        const QJsonValue parsedText = parsedContent.value(QStringLiteral("text"));
        if (parsedText.isString()) {
            text = parsedText.toString().trimmed();
        } else if (parsedText.isBool() || parsedText.isDouble()) {
            text = parsedText.toVariant().toString().trimmed();
        } else {
            text.clear();
        }

        if (resolvedFile.isNull() && parsedContent.contains(QStringLiteral("file"))) {
            resolvedFile = parseFileAttachmentValue(parsedContent.value(QStringLiteral("file")));
        }
    }

    if (effectiveFile) {
        *effectiveFile = resolvedFile;
    }
    return text;
}
}

void UserListDelegate::paint(QPainter* painter,
                             const QStyleOptionViewItem& option,
                             const QModelIndex& index) const
{
    painter->save();
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    option.widget->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, option.widget);

    QString fullText = index.data(Qt::DisplayRole).toString();
    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
    QRect rect = opt.rect;

    const int padding = 10;
    const int iconSize = 40;

    if (!icon.isNull()) {
        int iconY = rect.top() + (rect.height() - iconSize) / 2;
        icon.paint(painter, rect.left() + padding, iconY, iconSize, iconSize);
    }

    static QRegularExpression regex("^(.*)\\s\\[#([a-fA-F0-9]{6}),\\s*\\\"(.*)\\\"\\]$");
    QRegularExpressionMatch match = regex.match(fullText);

    QString name = fullText;
    QString tagText;
    QColor tagColor;
    bool hasTag = false;

    if (match.hasMatch()) {
        name = match.captured(1).trimmed();
        tagColor = QColor("#" + match.captured(2));
        tagText = match.captured(3);
        hasTag = true;
    }

    QColor textColor = (opt.state & QStyle::State_Selected) ? Qt::white : opt.palette.text().color();
    painter->setPen(textColor);

    QFont nameFont = opt.font;
    nameFont.setPixelSize(14);
    painter->setFont(nameFont);

    int textX = rect.left() + padding + iconSize + padding;
    QFontMetrics fm(nameFont);
    int nameWidth = fm.horizontalAdvance(name);
    int textY = rect.top() + (rect.height() - fm.height()) / 2 + fm.ascent();

    painter->drawText(textX, textY, name);

    if (hasTag) {
        int tagX = textX + nameWidth + 8;
        int tagH = 18;
        int tagY = rect.center().y() - tagH / 2;

        QFont tagFont = nameFont;
        tagFont.setPixelSize(10);
        tagFont.setBold(true);

        QFontMetrics tagFm(tagFont);
        int tagW = tagFm.horizontalAdvance(tagText) + 12;

        painter->setBrush(tagColor);
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(tagX, tagY, tagW, tagH, 4, 4);

        painter->setPen(Qt::white);
        painter->setFont(tagFont);
        painter->drawText(QRect(tagX, tagY, tagW, tagH), Qt::AlignCenter, tagText);
    }

    painter->restore();
}

QSize UserListDelegate::sizeHint(const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const
{
    Q_UNUSED(index);
    return QSize(option.rect.width(), 60);
}

class MessageDelegate : public QStyledItemDelegate {
    bool m_isDarkMode = false;
    MainWindow* m_mainWindow = nullptr;
    QString m_highlightedMessageId;

public:
    explicit MessageDelegate(MainWindow* mainWindow, QObject* parent = nullptr)
        : QStyledItemDelegate(parent), m_mainWindow(mainWindow) {}

    void setTheme(bool isDarkMode) {
        m_isDarkMode = isDarkMode;
    }

    void setHighlightedMessage(const QString& messageId) {
        m_highlightedMessageId = messageId;
    }

    void getBubbleLayout(const QString& text, const QString& sender, bool isMe, int viewWidth,
                         QRect& bubbleRect, QRect& textRect, QRect& nameRect, QRect& avatarRect,
                         int& neededHeight, bool hasReply, const QString& replyText, QRect& replyQuoteRect) const
    {
        const int safeViewWidth = (viewWidth > 0) ? viewWidth : 420;
        const int maxBubbleWidth = qBound(220, static_cast<int>(safeViewWidth * 0.72), 720);
        int nameHeight = isMe ? 0 : 18;
        int replyQuoteHeight = hasReply ? 48 : 0;
        int timeHeight = 12;
        int bubblePadding = 16;
        int avatarSize = 38;
        int avatarGap = 10;
        int sideMargin = 10;

        static QRegularExpression regex("^(.*)\\s\\[#([a-fA-F0-9]{6}),\\s*\\\"(.*)\\\"\\]$");
        QRegularExpressionMatch match = regex.match(sender);

        QString displayName = sender;
        QString tagText;
        bool hasTag = false;

        if (match.hasMatch()) {
            displayName = match.captured(1).trimmed();
            tagText = match.captured(3);
            hasTag = true;
        }

        int totalNameWidth = 0;
        if (!isMe) {
            QFont nameFont("Segoe UI", 11);
            nameFont.setBold(true);
            QFontMetrics fm(nameFont);
            int nameTextW = fm.horizontalAdvance(displayName);
            int tagW = 0;
            if (hasTag) {
                QFont tagFont("Segoe UI", 9);
                tagFont.setBold(true);
                QFontMetrics tagFm(tagFont);
                tagW = tagFm.horizontalAdvance(tagText) + 12;
            }
            totalNameWidth = nameTextW + (hasTag ? (tagW + 8) : 0);
        }

        int replyQuoteWidth = 0;
        if (hasReply && !replyText.isEmpty()) {
            QFont replyFont("Segoe UI", 9);
            replyFont.setItalic(true);
            QFontMetrics replyFm(replyFont);
            QString replyPreview = replyText.length() > 40 ? replyText.left(40) + "..." : replyText;
            replyQuoteWidth = replyFm.horizontalAdvance(replyPreview) + 30;
        }

        QTextDocument doc;
        QTextOption wrapOption;
        wrapOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        doc.setDefaultTextOption(wrapOption);
        doc.setDefaultFont(QFont("Segoe UI", 10));
        doc.setPlainText(text);
        doc.setTextWidth(qMax(140, maxBubbleWidth - 20));

        int textWidth = doc.idealWidth();
        int textHeight = doc.size().height();

        int contentWidth = isMe ? qMax(textWidth + 20, replyQuoteWidth)
                                : qMax(textWidth + 20, qMax(totalNameWidth + 30, replyQuoteWidth));

        int bubbleW = qMax(contentWidth, 100);
        int bubbleH = textHeight + nameHeight + replyQuoteHeight + timeHeight + bubblePadding;

        neededHeight = bubbleH + 10;

        int x;
        if (isMe) {
            x = safeViewWidth - bubbleW - sideMargin;
            avatarRect = QRect();
        } else {
            x = sideMargin + avatarSize + avatarGap;
            avatarRect = QRect(sideMargin, 5 + bubbleH - avatarSize, avatarSize, avatarSize);
        }

        bubbleRect = QRect(x, 5, bubbleW, bubbleH);

        if (!isMe) {
            nameRect = QRect(x + 10, 5 + 5, bubbleW - 20, 15);
            if (hasReply) {
                replyQuoteRect = QRect(x + 10, 5 + 23, bubbleW - 20, replyQuoteHeight - 8);
                textRect = QRect(x + 10, 5 + 23 + replyQuoteHeight, bubbleW - 20, textHeight);
            } else {
                textRect = QRect(x + 10, 5 + 23, bubbleW - 20, textHeight);
            }
        } else {
            if (hasReply) {
                replyQuoteRect = QRect(x + 10, 5 + 8, bubbleW - 20, replyQuoteHeight - 8);
                textRect = QRect(x + 10, 5 + 8 + replyQuoteHeight, bubbleW - 20, textHeight);
            } else {
                textRect = QRect(x + 10, 5 + 8, bubbleW - 20, textHeight);
            }
        }
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setClipRect(option.rect);

        QString text = index.data(Qt::UserRole + 1).toString();
        QString sender = index.data(Qt::UserRole + 2).toString();
        qint64 timestamp = index.data(Qt::UserRole + 3).toLongLong();
        bool isMe = index.data(Qt::UserRole + 4).toBool();
        QIcon avatar = index.data(Qt::DecorationRole).value<QIcon>();
        QString replyToId = index.data(Qt::UserRole + 9).toString();
        QString messageId = index.data(Qt::UserRole + 6).toString();
        bool hasReply = !replyToId.isEmpty();

        QRect bubbleRect, textRect, nameRect, avatarRect, replyQuoteRect;
        int neededHeight;
        QString replyText;
        if (hasReply) {
            replyText = index.data(Qt::UserRole + 10).toString();
        }

        const int viewWidth = option.widget ? option.widget->width() : option.rect.width();
        getBubbleLayout(text, sender, isMe, viewWidth,
                        bubbleRect, textRect, nameRect, avatarRect, neededHeight,
                        hasReply, replyText, replyQuoteRect);

        bubbleRect.translate(0, option.rect.top());
        textRect.translate(0, option.rect.top());
        nameRect.translate(0, option.rect.top());
        avatarRect.translate(0, option.rect.top());
        replyQuoteRect.translate(0, option.rect.top());

        if (!isMe && !avatar.isNull()) {
            avatar.paint(painter, avatarRect);
        }

        QColor bubbleColor;
        QColor borderColor;
        QColor textColor;
        QColor timeColor;

        bool isHighlighted = (messageId == m_highlightedMessageId);

        if (m_isDarkMode) {
            bubbleColor = isMe ? QColor("#2b5278") : QColor("#2d2d2d");
            borderColor = Qt::transparent;
            textColor = Qt::white;
            timeColor = QColor("#a0a0a0");
        } else {
            bubbleColor = isMe ? QColor("#EEFFDE") : Qt::white;
            borderColor = QColor("#d0d0d0");
            textColor = Qt::black;
            timeColor = QColor("#888888");
        }

        if (isHighlighted) {
            borderColor = QColor("#FFD700");
            painter->setBrush(bubbleColor);
            painter->setPen(QPen(borderColor, 3));
        } else {
            painter->setBrush(bubbleColor);
            painter->setPen(borderColor == Qt::transparent ? Qt::NoPen : QPen(borderColor, 1));
        }

        painter->drawRoundedRect(bubbleRect, 12, 12);

        if (!isMe) {
            static QRegularExpression regex("^(.*)\\s\\[#([a-fA-F0-9]{6}),\\s*\\\"(.*)\\\"\\]$");
            QRegularExpressionMatch match = regex.match(sender);
            QString displayName = sender;
            QString tagText;
            QColor tagColor;
            bool hasTag = false;

            if (match.hasMatch()) {
                displayName = match.captured(1).trimmed();
                tagColor = QColor("#" + match.captured(2));
                tagText = match.captured(3);
                hasTag = true;
            }

            painter->setPen(QColor("#E35967"));
            QFont nameFont = option.font;
            nameFont.setPixelSize(11);
            nameFont.setBold(true);
            painter->setFont(nameFont);
            QFontMetrics fm(nameFont);

            int nameW = fm.horizontalAdvance(displayName);
            painter->drawText(nameRect.left(), nameRect.top() + fm.ascent(), displayName);

            if (hasTag) {
                int tagX = nameRect.left() + nameW + 6;
                int tagY = nameRect.top();
                int tagW = fm.horizontalAdvance(tagText) + 10;
                int tagH = 14;

                painter->setBrush(tagColor);
                painter->setPen(Qt::NoPen);
                painter->drawRoundedRect(tagX, tagY, tagW, tagH, 3, 3);

                painter->setPen(Qt::white);
                QFont tagFont = nameFont;
                tagFont.setPixelSize(9);
                painter->setFont(tagFont);
                painter->drawText(QRect(tagX, tagY, tagW, tagH), Qt::AlignCenter, tagText);
            }
        }

        if (!replyToId.isEmpty()) {
            QString replySenderName = index.data(Qt::UserRole + 11).toString();
            int replyX = replyQuoteRect.left();
            int replyY = replyQuoteRect.top();

            if (!replySenderName.isEmpty()) {
                QFont senderFont("Segoe UI", 9);
                senderFont.setBold(true);
                painter->setFont(senderFont);
                painter->setPen(m_isDarkMode ? QColor("#60A5FA") : QColor("#2563EB"));
                QFontMetrics senderFm(senderFont);
                painter->drawText(replyX + 6, replyY + senderFm.ascent(), replySenderName);
                replyY += senderFm.height() + 2;
            }

            painter->setPen(m_isDarkMode ? QColor("#666") : QColor("#ccc"));
            painter->drawLine(replyX, replyY, replyX, replyY + 20);

            QFont replyFont("Segoe UI", 9);
            replyFont.setItalic(true);
            painter->setFont(replyFont);
            painter->setPen(m_isDarkMode ? QColor("#aaa") : QColor("#666"));

            if (replyText.isEmpty()) {
                replyText = "[Original message]";
            }
            QString replyPreview = replyText.length() > 40 ? replyText.left(40) + "..." : replyText;
            QFontMetrics replyFm(replyFont);
            painter->drawText(replyX + 6, replyY + replyFm.ascent() + 2, replyPreview);
        }

        painter->setPen(textColor);
        QTextDocument doc;
        QTextOption wrapOption;
        wrapOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        doc.setDefaultTextOption(wrapOption);
        QFont textFont("Segoe UI", 10);
        doc.setDefaultFont(textFont);

        QString html = QString("<div style=\"color:%1; white-space:pre-wrap; word-wrap:break-word;\">%2</div>")
                           .arg(textColor.name(), text.toHtmlEscaped().replace("\n", "<br>"));
        doc.setHtml(html);
        doc.setTextWidth(textRect.width());

        painter->translate(textRect.topLeft());
        doc.drawContents(painter);
        painter->translate(-textRect.topLeft());

        QDateTime dt;
        dt.setSecsSinceEpoch(timestamp);
        QString timeStr = dt.toString("hh:mm AP");

        painter->setPen(timeColor);
        QFont timeFont = option.font;
        timeFont.setPixelSize(9);
        painter->setFont(timeFont);
        QFontMetrics timeFm(timeFont);
        int timeWidth = timeFm.horizontalAdvance(timeStr);

        if (isMe) {
            int statusInt = index.data(Qt::UserRole + 5).toInt();
            MessageStatus status = static_cast<MessageStatus>(statusInt);

            QFont statusFont("Segoe UI", 9);
            painter->setFont(statusFont);
            QFontMetrics fm(statusFont);

            QString checkmark = QString(QChar(0x2713));
            QString circle = QString(QChar(0x25CB));
            bool hasCheckmark = fm.horizontalAdvance(checkmark) > 0;

            QString statusIcon;
            QColor statusColor = timeColor;
            int iconX = 0;

            if (status == MessageStatus::Pending) {
                statusIcon = circle;
                statusColor = QColor("#999999");
            } else if (status == MessageStatus::Sent) {
                statusIcon = hasCheckmark ? checkmark : "v";
                statusColor = QColor("#999999");
            } else if (status == MessageStatus::Seen) {
                if (hasCheckmark) {
                    painter->setPen(m_isDarkMode ? QColor("#93C5FD") : QColor("#60A5FA"));
                    int singleWidth = fm.horizontalAdvance(checkmark);
                    int overlapOffset = singleWidth / 2;
                    iconX = bubbleRect.right() - 8 - timeWidth - 4 - singleWidth - overlapOffset;
                    int iconY = bubbleRect.bottom() - 5 - timeFm.height();
                    painter->drawText(iconX, iconY + fm.ascent(), checkmark);
                    painter->drawText(iconX + overlapOffset, iconY + fm.ascent(), checkmark);
                    statusIcon.clear();
                } else {
                    statusIcon = "vv";
                    statusColor = m_isDarkMode ? QColor("#93C5FD") : QColor("#60A5FA");
                }
            }

            if (!statusIcon.isEmpty()) {
                painter->setPen(statusColor);
                int iconWidth = fm.horizontalAdvance(statusIcon);
                iconX = bubbleRect.right() - 8 - timeWidth - 4 - iconWidth;
                int iconY = bubbleRect.bottom() - 5 - timeFm.height();
                painter->drawText(iconX, iconY + fm.ascent(), statusIcon);
            }
        }

        painter->setPen(timeColor);
        painter->setFont(timeFont);
        painter->drawText(bubbleRect.adjusted(0, 0, -8, -5), Qt::AlignBottom | Qt::AlignRight, timeStr);

        qint64 editedAt = index.data(Qt::UserRole + 7).toLongLong();
        if (editedAt > 0) {
            QFont editedFont = timeFont;
            editedFont.setItalic(true);
            editedFont.setPixelSize(8);
            painter->setFont(editedFont);
            painter->setPen(QColor("#888888"));
            QFontMetrics editedFm(editedFont);
            QString editedText = "edited";
            int editedWidth = editedFm.horizontalAdvance(editedText);
            int editedX = bubbleRect.right() - 8 - editedWidth;
            int editedY = bubbleRect.bottom() - 5 - timeFm.height() - editedFm.height() - 2;
            painter->drawText(editedX, editedY + editedFm.ascent(), editedText);
        }

        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        QString text = index.data(Qt::UserRole + 1).toString();
        QString sender = index.data(Qt::UserRole + 2).toString();
        bool isMe = index.data(Qt::UserRole + 4).toBool();
        QString replyToId = index.data(Qt::UserRole + 9).toString();
        bool hasReply = !replyToId.isEmpty();

        QString replyText;
        if (hasReply) {
            replyText = index.data(Qt::UserRole + 10).toString();
        }

        QRect b, t, n, a, r;
        int h;
        int widthHint = option.rect.width();
        if (widthHint <= 0 && option.widget) {
            widthHint = option.widget->width();
        }
        if (widthHint <= 0) {
            widthHint = 420;
        }
        getBubbleLayout(text, sender, isMe, widthHint, b, t, n, a, h, hasReply, replyText, r);
        return QSize(widthHint, h);
    }

    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                QString replyToId = index.data(Qt::UserRole + 9).toString();
                if (!replyToId.isEmpty()) {
                    QString text = index.data(Qt::UserRole + 1).toString();
                    QString sender = index.data(Qt::UserRole + 2).toString();
                    bool isMe = index.data(Qt::UserRole + 4).toBool();

                    QRect bubbleRect, textRect, nameRect, avatarRect, replyQuoteRect;
                    int neededHeight;
                    QString replyText = index.data(Qt::UserRole + 10).toString();
                    bool hasReply = true;

                    const int viewWidth = option.widget ? option.widget->width() : option.rect.width();
                    getBubbleLayout(text, sender, isMe, viewWidth,
                                    bubbleRect, textRect, nameRect, avatarRect, neededHeight,
                                    hasReply, replyText, replyQuoteRect);

                    replyQuoteRect.translate(0, option.rect.top());
                    if (replyQuoteRect.contains(mouseEvent->pos())) {
                        if (m_mainWindow) {
                            m_mainWindow->focusOnMessage(replyToId);
                        }
                        return true;
                    }
                }
            }
        }
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_client(new WebSocketClient(this)),
      m_nam(new QNetworkAccessManager(this)),
      m_restClient(new RestClient(this)),
      m_updaterService(new UpdaterService(this)),
      m_voiceAudio(new VoiceAudioBridge(this))
{
    QSettings settings("Noveo", "MessengerClient");
    m_isDarkMode = settings.value("darkMode", false).toBool();
    m_notificationsEnabled = settings.value("notificationsEnabled", true).toBool();
    m_updaterService->setUpdaterExecutable(settings.value("updaterExecutablePath").toString());
    m_updaterService->setFeedUrl(settings.value("updaterFeedUrl", QString::fromUtf8(qgetenv("NOVEO_UPDATE_FEED"))).toString());

    setupUi();
    applyTheme();
    setupTrayIcon();
    qApp->installEventFilter(this);
    m_messageResizeDebounceTimer = new QTimer(this);
    m_messageResizeDebounceTimer->setSingleShot(true);
    m_messageResizeDebounceTimer->setInterval(45);
    connect(m_messageResizeDebounceTimer, &QTimer::timeout, this, &MainWindow::refreshMessageWidgetSizes);
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, [this]() {
        if (!m_client->isConnected()) {
            m_client->connectToServer();
        }
    });

    connect(m_client, &WebSocketClient::connected, this, &MainWindow::onConnected);
    connect(m_client, &WebSocketClient::disconnected, this, &MainWindow::onDisconnected);
    connect(m_client, &WebSocketClient::loginSuccess, this, &MainWindow::onLoginSuccess);
    connect(m_client, &WebSocketClient::authFailed, this, &MainWindow::onAuthFailed);
    connect(m_client, &WebSocketClient::chatHistoryReceived, this, &MainWindow::onChatHistoryReceived);
    connect(m_client, &WebSocketClient::messageReceived, this, &MainWindow::onMessageReceived);
    connect(m_client, &WebSocketClient::userListUpdated, this, &MainWindow::onUserListUpdated);
    connect(m_client, &WebSocketClient::newChatCreated, this, &MainWindow::onNewChatCreated);
    connect(m_client, &WebSocketClient::messageSeenUpdate, this, &MainWindow::onMessageSeenUpdate);
    connect(m_client, &WebSocketClient::messageUpdated, this, &MainWindow::onMessageUpdated);
    connect(m_client, &WebSocketClient::messageDeleted, this, &MainWindow::onMessageDeleted);
    connect(m_client, &WebSocketClient::presenceUpdated, this, &MainWindow::onPresenceUpdated);
    connect(m_client, &WebSocketClient::typingReceived, this, &MainWindow::onTypingReceived);
    connect(m_client, &WebSocketClient::passwordChanged, this, &MainWindow::onPasswordChanged);
    connect(m_client, &WebSocketClient::userUpdated, this, &MainWindow::onUserUpdated);
    connect(m_client, &WebSocketClient::channelInfoReceived, this, [this](const Chat& chat) {
        const bool existed = m_chats.contains(chat.chatId);
        m_chats[chat.chatId] = chat;
        if (!existed) {
            onNewChatCreated(chat);
        }
        m_currentChatId = chat.chatId;
        m_chatTitle->setText(resolveChatName(m_chats[chat.chatId]));
        renderMessages(chat.chatId);
        updateComposerStateForCurrentChat();
        updatePinnedMessageBar();
    });
    connect(m_client, &WebSocketClient::memberJoined, this, [this](const QString& chatId, const QStringList& members) {
        if (!m_chats.contains(chatId)) {
            return;
        }
        m_chats[chatId].members = members;
        if (m_currentChatId == chatId) {
            m_chatTitle->setText(resolveChatName(m_chats[chatId]));
            updateComposerStateForCurrentChat();
        }
    });
    connect(m_client, &WebSocketClient::messagePinned, this, [this](const QString& chatId, const Message& message) {
        if (!m_chats.contains(chatId)) {
            return;
        }
        m_chats[chatId].hasPinnedMessage = true;
        m_chats[chatId].pinnedMessage = message;
        if (m_currentChatId == chatId) {
            updatePinnedMessageBar();
            statusBar()->showMessage("Pinned message updated.", 2000);
        }
    });
    connect(m_client, &WebSocketClient::messageUnpinned, this, [this](const QString& chatId) {
        if (!m_chats.contains(chatId)) {
            return;
        }
        m_chats[chatId].hasPinnedMessage = false;
        if (m_currentChatId == chatId) {
            updatePinnedMessageBar();
            statusBar()->showMessage("Message unpinned.", 2000);
        }
    });
    connect(m_client, &WebSocketClient::errorOccurred, this, [this](const QString& message) {
        statusBar()->showMessage(message, 4000);
    });
    connect(m_client, &WebSocketClient::voiceChatUpdated, this, [this](const VoiceChatParticipants& participantsByChat) {
        if (m_currentVoiceChatId.isEmpty()) {
            return;
        }
        const QStringList participants = participantsByChat.value(m_currentVoiceChatId);
        if (!participants.contains(m_client->currentUserId())) {
            m_currentVoiceChatId.clear();
            m_voiceAudio->stop();
            if (m_voiceCallBtn) {
                m_voiceCallBtn->setText("Voice");
            }
            statusBar()->showMessage("Voice call ended.", 2000);
            return;
        }
        statusBar()->showMessage("Voice call participants updated.", 1500);
    });
    connect(m_client, &WebSocketClient::incomingCall, this, [this](const QString& chatId,
                                                                    const QString&,
                                                                    const QString& chatName,
                                                                    const QString& callerName,
                                                                    const QString&) {
        const auto answer = QMessageBox::question(
            this,
            "Incoming Call",
            QString("%1 is calling in %2. Accept?").arg(callerName, chatName),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            return;
        }
        if (m_chats.contains(chatId)) {
            m_currentChatId = chatId;
            m_chatTitle->setText(resolveChatName(m_chats[chatId]));
            renderMessages(chatId);
            updateComposerStateForCurrentChat();
            updatePinnedMessageBar();
        }
        if (!m_voiceAudio->start()) {
            return;
        }
        m_currentVoiceChatId = chatId;
        m_client->voiceJoin(chatId);
        if (m_voiceCallBtn) {
            m_voiceCallBtn->setText("Leave Voice");
        }
    });
    connect(m_client, &WebSocketClient::binaryAudioReceived, this, [this](const QString&, const QByteArray& payload) {
        if (m_voiceAudio && !m_currentVoiceChatId.isEmpty()) {
            m_voiceAudio->playPcm(payload);
        }
    });
    connect(m_voiceAudio, &VoiceAudioBridge::pcmCaptured, this, [this](const QByteArray& payload) {
        if (!m_currentVoiceChatId.isEmpty()) {
            m_client->sendBinaryAudio(payload);
        }
    });
    connect(m_voiceAudio, &VoiceAudioBridge::audioError, this, [this](const QString& message) {
        statusBar()->showMessage(message, 4000);
    });

    m_statusLabel->setText("Connecting to server...");
    m_client->connectToServer();
}

MainWindow::~MainWindow()
{
    qApp->removeEventFilter(this);
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    if (m_chatList) {
        m_chatList->doItemsLayout();
        m_chatList->viewport()->update();
    }
    if (m_messageResizeDebounceTimer) {
        m_messageResizeDebounceTimer->start();
    } else {
        refreshMessageWidgetSizes();
    }
    repositionStickerPanel();
    updateSettingsOverlayGeometry();
    updateContactsOverlayGeometry();
    updateSidebarMenuGeometry();
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (m_settingsDialog && watched == m_settingsDialog && event->type() == QEvent::Hide) {
        if (m_settingsOverlay) {
            m_settingsOverlay->hide();
        }
    }

    if (m_settingsDialog && m_settingsDialog->isVisible() && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            m_settingsDialog->hide();
            return true;
        }
    }

    if (m_settingsOverlay && watched == m_settingsOverlay && m_settingsOverlay->isVisible() &&
        event->type() == QEvent::MouseButtonPress && m_settingsDialog && m_settingsDialog->isVisible()) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (!m_settingsDialog->geometry().contains(mouseEvent->pos())) {
            m_settingsDialog->hide();
            return true;
        }
    }

    if (m_contactsOverlay && m_contactsOverlay->isVisible()) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            const QPoint globalPos = mouseEvent->globalPos();
            const QRect panelRect = m_contactsPanel
                                        ? QRect(m_contactsPanel->mapToGlobal(QPoint(0, 0)), m_contactsPanel->size())
                                        : QRect();
            if (!panelRect.contains(globalPos)) {
                m_contactsOverlay->hide();
                return true;
            }
        } else if (event->type() == QEvent::KeyPress) {
            auto* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Escape) {
                m_contactsOverlay->hide();
                return true;
            }
        }
    }

    if (m_sidebarMenuOverlay && m_sidebarMenuOverlay->isVisible()) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            const QPoint globalPos = mouseEvent->globalPos();
            const QRect panelRect = m_sidebarMenuPanel
                                        ? QRect(m_sidebarMenuPanel->mapToGlobal(QPoint(0, 0)), m_sidebarMenuPanel->size())
                                        : QRect();
            const QRect menuButtonRect = m_sidebarSettingsBtn
                                             ? QRect(m_sidebarSettingsBtn->mapToGlobal(QPoint(0, 0)), m_sidebarSettingsBtn->size())
                                             : QRect();
            if (!panelRect.contains(globalPos) && !menuButtonRect.contains(globalPos)) {
                hideSidebarMenu();
                return true;
            }
        } else if (event->type() == QEvent::KeyPress) {
            auto* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Escape) {
                hideSidebarMenu();
                return true;
            }
        }
    }

    if (m_stickerPanel && m_stickerPanelVisible) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            const QPoint globalPos = mouseEvent->globalPos();
            const QRect panelRect(m_stickerPanel->mapToGlobal(QPoint(0, 0)), m_stickerPanel->size());
            const QRect buttonRect = m_stickerBtn
                                         ? QRect(m_stickerBtn->mapToGlobal(QPoint(0, 0)), m_stickerBtn->size())
                                         : QRect();
            if (!panelRect.contains(globalPos) && !buttonRect.contains(globalPos)) {
                hideStickerPanel();
            }
        } else if (event->type() == QEvent::KeyPress) {
            auto* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Escape) {
                hideStickerPanel();
                return true;
            }
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::setupUi() {
    resize(1000, 700);
    setWindowTitle("Noveo Desktop");

    m_stackedWidget = new QStackedWidget(this);
    setCentralWidget(m_stackedWidget);

    m_loginPage = new QWidget();
    m_loginPage->setObjectName("loginPage");
    QVBoxLayout* loginLayout = new QVBoxLayout(m_loginPage);
    loginLayout->setContentsMargins(24, 24, 24, 24);
    loginLayout->setSpacing(0);

    auto* authCard = new QFrame(m_loginPage);
    authCard->setObjectName("authCard");
    authCard->setFixedWidth(420);
    auto* authCardLayout = new QVBoxLayout(authCard);
    authCardLayout->setContentsMargins(28, 28, 28, 24);
    authCardLayout->setSpacing(10);

    QLabel* brandTitle = new QLabel("Noveo");
    brandTitle->setObjectName("authBrandTitle");
    brandTitle->setAlignment(Qt::AlignCenter);
    authCardLayout->addWidget(brandTitle);

    m_authFormsStack = new QStackedWidget(authCard);
    m_authFormsStack->setObjectName("authFormsStack");

    auto* loginFormPage = new QWidget(m_authFormsStack);
    auto* loginFormLayout = new QVBoxLayout(loginFormPage);
    loginFormLayout->setContentsMargins(0, 0, 0, 0);
    loginFormLayout->setSpacing(10);

    auto* loginTitle = new QLabel("Welcome Back!");
    loginTitle->setObjectName("authTitle");
    loginTitle->setAlignment(Qt::AlignCenter);
    auto* loginSubtitle = new QLabel("Please log in to continue.");
    loginSubtitle->setObjectName("authSubtitle");
    loginSubtitle->setAlignment(Qt::AlignCenter);

    m_usernameInput = new QLineEdit();
    m_usernameInput->setPlaceholderText("Username");
    m_passwordInput = new QLineEdit();
    m_passwordInput->setPlaceholderText("Password");
    m_passwordInput->setEchoMode(QLineEdit::Password);

    m_loginBtn = new QPushButton("Login");
    m_loginBtn->setObjectName("primaryAuthButton");
    m_loginBtn->setCursor(Qt::PointingHandCursor);
    m_loginBtn->setMinimumHeight(42);

    m_showRegisterSwitchBtn = new QPushButton("Don't have an account? Register here.");
    m_showRegisterSwitchBtn->setObjectName("authLinkButton");
    m_showRegisterSwitchBtn->setFlat(true);
    m_showRegisterSwitchBtn->setCursor(Qt::PointingHandCursor);

    loginFormLayout->addWidget(loginTitle);
    loginFormLayout->addWidget(loginSubtitle);
    loginFormLayout->addSpacing(8);
    loginFormLayout->addWidget(m_usernameInput);
    loginFormLayout->addWidget(m_passwordInput);
    loginFormLayout->addWidget(m_loginBtn);
    loginFormLayout->addWidget(m_showRegisterSwitchBtn, 0, Qt::AlignHCenter);

    auto* registerFormPage = new QWidget(m_authFormsStack);
    auto* registerFormLayout = new QVBoxLayout(registerFormPage);
    registerFormLayout->setContentsMargins(0, 0, 0, 0);
    registerFormLayout->setSpacing(10);

    auto* registerTitle = new QLabel("Create Account");
    registerTitle->setObjectName("authTitle");
    registerTitle->setAlignment(Qt::AlignCenter);
    auto* registerSubtitle = new QLabel("Join the conversation in seconds.");
    registerSubtitle->setObjectName("authSubtitle");
    registerSubtitle->setAlignment(Qt::AlignCenter);

    m_registerUsernameInput = new QLineEdit();
    m_registerUsernameInput->setPlaceholderText("Username");
    m_registerPasswordInput = new QLineEdit();
    m_registerPasswordInput->setPlaceholderText("Choose a Password");
    m_registerPasswordInput->setEchoMode(QLineEdit::Password);

    m_registerBtn = new QPushButton("Register");
    m_registerBtn->setObjectName("secondaryAuthButton");
    m_registerBtn->setCursor(Qt::PointingHandCursor);
    m_registerBtn->setMinimumHeight(42);

    m_showLoginSwitchBtn = new QPushButton("Already have an account? Login here.");
    m_showLoginSwitchBtn->setObjectName("authLinkButton");
    m_showLoginSwitchBtn->setFlat(true);
    m_showLoginSwitchBtn->setCursor(Qt::PointingHandCursor);

    registerFormLayout->addWidget(registerTitle);
    registerFormLayout->addWidget(registerSubtitle);
    registerFormLayout->addSpacing(8);
    registerFormLayout->addWidget(m_registerUsernameInput);
    registerFormLayout->addWidget(m_registerPasswordInput);
    registerFormLayout->addWidget(m_registerBtn);
    registerFormLayout->addWidget(m_showLoginSwitchBtn, 0, Qt::AlignHCenter);

    m_authFormsStack->addWidget(loginFormPage);
    m_authFormsStack->addWidget(registerFormPage);
    m_authFormsStack->setCurrentIndex(0);

    m_statusLabel = new QLabel("Waiting for connection...");
    m_statusLabel->setObjectName("authStatusLabel");
    m_statusLabel->setAlignment(Qt::AlignCenter);

    authCardLayout->addWidget(m_authFormsStack);
    authCardLayout->addSpacing(8);
    authCardLayout->addWidget(m_statusLabel);

    loginLayout->addStretch();
    loginLayout->addWidget(authCard, 0, Qt::AlignHCenter);
    loginLayout->addStretch();

    m_stackedWidget->addWidget(m_loginPage);

    m_appPage = new QWidget();
    m_appPage->setMinimumHeight(0);
    QHBoxLayout* appLayout = new QHBoxLayout(m_appPage);
    appLayout->setContentsMargins(0, 0, 0, 0);
    appLayout->setSpacing(0);

    m_chatListWidget = new QListWidget();
    m_chatListWidget->setObjectName("sidebarChatList");
    m_chatListWidget->setIconSize(QSize(42, 42));
    m_chatListWidget->setItemDelegate(new UserListDelegate(this));
    m_chatListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_chatListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_chatListWidget, &QListWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QListWidgetItem* item = m_chatListWidget->itemAt(pos);
        if (!item) {
            return;
        }
        const QString chatId = item->data(Qt::UserRole).toString();
        if (!m_chats.contains(chatId)) {
            return;
        }
        const Chat& chat = m_chats[chatId];
        const bool canManage = (chat.ownerId == m_client->currentUserId()) &&
                               (chat.chatType == "group" || chat.chatType == "channel");
        if (!canManage) {
            return;
        }
        QMenu menu(this);
        QAction* openSettings = menu.addAction("Chat Settings");
        QAction* chosen = menu.exec(m_chatListWidget->mapToGlobal(pos));
        if (chosen == openSettings) {
            openChatSettingsDialog(chatId);
        }
    });

    m_contactListWidget = new QListWidget();
    m_contactListWidget->setObjectName("contactsModalList");
    m_contactListWidget->setIconSize(QSize(42, 42));
    m_contactListWidget->setItemDelegate(new UserListDelegate(this));
    m_contactListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(m_updaterService, &UpdaterService::statusChanged, this, [this](UpdaterService::Status status, const QString& message) {
        m_updaterStatusText = message;
        m_canDownloadUpdate = false;
        m_canInstallUpdate = false;
        if (status == UpdaterService::Status::UpdateAvailable) {
            m_canDownloadUpdate = true;
        } else if (status == UpdaterService::Status::Downloaded) {
            m_canInstallUpdate = true;
        }
        if (m_settingsDialog) {
            m_settingsDialog->setUpdaterState(m_updaterStatusText, m_canDownloadUpdate, m_canInstallUpdate);
        }
    });
    connect(m_updaterService, &UpdaterService::downloadProgress, this, [this](int percent) {
        m_updaterStatusText = QString("Downloading update... %1%").arg(percent);
        m_canDownloadUpdate = false;
        m_canInstallUpdate = false;
        if (m_settingsDialog) {
            m_settingsDialog->setUpdaterState(m_updaterStatusText, false, false);
        }
    });

    m_sidebarContainer = new QWidget();
    m_sidebarContainer->setFixedWidth(300);
    m_sidebarContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    auto* sidebarLayout = new QVBoxLayout(m_sidebarContainer);
    sidebarLayout->setContentsMargins(0, 0, 0, 0);
    sidebarLayout->setSpacing(0);

    auto* sidebarHeader = new QWidget(m_sidebarContainer);
    sidebarHeader->setObjectName("sidebarHeader");
    auto* sidebarHeaderLayout = new QHBoxLayout(sidebarHeader);
    sidebarHeaderLayout->setContentsMargins(8, 6, 8, 6);
    sidebarHeaderLayout->setSpacing(8);
    m_sidebarSettingsBtn = new QPushButton(QStringLiteral("Menu"), sidebarHeader);
    m_sidebarSettingsBtn->setObjectName("sidebarMenuBtn");
    m_sidebarSettingsBtn->setCursor(Qt::PointingHandCursor);
    m_sidebarSettingsBtn->setToolTip("Menu");
    m_sidebarSettingsBtn->setMinimumWidth(62);
    m_sidebarTitleLabel = new QLabel(QStringLiteral("Noveo"), sidebarHeader);
    m_sidebarTitleLabel->setObjectName("sidebarTitle");
    m_sidebarTitleLabel->setAlignment(Qt::AlignCenter);
    m_sidebarCreateBtn = new QPushButton(QStringLiteral("+"), sidebarHeader);
    m_sidebarCreateBtn->setObjectName("sidebarCreateBtn");
    m_sidebarCreateBtn->setCursor(Qt::PointingHandCursor);
    m_sidebarCreateBtn->setToolTip("Create");
    m_sidebarCreateBtn->setFixedWidth(36);
    auto* leftFill = new QWidget(sidebarHeader);
    auto* rightFill = new QWidget(sidebarHeader);
    leftFill->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    rightFill->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    sidebarHeaderLayout->addWidget(m_sidebarSettingsBtn);
    sidebarHeaderLayout->addWidget(leftFill, 1);
    sidebarHeaderLayout->addWidget(m_sidebarTitleLabel);
    sidebarHeaderLayout->addWidget(rightFill, 1);
    sidebarHeaderLayout->addWidget(m_sidebarCreateBtn);
    sidebarLayout->addWidget(sidebarHeader);

    sidebarLayout->addWidget(m_chatListWidget, 1);

    m_sidebarAudioPlayer = new QWidget(m_sidebarContainer);
    m_sidebarAudioPlayer->setObjectName(QStringLiteral("sidebarAudioPlayer"));
    auto* sidebarAudioLayout = new QVBoxLayout(m_sidebarAudioPlayer);
    sidebarAudioLayout->setContentsMargins(8, 8, 8, 8);
    sidebarAudioLayout->setSpacing(6);

    auto* audioTop = new QHBoxLayout();
    auto* nowPlaying = new QLabel(QStringLiteral("NOW PLAYING"), m_sidebarAudioPlayer);
    nowPlaying->setStyleSheet("font-size: 10px; color: #6b7280; font-weight: 700;");
    m_sidebarCloseBtn = new QPushButton(QStringLiteral("x"), m_sidebarAudioPlayer);
    m_sidebarCloseBtn->setCursor(Qt::PointingHandCursor);
    m_sidebarCloseBtn->setFixedWidth(24);
    m_sidebarCloseBtn->setStyleSheet("QPushButton { border: none; background: transparent; color: #6b7280; font-weight: 700; }"
                                     "QPushButton:hover { color: #111827; }");
    audioTop->addWidget(nowPlaying);
    audioTop->addStretch();
    audioTop->addWidget(m_sidebarCloseBtn);
    sidebarAudioLayout->addLayout(audioTop);

    auto* audioMain = new QHBoxLayout();
    m_sidebarPlayBtn = new QPushButton(QStringLiteral("Play"), m_sidebarAudioPlayer);
    m_sidebarPlayBtn->setCursor(Qt::PointingHandCursor);
    m_sidebarPlayBtn->setFixedWidth(58);
    audioMain->addWidget(m_sidebarPlayBtn);
    auto* audioInfoLayout = new QVBoxLayout();
    audioInfoLayout->setContentsMargins(0, 0, 0, 0);
    audioInfoLayout->setSpacing(2);
    m_sidebarTrackName = new QLabel(QStringLiteral("Track"), m_sidebarAudioPlayer);
    m_sidebarTrackName->setWordWrap(false);
    auto* timeRow = new QHBoxLayout();
    m_sidebarCurrentTime = new QLabel(QStringLiteral("0:00"), m_sidebarAudioPlayer);
    m_sidebarDuration = new QLabel(QStringLiteral("0:00"), m_sidebarAudioPlayer);
    m_sidebarSeekBar = new QSlider(Qt::Horizontal, m_sidebarAudioPlayer);
    m_sidebarSeekBar->setRange(0, 1000);
    timeRow->addWidget(m_sidebarCurrentTime);
    timeRow->addWidget(m_sidebarSeekBar, 1);
    timeRow->addWidget(m_sidebarDuration);
    audioInfoLayout->addWidget(m_sidebarTrackName);
    audioInfoLayout->addLayout(timeRow);
    audioMain->addLayout(audioInfoLayout, 1);
    m_sidebarVolumeToggleBtn = new QPushButton(QStringLiteral("Vol"), m_sidebarAudioPlayer);
    m_sidebarVolumeToggleBtn->setCursor(Qt::PointingHandCursor);
    m_sidebarVolumeToggleBtn->setFixedWidth(42);
    audioMain->addWidget(m_sidebarVolumeToggleBtn);
    sidebarAudioLayout->addLayout(audioMain);

    m_sidebarVolumeControl = new QWidget(m_sidebarAudioPlayer);
    auto* volumeLayout = new QHBoxLayout(m_sidebarVolumeControl);
    volumeLayout->setContentsMargins(0, 0, 0, 0);
    volumeLayout->setSpacing(4);
    auto* vLow = new QLabel("-", m_sidebarVolumeControl);
    auto* vHigh = new QLabel("+", m_sidebarVolumeControl);
    m_sidebarVolumeBar = new QSlider(Qt::Horizontal, m_sidebarVolumeControl);
    m_sidebarVolumeBar->setRange(0, 100);
    m_sidebarVolumeBar->setValue(100);
    volumeLayout->addWidget(vLow);
    volumeLayout->addWidget(m_sidebarVolumeBar, 1);
    volumeLayout->addWidget(vHigh);
    m_sidebarVolumeControl->hide();
    sidebarAudioLayout->addWidget(m_sidebarVolumeControl);
    m_sidebarAudioPlayer->hide();
    sidebarLayout->addWidget(m_sidebarAudioPlayer);

    m_chatAreaWidget = new QWidget();
    m_chatAreaWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_chatAreaWidget->setMinimumHeight(0);
    QVBoxLayout* chatAreaLayout = new QVBoxLayout(m_chatAreaWidget);
    chatAreaLayout->setContentsMargins(0, 0, 0, 0);
    chatAreaLayout->setSpacing(0);

    QWidget* header = new QWidget();
    header->setObjectName("chatHeader");
    header->setFixedHeight(60);
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    m_chatTitle = new QLabel("Select a chat");
    m_chatTitle->setStyleSheet("font-size: 16px; font-weight: bold; margin-left: 10px;");
    headerLayout->addWidget(m_chatTitle);
    headerLayout->addStretch();
    m_chatSettingsBtn = new QPushButton("Settings");
    m_chatSettingsBtn->setCursor(Qt::PointingHandCursor);
    m_chatSettingsBtn->setVisible(false);
    m_chatSettingsBtn->setStyleSheet("QPushButton { padding: 6px 10px; border-radius: 8px; }");
    headerLayout->addWidget(m_chatSettingsBtn);
    m_voiceCallBtn = new QPushButton("Voice");
    m_voiceCallBtn->setCursor(Qt::PointingHandCursor);
    m_voiceCallBtn->setEnabled(false);
    headerLayout->addWidget(m_voiceCallBtn);

    m_joinChannelBar = new QWidget();
    m_joinChannelBar->setObjectName("joinChannelBar");
    m_joinChannelBar->hide();
    auto* joinLayout = new QHBoxLayout(m_joinChannelBar);
    joinLayout->setContentsMargins(10, 6, 10, 6);
    auto* joinLabel = new QLabel("You are not a member of this channel.");
    m_joinChannelBtn = new QPushButton("Join Channel");
    m_joinChannelBtn->setCursor(Qt::PointingHandCursor);
    joinLayout->addWidget(joinLabel);
    joinLayout->addStretch();
    joinLayout->addWidget(m_joinChannelBtn);

    m_pinnedBar = new QWidget();
    m_pinnedBar->setObjectName("pinnedBar");
    m_pinnedBar->hide();
    auto* pinnedLayout = new QHBoxLayout(m_pinnedBar);
    pinnedLayout->setContentsMargins(10, 6, 10, 6);
    m_pinnedLabel = new QLabel();
    m_pinnedLabel->setTextInteractionFlags(Qt::NoTextInteraction);
    m_openPinnedBtn = new QPushButton("Go");
    m_openPinnedBtn->setCursor(Qt::PointingHandCursor);
    m_unpinPinnedBtn = new QPushButton("Unpin");
    m_unpinPinnedBtn->setCursor(Qt::PointingHandCursor);
    pinnedLayout->addWidget(m_pinnedLabel);
    pinnedLayout->addStretch();
    pinnedLayout->addWidget(m_openPinnedBtn);
    pinnedLayout->addWidget(m_unpinPinnedBtn);

    m_chatList = new QListWidget();
    m_chatList->setObjectName("chatList");
    m_chatList->setFrameShape(QFrame::NoFrame);
    m_chatList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_chatList->setUniformItemSizes(false);
    m_chatList->setResizeMode(QListView::Adjust);
    m_chatList->setLayoutMode(QListView::Batched);
    m_chatList->setBatchSize(36);
    m_chatList->setSelectionMode(QAbstractItemView::NoSelection);
    m_chatList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_chatList->setSpacing(2);
    m_chatList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_chatList->setMinimumHeight(0);
    m_chatList->setWordWrap(true);

    m_chatList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_chatList, &QListWidget::customContextMenuRequested, this, &MainWindow::onChatListContextMenu);
    connect(m_chatList, &QListWidget::clicked, this, &MainWindow::onChatListItemClicked);
    connect(m_chatList->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::onScrollValueChanged);

    m_replyBar = new QWidget();
    m_replyBar->setObjectName("replyBar");
    m_replyBar->setFixedHeight(40);
    m_replyBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_replyBar->hide();

    QHBoxLayout* replyBarLayout = new QHBoxLayout(m_replyBar);
    replyBarLayout->setContentsMargins(10, 5, 10, 5);

    QVBoxLayout* replyTextLayout = new QVBoxLayout();
    QLabel* replyTitle = new QLabel("Replying to:");
    replyTitle->setStyleSheet("color: #888; font-size: 10px;");
    m_replyLabel = new QLabel();
    m_replyLabel->setStyleSheet("color: #333; font-weight: 500;");
    replyTextLayout->addWidget(replyTitle);
    replyTextLayout->addWidget(m_replyLabel);

    m_cancelReplyBtn = new QPushButton("X");
    m_cancelReplyBtn->setFixedSize(25, 25);
    m_cancelReplyBtn->setCursor(Qt::PointingHandCursor);
    m_cancelReplyBtn->setStyleSheet("border: none; background: transparent; color: #888; font-size: 16px; font-weight: bold;");

    replyBarLayout->addLayout(replyTextLayout);
    replyBarLayout->addStretch();
    replyBarLayout->addWidget(m_cancelReplyBtn);

    connect(m_cancelReplyBtn, &QPushButton::clicked, this, &MainWindow::onCancelReply);

    m_editBar = new QWidget();
    m_editBar->setObjectName("editBar");
    m_editBar->setFixedHeight(35);
    m_editBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_editBar->hide();

    QHBoxLayout* editBarLayout = new QHBoxLayout(m_editBar);
    editBarLayout->setContentsMargins(10, 5, 10, 5);

    m_editLabel = new QLabel("Editing: ");
    m_editLabel->setStyleSheet("color: #888; font-style: italic;");

    m_cancelEditBtn = new QPushButton("X");
    m_cancelEditBtn->setFixedSize(25, 25);
    m_cancelEditBtn->setCursor(Qt::PointingHandCursor);
    m_cancelEditBtn->setStyleSheet("border: none; background: transparent; color: #888; font-size: 16px; font-weight: bold;");

    editBarLayout->addWidget(m_editLabel);
    editBarLayout->addStretch();
    editBarLayout->addWidget(m_cancelEditBtn);

    connect(m_cancelEditBtn, &QPushButton::clicked, this, &MainWindow::onCancelEdit);

    m_inputArea = new QWidget();
    m_inputArea->setObjectName("inputArea");
    m_inputArea->setFixedHeight(60);
    m_inputArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QHBoxLayout* inputLayout = new QHBoxLayout(m_inputArea);

    m_attachBtn = new QPushButton("+");
    m_attachBtn->setObjectName("composerIconButton");
    m_attachBtn->setCursor(Qt::PointingHandCursor);
    m_attachBtn->setEnabled(false);
    m_attachBtn->setFixedWidth(42);
    m_attachBtn->setToolTip("Attach file");
    m_stickerBtn = new QPushButton("S");
    m_stickerBtn->setObjectName("composerIconButton");
    m_stickerBtn->setCursor(Qt::PointingHandCursor);
    m_stickerBtn->setEnabled(false);
    m_stickerBtn->setFixedWidth(42);
    m_stickerBtn->setToolTip("Stickers");
    m_messageInput = new QLineEdit();
    m_messageInput->setObjectName("composerInput");
    m_messageInput->setPlaceholderText("Write a message...");
    m_sendBtn = new QPushButton(">");
    m_sendBtn->setObjectName("composerSendButton");
    m_sendBtn->setCursor(Qt::PointingHandCursor);
    m_sendBtn->setFixedWidth(42);
    m_sendBtn->setToolTip("Send");

    inputLayout->addWidget(m_attachBtn);
    inputLayout->addWidget(m_stickerBtn);
    inputLayout->addWidget(m_messageInput);
    inputLayout->addWidget(m_sendBtn);

    chatAreaLayout->addWidget(header);
    chatAreaLayout->addWidget(m_joinChannelBar);
    chatAreaLayout->addWidget(m_pinnedBar);
    chatAreaLayout->addWidget(m_chatList);
    chatAreaLayout->addWidget(m_replyBar);
    chatAreaLayout->addWidget(m_editBar);
    chatAreaLayout->addWidget(m_inputArea);
    chatAreaLayout->setStretch(0, 0);
    chatAreaLayout->setStretch(1, 0);
    chatAreaLayout->setStretch(2, 0);
    chatAreaLayout->setStretch(3, 1);
    chatAreaLayout->setStretch(4, 0);
    chatAreaLayout->setStretch(5, 0);
    chatAreaLayout->setStretch(6, 0);

    m_stickerPanel = new QFrame(m_chatAreaWidget);
    m_stickerPanel->setObjectName(QStringLiteral("stickerPanel"));
    m_stickerPanel->setFrameShape(QFrame::StyledPanel);
    m_stickerPanel->setFixedSize(328, 396);
    m_stickerPanel->hide();
    auto* stickerRoot = new QVBoxLayout(m_stickerPanel);
    stickerRoot->setContentsMargins(10, 10, 10, 10);
    stickerRoot->setSpacing(8);
    auto* stickerHeader = new QHBoxLayout();
    auto* stickerTitle = new QLabel(QStringLiteral("Stickers"), m_stickerPanel);
    stickerTitle->setObjectName(QStringLiteral("stickerPanelTitle"));
    auto* stickerClose = new QPushButton(QStringLiteral("x"), m_stickerPanel);
    stickerClose->setObjectName(QStringLiteral("stickerPanelCloseBtn"));
    stickerClose->setCursor(Qt::PointingHandCursor);
    stickerClose->setFixedWidth(24);
    stickerHeader->addWidget(stickerTitle);
    stickerHeader->addStretch();
    stickerHeader->addWidget(stickerClose);
    stickerRoot->addLayout(stickerHeader);
    m_stickerScrollArea = new QScrollArea(m_stickerPanel);
    m_stickerScrollArea->setFrameShape(QFrame::NoFrame);
    m_stickerScrollArea->setWidgetResizable(true);
    m_stickerGridHost = new QWidget(m_stickerScrollArea);
    m_stickerGridLayout = new QGridLayout(m_stickerGridHost);
    m_stickerGridLayout->setContentsMargins(0, 0, 0, 0);
    m_stickerGridLayout->setSpacing(6);
    m_stickerGridLayout->addWidget(new QLabel("Loading stickers...", m_stickerGridHost), 0, 0);
    m_stickerScrollArea->setWidget(m_stickerGridHost);
    stickerRoot->addWidget(m_stickerScrollArea, 1);
    connect(stickerClose, &QPushButton::clicked, this, &MainWindow::hideStickerPanel);

    appLayout->addWidget(m_sidebarContainer);
    appLayout->addWidget(m_chatAreaWidget);

    m_contactsOverlay = new QWidget(m_appPage);
    m_contactsOverlay->setObjectName("contactsOverlay");
    m_contactsOverlay->setAttribute(Qt::WA_StyledBackground, true);
    m_contactsOverlay->hide();
    m_contactsPanel = new QFrame(m_contactsOverlay);
    m_contactsPanel->setObjectName("contactsPanel");
    m_contactsPanel->setAttribute(Qt::WA_StyledBackground, true);
    m_contactsPanel->resize(460, 620);
    auto* contactsRoot = new QVBoxLayout(m_contactsPanel);
    contactsRoot->setContentsMargins(0, 0, 0, 0);
    contactsRoot->setSpacing(0);
    auto* contactsHeader = new QWidget(m_contactsPanel);
    contactsHeader->setObjectName("contactsDialogHeader");
    auto* contactsHeaderLayout = new QHBoxLayout(contactsHeader);
    contactsHeaderLayout->setContentsMargins(12, 10, 12, 10);
    contactsHeaderLayout->setSpacing(8);
    auto* contactsTitle = new QLabel("New Chat", contactsHeader);
    contactsTitle->setStyleSheet("font-size: 18px; font-weight: 700;");
    auto* contactsCloseBtn = new QPushButton("x", contactsHeader);
    contactsCloseBtn->setObjectName("contactsDialogCloseBtn");
    contactsCloseBtn->setCursor(Qt::PointingHandCursor);
    contactsCloseBtn->setFixedWidth(28);
    contactsHeaderLayout->addWidget(contactsTitle);
    contactsHeaderLayout->addStretch();
    contactsHeaderLayout->addWidget(contactsCloseBtn);
    contactsRoot->addWidget(contactsHeader);
    auto* contactsSearchWrap = new QWidget(m_contactsPanel);
    auto* contactsSearchLayout = new QHBoxLayout(contactsSearchWrap);
    contactsSearchLayout->setContentsMargins(12, 10, 12, 10);
    m_contactsSearchInput = new QLineEdit(contactsSearchWrap);
    m_contactsSearchInput->setObjectName("contactsSearchInput");
    m_contactsSearchInput->setPlaceholderText("Search contacts...");
    contactsSearchLayout->addWidget(m_contactsSearchInput);
    contactsRoot->addWidget(contactsSearchWrap);
    contactsRoot->addWidget(m_contactListWidget, 1);

    m_sidebarMenuOverlay = new QWidget(m_appPage);
    m_sidebarMenuOverlay->setObjectName("mainMenuOverlay");
    m_sidebarMenuOverlay->hide();
    auto* menuOverlayLayout = new QHBoxLayout(m_sidebarMenuOverlay);
    menuOverlayLayout->setContentsMargins(0, 0, 0, 0);
    menuOverlayLayout->setSpacing(0);
    m_sidebarMenuPanel = new QFrame(m_sidebarMenuOverlay);
    m_sidebarMenuPanel->setObjectName("mainMenuPanel");
    m_sidebarMenuPanel->setFixedWidth(280);
    auto* menuPanelLayout = new QVBoxLayout(m_sidebarMenuPanel);
    menuPanelLayout->setContentsMargins(0, 0, 0, 0);
    menuPanelLayout->setSpacing(0);
    auto* menuHeader = new QWidget(m_sidebarMenuPanel);
    menuHeader->setObjectName("mainMenuHeader");
    auto* menuHeaderLayout = new QHBoxLayout(menuHeader);
    menuHeaderLayout->setContentsMargins(14, 12, 14, 12);
    auto* menuTitle = new QLabel("Menu", menuHeader);
    menuTitle->setStyleSheet("font-size: 20px; font-weight: 700;");
    auto* menuCloseBtn = new QPushButton("x", menuHeader);
    menuCloseBtn->setObjectName("mainMenuCloseBtn");
    menuCloseBtn->setCursor(Qt::PointingHandCursor);
    menuCloseBtn->setFixedWidth(30);
    menuHeaderLayout->addWidget(menuTitle);
    menuHeaderLayout->addStretch();
    menuHeaderLayout->addWidget(menuCloseBtn);
    menuPanelLayout->addWidget(menuHeader);
    auto* menuItemsHost = new QWidget(m_sidebarMenuPanel);
    auto* menuItemsLayout = new QVBoxLayout(menuItemsHost);
    menuItemsLayout->setContentsMargins(12, 12, 12, 12);
    menuItemsLayout->setSpacing(8);
    m_menuContactsBtn = new QPushButton("All Contacts", menuItemsHost);
    m_menuContactsBtn->setObjectName("menuContactsBtn");
    m_menuContactsBtn->setCursor(Qt::PointingHandCursor);
    m_menuContactsBtn->setMinimumHeight(42);
    m_menuSettingsBtn = new QPushButton("Settings", menuItemsHost);
    m_menuSettingsBtn->setObjectName("menuSettingsBtn");
    m_menuSettingsBtn->setCursor(Qt::PointingHandCursor);
    m_menuSettingsBtn->setMinimumHeight(42);
    menuItemsLayout->addWidget(m_menuContactsBtn);
    menuItemsLayout->addWidget(m_menuSettingsBtn);
    menuItemsLayout->addStretch();
    menuPanelLayout->addWidget(menuItemsHost, 1);
    auto* menuFooter = new QLabel("Noveo Messenger", m_sidebarMenuPanel);
    menuFooter->setObjectName("mainMenuFooter");
    menuFooter->setAlignment(Qt::AlignCenter);
    menuFooter->setMinimumHeight(40);
    menuPanelLayout->addWidget(menuFooter);
    menuOverlayLayout->addWidget(m_sidebarMenuPanel);
    menuOverlayLayout->addStretch(1);

    m_settingsOverlay = new QWidget(m_appPage);
    m_settingsOverlay->setObjectName(QStringLiteral("settingsOverlay"));
    m_settingsOverlay->setStyleSheet(QStringLiteral("QWidget#settingsOverlay { background: rgba(15, 23, 42, 120); }"));
    m_settingsOverlay->hide();

    m_stackedWidget->addWidget(m_appPage);
    updateContactsOverlayGeometry();
    updateSidebarMenuGeometry();

    connect(m_loginBtn, &QPushButton::clicked, this, &MainWindow::onLoginBtnClicked);
    connect(m_registerBtn, &QPushButton::clicked, this, &MainWindow::onRegisterBtnClicked);
    connect(m_showRegisterSwitchBtn, &QPushButton::clicked, this, [this]() {
        if (m_authFormsStack) {
            m_authFormsStack->setCurrentIndex(1);
        }
        m_statusLabel->setStyleSheet("color: #6b7280;");
        m_statusLabel->clear();
    });
    connect(m_showLoginSwitchBtn, &QPushButton::clicked, this, [this]() {
        if (m_authFormsStack) {
            m_authFormsStack->setCurrentIndex(0);
        }
        m_statusLabel->setStyleSheet("color: #6b7280;");
        m_statusLabel->clear();
    });
    connect(m_chatListWidget, &QListWidget::itemClicked, this, &MainWindow::onChatSelected);
    connect(m_contactListWidget, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        onContactSelected(item);
        if (m_contactsOverlay) {
            m_contactsOverlay->hide();
        }
    });
    connect(m_sidebarSettingsBtn, &QPushButton::clicked, this, &MainWindow::showSidebarMenu);
    connect(m_sidebarCreateBtn, &QPushButton::clicked, this, &MainWindow::showCreateOptionsMenu);
    connect(m_menuContactsBtn, &QPushButton::clicked, this, [this]() {
        hideSidebarMenu();
        showContactsDialog();
    });
    connect(m_menuSettingsBtn, &QPushButton::clicked, this, [this]() {
        hideSidebarMenu();
        showSettingsDialog();
    });
    connect(menuCloseBtn, &QPushButton::clicked, this, &MainWindow::hideSidebarMenu);
    connect(contactsCloseBtn, &QPushButton::clicked, this, [this]() {
        if (m_contactsOverlay) {
            m_contactsOverlay->hide();
        }
    });
    connect(m_contactsSearchInput, &QLineEdit::textChanged, this, &MainWindow::applyContactsFilter);
    connect(m_chatSettingsBtn, &QPushButton::clicked, this, [this]() {
        openChatSettingsDialog(m_currentChatId);
    });
    connect(m_sendBtn, &QPushButton::clicked, this, &MainWindow::onSendBtnClicked);
    connect(m_stickerBtn, &QPushButton::clicked, this, [this]() {
        openStickerPicker();
    });
    connect(m_joinChannelBtn, &QPushButton::clicked, this, [this]() {
        if (m_currentChatId.isEmpty()) {
            return;
        }
        m_client->joinChannel(m_currentChatId);
        statusBar()->showMessage("Join request sent.", 2000);
    });
    connect(m_openPinnedBtn, &QPushButton::clicked, this, [this]() {
        if (!m_currentChatId.isEmpty() && m_chats.contains(m_currentChatId) && m_chats[m_currentChatId].hasPinnedMessage) {
            focusOnMessage(m_chats[m_currentChatId].pinnedMessage.messageId);
        }
    });
    connect(m_unpinPinnedBtn, &QPushButton::clicked, this, [this]() {
        if (!m_currentChatId.isEmpty()) {
            m_client->unpinMessage(m_currentChatId);
        }
    });
    connect(m_attachBtn, &QPushButton::clicked, this, [this]() {
        if (m_currentChatId.isEmpty()) {
            return;
        }
        const QString selectedFile = QFileDialog::getOpenFileName(this, "Select File to Send");
        if (selectedFile.isEmpty()) {
            return;
        }

        constexpr qint64 kMaxUploadBytes = 100LL * 1024LL * 1024LL;
        const QFileInfo selectedInfo(selectedFile);
        if (selectedInfo.size() > kMaxUploadBytes) {
            statusBar()->showMessage("File exceeds 100MB limit.", 5000);
            return;
        }
        const QString targetChatId = m_currentChatId;
        const QString targetReplyId = m_replyingToMessageId;
        const QString typedText = m_messageInput->text().trimmed();

        statusBar()->showMessage("Uploading file...");
        QNetworkReply* reply = m_restClient->uploadFile("/upload/file", selectedFile);
        if (!reply) {
            statusBar()->showMessage("Could not open selected file.", 4000);
            return;
        }

        connect(reply, &QNetworkReply::finished, this, [this, reply, targetChatId, targetReplyId, typedText]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                statusBar()->showMessage("Upload failed: " + reply->errorString(), 5000);
                return;
            }

            const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (!doc.isObject() || !doc.object().value("success").toBool(false)) {
                statusBar()->showMessage("Upload failed: invalid server response.", 5000);
                return;
            }

            const QJsonObject fileObj = doc.object().value("file").toObject();
            if (fileObj.isEmpty()) {
                statusBar()->showMessage("Upload failed: missing file metadata.", 5000);
                return;
            }

            QJsonObject content;
            content.insert("text", typedText.isEmpty() ? QJsonValue::Null : QJsonValue(typedText));
            content.insert("file", fileObj);
            content.insert("theme", QJsonValue::Null);

            const QString recipientId = resolveRecipientIdForChat(targetChatId);

            m_client->sendMessagePayload(targetChatId, content, targetReplyId, recipientId);

            Message pendingMsg;
            pendingMsg.messageId = "temp_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
            pendingMsg.chatId = targetChatId;
            pendingMsg.senderId = m_client->currentUserId();
            pendingMsg.timestamp = QDateTime::currentSecsSinceEpoch();
            pendingMsg.status = MessageStatus::Pending;
            pendingMsg.pending = true;
            pendingMsg.replyToId = targetReplyId;
            pendingMsg.file.url = fileObj.value("url").toString();
            pendingMsg.file.name = fileObj.value("name").toString();
            pendingMsg.file.type = fileObj.value("type").toString();
            pendingMsg.file.size = static_cast<qint64>(fileObj.value("size").toDouble());
            pendingMsg.text = typedText;

            m_pendingMessages[pendingMsg.messageId] = pendingMsg;
            if (m_currentChatId == targetChatId) {
                addMessageBubble(pendingMsg, false, false);
                smoothScrollToBottom();
            }

            if (m_currentChatId == targetChatId) {
                m_messageInput->clear();
            }
            if (!targetReplyId.isEmpty() && m_currentChatId == targetChatId) {
                onCancelReply();
            }
            statusBar()->showMessage("File sent.", 2000);
        });
    });
    auto* pasteAttachmentShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+V")), m_messageInput);
    connect(pasteAttachmentShortcut, &QShortcut::activated, this, [this]() {
        if (m_currentChatId.isEmpty()) {
            return;
        }
        const QMimeData* mime = QApplication::clipboard()->mimeData();
        if (!mime || !mime->hasImage()) {
            statusBar()->showMessage("Clipboard has no image attachment.", 2500);
            return;
        }

        const QImage image = qvariant_cast<QImage>(mime->imageData());
        if (image.isNull()) {
            statusBar()->showMessage("Clipboard image is invalid.", 3000);
            return;
        }

        QByteArray pngData;
        QBuffer buffer(&pngData);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");
        buffer.close();

        constexpr qint64 kMaxUploadBytes = 100LL * 1024LL * 1024LL;
        if (pngData.size() > kMaxUploadBytes) {
            statusBar()->showMessage("Clipboard image exceeds 100MB limit.", 5000);
            return;
        }

        const QString targetChatId = m_currentChatId;
        const QString targetReplyId = m_replyingToMessageId;
        const QString typedText = m_messageInput->text().trimmed();

        statusBar()->showMessage("Uploading clipboard image...");
        QNetworkReply* reply = m_restClient->uploadBytes("/upload/file", pngData, "clipboard.png", "image/png");
        if (!reply) {
            statusBar()->showMessage("Failed to upload clipboard image.", 4000);
            return;
        }

        connect(reply, &QNetworkReply::finished, this, [this, reply, targetChatId, targetReplyId, typedText]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                statusBar()->showMessage("Upload failed: " + reply->errorString(), 5000);
                return;
            }

            const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (!doc.isObject() || !doc.object().value("success").toBool(false)) {
                statusBar()->showMessage("Upload failed: invalid server response.", 5000);
                return;
            }

            const QJsonObject fileObj = doc.object().value("file").toObject();
            if (fileObj.isEmpty()) {
                statusBar()->showMessage("Upload failed: missing file metadata.", 5000);
                return;
            }

            QJsonObject content;
            content.insert("text", typedText.isEmpty() ? QJsonValue::Null : QJsonValue(typedText));
            content.insert("file", fileObj);
            content.insert("theme", QJsonValue::Null);

            const QString recipientId = resolveRecipientIdForChat(targetChatId);
            m_client->sendMessagePayload(targetChatId, content, targetReplyId, recipientId);

            Message pendingMsg;
            pendingMsg.messageId = "temp_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
            pendingMsg.chatId = targetChatId;
            pendingMsg.senderId = m_client->currentUserId();
            pendingMsg.timestamp = QDateTime::currentSecsSinceEpoch();
            pendingMsg.status = MessageStatus::Pending;
            pendingMsg.pending = true;
            pendingMsg.replyToId = targetReplyId;
            pendingMsg.file.url = fileObj.value("url").toString();
            pendingMsg.file.name = fileObj.value("name").toString();
            pendingMsg.file.type = fileObj.value("type").toString();
            pendingMsg.file.size = static_cast<qint64>(fileObj.value("size").toDouble());
            pendingMsg.text = typedText;

            m_pendingMessages[pendingMsg.messageId] = pendingMsg;
            if (m_currentChatId == targetChatId) {
                addMessageBubble(pendingMsg, false, false);
                smoothScrollToBottom();
            }

            if (m_currentChatId == targetChatId) {
                m_messageInput->clear();
            }
            if (!targetReplyId.isEmpty() && m_currentChatId == targetChatId) {
                onCancelReply();
            }
            statusBar()->showMessage("Clipboard image sent.", 2000);
        });
    });
    connect(m_messageInput, &QLineEdit::returnPressed, this, &MainWindow::onSendBtnClicked);
    connect(m_usernameInput, &QLineEdit::returnPressed, this, &MainWindow::onLoginBtnClicked);
    connect(m_passwordInput, &QLineEdit::returnPressed, this, &MainWindow::onLoginBtnClicked);
    connect(m_registerUsernameInput, &QLineEdit::returnPressed, this, &MainWindow::onRegisterBtnClicked);
    connect(m_registerPasswordInput, &QLineEdit::returnPressed, this, &MainWindow::onRegisterBtnClicked);
    connect(m_messageInput, &QLineEdit::textEdited, this, [this](const QString&) {
        if (!m_currentChatId.isEmpty()) {
            m_client->sendTyping(m_currentChatId);
        }
    });
    connect(m_voiceCallBtn, &QPushButton::clicked, this, [this]() {
        if (m_currentChatId.isEmpty()) {
            return;
        }

        if (m_currentVoiceChatId.isEmpty()) {
            if (!m_voiceAudio->start()) {
                return;
            }
            m_currentVoiceChatId = m_currentChatId;
            m_client->voiceStart(m_currentChatId);
            m_client->voiceJoin(m_currentChatId);
            m_voiceCallBtn->setText("Leave Voice");
            statusBar()->showMessage("Voice chat connected.", 2000);
        } else {
            m_client->voiceLeave(m_currentVoiceChatId);
            m_currentVoiceChatId.clear();
            m_voiceAudio->stop();
            m_voiceCallBtn->setText("Voice");
            statusBar()->showMessage("Voice chat ended.", 2000);
        }
    });

    m_sidebarAudioPlayback = new QMediaPlayer(this);
    connect(m_sidebarPlayBtn, &QPushButton::clicked, this, [this]() {
        if (!m_sidebarAudioPlayback || m_currentAudioUrl.isEmpty()) {
            return;
        }
        if (m_sidebarAudioPlayback->state() == QMediaPlayer::PlayingState) {
            m_sidebarAudioPlayback->pause();
        } else {
            m_sidebarAudioPlayback->play();
        }
    });
    connect(m_sidebarCloseBtn, &QPushButton::clicked, this, [this]() {
        if (m_sidebarAudioPlayback) {
            m_sidebarAudioPlayback->stop();
        }
        if (m_currentAudioSourceWidget) {
            m_currentAudioSourceWidget->setAudioPlaying(false);
        }
        m_currentAudioSourceWidget = nullptr;
        m_currentAudioUrl.clear();
        if (m_sidebarAudioPlayer) {
            m_sidebarAudioPlayer->hide();
        }
    });
    connect(m_sidebarVolumeToggleBtn, &QPushButton::clicked, this, [this]() {
        if (m_sidebarVolumeControl) {
            m_sidebarVolumeControl->setVisible(!m_sidebarVolumeControl->isVisible());
        }
    });
    connect(m_sidebarSeekBar, &QSlider::sliderMoved, this, [this](int value) {
        if (!m_sidebarAudioPlayback) {
            return;
        }
        if (m_sidebarAudioPlayback->duration() > 0) {
            const qint64 pos = static_cast<qint64>((static_cast<double>(value) / 1000.0) * m_sidebarAudioPlayback->duration());
            m_sidebarAudioPlayback->setPosition(pos);
        }
    });
    connect(m_sidebarVolumeBar, &QSlider::valueChanged, this, [this](int value) {
        if (m_sidebarAudioPlayback) {
            m_sidebarAudioPlayback->setVolume(value);
        }
    });
    connect(m_sidebarAudioPlayback, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
        if (m_sidebarDuration) {
            const int sec = static_cast<int>(duration / 1000);
            m_sidebarDuration->setText(QString("%1:%2").arg(sec / 60).arg(sec % 60, 2, 10, QLatin1Char('0')));
        }
    });
    connect(m_sidebarAudioPlayback, &QMediaPlayer::positionChanged, this, [this](qint64 position) {
        if (m_sidebarCurrentTime) {
            const int sec = static_cast<int>(position / 1000);
            m_sidebarCurrentTime->setText(QString("%1:%2").arg(sec / 60).arg(sec % 60, 2, 10, QLatin1Char('0')));
        }
        if (m_sidebarSeekBar && m_sidebarAudioPlayback && m_sidebarAudioPlayback->duration() > 0 && !m_sidebarSeekBar->isSliderDown()) {
            const double ratio = static_cast<double>(position) / static_cast<double>(m_sidebarAudioPlayback->duration());
            m_sidebarSeekBar->setValue(static_cast<int>(ratio * 1000.0));
        }
    });
    connect(m_sidebarAudioPlayback, &QMediaPlayer::stateChanged, this, [this](QMediaPlayer::State state) {
        if (m_sidebarPlayBtn) {
            m_sidebarPlayBtn->setText(state == QMediaPlayer::PlayingState ? "Pause" : "Play");
        }
        if (m_currentAudioSourceWidget) {
            m_currentAudioSourceWidget->setAudioPlaying(state == QMediaPlayer::PlayingState);
        }
    });

    m_mediaViewerDialog = new MediaViewerDialog(this);

    m_settingsDialog = new SettingsDialog(m_settingsOverlay ? m_settingsOverlay : this);
    m_settingsDialog->setWindowFlags(Qt::Widget | Qt::FramelessWindowHint);
    m_settingsDialog->setParent(m_settingsOverlay);
    m_settingsDialog->setAttribute(Qt::WA_StyledBackground, true);
    m_settingsDialog->hide();
    updateSettingsOverlayGeometry();
    connect(m_settingsDialog, &SettingsDialog::saveDisplayNameRequested, this, [this](const QString& newName) {
        if (!newName.trimmed().isEmpty()) {
            m_client->updateUsername(newName.trimmed());
        }
    });
    connect(m_settingsDialog, &SettingsDialog::changePasswordRequested, this, [this]() {
        bool ok = false;
        const QString oldPwd = QInputDialog::getText(this, "Change Password", "Current password:", QLineEdit::Password, "", &ok);
        if (!ok || oldPwd.isEmpty()) {
            return;
        }
        const QString newPwd = QInputDialog::getText(this, "Change Password", "New password:", QLineEdit::Password, "", &ok);
        if (!ok || newPwd.isEmpty()) {
            return;
        }
        m_client->changePassword(oldPwd, newPwd);
    });
    connect(m_settingsDialog, &SettingsDialog::deleteAccountRequested, this, [this]() {
        const auto answer = QMessageBox::warning(this,
                                                 "Delete Account",
                                                 "This action is permanent. Continue?",
                                                 QMessageBox::Yes | QMessageBox::No,
                                                 QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            return;
        }
        bool ok = false;
        const QString pwd = QInputDialog::getText(this, "Delete Account", "Confirm password:", QLineEdit::Password, "", &ok);
        if (!ok || pwd.isEmpty()) {
            return;
        }
        m_client->deleteAccount(pwd);
    });
    connect(m_settingsDialog, &SettingsDialog::logoutRequested, this, &MainWindow::onLogoutClicked);
    connect(m_settingsDialog, &SettingsDialog::darkModeToggled, this, &MainWindow::onDarkModeToggled);
    connect(m_settingsDialog, &SettingsDialog::notificationsToggled, this, &MainWindow::onNotificationsToggled);
    connect(m_settingsDialog, &SettingsDialog::blockGroupInvitesToggled, this, [this](bool checked) {
        const QString selfId = m_client ? m_client->currentUserId() : QString();
        if (!selfId.isEmpty() && m_users.contains(selfId)) {
            m_users[selfId].blockGroupInvites = checked;
        }
        QNetworkReply* reply = m_restClient->updatePrivacy(checked);
        if (!reply) {
            statusBar()->showMessage("Failed to send privacy update.", 3000);
            return;
        }
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                statusBar()->showMessage("Privacy update failed: " + reply->errorString(), 4000);
                return;
            }
            statusBar()->showMessage("Privacy updated.", 2000);
        });
    });
    connect(m_settingsDialog, &SettingsDialog::checkForUpdatesRequested, m_updaterService, &UpdaterService::checkForUpdates);
    connect(m_settingsDialog, &SettingsDialog::downloadUpdateRequested, m_updaterService, &UpdaterService::downloadUpdate);
    connect(m_settingsDialog, &SettingsDialog::installUpdateRequested, m_updaterService, &UpdaterService::restartAndInstall);

    m_chatSettingsDialog = new ChatSettingsDialog(this);
    m_chatSettingsDialog->setDarkMode(m_isDarkMode);
    connect(m_chatSettingsDialog, &ChatSettingsDialog::removeMemberRequested, this, [this](const QString& chatId, const QString& memberId) {
        const auto answer = QMessageBox::question(this,
                                                  "Remove Member",
                                                  QString("Remove %1 from this chat?").arg(memberId),
                                                  QMessageBox::Yes | QMessageBox::No,
                                                  QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            return;
        }
        QJsonObject extra;
        extra.insert("memberId", memberId);
        performChatSettingsAction("remove_member", chatId, extra);
    });
    connect(m_chatSettingsDialog, &ChatSettingsDialog::addMembersRequested, this, [this](const QString& chatId) {
        openAddMembersDialogForChat(chatId);
    });
    connect(m_chatSettingsDialog, &ChatSettingsDialog::removeHandleRequested, this, [this](const QString& chatId) {
        const auto answer = QMessageBox::question(this,
                                                  "Remove Handle",
                                                  "Remove handle for this chat?",
                                                  QMessageBox::Yes | QMessageBox::No,
                                                  QMessageBox::No);
        if (answer == QMessageBox::Yes) {
            performChatSettingsAction("remove_handle", chatId);
        }
    });
    connect(m_chatSettingsDialog, &ChatSettingsDialog::deleteChatRequested, this, [this](const QString& chatId) {
        const auto answer = QMessageBox::warning(this,
                                                 "Delete Chat",
                                                 "Permanently delete this chat?",
                                                 QMessageBox::Yes | QMessageBox::No,
                                                 QMessageBox::No);
        if (answer == QMessageBox::Yes) {
            performChatSettingsAction("delete_chat", chatId);
        }
    });
}

void MainWindow::applyTheme() {
    QString bg = m_isDarkMode ? "#1e1e1e" : "#f5f5f5";
    QString panelBg = m_isDarkMode ? "#2d2d2d" : "#ffffff";
    QString text = m_isDarkMode ? "#ffffff" : "#000000";
    QString border = m_isDarkMode ? "#3d3d3d" : "#e0e0e0";
    QString inputBg = m_isDarkMode ? "#3d3d3d" : "#ffffff";
    QString listHover = m_isDarkMode ? "#3a3a3a" : "#f0f0f0";
    QString scrollHandle = m_isDarkMode ? "#505050" : "#c0c0c0";
    QString scrollHandleHover = m_isDarkMode ? "#606060" : "#a0a0a0";
    QString stickerCloseColor = m_isDarkMode ? "#9ca3af" : "#6b7280";
    QString stickerCloseHover = m_isDarkMode ? "#f3f4f6" : "#111827";

    QString style = QString(
        "QMainWindow { background-color: %1; }"
        "QWidget { color: %3; }"
        "QLineEdit { padding: 10px; border: 1px solid %4; border-radius: 5px; background-color: %5; color: %3; }"
        "QPushButton { padding: 8px 15px; border-radius: 5px; background-color: #0088cc; color: white; border: none; }"
        "QPushButton:hover { background-color: #0077b3; }"
        "QListWidget { border: none; outline: none; }"
        "#sidebarChatList, #contactsModalList { background-color: %2; }"
        "#sidebarChatList::item, #contactsModalList::item { padding: 10px; border-bottom: 1px solid %4; }"
        "#sidebarChatList::item:selected, #contactsModalList::item:selected { background-color: #0088cc; color: white; }"
        "#sidebarChatList::item:hover, #contactsModalList::item:hover { background-color: %6; }"
        "#sidebarHeader { background-color: %2; border-bottom: 1px solid %4; }"
        "#sidebarTitle { font-size: 20px; font-weight: 700; color: %3; }"
        "#sidebarMenuBtn { background: #0ea5e9; color: #ffffff; border: none; border-radius: 9px; font-weight: 700; padding: 6px 12px; }"
        "#sidebarMenuBtn:hover { background: #0284c7; }"
        "#sidebarCreateBtn { background: transparent; border: none; color: %3; border-radius: 8px; font-weight: 700; font-size: 18px; }"
        "#sidebarCreateBtn:hover { background: %6; }"
        "#sidebarAudioPlayer { background-color: %2; border-top: 1px solid %4; }"
        "#chatHeader { background-color: %2; border-bottom: 1px solid %4; }"
        "#joinChannelBar { background-color: %2; border-bottom: 1px solid %4; }"
        "#pinnedBar { background-color: %2; border-bottom: 1px solid %4; }"
        "#inputArea { background-color: %2; border-top: 1px solid %4; }"
        "#mainMenuOverlay { background: rgba(0,0,0,120); }"
        "#mainMenuPanel { background-color: %2; border-right: 1px solid %4; }"
        "#mainMenuHeader { background-color: %2; border-bottom: 1px solid %4; }"
        "#mainMenuCloseBtn { border: none; background: transparent; color: %3; font-weight: 700; }"
        "#mainMenuCloseBtn:hover { color: #ef4444; background: transparent; }"
        "#menuContactsBtn, #menuSettingsBtn { text-align: left; padding: 10px 12px; border-radius: 10px; background: transparent; color: %3; border: 1px solid transparent; }"
        "#menuContactsBtn:hover, #menuSettingsBtn:hover { background: %6; border: 1px solid %4; }"
        "#mainMenuFooter { color: #94a3b8; border-top: 1px solid %4; }"
        "#contactsOverlay { background: rgba(0,0,0,120); }"
        "#contactsPanel { background-color: %2; border: 1px solid %4; border-radius: 12px; }"
        "#contactsDialogHeader { background-color: %2; border-bottom: 1px solid %4; }"
        "#contactsDialogCloseBtn { border: none; background: transparent; color: %3; font-weight: 700; }"
        "#contactsDialogCloseBtn:hover { color: #ef4444; background: transparent; }"
        "#contactsSearchInput { border-radius: 10px; padding: 7px 10px; }"
        "#stickerPanel { background-color: %2; border: 1px solid %4; border-radius: 12px; }"
        "#stickerPanelTitle { font-size: 15px; font-weight: 700; color: %3; }"
        "#stickerPanelCloseBtn { border: none; background: transparent; color: %9; font-weight: 700; }"
        "#stickerPanelCloseBtn:hover { color: %10; }"
        "#chatList { background-color: %1; border: none; padding: 4px 0; }"
        "#chatList::item { border: none; margin: 0px; padding: 0px; background: transparent; }"
        "#chatList::item:hover { background: transparent; }"
        "#chatList::item:selected { background: transparent; }"
        "QScrollBar:vertical { border: none; background: %2; width: 10px; margin: 0px; }"
        "QScrollBar::handle:vertical { background: %7; min-height: 20px; border-radius: 5px; }"
        "QScrollBar::handle:vertical:hover { background: %8; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
        "QCheckBox { color: %3; spacing: 5px; }"
        "QCheckBox::indicator { width: 18px; height: 18px; border: 1px solid %4; border-radius: 4px; background: %5; }"
        "QCheckBox::indicator:checked { background-color: #0088cc; border: 1px solid #0088cc; }"
        "#loginPage { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #dbeafe, stop:0.5 #f8fafc, stop:1 #e0f2fe); }"
        "#authCard { background-color: %2; border: 1px solid %4; border-radius: 14px; }"
        "#authBrandTitle { font-size: 34px; font-weight: 700; margin-bottom: 8px; }"
        "#authTitle { font-size: 28px; font-weight: 700; margin-top: 2px; }"
        "#authSubtitle { color: #6b7280; margin-bottom: 8px; }"
        "#authStatusLabel { color: #6b7280; font-size: 12px; min-height: 20px; }"
        "#authLinkButton { color: #3b82f6; border: none; background: transparent; padding: 4px 0; }"
        "#authLinkButton:hover { color: #2563eb; text-decoration: underline; }"
        "#primaryAuthButton { background-color: #3b82f6; color: white; font-weight: 600; border-radius: 8px; }"
        "#primaryAuthButton:hover { background-color: #2563eb; }"
        "#secondaryAuthButton { background-color: #10b981; color: white; font-weight: 600; border-radius: 8px; }"
        "#secondaryAuthButton:hover { background-color: #059669; }"
        "#composerInput { border-radius: 18px; padding: 8px 12px; }"
        "#composerIconButton { background-color: transparent; border: none; color: %3; font-weight: 700; }"
        "#composerIconButton:hover { background-color: %6; border-radius: 12px; }"
        "#composerSendButton { background-color: #2563eb; color: #ffffff; border-radius: 18px; font-weight: 700; }"
        "#composerSendButton:hover { background-color: #1d4ed8; }"
    ).arg(bg)
     .arg(panelBg)
     .arg(text)
     .arg(border)
     .arg(inputBg)
     .arg(listHover)
     .arg(scrollHandle)
     .arg(scrollHandleHover)
     .arg(stickerCloseColor)
     .arg(stickerCloseHover);

    setStyleSheet(style);
    if (m_settingsOverlay) {
        m_settingsOverlay->setStyleSheet(QStringLiteral("QWidget#settingsOverlay { background: rgba(15, 23, 42, 120); }"));
    }

    for (auto it = m_messageWidgetsById.begin(); it != m_messageWidgetsById.end(); ++it) {
        if (it.value()) {
            it.value()->setTheme(m_isDarkMode);
        }
    }
    m_chatList->viewport()->update();
}

void MainWindow::onScrollValueChanged(int value) {
    if (value == 0 && !m_isLoadingHistory && !m_currentChatId.isEmpty()) {
        if (m_chats.contains(m_currentChatId)) {
            const auto& msgs = m_chats[m_currentChatId].messages;
            if (!msgs.empty()) {
                qint64 oldestTime = msgs.front().timestamp;
                m_isLoadingHistory = true;
                m_client->fetchHistory(m_currentChatId, oldestTime);
            }
        }
    }
}

void MainWindow::onDarkModeToggled(bool checked) {
    m_isDarkMode = checked;
    QSettings settings("Noveo", "MessengerClient");
    settings.setValue("darkMode", m_isDarkMode);
    if (m_settingsDialog) {
        m_settingsDialog->setDarkMode(checked);
    }
    if (m_chatSettingsDialog) {
        m_chatSettingsDialog->setDarkMode(checked);
    }
    applyTheme();
    if (m_stickerGridHost) {
        const QString stickerBtnStyle = m_isDarkMode
                                            ? QStringLiteral("QToolButton { border: 1px solid #4b5563; border-radius: 8px; background: #1f2937; color: #f3f4f6; }"
                                                             "QToolButton:hover { border-color: #60a5fa; background: #111827; }")
                                            : QStringLiteral("QToolButton { border: 1px solid #d1d5db; border-radius: 8px; background: #ffffff; }"
                                                             "QToolButton:hover { border-color: #60a5fa; background: #eff6ff; }");
        const auto stickerButtons = m_stickerGridHost->findChildren<QToolButton*>();
        for (QToolButton* btn : stickerButtons) {
            if (btn) {
                btn->setStyleSheet(stickerBtnStyle);
            }
        }
    }

}

void MainWindow::onLogoutClicked() {
    m_manualDisconnect = true;
    if (m_reconnectTimer) {
        m_reconnectTimer->stop();
    }
    m_reconnectAttempts = 0;
    m_waitingForSessionReconnectResult = false;
    m_hasAuthenticatedSession = false;
    m_blockAutoSessionReconnect = false;
    if (!m_currentVoiceChatId.isEmpty()) {
        m_client->voiceLeave(m_currentVoiceChatId);
    }
    m_currentVoiceChatId.clear();
    if (m_voiceAudio) {
        m_voiceAudio->stop();
    }
    if (m_voiceCallBtn) {
        m_voiceCallBtn->setText("Voice");
        m_voiceCallBtn->setEnabled(false);
    }
    if (m_attachBtn) {
        m_attachBtn->setEnabled(false);
    }
    if (m_stickerBtn) {
        m_stickerBtn->setEnabled(false);
    }
    if (m_joinChannelBar) {
        m_joinChannelBar->hide();
    }
    if (m_pinnedBar) {
        m_pinnedBar->hide();
    }
    hideStickerPanel();
    if (m_sidebarAudioPlayback) {
        m_sidebarAudioPlayback->stop();
    }
    if (m_sidebarAudioPlayer) {
        m_sidebarAudioPlayer->hide();
    }
    m_currentAudioUrl.clear();
    if (m_currentAudioSourceWidget) {
        m_currentAudioSourceWidget->setAudioPlaying(false);
    }
    m_currentAudioSourceWidget = nullptr;
    if (m_settingsDialog) {
        m_settingsDialog->hide();
    }
    hideSidebarMenu();
    if (m_contactsOverlay) {
        m_contactsOverlay->hide();
    }
    if (m_settingsOverlay) {
        m_settingsOverlay->hide();
    }
    if (m_chatSettingsDialog) {
        m_chatSettingsDialog->hide();
    }

    SessionStore::clear();
    m_authToken.clear();
    m_authExpiresAt = 0;
    m_restClient->clearAuthContext();

    m_usernameInput->clear();
    m_passwordInput->clear();
    if (m_registerUsernameInput) {
        m_registerUsernameInput->clear();
    }
    if (m_registerPasswordInput) {
        m_registerPasswordInput->clear();
    }
    if (m_authFormsStack) {
        m_authFormsStack->setCurrentIndex(0);
    }
    m_chats.clear();
    m_users.clear();
    m_stickerCache.clear();
    m_currentChatId.clear();
    m_isLoadingHistory = false;
    m_avatarCache.clear();
    m_pendingDownloads.clear();

    m_chatListWidget->clear();
    m_contactListWidget->clear();
    m_chatList->clear();
    m_messageItemsById.clear();
    m_messageWidgetsById.clear();
    m_currentMessagePreviewById.clear();
    m_currentMessageSenderById.clear();
    m_lastMessageViewportWidth = -1;
    m_chatTitle->setText("Select a chat");
    if (m_chatSettingsBtn) {
        m_chatSettingsBtn->setVisible(false);
    }

    m_loginBtn->setEnabled(false);
    m_statusLabel->setStyleSheet("color: #6b7280;");
    m_statusLabel->setText("Resetting connection...");

    m_client->logout();
    m_stackedWidget->setCurrentWidget(m_loginPage);
    QTimer::singleShot(500, this, [this]() {
        m_manualDisconnect = false;
        m_client->connectToServer();
    });
}

void MainWindow::tryReconnectWithSavedSession()
{
    if (m_waitingForSessionReconnectResult || m_blockAutoSessionReconnect) {
        return;
    }

    const SessionData session = SessionStore::load();
    if (!session.isPresent()) {
        return;
    }

    m_waitingForSessionReconnectResult = true;
    m_statusLabel->setText("Reconnecting session...");
    m_client->reconnectWithToken(session.userId, session.token);
}

void MainWindow::onDisconnected()
{
    if (m_manualDisconnect) {
        return;
    }

    if (m_waitingForSessionReconnectResult) {
        m_waitingForSessionReconnectResult = false;
        m_hasAuthenticatedSession = false;
        m_blockAutoSessionReconnect = true;
        SessionStore::clear();
        m_restClient->clearAuthContext();
        m_authToken.clear();
        m_authExpiresAt = 0;
        m_stackedWidget->setCurrentWidget(m_loginPage);
        m_statusLabel->setStyleSheet("color: #ef4444;");
        m_statusLabel->setText("Saved session expired. Please log in again.");
        if (m_sidebarTitleLabel) {
            m_sidebarTitleLabel->setText("Noveo");
        }
        hideSidebarMenu();
        if (m_contactsOverlay) {
            m_contactsOverlay->hide();
        }
        hideStickerPanel();
        if (m_settingsDialog) {
            m_settingsDialog->hide();
        }
        if (m_settingsOverlay) {
            m_settingsOverlay->hide();
        }
        if (m_chatSettingsDialog) {
            m_chatSettingsDialog->hide();
        }
        m_loginBtn->setEnabled(true);
        m_registerBtn->setEnabled(true);
        if (m_showRegisterSwitchBtn) {
            m_showRegisterSwitchBtn->setEnabled(true);
        }
        if (m_showLoginSwitchBtn) {
            m_showLoginSwitchBtn->setEnabled(true);
        }
        return;
    }

    const SessionData session = SessionStore::load();
    if (!session.isPresent()) {
        m_statusLabel->setStyleSheet("color: #6b7280;");
        m_statusLabel->setText("Disconnected.");
        if (m_sidebarTitleLabel) {
            m_sidebarTitleLabel->setText("Connecting...");
        }
        hideSidebarMenu();
        if (m_contactsOverlay) {
            m_contactsOverlay->hide();
        }
        hideStickerPanel();
        if (m_settingsDialog) {
            m_settingsDialog->hide();
        }
        if (m_settingsOverlay) {
            m_settingsOverlay->hide();
        }
        if (m_chatSettingsDialog) {
            m_chatSettingsDialog->hide();
        }
        return;
    }

    m_reconnectAttempts++;
    if (m_reconnectAttempts > 5) {
        m_statusLabel->setStyleSheet("color: #ef4444;");
        m_statusLabel->setText("Connection lost. Please log in again.");
        SessionStore::clear();
        m_restClient->clearAuthContext();
        m_authToken.clear();
        m_authExpiresAt = 0;
        m_hasAuthenticatedSession = false;
        m_stackedWidget->setCurrentWidget(m_loginPage);
        hideSidebarMenu();
        if (m_contactsOverlay) {
            m_contactsOverlay->hide();
        }
        if (m_chatSettingsDialog) {
            m_chatSettingsDialog->hide();
        }
        m_loginBtn->setEnabled(true);
        m_registerBtn->setEnabled(true);
        if (m_showRegisterSwitchBtn) {
            m_showRegisterSwitchBtn->setEnabled(true);
        }
        if (m_showLoginSwitchBtn) {
            m_showLoginSwitchBtn->setEnabled(true);
        }
        return;
    }

    const int delayMs = qMin(1000 * (1 << m_reconnectAttempts), 30000);
    m_statusLabel->setStyleSheet("color: #6b7280;");
    m_statusLabel->setText(QString("Connection lost. Reconnecting in %1s...")
                               .arg(QString::number(delayMs / 1000)));
    if (m_sidebarTitleLabel) {
        m_sidebarTitleLabel->setText("Connecting...");
    }
    if (m_reconnectTimer) {
        m_reconnectTimer->start(delayMs);
    }
}

void MainWindow::onConnected() {
    if (m_reconnectTimer) {
        m_reconnectTimer->stop();
    }
    m_manualDisconnect = false;
    m_reconnectAttempts = 0;
    m_statusLabel->setStyleSheet("color: #6b7280;");
    m_statusLabel->setText("Connected.");
    if (m_sidebarTitleLabel) {
        m_sidebarTitleLabel->setText("Noveo");
    }
    m_loginBtn->setEnabled(true);
    m_registerBtn->setEnabled(true);
    if (m_showRegisterSwitchBtn) {
        m_showRegisterSwitchBtn->setEnabled(true);
    }
    if (m_showLoginSwitchBtn) {
        m_showLoginSwitchBtn->setEnabled(true);
    }
    tryReconnectWithSavedSession();
}

void MainWindow::onLoginBtnClicked() {
    QString user = m_usernameInput->text().trimmed();
    QString pass = m_passwordInput->text();

    if (user.isEmpty() || pass.isEmpty()) {
        m_statusLabel->setStyleSheet("color: #ef4444;");
        m_statusLabel->setText("Username and password are required.");
        return;
    }

    m_waitingForSessionReconnectResult = false;
    m_blockAutoSessionReconnect = false;
    m_statusLabel->setStyleSheet("color: #6b7280;");
    m_statusLabel->setText("Logging in...");
    m_loginBtn->setEnabled(false);
    m_registerBtn->setEnabled(false);
    if (m_showRegisterSwitchBtn) {
        m_showRegisterSwitchBtn->setEnabled(false);
    }
    if (m_showLoginSwitchBtn) {
        m_showLoginSwitchBtn->setEnabled(false);
    }
    m_client->login(user, pass);
}

void MainWindow::onRegisterBtnClicked()
{
    const QString user = m_registerUsernameInput->text().trimmed();
    const QString pass = m_registerPasswordInput->text();
    if (user.isEmpty() || pass.isEmpty()) {
        m_statusLabel->setStyleSheet("color: #ef4444;");
        m_statusLabel->setText("Username and password are required.");
        return;
    }
    if (pass.size() < 4) {
        m_statusLabel->setStyleSheet("color: #ef4444;");
        m_statusLabel->setText("Password must be at least 4 characters.");
        return;
    }

    m_waitingForSessionReconnectResult = false;
    m_blockAutoSessionReconnect = false;
    m_statusLabel->setStyleSheet("color: #6b7280;");
    m_statusLabel->setText("Registering...");
    m_loginBtn->setEnabled(false);
    m_registerBtn->setEnabled(false);
    if (m_showRegisterSwitchBtn) {
        m_showRegisterSwitchBtn->setEnabled(false);
    }
    if (m_showLoginSwitchBtn) {
        m_showLoginSwitchBtn->setEnabled(false);
    }
    m_client->registerUser(user, pass);
}

void MainWindow::onLoginSuccess(const User& user, const QString& token, qint64 expiresAt) {
    m_manualDisconnect = false;
    m_reconnectAttempts = 0;
    m_waitingForSessionReconnectResult = false;
    m_hasAuthenticatedSession = true;
    m_blockAutoSessionReconnect = false;
    if (m_reconnectTimer) {
        m_reconnectTimer->stop();
    }
    if (expiresAt <= 0) {
        expiresAt = QDateTime::currentSecsSinceEpoch() + 7 * 24 * 60 * 60;
    }
    m_authToken = token;
    m_authExpiresAt = expiresAt;
    SessionStore::save(SessionData{m_client->currentUserId(), token, expiresAt});
    m_restClient->setAuthContext(m_client->currentUserId(), token);
    if (m_settingsDialog) {
        m_settingsDialog->setBlockGroupInvites(user.blockGroupInvites);
        m_settingsDialog->setDarkMode(m_isDarkMode);
        m_settingsDialog->setNotifications(m_notificationsEnabled);
    }
    m_statusLabel->setStyleSheet("color: #6b7280;");
    m_statusLabel->setText("Authenticated");
    if (m_authFormsStack) {
        m_authFormsStack->setCurrentIndex(0);
    }
    if (m_registerPasswordInput) {
        m_registerPasswordInput->clear();
    }
    m_loginBtn->setEnabled(true);
    m_registerBtn->setEnabled(true);
    if (m_showRegisterSwitchBtn) {
        m_showRegisterSwitchBtn->setEnabled(true);
    }
    if (m_showLoginSwitchBtn) {
        m_showLoginSwitchBtn->setEnabled(true);
    }
    m_stackedWidget->setCurrentWidget(m_appPage);
    updateComposerStateForCurrentChat();
    updatePinnedMessageBar();
}

void MainWindow::onAuthFailed(const QString& msg) {
    if (m_reconnectTimer) {
        m_reconnectTimer->stop();
    }
    const bool reconnectFailure = m_waitingForSessionReconnectResult;
    m_reconnectAttempts = 0;
    m_waitingForSessionReconnectResult = false;
    m_hasAuthenticatedSession = false;
    m_blockAutoSessionReconnect = reconnectFailure;
    SessionStore::clear();
    m_restClient->clearAuthContext();
    m_authToken.clear();
    m_authExpiresAt = 0;
    m_currentChatId.clear();
    m_stickerCache.clear();
    hideStickerPanel();
    if (m_settingsDialog) {
        m_settingsDialog->hide();
    }
    hideSidebarMenu();
    if (m_contactsOverlay) {
        m_contactsOverlay->hide();
    }
    if (m_settingsOverlay) {
        m_settingsOverlay->hide();
    }
    if (m_chatSettingsDialog) {
        m_chatSettingsDialog->hide();
    }
    if (m_chatList) {
        m_chatList->clear();
    }
    m_messageItemsById.clear();
    m_messageWidgetsById.clear();
    m_currentMessagePreviewById.clear();
    m_currentMessageSenderById.clear();
    m_lastMessageViewportWidth = -1;
    updateComposerStateForCurrentChat();
    updatePinnedMessageBar();
    m_statusLabel->setStyleSheet("color: #ef4444;");
    m_stackedWidget->setCurrentWidget(m_loginPage);
    m_statusLabel->setText(reconnectFailure ? "Saved session expired. Please log in again." : "Auth failed: " + msg);
    m_loginBtn->setEnabled(true);
    m_registerBtn->setEnabled(true);
    if (m_showRegisterSwitchBtn) {
        m_showRegisterSwitchBtn->setEnabled(true);
    }
    if (m_showLoginSwitchBtn) {
        m_showLoginSwitchBtn->setEnabled(true);
    }
}

void MainWindow::onUserListUpdated(const std::vector<User>& users) {
    m_users.clear();
    m_contactListWidget->clear();

    std::vector<User> sortedUsers = users;
    std::sort(sortedUsers.begin(), sortedUsers.end(), [](const User& a, const User& b) {
        return a.username.toLower() < b.username.toLower();
    });

    for (const auto& u : sortedUsers) {
        m_users.insert(u.userId, u);
        if (u.userId == m_client->currentUserId()) continue;

        QListWidgetItem* item = new QListWidgetItem(m_contactListWidget);
        item->setText(u.username);
        item->setData(Qt::UserRole, u.userId);

        QString fullUrl = u.avatarUrl;
        if (!fullUrl.startsWith("http")) {
            fullUrl = API_BASE_URL + fullUrl;
        }
        item->setData(AvatarUrlRole, fullUrl);
        item->setIcon(getAvatar(u.username, fullUrl));
    }

    for (int i = 0; i < m_chatListWidget->count(); i++) {
        QListWidgetItem* item = m_chatListWidget->item(i);
        QString chatId = item->data(Qt::UserRole).toString();
        if (m_chats.contains(chatId)) {
            QString name = resolveChatName(m_chats[chatId]);
            QString fullUrl = m_chats[chatId].avatarUrl;
            if (!fullUrl.startsWith("http")) {
                 fullUrl = API_BASE_URL + fullUrl;
            }
            item->setText(name);
            item->setData(AvatarUrlRole, fullUrl);
            item->setIcon(getAvatar(name, fullUrl));
        }
    }

    if (m_settingsDialog && m_users.contains(m_client->currentUserId())) {
        m_settingsDialog->setBlockGroupInvites(m_users[m_client->currentUserId()].blockGroupInvites);
    }
    if (!m_currentChatId.isEmpty() && m_chats.contains(m_currentChatId)) {
        renderMessages(m_currentChatId);
    }
}

void MainWindow::onChatHistoryReceived(const std::vector<Chat>& incomingChats) {
    bool initialLoad = m_chats.isEmpty();

    for (const auto& inChat : incomingChats) {
        if (m_chats.contains(inChat.chatId)) {
            Chat& existingChat = m_chats[inChat.chatId];
            QSet<QString> existingIds;
            for (const auto& m : existingChat.messages) existingIds.insert(m.messageId);

            std::vector<Message> newMessages;
            for (const auto& m : inChat.messages) {
                if (!existingIds.contains(m.messageId)) {
                    newMessages.push_back(m);
                }
            }

            if (!newMessages.empty()) {
                existingChat.messages.insert(existingChat.messages.end(), newMessages.begin(), newMessages.end());
                std::sort(existingChat.messages.begin(), existingChat.messages.end(), [](const Message& a, const Message& b) {
                    return a.timestamp < b.timestamp;
                });

                if (m_currentChatId == inChat.chatId && m_isLoadingHistory) {
                    m_isLoadingHistory = false;

                    std::sort(newMessages.begin(), newMessages.end(), [](const Message& a, const Message& b) {
                        return a.timestamp < b.timestamp;
                    });

                    int oldMax = m_chatList->verticalScrollBar()->maximum();
                    m_chatList->setUpdatesEnabled(false);
                    for (auto it = newMessages.rbegin(); it != newMessages.rend(); ++it) {
                        prependMessageBubble(*it);
                    }
                    m_chatList->setUpdatesEnabled(true);
                    int newMax = m_chatList->verticalScrollBar()->maximum();
                    m_chatList->verticalScrollBar()->setValue(newMax - oldMax);
                }
            }
        } else {
            m_chats.insert(inChat.chatId, inChat);
        }
    }

    if (initialLoad) {
        m_chatListWidget->clear();
        std::vector<Chat> sortedChats;
        for (auto k : m_chats.keys()) sortedChats.push_back(m_chats[k]);

        std::sort(sortedChats.begin(), sortedChats.end(), [](const Chat& a, const Chat& b) {
            qint64 timeA = a.messages.empty() ? 0 : a.messages.back().timestamp;
            qint64 timeB = b.messages.empty() ? 0 : b.messages.back().timestamp;
            return timeA > timeB;
        });

        for (const auto& chat : sortedChats) {
            QListWidgetItem* item = new QListWidgetItem(m_chatListWidget);
            QString name = resolveChatName(chat);
            QString url = chat.avatarUrl;

            if (chat.chatType == "private" && url.isEmpty()) {
                for (const auto& memberId : chat.members) {
                    if (memberId != m_client->currentUserId() && m_users.contains(memberId)) {
                        url = m_users[memberId].avatarUrl;
                        break;
                    }
                }
            }

            if (url.startsWith("/")) url = API_BASE_URL + url;

            item->setText(name);
            item->setData(Qt::UserRole, chat.chatId);
            item->setData(AvatarUrlRole, url);
            item->setIcon(getAvatar(name, url));
            m_chatListWidget->addItem(item);
        }
    }

    if (!m_currentChatId.isEmpty()) {
        updateComposerStateForCurrentChat();
        updatePinnedMessageBar();
    }
    if (m_isLoadingHistory) {
        m_isLoadingHistory = false;
    }
}

void MainWindow::onNewChatCreated(const Chat& chat) {
    if (!m_chats.contains(chat.chatId)) {
        m_chats.insert(chat.chatId, chat);
        QListWidgetItem* item = new QListWidgetItem(m_chatListWidget);

        QString name = resolveChatName(chat);
        QString url = chat.avatarUrl;
        if (chat.chatType == "private" && url.isEmpty()) {
            for (const auto& memberId : chat.members) {
                if (memberId != m_client->currentUserId() && m_users.contains(memberId)) {
                    url = m_users[memberId].avatarUrl;
                    break;
                }
            }
        }
        if (url.startsWith("/")) url = API_BASE_URL + url;

        item->setText(name);
        item->setData(Qt::UserRole, chat.chatId);
        item->setData(AvatarUrlRole, url);
        item->setIcon(getAvatar(name, url));
        m_chatListWidget->insertItem(0, item);
    }
    if (m_currentChatId == chat.chatId) {
        updateComposerStateForCurrentChat();
        updatePinnedMessageBar();
    }
}

QString MainWindow::resolveChatName(const Chat& chat) {
    if (!chat.chatName.isEmpty()) return chat.chatName;
    if (chat.chatType == "private") {
        for (const auto& memberId : chat.members) {
            if (memberId != m_client->currentUserId()) {
                if (m_users.contains(memberId)) return m_users[memberId].username;
            }
        }
        return "Unknown User";
    }
    return "Chat";
}

QColor MainWindow::getColorForName(const QString& name) {
    unsigned int hash = 0;
    QByteArray bytes = name.toUtf8();
    for (char c : bytes) hash = c + (hash << 6) + (hash << 16) - hash;

    static const QList<QColor> colors = {
        QColor("#e57373"), QColor("#f06292"), QColor("#ba68c8"), QColor("#9575cd"),
        QColor("#7986cb"), QColor("#64b5f6"), QColor("#4fc3f7"), QColor("#4dd0e1"),
        QColor("#4db6ac"), QColor("#81c784"), QColor("#aed581"), QColor("#ff8a65"),
        QColor("#d4e157"), QColor("#ffd54f"), QColor("#ffb74d"), QColor("#a1887f")
    };
    return colors[hash % colors.size()];
}

QIcon MainWindow::generateGenericAvatar(const QString& name) {
    QPixmap pixmap(42, 42);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setBrush(getColorForName(name));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, 42, 42);

    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPixelSize(20);
    font.setBold(true);
    painter.setFont(font);

    QString letter = name.isEmpty() ? "?" : name.left(1).toUpper();
    painter.drawText(QRect(0, 0, 42, 42), Qt::AlignCenter, letter);

    return QIcon(pixmap);
}

QIcon MainWindow::getAvatar(const QString& name, const QString& urlIn) {
    if (urlIn.isEmpty() || urlIn == "default.png" || urlIn == "/default.png" || urlIn.endsWith("/default.png")) {
        return generateGenericAvatar(name);
    }

    QString fullUrl = urlIn;
    if (fullUrl.startsWith("/")) {
        fullUrl = API_BASE_URL + fullUrl;
    } else if (!fullUrl.startsWith("http://") && !fullUrl.startsWith("https://")) {
        fullUrl = API_BASE_URL + "/" + fullUrl;
    }

    if (m_avatarCache.contains(fullUrl)) {
        return QIcon(m_avatarCache[fullUrl]);
    }

    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/avatars";
    QDir().mkpath(cacheDir);
    QString urlHash = QString(QCryptographicHash::hash(fullUrl.toUtf8(), QCryptographicHash::Md5).toHex());
    QString cachedFilePath = cacheDir + "/" + urlHash + ".png";

    if (QFile::exists(cachedFilePath)) {
        QPixmap pixmap;
        if (pixmap.load(cachedFilePath)) {
            m_avatarCache.insert(fullUrl, pixmap);
            return QIcon(pixmap);
        }
    }

    if (!m_pendingDownloads.contains(fullUrl)) {
        m_pendingDownloads.insert(fullUrl);
        QUrl qurl(fullUrl);
        QNetworkRequest request(qurl);
        request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
        request.setTransferTimeout(60000);

        QNetworkReply* reply = m_nam->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, fullUrl, name]() {
            reply->deleteLater();
            m_pendingDownloads.remove(fullUrl);

            if (reply->error() == QNetworkReply::NoError) {
                QByteArray data = reply->readAll();
                QPixmap pixmap;
                if (pixmap.loadFromData(data)) {
                    QPixmap circular(42, 42);
                    circular.fill(Qt::transparent);
                    QPainter p(&circular);
                    p.setRenderHint(QPainter::Antialiasing);
                    QPainterPath path;
                    path.addEllipse(0, 0, 42, 42);
                    p.setClipPath(path);

                    int sourceSize = qMin(pixmap.width(), pixmap.height());
                    int x = (pixmap.width() - sourceSize) / 2;
                    int y = (pixmap.height() - sourceSize) / 2;
                    QPixmap cropped = pixmap.copy(x, y, sourceSize, sourceSize);

                    p.drawPixmap(0, 0, 42, 42, cropped.scaled(42, 42, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    m_avatarCache.insert(fullUrl, circular);

                    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/avatars";
                    QString urlHash = QString(QCryptographicHash::hash(fullUrl.toUtf8(), QCryptographicHash::Md5).toHex());
                    QString cachedFilePath = cacheDir + "/" + urlHash + ".png";
                    circular.save(cachedFilePath, "PNG");

                    updateAvatarOnItems(fullUrl, circular);
                }
            } else {
            }
        });
    }

    return generateGenericAvatar(name);
}

void MainWindow::updateAvatarOnItems(const QString& url, const QPixmap& pixmap) {
    QIcon icon(pixmap);
    auto updateList = [&](QListWidget* list) {
        bool updated = false;
        for (int i = 0; i < list->count(); i++) {
            QListWidgetItem* item = list->item(i);
            if (item && item->data(AvatarUrlRole).toString() == url) {
                item->setIcon(icon);
                updated = true;
            }
        }
        if (updated) {
            list->viewport()->update();
        }
    };

    updateList(m_chatListWidget);
    updateList(m_contactListWidget);
    updateList(m_chatList);
    for (auto it = m_messageWidgetsById.begin(); it != m_messageWidgetsById.end(); ++it) {
        MessageItemWidget* widget = it.value();
        if (widget && widget->representsSenderAvatarUrl(url)) {
            widget->setSenderAvatar(pixmap);
        }
    }
}

void MainWindow::scrollToBottom() {
    if (m_chatList->count() > 0)
        m_chatList->scrollToBottom();
}

void MainWindow::smoothScrollToBottom() {
    if (m_chatList->count() > 0)
        m_chatList->scrollToBottom();
}

bool MainWindow::isScrolledToBottom() const {
    QScrollBar* sb = m_chatList->verticalScrollBar();
    return sb->value() >= (sb->maximum() - 50);
}

void MainWindow::onChatSelected(QListWidgetItem* item) {
    QString chatId = item->data(Qt::UserRole).toString();
    m_currentChatId = chatId;
    m_highlightedMessageId.clear();
    hideStickerPanel();
    hideSidebarMenu();
    if (m_contactsOverlay) {
        m_contactsOverlay->hide();
    }
    if (m_chatSettingsDialog && m_chatSettingsDialog->isVisible() && m_chatSettingsDialog->chatId() != chatId) {
        m_chatSettingsDialog->hide();
    }

    if (m_chats.contains(chatId)) {
        Chat chat = m_chats[chatId];
        m_chatTitle->setText(resolveChatName(chat));
        const bool isOwnerSettingsChat = (chat.ownerId == m_client->currentUserId()) &&
                                         (chat.chatType == "group" || chat.chatType == "channel");
        if (m_chatSettingsBtn) {
            m_chatSettingsBtn->setVisible(isOwnerSettingsChat);
        }
        if (!m_currentVoiceChatId.isEmpty() && m_currentVoiceChatId != chatId) {
            m_voiceCallBtn->setEnabled(false);
            m_voiceCallBtn->setText("In Other Call");
        } else {
            m_voiceCallBtn->setEnabled(chat.chatType != "channel");
            m_voiceCallBtn->setText(m_currentVoiceChatId == chatId ? "Leave Voice" : "Voice");
        }
        updateComposerStateForCurrentChat();
        updatePinnedMessageBar();
        renderMessages(chatId);
    }
}

void MainWindow::onContactSelected(QListWidgetItem* item) {
    QString userId = item->data(Qt::UserRole).toString();
    QString selfId = m_client->currentUserId();
    hideStickerPanel();
    hideSidebarMenu();
    if (m_contactsOverlay) {
        m_contactsOverlay->hide();
    }
    if (m_chatSettingsDialog && m_chatSettingsDialog->isVisible()) {
        m_chatSettingsDialog->hide();
    }
    QStringList ids = { userId, selfId };
    ids.sort();
    QString potentialChatId = ids.join("_");
    if (m_chatSettingsBtn) {
        m_chatSettingsBtn->setVisible(false);
    }

    if (m_chats.contains(potentialChatId)) {
        m_currentChatId = potentialChatId;
        m_highlightedMessageId.clear();
        m_chatTitle->setText(resolveChatName(m_chats[potentialChatId]));
        if (!m_currentVoiceChatId.isEmpty() && m_currentVoiceChatId != potentialChatId) {
            m_voiceCallBtn->setEnabled(false);
            m_voiceCallBtn->setText("In Other Call");
        } else {
            m_voiceCallBtn->setEnabled(true);
            m_voiceCallBtn->setText(m_currentVoiceChatId == potentialChatId ? "Leave Voice" : "Voice");
        }
        updateComposerStateForCurrentChat();
        updatePinnedMessageBar();
        renderMessages(potentialChatId);
    } else {
        m_currentChatId = potentialChatId;
        m_highlightedMessageId.clear();
        m_chatTitle->setText(item->text());
        if (!m_currentVoiceChatId.isEmpty() && m_currentVoiceChatId != potentialChatId) {
            m_voiceCallBtn->setEnabled(false);
            m_voiceCallBtn->setText("In Other Call");
        } else {
            m_voiceCallBtn->setEnabled(true);
            m_voiceCallBtn->setText(m_currentVoiceChatId == potentialChatId ? "Leave Voice" : "Voice");
        }
        updateComposerStateForCurrentChat();
        updatePinnedMessageBar();
        m_chatList->clear();
        m_currentAudioSourceWidget = nullptr;
        m_messageItemsById.clear();
        m_messageWidgetsById.clear();
        m_currentMessagePreviewById.clear();
        m_currentMessageSenderById.clear();
        m_lastMessageViewportWidth = -1;
    }
}

QString MainWindow::resolveRecipientIdForChat(const QString& chatId) const
{
    if (chatId.startsWith("temp_")) {
        return chatId.mid(5);
    }
    if (chatId.contains('_')) {
        const QStringList ids = chatId.split('_', Qt::SkipEmptyParts);
        if (ids.size() == 2) {
            return (ids[0] == m_client->currentUserId()) ? ids[1] : ids[0];
        }
    }
    return QString();
}

QString MainWindow::resolveFileUrl(const QString& rawUrl) const
{
    if (rawUrl.isEmpty()) {
        return QString();
    }
    if (rawUrl.startsWith("http://") || rawUrl.startsWith("https://")) {
        return rawUrl;
    }
    if (rawUrl.startsWith('/')) {
        return API_BASE_URL + rawUrl;
    }
    return API_BASE_URL + "/" + rawUrl;
}

QString MainWindow::displayTextForMessage(const Message& msg) const
{
    FileAttachment effectiveFile;
    QString text = extractRenderableMessageText(msg, &effectiveFile);

    if (text.isEmpty() && !effectiveFile.isNull()) {
        const QString type = effectiveFile.type.toLower();
        const QString name = effectiveFile.name.isEmpty() ? QStringLiteral("Attachment") : effectiveFile.name;
        if (type.startsWith("image/")) {
            text = QStringLiteral("[Image]");
        } else if (type.startsWith("audio/")) {
            text = QStringLiteral("[Audio] %1").arg(name);
        } else if (type.startsWith("video/")) {
            text = QStringLiteral("[Video] %1").arg(name);
        } else {
            text = QStringLiteral("[File] %1").arg(name);
        }
    } else if (!effectiveFile.isNull()) {
        text += QStringLiteral(" [Attachment]");
    }

    if (text.isEmpty()) {
        text = QStringLiteral("[Message]");
    }

    QString trimmed = text.trimmed();
    if (trimmed.startsWith("http") && (trimmed.contains("/static/upload") || trimmed.contains("/static/upl"))) {
        QStringList words = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (words.size() == 1) {
            if (trimmed.contains(".")) {
                QStringList parts = trimmed.split("/");
                QString filename = parts.last();
                if (filename.contains(".") &&
                    !filename.endsWith(".png") && !filename.endsWith(".jpg") &&
                    !filename.endsWith(".jpeg") && !filename.endsWith(".gif") &&
                    !filename.endsWith(".webp")) {
                    text = "[" + filename + "]";
                } else {
                    text = "[Image]";
                }
            } else {
                text = "[Image]";
            }
        } else if (words.size() == 2 && words.last().startsWith("http")) {
            text = words.first() + " [Image]";
        }
    }

    if (!msg.forwardedInfo.isNull()) {
        text = QStringLiteral("Forwarded: %1").arg(text);
    }
    return text;
}

void MainWindow::updateComposerStateForCurrentChat()
{
    bool canSend = false;
    bool showJoin = false;
    QString placeholder = "Select a chat";

    if (!m_currentChatId.isEmpty()) {
        canSend = true;
        placeholder = "Write a message...";
        if (m_chats.contains(m_currentChatId)) {
            const Chat& chat = m_chats[m_currentChatId];
            const bool isOwner = (chat.ownerId == m_client->currentUserId());
            const bool isChannel = (chat.chatType == "channel");
            const bool isMember = !isChannel || isOwner || chat.members.contains(m_client->currentUserId());
            if (m_chatSettingsBtn) {
                m_chatSettingsBtn->setVisible(isOwner && (chat.chatType == "group" || chat.chatType == "channel"));
            }
            if (isChannel && !isMember) {
                canSend = false;
                showJoin = true;
                placeholder = "Join channel to participate.";
            } else if (isChannel && !isOwner) {
                canSend = false;
                placeholder = "Read-only channel (owner posting only).";
            }
        } else if (m_chatSettingsBtn) {
            m_chatSettingsBtn->setVisible(false);
        }
    } else if (m_chatSettingsBtn) {
        m_chatSettingsBtn->setVisible(false);
    }

    if (m_messageInput) {
        m_messageInput->setEnabled(canSend);
        m_messageInput->setPlaceholderText(placeholder);
    }
    if (m_sendBtn) {
        m_sendBtn->setEnabled(canSend);
    }
    if (m_attachBtn) {
        m_attachBtn->setEnabled(canSend);
    }
    if (m_stickerBtn) {
        m_stickerBtn->setEnabled(canSend);
    }
    if (m_joinChannelBar) {
        m_joinChannelBar->setVisible(showJoin);
    }
    if (!canSend) {
        hideStickerPanel();
    }
}

void MainWindow::updatePinnedMessageBar()
{
    if (!m_pinnedBar || !m_pinnedLabel) {
        return;
    }
    if (m_currentChatId.isEmpty() || !m_chats.contains(m_currentChatId)) {
        m_pinnedBar->hide();
        return;
    }

    const Chat& chat = m_chats[m_currentChatId];
    if (!chat.hasPinnedMessage || chat.pinnedMessage.messageId.isEmpty()) {
        m_pinnedBar->hide();
        return;
    }

    QString sender = chat.pinnedMessage.senderName.trimmed();
    if (sender.isEmpty()) {
        sender = chat.pinnedMessage.senderId;
    }
    if (m_users.contains(chat.pinnedMessage.senderId) && !m_users[chat.pinnedMessage.senderId].username.trimmed().isEmpty()) {
        sender = m_users[chat.pinnedMessage.senderId].username.trimmed();
    }
    QString preview = displayTextForMessage(chat.pinnedMessage);
    if (preview.size() > 80) {
        preview = preview.left(77) + "...";
    }

    m_pinnedLabel->setText(QString("Pinned: %1 - %2").arg(sender, preview));
    if (m_openPinnedBtn) {
        m_openPinnedBtn->setEnabled(!chat.pinnedMessage.messageId.isEmpty());
    }
    if (m_unpinPinnedBtn) {
        const bool canUnpin = (chat.chatType == "private") || (chat.ownerId == m_client->currentUserId());
        m_unpinPinnedBtn->setVisible(canUnpin);
    }
    m_pinnedBar->show();
}

void MainWindow::updateSettingsOverlayGeometry()
{
    if (!m_settingsOverlay || !m_settingsDialog || !m_appPage) {
        return;
    }

    m_settingsOverlay->setGeometry(m_appPage->rect());
    const int margin = 12;
    const int maxWidth = qMax(320, m_settingsOverlay->width() - margin * 2);
    const int maxHeight = qMax(320, m_settingsOverlay->height() - margin * 2);
    const QSize currentSize = m_settingsDialog->size().isValid() ? m_settingsDialog->size() : m_settingsDialog->sizeHint();
    const int w = qMin(currentSize.width(), maxWidth);
    const int h = qMin(currentSize.height(), maxHeight);
    m_settingsDialog->resize(w, h);

    const int x = qMax(margin, (m_settingsOverlay->width() - w) / 2);
    const int y = qMax(margin, (m_settingsOverlay->height() - h) / 2);
    m_settingsDialog->move(x, y);
}

void MainWindow::updateContactsOverlayGeometry()
{
    if (!m_contactsOverlay || !m_contactsPanel || !m_appPage) {
        return;
    }

    m_contactsOverlay->setGeometry(m_appPage->rect());
    const int margin = 12;
    const int maxWidth = qMax(340, m_contactsOverlay->width() - margin * 2);
    const int maxHeight = qMax(320, m_contactsOverlay->height() - margin * 2);
    const QSize preferred(460, 620);
    const int w = qMin(preferred.width(), maxWidth);
    const int h = qMin(preferred.height(), maxHeight);
    m_contactsPanel->resize(w, h);
    const int x = qMax(margin, (m_contactsOverlay->width() - w) / 2);
    const int y = qMax(margin, (m_contactsOverlay->height() - h) / 2);
    m_contactsPanel->move(x, y);
}

void MainWindow::updateSidebarMenuGeometry()
{
    if (!m_sidebarMenuOverlay || !m_appPage || !m_sidebarMenuPanel) {
        return;
    }
    m_sidebarMenuOverlay->setGeometry(m_appPage->rect());
    m_sidebarMenuPanel->setFixedHeight(m_appPage->height());
    m_sidebarMenuPanel->move(0, 0);
}

void MainWindow::showSidebarMenu()
{
    if (!m_sidebarMenuOverlay) {
        return;
    }
    hideStickerPanel();
    if (m_contactsOverlay) {
        m_contactsOverlay->hide();
    }
    updateSidebarMenuGeometry();
    m_sidebarMenuOverlay->show();
    m_sidebarMenuOverlay->raise();
    if (m_sidebarMenuPanel) {
        m_sidebarMenuPanel->raise();
    }
}

void MainWindow::hideSidebarMenu()
{
    if (m_sidebarMenuOverlay) {
        m_sidebarMenuOverlay->hide();
    }
}

void MainWindow::showContactsDialog()
{
    if (!m_contactsOverlay || !m_contactsPanel) {
        return;
    }
    hideStickerPanel();
    hideSidebarMenu();
    if (m_settingsDialog && m_settingsDialog->isVisible()) {
        m_settingsDialog->hide();
    }
    if (m_contactsSearchInput) {
        m_contactsSearchInput->clear();
    }
    applyContactsFilter(QString());
    updateContactsOverlayGeometry();
    m_contactsOverlay->show();
    m_contactsOverlay->raise();
    m_contactsPanel->raise();
    if (m_contactsSearchInput) {
        m_contactsSearchInput->setFocus();
    }
}

void MainWindow::applyContactsFilter(const QString& filterText)
{
    if (!m_contactListWidget) {
        return;
    }
    const QString needle = filterText.trimmed().toLower();
    for (int i = 0; i < m_contactListWidget->count(); ++i) {
        QListWidgetItem* item = m_contactListWidget->item(i);
        if (!item) {
            continue;
        }
        item->setHidden(!needle.isEmpty() && !item->text().toLower().contains(needle));
    }
}

void MainWindow::showCreateOptionsMenu()
{
    if (!m_sidebarCreateBtn) {
        return;
    }
    QMenu menu(this);
    QAction* createGroupAction = menu.addAction("Create Group");
    QAction* createChannelAction = menu.addAction("Create Channel");
    QAction* chosen = menu.exec(m_sidebarCreateBtn->mapToGlobal(QPoint(0, m_sidebarCreateBtn->height())));
    if (chosen == createGroupAction) {
        createGroupFromSidebar();
    } else if (chosen == createChannelAction) {
        createChannelFromSidebar();
    }
}

void MainWindow::createGroupFromSidebar()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Create Group");
    auto* layout = new QVBoxLayout(&dialog);
    auto* nameInput = new QLineEdit(&dialog);
    nameInput->setPlaceholderText("Group name");
    layout->addWidget(nameInput);

    auto* list = new QListWidget(&dialog);
    int candidateCount = 0;
    for (auto it = m_users.constBegin(); it != m_users.constEnd(); ++it) {
        if (it.key() == m_client->currentUserId()) {
            continue;
        }
        auto* item = new QListWidgetItem(QString("%1 (%2)").arg(it.value().username, it.key()), list);
        item->setData(Qt::UserRole, it.key());
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        candidateCount++;
    }
    if (candidateCount == 0) {
        statusBar()->showMessage("No users available to add.", 3000);
        return;
    }
    layout->addWidget(list, 1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const QString name = nameInput->text().trimmed();
    if (name.isEmpty()) {
        statusBar()->showMessage("Group name is required.", 3000);
        return;
    }

    QStringList members;
    for (int i = 0; i < list->count(); ++i) {
        QListWidgetItem* item = list->item(i);
        if (item && item->checkState() == Qt::Checked) {
            members.push_back(item->data(Qt::UserRole).toString());
        }
    }
    if (members.isEmpty()) {
        statusBar()->showMessage("Select at least one member.", 3000);
        return;
    }

    QNetworkReply* reply = m_restClient->createGroup(name, members);
    if (!reply) {
        statusBar()->showMessage("Failed to start group creation.", 3000);
        return;
    }
    statusBar()->showMessage("Creating group...", 2000);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            statusBar()->showMessage("Create group failed: " + reply->errorString(), 5000);
            return;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject() || !doc.object().value("success").toBool(false)) {
            const QString err = doc.isObject() ? doc.object().value("error").toString() : QString();
            statusBar()->showMessage(err.isEmpty() ? "Create group failed." : err, 5000);
            return;
        }
        statusBar()->showMessage("Group created.", 2500);
    });
}

void MainWindow::createChannelFromSidebar()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Create Channel");
    auto* layout = new QFormLayout(&dialog);
    auto* nameInput = new QLineEdit(&dialog);
    auto* handleInput = new QLineEdit(&dialog);
    nameInput->setPlaceholderText("Channel name");
    handleInput->setPlaceholderText("handle");
    layout->addRow("Name", nameInput);
    layout->addRow("Handle", handleInput);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const QString name = nameInput->text().trimmed();
    const QString handle = handleInput->text().trimmed();
    if (name.isEmpty() || handle.isEmpty()) {
        statusBar()->showMessage("Channel name and handle are required.", 3000);
        return;
    }

    QNetworkReply* reply = m_restClient->createChannel(name, handle);
    if (!reply) {
        statusBar()->showMessage("Failed to start channel creation.", 3000);
        return;
    }
    statusBar()->showMessage("Creating channel...", 2000);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            statusBar()->showMessage("Create channel failed: " + reply->errorString(), 5000);
            return;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject() || !doc.object().value("success").toBool(false)) {
            const QString err = doc.isObject() ? doc.object().value("error").toString() : QString();
            statusBar()->showMessage(err.isEmpty() ? "Create channel failed." : err, 5000);
            return;
        }
        statusBar()->showMessage("Channel created.", 2500);
    });
}

void MainWindow::showSettingsDialog()
{
    if (!m_settingsDialog || !m_client || !m_settingsOverlay) {
        return;
    }
    hideStickerPanel();
    hideSidebarMenu();
    if (m_contactsOverlay) {
        m_contactsOverlay->hide();
    }
    const QString selfId = m_client->currentUserId();
    if (!selfId.isEmpty() && m_users.contains(selfId)) {
        const User& self = m_users[selfId];
        m_settingsDialog->setUserInfo(self.username, self.userId);
        m_settingsDialog->setBlockGroupInvites(self.blockGroupInvites);
    }
    m_settingsDialog->setDarkMode(m_isDarkMode);
    m_settingsDialog->setNotifications(m_notificationsEnabled);
    m_settingsDialog->setUpdaterState(m_updaterStatusText, m_canDownloadUpdate, m_canInstallUpdate);
    m_settingsDialog->showMenu();

    updateSettingsOverlayGeometry();
    m_settingsOverlay->show();
    m_settingsOverlay->raise();
    m_settingsDialog->show();
    m_settingsDialog->raise();
    m_settingsDialog->activateWindow();
}

void MainWindow::openChatSettingsDialog(const QString& chatId)
{
    if (chatId.isEmpty() || !m_chats.contains(chatId) || !m_chatSettingsDialog) {
        return;
    }
    hideStickerPanel();
    const Chat& chat = m_chats[chatId];
    if (chat.ownerId != m_client->currentUserId()) {
        statusBar()->showMessage("Only the chat owner can manage chat settings.", 3000);
        return;
    }
    m_chatSettingsDialog->setDarkMode(m_isDarkMode);
    m_chatSettingsDialog->setChatData(chat, m_users, m_client->currentUserId());
    m_chatSettingsDialog->show();
    m_chatSettingsDialog->raise();
    m_chatSettingsDialog->activateWindow();
}

void MainWindow::openAddMembersDialogForChat(const QString& chatId)
{
    if (chatId.isEmpty() || !m_chats.contains(chatId)) {
        return;
    }
    const Chat& chat = m_chats[chatId];
    if (chat.chatType != "group") {
        statusBar()->showMessage("Add members is available for groups.", 3000);
        return;
    }
    if (chat.ownerId != m_client->currentUserId()) {
        statusBar()->showMessage("Only the chat owner can add members.", 3000);
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Add Members");
    auto* layout = new QVBoxLayout(&dialog);
    auto* list = new QListWidget(&dialog);
    bool hasCandidates = false;
    for (auto it = m_users.constBegin(); it != m_users.constEnd(); ++it) {
        const QString uid = it.key();
        if (uid == m_client->currentUserId() || chat.members.contains(uid)) {
            continue;
        }
        hasCandidates = true;
        auto* item = new QListWidgetItem(QString("%1 (%2)").arg(it.value().username, uid), list);
        item->setData(Qt::UserRole, uid);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
    if (!hasCandidates) {
        statusBar()->showMessage("No users available to add.", 3000);
        return;
    }
    layout->addWidget(list);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QJsonArray memberIds;
    for (int i = 0; i < list->count(); ++i) {
        QListWidgetItem* item = list->item(i);
        if (item->checkState() == Qt::Checked) {
            memberIds.append(item->data(Qt::UserRole).toString());
        }
    }
    if (memberIds.isEmpty()) {
        statusBar()->showMessage("Select at least one member.", 3000);
        return;
    }
    QJsonObject extra;
    extra.insert("memberIds", memberIds);
    performChatSettingsAction("add_members", chatId, extra);
}

void MainWindow::startReplyToMessage(const QString& messageId)
{
    if (messageId.isEmpty() || !m_chats.contains(m_currentChatId)) {
        return;
    }
    Message* replyMsg = nullptr;
    for (auto& msg : m_chats[m_currentChatId].messages) {
        if (msg.messageId == messageId) {
            replyMsg = &msg;
            break;
        }
    }
    if (!replyMsg) {
        return;
    }

    m_replyingToMessageId = messageId;
    m_replyingToText = displayTextForMessage(*replyMsg);
    QString senderName = replyMsg->senderName.trimmed();
    if (m_users.contains(replyMsg->senderId) && !m_users[replyMsg->senderId].username.trimmed().isEmpty()) {
        senderName = m_users[replyMsg->senderId].username.trimmed();
    }
    if (senderName.isEmpty()) {
        senderName = "Unknown";
    }
    m_replyingToSender = senderName;
    QString preview = m_replyingToText.length() > 50 ? m_replyingToText.left(50) + "..." : m_replyingToText;
    m_replyLabel->setText(senderName + ": " + preview);
    m_replyBar->show();
    m_messageInput->setFocus();
}

void MainWindow::startEditMessage(const QString& messageId)
{
    if (messageId.isEmpty() || !m_chats.contains(m_currentChatId)) {
        return;
    }
    const Message* message = nullptr;
    for (const auto& msg : m_chats[m_currentChatId].messages) {
        if (msg.messageId == messageId) {
            message = &msg;
            break;
        }
    }
    if (!message || !message->file.isNull()) {
        return;
    }

    const QString editableText = extractRenderableMessageText(*message);
    m_editingMessageId = messageId;
    m_editingOriginalText = editableText;
    QString preview = editableText.length() > 30 ? editableText.left(30) + "..." : editableText;
    m_editLabel->setText("Editing: " + preview);
    m_editBar->show();
    m_messageInput->setText(editableText);
    m_messageInput->setFocus();
    m_messageInput->selectAll();
    m_sendBtn->setText("Save");
}

void MainWindow::deleteMessageById(const QString& messageId)
{
    if (messageId.isEmpty() || m_currentChatId.isEmpty()) {
        return;
    }
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Delete Message");
    msgBox.setText("Are you sure you want to delete this message?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    if (msgBox.exec() == QMessageBox::Yes) {
        m_client->deleteMessage(m_currentChatId, messageId);
    }
}

void MainWindow::forwardMessageById(const QString& messageId)
{
    if (!m_chats.contains(m_currentChatId) || messageId.isEmpty()) {
        return;
    }

    const Message* original = nullptr;
    for (const auto& msg : m_chats[m_currentChatId].messages) {
        if (msg.messageId == messageId) {
            original = &msg;
            break;
        }
    }
    if (!original) {
        return;
    }

    QStringList options;
    QMap<QString, QString> optionToChatId;
    for (auto it = m_chats.constBegin(); it != m_chats.constEnd(); ++it) {
        if (it.key() == m_currentChatId) {
            continue;
        }
        const QString label = QString("%1 (%2)").arg(resolveChatName(it.value()), it.key());
        options.append(label);
        optionToChatId[label] = it.key();
    }
    if (options.isEmpty()) {
        QMessageBox::information(this, "Forward Message", "No other chat is available.");
        return;
    }
    bool ok = false;
    const QString selected = QInputDialog::getItem(this, "Forward Message", "Send to:", options, 0, false, &ok);
    if (!ok || selected.isEmpty()) {
        return;
    }
    const QString targetChatId = optionToChatId.value(selected);
    if (targetChatId.isEmpty()) {
        return;
    }
    if (m_chats.contains(targetChatId)) {
        const Chat& targetChat = m_chats[targetChatId];
        if (targetChat.chatType == "channel" && targetChat.ownerId != m_client->currentUserId()) {
            QMessageBox::warning(this,
                                 "Permission Denied",
                                 QString("You cannot send messages to the channel \"%1\".")
                                     .arg(resolveChatName(targetChat)));
            return;
        }
    }

    QJsonObject content;
    const QString forwardText = extractRenderableMessageText(*original);
    content.insert("text", forwardText.isEmpty() ? QJsonValue::Null : QJsonValue(forwardText));
    content.insert("theme", QJsonValue::Null);
    if (!original->file.isNull()) {
        QJsonObject file;
        file.insert("url", original->file.url);
        file.insert("name", original->file.name);
        file.insert("type", original->file.type);
        if (original->file.size > 0) {
            file.insert("size", static_cast<double>(original->file.size));
        }
        content.insert("file", file);
    } else {
        content.insert("file", QJsonValue::Null);
    }

    QJsonObject forwardedInfo;
    QString forwardedFrom = original->senderName.trimmed();
    if (m_users.contains(original->senderId) && !m_users[original->senderId].username.trimmed().isEmpty()) {
        forwardedFrom = m_users[original->senderId].username.trimmed();
    }
    if (forwardedFrom.isEmpty()) {
        forwardedFrom = "Someone";
    }
    forwardedInfo.insert("from", forwardedFrom);
    forwardedInfo.insert("originalTs", static_cast<double>(original->timestamp));
    content.insert("forwardedInfo", forwardedInfo);
    m_client->sendMessagePayload(targetChatId, content, QString(), QString());
}

void MainWindow::playAudioTrack(const QString& fileUrl, const QString& trackName, MessageItemWidget* sourceWidget)
{
    if (!m_sidebarAudioPlayback || fileUrl.isEmpty()) {
        return;
    }
    const QUrl requested(fileUrl);
    if (m_currentAudioUrl == fileUrl && m_sidebarAudioPlayback->state() == QMediaPlayer::PlayingState) {
        m_sidebarAudioPlayback->pause();
        return;
    }
    if (m_currentAudioUrl == fileUrl && m_sidebarAudioPlayback->state() == QMediaPlayer::PausedState) {
        m_sidebarAudioPlayback->play();
        return;
    }

    if (m_currentAudioSourceWidget && m_currentAudioSourceWidget != sourceWidget) {
        m_currentAudioSourceWidget->setAudioPlaying(false);
    }
    m_currentAudioSourceWidget = sourceWidget;
    m_currentAudioUrl = fileUrl;
    if (m_sidebarTrackName) {
        m_sidebarTrackName->setText(trackName);
    }
    if (m_sidebarSeekBar) {
        m_sidebarSeekBar->setValue(0);
    }
    if (m_sidebarCurrentTime) {
        m_sidebarCurrentTime->setText("0:00");
    }
    if (m_sidebarDuration) {
        m_sidebarDuration->setText("0:00");
    }
    if (m_sidebarAudioPlayer) {
        m_sidebarAudioPlayer->show();
    }
    m_sidebarAudioPlayback->setMedia(QMediaContent(requested));
    m_sidebarAudioPlayback->play();
}

void MainWindow::openMediaInViewer(const QString& mediaType, const QString& fileUrl)
{
    if (!m_mediaViewerDialog || fileUrl.isEmpty()) {
        return;
    }
    const QUrl url(fileUrl);
    if (mediaType == "video") {
        m_mediaViewerDialog->showVideo(url);
    } else {
        m_mediaViewerDialog->showImage(url);
    }
}

void MainWindow::syncMessageWidgetSize(QListWidgetItem* item)
{
    if (!m_chatList || !item) {
        return;
    }
    QWidget* widget = m_chatList->itemWidget(item);
    if (!widget) {
        return;
    }
    const int targetWidth = qMax(0, m_chatList->viewport()->width() - 2);
    if (targetWidth > 0) {
        if (widget->minimumWidth() != targetWidth || widget->maximumWidth() != targetWidth) {
            widget->setMinimumWidth(targetWidth);
            widget->setMaximumWidth(targetWidth);
            widget->updateGeometry();
        }
    }
    const QSize hint = widget->sizeHint();
    if (item->sizeHint() != hint) {
        item->setSizeHint(hint);
    }
}

void MainWindow::refreshMessageWidgetSizes()
{
    if (!m_chatList) {
        return;
    }
    const int viewportWidth = qMax(0, m_chatList->viewport()->width());
    if (viewportWidth <= 0 || viewportWidth == m_lastMessageViewportWidth) {
        return;
    }
    m_lastMessageViewportWidth = viewportWidth;
    for (int i = 0; i < m_chatList->count(); ++i) {
        QListWidgetItem* item = m_chatList->item(i);
        if (!item) {
            continue;
        }
        syncMessageWidgetSize(item);
    }
}

void MainWindow::openStickerPicker()
{
    if (m_currentChatId.isEmpty()) {
        return;
    }
    if (m_stickerBtn && !m_stickerBtn->isEnabled()) {
        statusBar()->showMessage("You cannot send stickers in this chat.", 3000);
        return;
    }

    auto sendSticker = [this](const QString& selectedUrl) {
        if (selectedUrl.isEmpty() || m_currentChatId.isEmpty()) {
            return;
        }
        const QString targetChatId = m_currentChatId;
        const QString targetReplyId = m_replyingToMessageId;

        QJsonObject file;
        file.insert("url", selectedUrl);
        file.insert("name", "sticker.png");
        file.insert("type", "image/png");

        QJsonObject content;
        content.insert("text", QJsonValue::Null);
        content.insert("file", file);
        content.insert("theme", QJsonValue::Null);

        const QString recipientId = resolveRecipientIdForChat(targetChatId);
        m_client->sendMessagePayload(targetChatId, content, targetReplyId, recipientId);

        Message pendingMsg;
        pendingMsg.messageId = "temp_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        pendingMsg.chatId = targetChatId;
        pendingMsg.senderId = m_client->currentUserId();
        pendingMsg.timestamp = QDateTime::currentSecsSinceEpoch();
        pendingMsg.status = MessageStatus::Pending;
        pendingMsg.pending = true;
        pendingMsg.replyToId = targetReplyId;
        pendingMsg.file.url = selectedUrl;
        pendingMsg.file.name = "sticker.png";
        pendingMsg.file.type = "image/png";
        pendingMsg.text.clear();

        m_pendingMessages[pendingMsg.messageId] = pendingMsg;
        if (m_currentChatId == targetChatId) {
            addMessageBubble(pendingMsg, false, false);
            smoothScrollToBottom();
            if (!targetReplyId.isEmpty()) {
                onCancelReply();
            }
        }
        hideStickerPanel();
    };

    auto showPicker = [this, sendSticker](const QStringList& stickers) {
        if (!m_stickerGridLayout || !m_stickerGridHost) {
            return;
        }

        QLayoutItem* child = nullptr;
        while ((child = m_stickerGridLayout->takeAt(0)) != nullptr) {
            if (child->widget()) {
                child->widget()->deleteLater();
            }
            delete child;
        }

        if (stickers.isEmpty()) {
            m_stickerGridLayout->addWidget(new QLabel("No stickers available.", m_stickerGridHost), 0, 0);
            return;
        }

        const QStringList limited = stickers.mid(0, 250);
        constexpr int kColumns = 3;
        const QString stickerBtnStyle = m_isDarkMode
                                            ? QStringLiteral("QToolButton { border: 1px solid #4b5563; border-radius: 8px; background: #1f2937; color: #f3f4f6; }"
                                                             "QToolButton:hover { border-color: #60a5fa; background: #111827; }")
                                            : QStringLiteral("QToolButton { border: 1px solid #d1d5db; border-radius: 8px; background: #ffffff; }"
                                                             "QToolButton:hover { border-color: #60a5fa; background: #eff6ff; }");
        for (int i = 0; i < limited.size(); ++i) {
            const QString stickerUrl = limited[i];
            auto* btn = new QToolButton(m_stickerGridHost);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setFixedSize(96, 96);
            btn->setIconSize(QSize(82, 82));
            btn->setText("...");
            btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            btn->setStyleSheet(stickerBtnStyle);
            connect(btn, &QToolButton::clicked, this, [sendSticker, stickerUrl]() {
                sendSticker(stickerUrl);
            });

            QUrl imageUrl(stickerUrl);
            if (imageUrl.isRelative()) {
                imageUrl = QUrl("https://noveo.ir").resolved(imageUrl);
            }
            QNetworkReply* imageReply = m_nam->get(QNetworkRequest(imageUrl));
            connect(imageReply, &QNetworkReply::finished, imageReply, &QObject::deleteLater);
            connect(imageReply, &QNetworkReply::finished, btn, [imageReply, btn]() {
                if (imageReply->error() != QNetworkReply::NoError) {
                    btn->setText("x");
                    return;
                }
                QPixmap px;
                if (!px.loadFromData(imageReply->readAll())) {
                    btn->setText("x");
                    return;
                }
                btn->setText(QString());
                btn->setIcon(QIcon(px));
            });

            m_stickerGridLayout->addWidget(btn, i / kColumns, i % kColumns);
        }
    };

    if (!m_stickerPanel) {
        return;
    }

    if (m_stickerPanelVisible) {
        hideStickerPanel();
        return;
    }

    m_stickerPanel->show();
    m_stickerPanelVisible = true;
    repositionStickerPanel();
    m_stickerPanel->raise();

    if (!m_stickerCache.isEmpty()) {
        showPicker(m_stickerCache);
        return;
    }

    if (m_stickerGridLayout && m_stickerGridHost) {
        QLayoutItem* child = nullptr;
        while ((child = m_stickerGridLayout->takeAt(0)) != nullptr) {
            if (child->widget()) {
                child->widget()->deleteLater();
            }
            delete child;
        }
        m_stickerGridLayout->addWidget(new QLabel("Loading stickers...", m_stickerGridHost), 0, 0);
    }

    QNetworkRequest req(QUrl("https://noveo.ir/stickers.json"));
    req.setRawHeader("Accept", "application/json");
    QNetworkReply* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, showPicker]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            statusBar()->showMessage("Failed to load stickers: " + reply->errorString(), 4000);
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isArray()) {
            statusBar()->showMessage("Sticker response is invalid.", 4000);
            return;
        }

        QStringList stickers;
        const QJsonArray arr = doc.array();
        stickers.reserve(arr.size());
        for (const QJsonValue& value : arr) {
            if (value.isString() && !value.toString().isEmpty()) {
                stickers.push_back(value.toString());
            }
        }
        m_stickerCache = stickers;
        showPicker(m_stickerCache);
    });
}

void MainWindow::hideStickerPanel()
{
    if (!m_stickerPanel) {
        return;
    }
    m_stickerPanelVisible = false;
    m_stickerPanel->hide();
}

void MainWindow::repositionStickerPanel()
{
    if (!m_stickerPanel || !m_stickerBtn || !m_chatAreaWidget || !m_stickerPanelVisible) {
        return;
    }

    QPoint anchor = m_stickerBtn->mapTo(m_chatAreaWidget, QPoint(0, 0));
    int x = anchor.x() - m_stickerPanel->width() + m_stickerBtn->width();
    int y = anchor.y() - m_stickerPanel->height() - 8;

    const QRect bounds = m_chatAreaWidget->rect();
    if (y < bounds.top() + 4) {
        y = anchor.y() + m_stickerBtn->height() + 8;
    }
    if (x < bounds.left() + 4) {
        x = bounds.left() + 4;
    }
    if (x + m_stickerPanel->width() > bounds.right() - 4) {
        x = bounds.right() - m_stickerPanel->width() - 4;
    }
    if (y + m_stickerPanel->height() > bounds.bottom() - 4) {
        y = bounds.bottom() - m_stickerPanel->height() - 4;
    }
    if (y < bounds.top() + 4) {
        y = bounds.top() + 4;
    }

    m_stickerPanel->move(x, y);
}

void MainWindow::performChatSettingsAction(const QString& action, const QString& chatId, const QJsonObject& extra)
{
    if (chatId.isEmpty()) {
        statusBar()->showMessage("Select a chat first.", 3000);
        return;
    }
    QNetworkReply* reply = m_restClient->chatSettingsAction(action, chatId, extra);
    if (!reply) {
        statusBar()->showMessage("Failed to send chat settings action.", 4000);
        return;
    }

    connect(reply, &QNetworkReply::finished, this, [this, reply, action, chatId, extra]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            statusBar()->showMessage("Action failed: " + reply->errorString(), 5000);
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject() || !doc.object().value("success").toBool(false)) {
            const QString error = doc.isObject() ? doc.object().value("error").toString() : QString();
            statusBar()->showMessage(error.isEmpty() ? "Action rejected by server." : error, 5000);
            return;
        }

        const QJsonObject root = doc.object();
        if (action == "remove_handle") {
            if (m_chats.contains(chatId)) {
                m_chats[chatId].handle.clear();
            }
            statusBar()->showMessage("Handle removed.", 3000);
        } else if (action == "remove_member") {
            if (m_chats.contains(chatId)) {
                QStringList members = m_chats[chatId].members;
                if (root.contains("members") && root.value("members").isArray()) {
                    members.clear();
                    const QJsonArray arr = root.value("members").toArray();
                    for (const QJsonValue& value : arr) {
                        members.push_back(value.toString());
                    }
                } else {
                    members.removeAll(extra.value("memberId").toString());
                }
                m_chats[chatId].members = members;
            }
            if (extra.value("memberId").toString() == m_client->currentUserId() && m_currentChatId == chatId) {
                m_currentChatId.clear();
                m_chatTitle->setText("Select a chat");
                m_chatList->clear();
                m_messageItemsById.clear();
                m_messageWidgetsById.clear();
                m_currentMessagePreviewById.clear();
                m_currentMessageSenderById.clear();
                m_lastMessageViewportWidth = -1;
                if (m_chatSettingsBtn) {
                    m_chatSettingsBtn->setVisible(false);
                }
            }
            statusBar()->showMessage("Member removed.", 3000);
        } else if (action == "add_members") {
            if (m_chats.contains(chatId) && root.contains("members") && root.value("members").isArray()) {
                QStringList members;
                const QJsonArray arr = root.value("members").toArray();
                for (const QJsonValue& value : arr) {
                    members.push_back(value.toString());
                }
                m_chats[chatId].members = members;
            } else if (m_chats.contains(chatId) && extra.contains("memberIds") && extra.value("memberIds").isArray()) {
                QStringList members = m_chats[chatId].members;
                const QJsonArray arr = extra.value("memberIds").toArray();
                for (const QJsonValue& value : arr) {
                    const QString memberId = value.toString();
                    if (!members.contains(memberId)) {
                        members.push_back(memberId);
                    }
                }
                m_chats[chatId].members = members;
            }
            statusBar()->showMessage("Members updated.", 3000);
        } else if (action == "delete_chat") {
            m_chats.remove(chatId);
            for (int i = 0; i < m_chatListWidget->count(); ++i) {
                QListWidgetItem* item = m_chatListWidget->item(i);
                if (item && item->data(Qt::UserRole).toString() == chatId) {
                    delete m_chatListWidget->takeItem(i);
                    break;
                }
            }

            if (m_currentChatId == chatId) {
                m_currentChatId.clear();
                m_chatTitle->setText("Select a chat");
                m_chatList->clear();
                m_messageItemsById.clear();
                m_messageWidgetsById.clear();
                m_currentMessagePreviewById.clear();
                m_currentMessageSenderById.clear();
                m_lastMessageViewportWidth = -1;
                if (m_chatSettingsBtn) {
                    m_chatSettingsBtn->setVisible(false);
                }
            }
            statusBar()->showMessage("Chat deleted.", 3000);
        }

        if (m_currentChatId == chatId && m_chats.contains(chatId)) {
            renderMessages(chatId);
        }
        updateComposerStateForCurrentChat();
        updatePinnedMessageBar();
        if (m_chatSettingsDialog) {
            if (m_chats.contains(chatId) && m_chatSettingsDialog->isVisible() && m_chatSettingsDialog->chatId() == chatId) {
                m_chatSettingsDialog->setChatData(m_chats[chatId], m_users, m_client->currentUserId());
            } else if (!m_chats.contains(chatId) && m_chatSettingsDialog->isVisible() && m_chatSettingsDialog->chatId() == chatId) {
                m_chatSettingsDialog->hide();
            }
        }
    });
}

void MainWindow::rebuildCurrentMessageCaches(const QString& chatId)
{
    m_currentMessagePreviewById.clear();
    m_currentMessageSenderById.clear();
    if (!m_chats.contains(chatId)) {
        return;
    }
    const Chat& chat = m_chats[chatId];
    for (const Message& msg : chat.messages) {
        QString senderName = msg.senderName.trimmed();
        if (m_users.contains(msg.senderId) && !m_users[msg.senderId].username.trimmed().isEmpty()) {
            senderName = m_users[msg.senderId].username.trimmed();
        }
        if (senderName.isEmpty()) {
            senderName = QStringLiteral("Unknown");
        }
        m_currentMessageSenderById.insert(msg.messageId, senderName);
        m_currentMessagePreviewById.insert(msg.messageId, displayTextForMessage(msg));
    }
}

void MainWindow::renderMessages(const QString& chatId) {
    m_currentAudioSourceWidget = nullptr;
    m_chatList->setUpdatesEnabled(false);
    m_chatList->clear();
    m_messageItemsById.clear();
    m_messageWidgetsById.clear();
    m_lastMessageViewportWidth = -1;
    rebuildCurrentMessageCaches(chatId);
    if (m_chats.contains(chatId)) {
        Chat& chat = m_chats[chatId];
        for (auto& msg : chat.messages) {
            msg.status = calculateMessageStatus(msg, chat);
            addMessageBubble(msg, false, false);

            if (msg.senderId != m_client->currentUserId() && !msg.seenBy.contains(m_client->currentUserId())) {
                m_client->sendMessageSeen(chatId, msg.messageId);
                msg.seenBy.append(m_client->currentUserId());
            }
        }
        scrollToBottom();
        updatePinnedMessageBar();
    }
    m_chatList->setUpdatesEnabled(true);
    m_chatList->viewport()->update();
}

QString MainWindow::getReplyPreviewText(const QString& replyToId, const QString& chatId) {
    QString replyText;
    if (m_currentMessagePreviewById.contains(replyToId)) {
        replyText = m_currentMessagePreviewById.value(replyToId);
    }

    if (replyText.isEmpty() && !chatId.isEmpty() && m_chats.contains(chatId)) {
        for (const auto& m : m_chats[chatId].messages) {
            if (m.messageId == replyToId) {
                replyText = displayTextForMessage(m);
                break;
            }
        }
    }

    if (!replyText.isEmpty()) {
        QString trimmed = replyText.trimmed();
        bool isJustURL = trimmed.startsWith("http") &&
                         (trimmed.contains("/static/upload") || trimmed.contains("/static/upl"));

        if (isJustURL) {
            QStringList words = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (words.size() == 1) {
                replyText = "[Image]";
            } else if (words.size() == 2 && words.last().startsWith("http")) {
                replyText = words.first() + " [Image]";
            }
        } else if (trimmed.startsWith("[FILE:") && trimmed.endsWith("]")) {
            replyText = "[Image]";
        }
    }

    if (replyText.isEmpty()) {
        replyText = "[Original message]";
    }

    return replyText;
}

void MainWindow::addMessageBubble(const Message& msg, bool appendStretch, bool animate) {
    Q_UNUSED(appendStretch);
    Q_UNUSED(animate);

    if (m_messageItemsById.contains(msg.messageId)) {
        QListWidgetItem* existing = m_messageItemsById.value(msg.messageId);
        const int row = m_chatList->row(existing);
        if (row >= 0) {
            delete m_chatList->takeItem(row);
        }
        m_messageItemsById.remove(msg.messageId);
        m_messageWidgetsById.remove(msg.messageId);
    }

    Message widgetMessage = msg;
    if (widgetMessage.chatId.isEmpty()) {
        widgetMessage.chatId = m_currentChatId;
    }
    widgetMessage.text = extractRenderableMessageText(msg, &widgetMessage.file);

    bool isMe = (widgetMessage.senderId == m_client->currentUserId());
    QString senderName = widgetMessage.senderName.trimmed();
    if (senderName.isEmpty()) {
        senderName = "Unknown";
    }
    QString senderAvatarUrl = widgetMessage.senderAvatarUrl.trimmed();
    if (m_users.contains(widgetMessage.senderId)) {
        if (!m_users[widgetMessage.senderId].username.trimmed().isEmpty()) {
            senderName = m_users[widgetMessage.senderId].username.trimmed();
        }
        if (!m_users[widgetMessage.senderId].avatarUrl.trimmed().isEmpty()) {
            senderAvatarUrl = m_users[widgetMessage.senderId].avatarUrl.trimmed();
        }
    }
    if (!senderAvatarUrl.isEmpty() && !senderAvatarUrl.startsWith("http://") && !senderAvatarUrl.startsWith("https://")) {
        if (senderAvatarUrl.startsWith('/')) {
            senderAvatarUrl = API_BASE_URL + senderAvatarUrl;
        } else {
            senderAvatarUrl = API_BASE_URL + "/" + senderAvatarUrl;
        }
    }
    const QPixmap senderAvatar = getAvatar(senderName, senderAvatarUrl).pixmap(34, 34);

    const QString displayText = displayTextForMessage(widgetMessage);

    QListWidgetItem* item = new QListWidgetItem();
    item->setData(Qt::UserRole + 1, displayText);
    item->setData(Qt::UserRole + 2, senderName);
    item->setData(Qt::UserRole + 3, widgetMessage.timestamp);
    item->setData(Qt::UserRole + 4, isMe);
    item->setData(Qt::UserRole + 5, static_cast<int>(widgetMessage.status));
    item->setData(Qt::UserRole + 6, widgetMessage.messageId);
    item->setData(Qt::UserRole + 7, widgetMessage.editedAt);
    item->setData(Qt::UserRole + 8, widgetMessage.senderId);
    item->setData(Qt::UserRole + 9, widgetMessage.replyToId);
    item->setData(FileUrlRole, resolveFileUrl(widgetMessage.file.url));
    item->setData(FileTypeRole, widgetMessage.file.type);

    QString replyText;
    QString replySenderName;
    if (!widgetMessage.replyToId.isEmpty()) {
        replyText = getReplyPreviewText(widgetMessage.replyToId, widgetMessage.chatId);
        item->setData(Qt::UserRole + 10, replyText);
        replySenderName = m_currentMessageSenderById.value(widgetMessage.replyToId);
        if (replySenderName.isEmpty() && m_chats.contains(widgetMessage.chatId)) {
            for (const Message& candidate : m_chats[widgetMessage.chatId].messages) {
                if (candidate.messageId == widgetMessage.replyToId) {
                    replySenderName = m_users.contains(candidate.senderId)
                                          ? m_users[candidate.senderId].username
                                          : QStringLiteral("Unknown");
                    break;
                }
            }
        }
        item->setData(Qt::UserRole + 11, replySenderName);
    }

    QString chatType = "private";
    bool isChannelOwner = false;
    if (m_chats.contains(widgetMessage.chatId)) {
        const Chat& chat = m_chats[widgetMessage.chatId];
        chatType = chat.chatType;
        isChannelOwner = (chat.chatType == "channel" && chat.ownerId == m_client->currentUserId());
    }

    auto* widget = new MessageItemWidget(widgetMessage,
                                         senderName,
                                         senderAvatarUrl,
                                         senderAvatar,
                                         resolveFileUrl(widgetMessage.file.url),
                                         replySenderName,
                                         replyText,
                                         m_client->currentUserId(),
                                         chatType,
                                         isChannelOwner,
                                         m_isDarkMode,
                                         m_chatList);
    connect(widget, &MessageItemWidget::replyRequested, this, &MainWindow::startReplyToMessage);
    connect(widget, &MessageItemWidget::editRequested, this, &MainWindow::startEditMessage);
    connect(widget, &MessageItemWidget::deleteRequested, this, &MainWindow::deleteMessageById);
    connect(widget, &MessageItemWidget::forwardRequested, this, &MainWindow::forwardMessageById);
    connect(widget, &MessageItemWidget::pinRequested, this, [this](const QString& messageId) {
        if (!m_currentChatId.isEmpty()) {
            m_client->pinMessage(m_currentChatId, messageId);
        }
    });
    connect(widget, &MessageItemWidget::openMediaRequested, this, &MainWindow::openMediaInViewer);
    connect(widget, &MessageItemWidget::openFileRequested, this, [this](const QString& fileUrl) {
        if (!fileUrl.isEmpty()) {
            QDesktopServices::openUrl(QUrl(fileUrl));
        }
    });
    connect(widget, &MessageItemWidget::playAudioRequested, this, &MainWindow::playAudioTrack);
    connect(widget, &MessageItemWidget::replyAnchorClicked, this, &MainWindow::focusOnMessage);

    m_chatList->addItem(item);
    m_chatList->setItemWidget(item, widget);
    syncMessageWidgetSize(item);
    m_messageItemsById[widgetMessage.messageId] = item;
    m_messageWidgetsById[widgetMessage.messageId] = widget;
    m_currentMessagePreviewById.insert(widgetMessage.messageId, displayText);
    m_currentMessageSenderById.insert(widgetMessage.messageId, senderName);
    if (m_highlightedMessageId == widgetMessage.messageId) {
        widget->setHighlighted(true);
    }
}

void MainWindow::prependMessageBubble(const Message& msg) {
    if (m_messageItemsById.contains(msg.messageId)) {
        QListWidgetItem* existing = m_messageItemsById.value(msg.messageId);
        const int row = m_chatList->row(existing);
        if (row >= 0) {
            delete m_chatList->takeItem(row);
        }
        m_messageItemsById.remove(msg.messageId);
        m_messageWidgetsById.remove(msg.messageId);
    }

    Message widgetMessage = msg;
    if (widgetMessage.chatId.isEmpty()) {
        widgetMessage.chatId = m_currentChatId;
    }
    widgetMessage.text = extractRenderableMessageText(msg, &widgetMessage.file);

    bool isMe = (widgetMessage.senderId == m_client->currentUserId());
    QString senderName = widgetMessage.senderName.trimmed();
    if (senderName.isEmpty()) {
        senderName = "Unknown";
    }
    QString senderAvatarUrl = widgetMessage.senderAvatarUrl.trimmed();
    if (m_users.contains(widgetMessage.senderId)) {
        if (!m_users[widgetMessage.senderId].username.trimmed().isEmpty()) {
            senderName = m_users[widgetMessage.senderId].username.trimmed();
        }
        if (!m_users[widgetMessage.senderId].avatarUrl.trimmed().isEmpty()) {
            senderAvatarUrl = m_users[widgetMessage.senderId].avatarUrl.trimmed();
        }
    }
    if (!senderAvatarUrl.isEmpty() && !senderAvatarUrl.startsWith("http://") && !senderAvatarUrl.startsWith("https://")) {
        if (senderAvatarUrl.startsWith('/')) {
            senderAvatarUrl = API_BASE_URL + senderAvatarUrl;
        } else {
            senderAvatarUrl = API_BASE_URL + "/" + senderAvatarUrl;
        }
    }
    const QPixmap senderAvatar = getAvatar(senderName, senderAvatarUrl).pixmap(34, 34);

    const QString displayText = displayTextForMessage(widgetMessage);

    QListWidgetItem* item = new QListWidgetItem();
    item->setData(Qt::UserRole + 1, displayText);
    item->setData(Qt::UserRole + 2, senderName);
    item->setData(Qt::UserRole + 3, widgetMessage.timestamp);
    item->setData(Qt::UserRole + 4, isMe);
    item->setData(Qt::UserRole + 5, static_cast<int>(widgetMessage.status));
    item->setData(Qt::UserRole + 6, widgetMessage.messageId);
    item->setData(Qt::UserRole + 7, widgetMessage.editedAt);
    item->setData(Qt::UserRole + 8, widgetMessage.senderId);
    item->setData(Qt::UserRole + 9, widgetMessage.replyToId);
    item->setData(FileUrlRole, resolveFileUrl(widgetMessage.file.url));
    item->setData(FileTypeRole, widgetMessage.file.type);

    QString replyText;
    QString replySenderName;
    if (!widgetMessage.replyToId.isEmpty()) {
        replyText = getReplyPreviewText(widgetMessage.replyToId, widgetMessage.chatId);
        item->setData(Qt::UserRole + 10, replyText);
        replySenderName = m_currentMessageSenderById.value(widgetMessage.replyToId);
        if (replySenderName.isEmpty() && m_chats.contains(widgetMessage.chatId)) {
            for (const Message& candidate : m_chats[widgetMessage.chatId].messages) {
                if (candidate.messageId == widgetMessage.replyToId) {
                    replySenderName = m_users.contains(candidate.senderId)
                                          ? m_users[candidate.senderId].username
                                          : QStringLiteral("Unknown");
                    break;
                }
            }
        }
        item->setData(Qt::UserRole + 11, replySenderName);
    }

    QString chatType = "private";
    bool isChannelOwner = false;
    if (m_chats.contains(widgetMessage.chatId)) {
        const Chat& chat = m_chats[widgetMessage.chatId];
        chatType = chat.chatType;
        isChannelOwner = (chat.chatType == "channel" && chat.ownerId == m_client->currentUserId());
    }
    auto* widget = new MessageItemWidget(widgetMessage,
                                         senderName,
                                         senderAvatarUrl,
                                         senderAvatar,
                                         resolveFileUrl(widgetMessage.file.url),
                                         replySenderName,
                                         replyText,
                                         m_client->currentUserId(),
                                         chatType,
                                         isChannelOwner,
                                         m_isDarkMode,
                                         m_chatList);
    connect(widget, &MessageItemWidget::replyRequested, this, &MainWindow::startReplyToMessage);
    connect(widget, &MessageItemWidget::editRequested, this, &MainWindow::startEditMessage);
    connect(widget, &MessageItemWidget::deleteRequested, this, &MainWindow::deleteMessageById);
    connect(widget, &MessageItemWidget::forwardRequested, this, &MainWindow::forwardMessageById);
    connect(widget, &MessageItemWidget::pinRequested, this, [this](const QString& messageId) {
        if (!m_currentChatId.isEmpty()) {
            m_client->pinMessage(m_currentChatId, messageId);
        }
    });
    connect(widget, &MessageItemWidget::openMediaRequested, this, &MainWindow::openMediaInViewer);
    connect(widget, &MessageItemWidget::openFileRequested, this, [this](const QString& fileUrl) {
        if (!fileUrl.isEmpty()) {
            QDesktopServices::openUrl(QUrl(fileUrl));
        }
    });
    connect(widget, &MessageItemWidget::playAudioRequested, this, &MainWindow::playAudioTrack);
    connect(widget, &MessageItemWidget::replyAnchorClicked, this, &MainWindow::focusOnMessage);

    m_chatList->insertItem(0, item);
    m_chatList->setItemWidget(item, widget);
    syncMessageWidgetSize(item);
    m_messageItemsById[widgetMessage.messageId] = item;
    m_messageWidgetsById[widgetMessage.messageId] = widget;
    m_currentMessagePreviewById.insert(widgetMessage.messageId, displayText);
    m_currentMessageSenderById.insert(widgetMessage.messageId, senderName);
}

void MainWindow::updateMessageStatus(const QString& messageId, MessageStatus newStatus) {
    QListWidgetItem* item = m_messageItemsById.value(messageId, nullptr);
    if (!item) {
        for (int i = 0; i < m_chatList->count(); i++) {
            QListWidgetItem* candidate = m_chatList->item(i);
            if (candidate && candidate->data(Qt::UserRole + 6).toString() == messageId) {
                item = candidate;
                break;
            }
        }
    }
    if (item) {
        item->setData(Qt::UserRole + 5, static_cast<int>(newStatus));
        if (m_messageWidgetsById.contains(messageId) && m_messageWidgetsById[messageId]) {
            m_messageWidgetsById[messageId]->setMessageStatus(newStatus);
        }
    }

    for (auto& chat : m_chats) {
        for (auto& msg : chat.messages) {
            if (msg.messageId == messageId) {
                msg.status = newStatus;
                break;
            }
        }
    }
}

void MainWindow::onMessageReceived(const Message& msg) {
    Message normalizedMsg = msg;
    normalizedMsg.text = extractRenderableMessageText(msg, &normalizedMsg.file);
    if (normalizedMsg.chatId.isEmpty()) {
        normalizedMsg.chatId = m_currentChatId;
    }
    if (!normalizedMsg.senderId.isEmpty()) {
        User sender = m_users.value(normalizedMsg.senderId);
        bool changed = false;
        if (sender.userId.isEmpty()) {
            sender.userId = normalizedMsg.senderId;
            changed = true;
        }
        if (sender.username.trimmed().isEmpty() && !normalizedMsg.senderName.trimmed().isEmpty()) {
            sender.username = normalizedMsg.senderName.trimmed();
            changed = true;
        }
        if (sender.avatarUrl.trimmed().isEmpty() && !normalizedMsg.senderAvatarUrl.trimmed().isEmpty()) {
            sender.avatarUrl = normalizedMsg.senderAvatarUrl.trimmed();
            changed = true;
        }
        if (changed) {
            m_users[sender.userId] = sender;
        }
    }

    bool isPendingConfirmation = false;
    QString pendingIdToRemove;

    if (normalizedMsg.senderId == m_client->currentUserId()) {
        for (auto it = m_pendingMessages.begin(); it != m_pendingMessages.end(); ++it) {
            const bool sameChat = (it.value().chatId == normalizedMsg.chatId);
            const bool sameText = (it.value().text == normalizedMsg.text && it.value().replyToId == normalizedMsg.replyToId);
            const bool sameFile = !it.value().file.isNull() && !normalizedMsg.file.isNull() &&
                                  (resolveFileUrl(it.value().file.url) == resolveFileUrl(normalizedMsg.file.url));
            if (sameChat && (sameFile || sameText)) {
                isPendingConfirmation = true;
                pendingIdToRemove = it.key();
                break;
            }
        }
    }

    if (m_chats.contains(normalizedMsg.chatId)) {
        auto& chatMessages = m_chats[normalizedMsg.chatId].messages;
        auto existingIt = std::find_if(chatMessages.begin(), chatMessages.end(), [&normalizedMsg](const Message& existing) {
            return existing.messageId == normalizedMsg.messageId;
        });
        if (existingIt == chatMessages.end()) {
            chatMessages.push_back(normalizedMsg);
        } else {
            *existingIt = normalizedMsg;
        }
        for (int i = 0; i < m_chatListWidget->count(); i++) {
            if (m_chatListWidget->item(i)->data(Qt::UserRole).toString() == normalizedMsg.chatId) {
                QListWidgetItem* item = m_chatListWidget->takeItem(i);
                m_chatListWidget->insertItem(0, item);
                break;
            }
        }
    }

    if (m_currentChatId == normalizedMsg.chatId) {
        bool wasAtBottom = isScrolledToBottom();

        if (isPendingConfirmation) {
            for (int i = 0; i < m_chatList->count(); i++) {
                QListWidgetItem* item = m_chatList->item(i);
                if (item->data(Qt::UserRole + 6).toString() == pendingIdToRemove) {
                    if (m_messageWidgetsById.contains(pendingIdToRemove) &&
                        m_messageWidgetsById[pendingIdToRemove] == m_currentAudioSourceWidget) {
                        m_currentAudioSourceWidget = nullptr;
                    }
                    m_messageWidgetsById.remove(pendingIdToRemove);
                    m_messageItemsById.remove(pendingIdToRemove);
                    m_currentMessagePreviewById.remove(pendingIdToRemove);
                    m_currentMessageSenderById.remove(pendingIdToRemove);
                    delete m_chatList->takeItem(i);
                    break;
                }
            }
            m_pendingMessages.remove(pendingIdToRemove);
        }

        if (m_messageItemsById.contains(normalizedMsg.messageId)) {
            QListWidgetItem* existing = m_messageItemsById.value(normalizedMsg.messageId);
            const int row = m_chatList->row(existing);
            if (row >= 0) {
                delete m_chatList->takeItem(row);
            }
            m_messageItemsById.remove(normalizedMsg.messageId);
            m_messageWidgetsById.remove(normalizedMsg.messageId);
            m_currentMessagePreviewById.remove(normalizedMsg.messageId);
            m_currentMessageSenderById.remove(normalizedMsg.messageId);
        }

        addMessageBubble(normalizedMsg, false, false);

        if (normalizedMsg.senderId != m_client->currentUserId()) {
            m_client->sendMessageSeen(normalizedMsg.chatId, normalizedMsg.messageId);
        }

        if (wasAtBottom || normalizedMsg.senderId == m_client->currentUserId()) {
            smoothScrollToBottom();
        }
    } else if (isPendingConfirmation) {
        m_pendingMessages.remove(pendingIdToRemove);
    }

    showNotificationForMessage(normalizedMsg);
}

MessageStatus MainWindow::calculateMessageStatus(const Message& msg, const Chat& chat) {
    if (msg.senderId != m_client->currentUserId()) {
        return MessageStatus::Sent;
    }
    if (msg.messageId.startsWith("temp_")) {
        return MessageStatus::Pending;
    }

    int otherMembersCount = 0;
    int seenByOthers = 0;

    for (const QString& memberId : chat.members) {
        if (memberId != m_client->currentUserId()) {
            otherMembersCount++;
            if (msg.seenBy.contains(memberId)) {
                seenByOthers++;
            }
        }
    }

    if (seenByOthers > 0) {
        return MessageStatus::Seen;
    }
    return MessageStatus::Sent;
}

void MainWindow::onMessageSeenUpdate(const QString& chatId, const QString& messageId, const QString& userId) {
    if (m_chats.contains(chatId)) {
        for (auto& msg : m_chats[chatId].messages) {
            if (msg.messageId == messageId) {
                if (!msg.seenBy.contains(userId)) {
                    msg.seenBy.append(userId);
                }
                MessageStatus newStatus = calculateMessageStatus(msg, m_chats[chatId]);
                msg.status = newStatus;

                if (m_currentChatId == chatId) {
                    updateMessageStatus(messageId, newStatus);
                }
                break;
            }
        }
    }
}

void MainWindow::onChatListContextMenu(const QPoint& pos) {
    QListWidgetItem* item = m_chatList->itemAt(pos);
    if (!item) return;

    QString messageId = item->data(Qt::UserRole + 6).toString();
    QString senderId = item->data(Qt::UserRole + 8).toString();
    bool isMyMessage = (senderId == m_client->currentUserId());
    const bool hasAttachment = !item->data(FileUrlRole).toString().isEmpty();

    QMenu contextMenu(this);
    if (m_isDarkMode) {
        contextMenu.setStyleSheet(
            "QMenu {"
            " background-color: #2d2d2d;"
            " color: #ffffff;"
            " border: 1px solid #444444;"
            " padding: 4px;"
            "}"
            "QMenu::item {"
            " padding: 8px 24px;"
            " background-color: transparent;"
            "}"
            "QMenu::item:selected {"
            " background-color: #3b5278;"
            "}"
        );
    } else {
        contextMenu.setStyleSheet(
            "QMenu {"
            " background-color: #ffffff;"
            " color: #000000;"
            " border: 1px solid #cccccc;"
            " padding: 4px;"
            "}"
            "QMenu::item {"
            " padding: 8px 24px;"
            " background-color: transparent;"
            "}"
            "QMenu::item:selected {"
            " background-color: #e3f2fd;"
            "}"
        );
    }

    if (isMyMessage) {
        QAction* editAction = nullptr;
        if (!hasAttachment) {
            editAction = contextMenu.addAction("Edit");
        }
        QAction* deleteAction = contextMenu.addAction("Delete");
        if (editAction) {
            connect(editAction, &QAction::triggered, this, &MainWindow::onEditMessage);
        }
        connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteMessage);
        contextMenu.addSeparator();
    }

    QAction* replyAction = contextMenu.addAction("Reply");
    QAction* forwardAction = contextMenu.addAction("Forward");
    QAction* pinAction = nullptr;
    QAction* unpinAction = nullptr;
    if (m_chats.contains(m_currentChatId)) {
        const Chat& chat = m_chats[m_currentChatId];
        const bool canPin = (chat.chatType == "private") ||
                            (chat.chatType == "channel" && chat.ownerId == m_client->currentUserId());
        if (canPin) {
            pinAction = contextMenu.addAction("Pin");
            unpinAction = contextMenu.addAction("Unpin");
        }
    }

    connect(replyAction, &QAction::triggered, this, &MainWindow::onReplyToMessage);
    connect(forwardAction, &QAction::triggered, this, &MainWindow::onForwardMessage);
    if (pinAction) {
        connect(pinAction, &QAction::triggered, this, [this, messageId]() {
            if (!m_currentChatId.isEmpty()) {
                m_client->pinMessage(m_currentChatId, messageId);
            }
        });
    }
    if (unpinAction) {
        connect(unpinAction, &QAction::triggered, this, [this]() {
            if (!m_currentChatId.isEmpty()) {
                m_client->unpinMessage(m_currentChatId);
            }
        });
    }

    contextMenu.setProperty("messageId", messageId);
    contextMenu.setProperty("messageText", item->data(Qt::UserRole + 1).toString());

    contextMenu.exec(m_chatList->mapToGlobal(pos));
}

void MainWindow::onEditMessage() {
    QMenu* menu = qobject_cast<QMenu*>(sender()->parent());
    if (!menu) return;

    QString messageId = menu->property("messageId").toString();
    startEditMessage(messageId);
}

void MainWindow::onDeleteMessage() {
    QMenu* menu = qobject_cast<QMenu*>(sender()->parent());
    if (!menu) return;
    QString messageId = menu->property("messageId").toString();
    deleteMessageById(messageId);
}

void MainWindow::onReplyToMessage() {
    QMenu* menu = qobject_cast<QMenu*>(sender()->parent());
    if (!menu) return;
    QString messageId = menu->property("messageId").toString();
    startReplyToMessage(messageId);
}

void MainWindow::onForwardMessage() {
    QMenu* menu = qobject_cast<QMenu*>(sender()->parent());
    if (!menu) return;
    QString messageId = menu->property("messageId").toString();
    forwardMessageById(messageId);
}

void MainWindow::onCancelEdit() {
    m_editingMessageId.clear();
    m_editingOriginalText.clear();
    m_editBar->hide();
    m_messageInput->clear();
    m_sendBtn->setText("Send");
}

void MainWindow::onCancelReply() {
    m_replyingToMessageId.clear();
    m_replyingToText.clear();
    m_replyingToSender.clear();
    m_replyBar->hide();
}

void MainWindow::onMessageUpdated(const QString& chatId, const QString& messageId, const QString& newContent, qint64 editedAt) {
    Message updatedSnapshot;
    bool foundMessage = false;
    if (m_chats.contains(chatId)) {
        for (auto& msg : m_chats[chatId].messages) {
            if (msg.messageId == messageId) {
                msg.text = newContent;
                msg.text = extractRenderableMessageText(msg, &msg.file);
                msg.editedAt = editedAt;
                updatedSnapshot = msg;
                foundMessage = true;
                break;
            }
        }
    }
    const QString renderedText = foundMessage ? displayTextForMessage(updatedSnapshot) : newContent;
    if (foundMessage && chatId == m_currentChatId) {
        m_currentMessagePreviewById[messageId] = renderedText;
    }

    if (m_currentChatId == chatId) {
        for (int i = 0; i < m_chatList->count(); i++) {
            QListWidgetItem* item = m_chatList->item(i);
            if (item->data(Qt::UserRole + 6).toString() == messageId) {
                item->setData(Qt::UserRole + 1, renderedText);
                item->setData(Qt::UserRole + 7, editedAt);
                if (m_messageWidgetsById.contains(messageId) && m_messageWidgetsById[messageId]) {
                    m_messageWidgetsById[messageId]->setMessageText(renderedText);
                    m_messageWidgetsById[messageId]->setEditedAt(editedAt);
                    syncMessageWidgetSize(item);
                }
                break;
            }
        }
        updatePinnedMessageBar();
    }
}

void MainWindow::onMessageDeleted(const QString& chatId, const QString& messageId) {
    if (m_chats.contains(chatId)) {
        auto& messages = m_chats[chatId].messages;
        messages.erase(std::remove_if(messages.begin(), messages.end(),
            [&messageId](const Message& m) { return m.messageId == messageId; }), messages.end());
        if (m_chats[chatId].hasPinnedMessage && m_chats[chatId].pinnedMessage.messageId == messageId) {
            m_chats[chatId].hasPinnedMessage = false;
        }
    }

    if (m_currentChatId == chatId) {
        if (m_messageWidgetsById.contains(messageId) && m_messageWidgetsById[messageId] == m_currentAudioSourceWidget) {
            m_currentAudioSourceWidget = nullptr;
        }
        for (int i = 0; i < m_chatList->count(); i++) {
            QListWidgetItem* item = m_chatList->item(i);
            if (item->data(Qt::UserRole + 6).toString() == messageId) {
                delete m_chatList->takeItem(i);
                break;
            }
        }
        m_messageItemsById.remove(messageId);
        m_messageWidgetsById.remove(messageId);
        m_currentMessagePreviewById.remove(messageId);
        m_currentMessageSenderById.remove(messageId);
        updatePinnedMessageBar();
    }
    m_messageItemsById.remove(messageId);
    m_messageWidgetsById.remove(messageId);
    m_currentMessagePreviewById.remove(messageId);
    m_currentMessageSenderById.remove(messageId);
}

void MainWindow::onPresenceUpdated(const QString& userId, bool online)
{
    if (!m_users.contains(userId)) {
        return;
    }
    m_users[userId].online = online;

    std::vector<User> users;
    users.reserve(static_cast<size_t>(m_users.size()));
    for (auto it = m_users.constBegin(); it != m_users.constEnd(); ++it) {
        users.push_back(it.value());
    }
    onUserListUpdated(users);
}

void MainWindow::onTypingReceived(const QString& chatId, const QString& senderId)
{
    if (chatId != m_currentChatId || senderId == m_client->currentUserId()) {
        return;
    }

    QString senderName = senderId;
    if (m_users.contains(senderId) && !m_users[senderId].username.isEmpty()) {
        senderName = m_users[senderId].username;
    }
    statusBar()->showMessage(senderName + " is typing...", 2000);
}

void MainWindow::onPasswordChanged(const QString& token, qint64 expiresAt, const QString& warning)
{
    if (token.isEmpty()) {
        return;
    }
    if (expiresAt <= 0) {
        expiresAt = QDateTime::currentSecsSinceEpoch() + 7 * 24 * 60 * 60;
    }

    m_authToken = token;
    m_authExpiresAt = expiresAt;
    SessionStore::save(SessionData{m_client->currentUserId(), token, expiresAt});
    m_restClient->setAuthContext(m_client->currentUserId(), token);
    if (!warning.isEmpty()) {
        QMessageBox::information(this, "Password Changed", warning);
    } else {
        QMessageBox::information(this, "Password Changed", "Password updated successfully.");
    }
}

void MainWindow::onUserUpdated(const User& user)
{
    if (user.userId.isEmpty()) {
        return;
    }
    m_users[user.userId] = user;

    std::vector<User> users;
    users.reserve(static_cast<size_t>(m_users.size()));
    for (auto it = m_users.constBegin(); it != m_users.constEnd(); ++it) {
        users.push_back(it.value());
    }
    onUserListUpdated(users);
    updatePinnedMessageBar();
}

void MainWindow::onSendBtnClicked() {
    hideStickerPanel();
    QString text = m_messageInput->text().trimmed();
    if (text.isEmpty() || m_currentChatId.isEmpty()) return;

    if (!m_editingMessageId.isEmpty()) {
        if (text == m_editingOriginalText) {
            onCancelEdit();
            return;
        }
        m_client->editMessage(m_currentChatId, m_editingMessageId, text);
        onCancelEdit();
        return;
    }

    QString tempId = "temp_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    Message pendingMsg;
    pendingMsg.messageId = tempId;
    pendingMsg.chatId = m_currentChatId;
    pendingMsg.senderId = m_client->currentUserId();
    pendingMsg.text = text;
    pendingMsg.timestamp = QDateTime::currentSecsSinceEpoch();
    pendingMsg.status = MessageStatus::Pending;

    if (!m_replyingToMessageId.isEmpty()) {
        pendingMsg.replyToId = m_replyingToMessageId;
    }

    m_pendingMessages[tempId] = pendingMsg;

    bool wasAtBottom = isScrolledToBottom();
    addMessageBubble(pendingMsg, false, false);

    if (wasAtBottom) {
        smoothScrollToBottom();
    }

    m_client->sendMessage(m_currentChatId, text, m_replyingToMessageId);

    m_messageInput->clear();
    if (!m_replyingToMessageId.isEmpty()) {
        onCancelReply();
    }
}

void MainWindow::focusOnMessage(const QString& messageId) {
    for (int i = 0; i < m_chatList->count(); i++) {
        QListWidgetItem* item = m_chatList->item(i);
        QString itemMsgId = item->data(Qt::UserRole + 6).toString();
        if (itemMsgId == messageId) {
            m_chatList->scrollToItem(item, QAbstractItemView::PositionAtCenter);

            if (!m_highlightedMessageId.isEmpty() && m_messageWidgetsById.contains(m_highlightedMessageId)) {
                m_messageWidgetsById[m_highlightedMessageId]->setHighlighted(false);
            }
            m_highlightedMessageId = messageId;
            if (m_messageWidgetsById.contains(messageId)) {
                m_messageWidgetsById[messageId]->setHighlighted(true);
            }
            QTimer::singleShot(3000, this, [this]() {
                if (m_messageWidgetsById.contains(m_highlightedMessageId)) {
                    m_messageWidgetsById[m_highlightedMessageId]->setHighlighted(false);
                }
                m_highlightedMessageId.clear();
            });
            break;
        }
    }
}

void MainWindow::onChatListItemClicked(const QModelIndex& index) {
    const QString fileUrl = index.data(FileUrlRole).toString();
    if (!fileUrl.isEmpty()) {
        QString type = index.data(FileTypeRole).toString().toLower();
        if (type.isEmpty()) {
            const QString lower = fileUrl.toLower();
            if (lower.endsWith(".png") || lower.endsWith(".jpg") || lower.endsWith(".jpeg") || lower.endsWith(".gif") || lower.endsWith(".webp")) {
                type = "image/*";
            } else if (lower.endsWith(".mp4") || lower.endsWith(".webm") || lower.endsWith(".mov")) {
                type = "video/*";
            } else if (lower.endsWith(".mp3") || lower.endsWith(".wav") || lower.endsWith(".ogg") || lower.endsWith(".m4a")) {
                type = "audio/*";
            }
        }

        if (type.startsWith("image/")) {
            openMediaInViewer("image", fileUrl);
        } else if (type.startsWith("video/")) {
            openMediaInViewer("video", fileUrl);
        } else if (type.startsWith("audio/")) {
            const QString name = QFileInfo(QUrl(fileUrl).path()).fileName();
            playAudioTrack(fileUrl, name.isEmpty() ? QStringLiteral("Audio") : name, nullptr);
        } else {
            QDesktopServices::openUrl(QUrl(fileUrl));
        }
        return;
    }

    const QString text = index.data(Qt::UserRole + 1).toString().trimmed();
    if (text.isEmpty()) {
        return;
    }
    static const QRegularExpression urlRe(QStringLiteral("(https?://\\S+)"));
    const QRegularExpressionMatch match = urlRe.match(text);
    if (match.hasMatch()) {
        QDesktopServices::openUrl(QUrl(match.captured(1)));
    }
}

void MainWindow::setupTrayIcon()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }
    if (m_trayIcon) {
        return;
    }

    m_trayIcon = new QSystemTrayIcon(this);

    QIcon icon = windowIcon();
    if (icon.isNull()) {
        if (QIcon::hasThemeIcon("mail-message-new")) {
             icon = QIcon::fromTheme("mail-message-new");
        } else {
             QPixmap p(16,16);
             p.fill(Qt::red);
             icon = QIcon(p);
        }
    }
    m_trayIcon->setIcon(icon);
    m_trayIcon->setToolTip("Noveo Desktop");

    m_trayMenu = new QMenu(this);
    m_trayShowHideAction = m_trayMenu->addAction("Show/Hide");
    m_trayQuitAction = m_trayMenu->addAction("Quit");

    connect(m_trayShowHideAction, &QAction::triggered, this, &MainWindow::onTrayShowHide);
    connect(m_trayQuitAction, &QAction::triggered, this, &MainWindow::onTrayQuit);

    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);

    m_trayIcon->show();
}

void MainWindow::showNotificationForMessage(const Message& msg)
{
    if (!m_notificationsEnabled) {
        return;
    }
    if (!m_trayIcon || !m_trayIcon->isVisible()) {
        return;
    }

    if (msg.senderId == m_client->currentUserId()) {
        return;
    }

    if (isVisible() && isActiveWindow() && !isMinimized()) {
        return;
    }

    QString title = "New message";
    if (m_chats.contains(msg.chatId)) {
        title = resolveChatName(m_chats[msg.chatId]);
    }

    QString body = displayTextForMessage(msg).trimmed();
    if (body.isEmpty()) {
        body = msg.file.isNull() ? "New message" : "Sent an attachment";
    }
    if (body.size() > 100) {
        body = body.left(97) + "...";
    }

    m_trayIcon->showMessage(title, body, QSystemTrayIcon::Information, 5000);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_trayIcon && m_trayIcon->isVisible()) {
        hide();
        event->ignore();
        return;
    }

    m_manualDisconnect = true;
    if (m_reconnectTimer) {
        m_reconnectTimer->stop();
    }
    m_client->disconnectFromServer();
    QMainWindow::closeEvent(event);
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        onTrayShowHide();
    }
}

void MainWindow::onTrayShowHide()
{
    if (isVisible() && !isMinimized()) {
        hide();
    } else {
        showNormal();
        raise();
        activateWindow();
    }
}

void MainWindow::onTrayQuit()
{
    m_manualDisconnect = true;
    if (m_reconnectTimer) {
        m_reconnectTimer->stop();
    }
    m_client->disconnectFromServer();
    if (m_trayIcon) {
        m_trayIcon->hide();
    }
    qApp->quit();
}

void MainWindow::onNotificationsToggled(bool checked)
{
    m_notificationsEnabled = checked;
    QSettings settings("Noveo", "MessengerClient");
    settings.setValue("notificationsEnabled", m_notificationsEnabled);
    if (m_settingsDialog) {
        m_settingsDialog->setNotifications(checked);
    }
}
