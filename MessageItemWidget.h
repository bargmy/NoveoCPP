#ifndef MESSAGEITEMWIDGET_H
#define MESSAGEITEMWIDGET_H

#include <QWidget>

#include "DataStructures.h"

class QLabel;
class QMediaPlayer;
class QPixmap;
class QShowEvent;
class QPushButton;
class QToolButton;
class QVBoxLayout;
class QVideoWidget;

class MessageItemWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MessageItemWidget(const Message& message,
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
                               QWidget* parent = nullptr);

    QString messageId() const;
    void setMessageStatus(MessageStatus status);
    void setEditedAt(qint64 editedAt);
    void setMessageText(const QString& text);
    void setHighlighted(bool highlighted);
    void setTheme(bool darkMode);
    bool representsAudioUrl(const QString& url) const;
    void setAudioPlaying(bool playing);
    bool representsSenderAvatarUrl(const QString& url) const;
    void setSenderAvatar(const QPixmap& avatar);

signals:
    void replyRequested(const QString& messageId);
    void editRequested(const QString& messageId);
    void deleteRequested(const QString& messageId);
    void forwardRequested(const QString& messageId);
    void pinRequested(const QString& messageId);
    void openMediaRequested(const QString& mediaType, const QString& fileUrl);
    void openFileRequested(const QString& fileUrl);
    void playAudioRequested(const QString& fileUrl, const QString& displayName, MessageItemWidget* source);
    void replyAnchorClicked(const QString& replyToId);

protected:
    void showEvent(QShowEvent* event) override;

private:
    QString inferFileType() const;
    QString fileDisplayName() const;
    QString formatTimestamp() const;
    QString statusGlyph() const;
    void rebuildMetaText();
    void applyBubbleStyle();
    void applyControlStyles();
    void renderTextContent(QVBoxLayout* bubbleLayout);
    void renderAttachmentContent(QVBoxLayout* bubbleLayout);
    void renderActionRow(QVBoxLayout* bubbleLayout);
    bool shouldRenderTextContent() const;
    void maybeLoadImagePreview();
    void ensureVideoPlayer();

    Message m_message;
    QString m_senderName;
    QString m_senderAvatarUrl;
    QString m_fileUrl;
    QString m_replySender;
    QString m_replyPreviewText;
    QString m_currentUserId;
    QString m_chatType;
    bool m_isChannelOwner = false;
    bool m_darkMode = false;
    bool m_isHighlighted = false;

    QLabel* m_textLabel = nullptr;
    QLabel* m_metaLabel = nullptr;
    QLabel* m_senderLabel = nullptr;
    QLabel* m_avatarLabel = nullptr;
    QLabel* m_forwardedLabel = nullptr;
    QPushButton* m_replyAnchorButton = nullptr;
    QToolButton* m_imageButton = nullptr;
    QPushButton* m_fileButton = nullptr;
    QPushButton* m_audioButton = nullptr;
    QPushButton* m_videoPlayButton = nullptr;
    QPushButton* m_videoOpenButton = nullptr;
    QMediaPlayer* m_videoPlayer = nullptr;
    QVideoWidget* m_videoWidget = nullptr;
    QWidget* m_bubble = nullptr;
    bool m_imagePreviewRequested = false;
    bool m_imagePreviewLoaded = false;
    bool m_videoPlayerReady = false;
};

#endif // MESSAGEITEMWIDGET_H
