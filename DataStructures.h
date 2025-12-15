#ifndef DATASTRUCTURES_H
#define DATASTRUCTURES_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <vector>

// Message delivery status
enum class MessageStatus {
    Pending,   // Waiting for server confirmation (⏱)
    Sent,      // Confirmed by server (✓)
    Seen       // Seen by recipient(s) (✓✓)
};

struct User {
    QString userId;
    QString username;
    QString avatarUrl;
    bool online;
};

struct Message {
    QString messageId;
    QString chatId;
    QString senderId;
    QString text;
    qint64 timestamp;

    // NEW: Status tracking
    MessageStatus status = MessageStatus::Sent;  // Default for received messages

    // NEW: Seen tracking
    QStringList seenBy;  // List of user IDs who have seen this message

    // NEW: Edit tracking
    qint64 editedAt = 0;  // Timestamp when edited (0 = not edited)
};

struct Chat {
    QString chatId;
    QString chatName; // For groups/channels
    QString chatType; // 'private', 'group', 'channel'
    QStringList members;
    QString ownerId;
    std::vector<Message> messages;
    int unreadCount;
    QString avatarUrl;
};

#endif // DATASTRUCTURES_H