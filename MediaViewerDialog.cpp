#include "MediaViewerDialog.h"

#include <QHBoxLayout>
#include <QHideEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QMediaContent>
#include <QMediaPlayer>
#include <QMouseEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QStackedWidget>
#include <QUrl>
#include <QVBoxLayout>
#include <QVideoWidget>

MediaViewerDialog::MediaViewerDialog(QWidget* parent)
    : QDialog(parent),
      m_nam(new QNetworkAccessManager(this))
{
    setWindowTitle(QStringLiteral("Media Viewer"));
    setModal(true);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose, false);
    resize(960, 720);
    setStyleSheet(
        "QDialog { background: rgba(10, 10, 10, 220); }"
        "QLabel { color: #ffffff; }"
        "QPushButton#closeViewerBtn {"
        " background: rgba(0, 0, 0, 120);"
        " color: #ffffff;"
        " border: 1px solid rgba(255,255,255,100);"
        " border-radius: 14px;"
        " font-size: 16px;"
        " font-weight: 700;"
        "}"
        "QPushButton#closeViewerBtn:hover { background: rgba(0, 0, 0, 170); }");

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(8);

    m_stack = new QStackedWidget(this);
    m_stack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_imageLabel = new QLabel(m_stack);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setText(QStringLiteral("Loading image..."));
    m_imageLabel->setWordWrap(true);
    m_imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_stack->addWidget(m_imageLabel);

    auto* videoPage = new QWidget(m_stack);
    auto* videoLayout = new QVBoxLayout(videoPage);
    videoLayout->setContentsMargins(0, 0, 0, 0);
    videoLayout->setSpacing(0);
    m_videoWidget = new QVideoWidget(videoPage);
    m_videoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    videoLayout->addWidget(m_videoWidget);
    m_stack->addWidget(videoPage);

    root->addWidget(m_stack);

    m_videoPlayer = new QMediaPlayer(this);
    m_videoPlayer->setVideoOutput(m_videoWidget);

    m_closeButton = new QPushButton(QStringLiteral("x"), this);
    m_closeButton->setObjectName(QStringLiteral("closeViewerBtn"));
    m_closeButton->setFixedSize(28, 28);
    m_closeButton->setCursor(Qt::PointingHandCursor);
    connect(m_closeButton, &QPushButton::clicked, this, &MediaViewerDialog::hide);
    positionCloseButton();
}

MediaViewerDialog::~MediaViewerDialog()
{
    if (m_pendingReply) {
        m_pendingReply->abort();
    }
}

void MediaViewerDialog::showImage(const QUrl& url)
{
    if (m_videoPlayer) {
        m_videoPlayer->stop();
    }
    if (parentWidget()) {
        setGeometry(parentWidget()->frameGeometry());
    }
    m_stack->setCurrentWidget(m_imageLabel);
    m_imageLabel->setText(QStringLiteral("Loading image..."));
    m_currentImage = QPixmap();
    loadImageFromUrl(url);
    show();
    raise();
    activateWindow();
}

void MediaViewerDialog::showVideo(const QUrl& url)
{
    if (!m_videoPlayer) {
        return;
    }
    if (parentWidget()) {
        setGeometry(parentWidget()->frameGeometry());
    }
    if (m_pendingReply) {
        m_pendingReply->abort();
        m_pendingReply = nullptr;
    }
    m_stack->setCurrentWidget(m_videoWidget->parentWidget());
    m_videoPlayer->setMedia(QMediaContent(url));
    m_videoPlayer->play();
    show();
    raise();
    activateWindow();
}

void MediaViewerDialog::clearMedia()
{
    if (m_pendingReply) {
        m_pendingReply->abort();
        m_pendingReply = nullptr;
    }
    if (m_videoPlayer) {
        m_videoPlayer->stop();
    }
    m_currentImage = QPixmap();
    if (m_imageLabel) {
        m_imageLabel->clear();
    }
}

void MediaViewerDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        hide();
        return;
    }
    QDialog::keyPressEvent(event);
}

void MediaViewerDialog::mousePressEvent(QMouseEvent* event)
{
    if (!m_stack || !m_stack->geometry().contains(event->pos())) {
        hide();
        return;
    }
    QDialog::mousePressEvent(event);
}

void MediaViewerDialog::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    positionCloseButton();
    updateImageDisplay();
}

void MediaViewerDialog::hideEvent(QHideEvent* event)
{
    clearMedia();
    QDialog::hideEvent(event);
}

void MediaViewerDialog::positionCloseButton()
{
    if (!m_closeButton) {
        return;
    }
    const int margin = 16;
    m_closeButton->move(width() - m_closeButton->width() - margin, margin);
    m_closeButton->raise();
}

void MediaViewerDialog::loadImageFromUrl(const QUrl& url)
{
    if (!url.isValid()) {
        m_imageLabel->setText(QStringLiteral("Unable to load image."));
        return;
    }

    if (url.isLocalFile() || url.scheme().isEmpty()) {
        QPixmap px(url.isLocalFile() ? url.toLocalFile() : url.toString());
        if (px.isNull()) {
            m_imageLabel->setText(QStringLiteral("Unable to load image."));
            return;
        }
        m_currentImage = px;
        updateImageDisplay();
        return;
    }

    if (m_pendingReply) {
        m_pendingReply->abort();
        m_pendingReply = nullptr;
    }

    QNetworkRequest request(url);
    m_pendingReply = m_nam->get(request);
    connect(m_pendingReply, &QNetworkReply::finished, this, [this]() {
        QNetworkReply* reply = m_pendingReply;
        m_pendingReply = nullptr;
        if (!reply) {
            return;
        }
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            m_imageLabel->setText(QStringLiteral("Unable to load image."));
            return;
        }
        QPixmap px;
        if (!px.loadFromData(reply->readAll())) {
            m_imageLabel->setText(QStringLiteral("Unable to load image."));
            return;
        }
        m_currentImage = px;
        updateImageDisplay();
    });
}

void MediaViewerDialog::updateImageDisplay()
{
    if (!m_imageLabel) {
        return;
    }
    if (m_currentImage.isNull()) {
        return;
    }
    const QSize target = m_imageLabel->size() - QSize(12, 12);
    if (target.width() <= 0 || target.height() <= 0) {
        return;
    }
    m_imageLabel->setPixmap(m_currentImage.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
