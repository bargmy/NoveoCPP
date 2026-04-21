#ifndef DATASTRUCTURES_H
#define DATASTRUCTURES_H

#include <QMap>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QStringList>

#include <vector>

// Message delivery status
enum class MessageStatus {
    Pending,   // Waiting for server confirmation
    Sent,      // Confirmed by server
    Seen       // Seen by recipient(s)
};

struct FileAttachment {
    QString url;
    QString name;
    QString type;
    qint64 size = 0;

    bool isNull() const {
        return url.isEmpty() && name.isEmpty() && type.isEmpty() && size == 0;
    }
};

struct ForwardedInfo {
    QString from;
    qint64 originalTs = 0;

    bool isNull() const {
        return from.isEmpty() && originalTs <= 0;
    }
};

struct User {
    QString userId;
    QString username;
    QString avatarUrl;
    bool online = false;
    bool blockGroupInvites = false;
};

struct Message {
    QString messageId;
    QString chatId;
    QString senderId;
    QString senderName;
    QString senderAvatarUrl;
    QString text;
    qint64 timestamp = 0;

    // Keep raw content payload for forward compatibility with server updates.
    QString rawContent;
    FileAttachment file;
    QString theme;
    ForwardedInfo forwardedInfo;

    MessageStatus status = MessageStatus::Sent;
    QStringList seenBy;
    qint64 editedAt = 0;
    QString replyToId;
    bool pending = false;
};

struct Chat {
    QString chatId;
    QString chatName; // For groups/channels
    QString chatType; // "private", "group", "channel"
    QString handle;
    QStringList members;
    QString ownerId;
    std::vector<Message> messages;
    int unreadCount = 0;
    QString avatarUrl;
    bool isVerified = false;
    qint64 createdAt = 0;
    bool hasPinnedMessage = false;
    Message pinnedMessage;
};

using VoiceChatParticipants = QMap<QString, QStringList>;

#endif // DATASTRUCTURES_H
