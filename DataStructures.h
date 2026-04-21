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
    QString displayName;
    QString alias;
    QString handle;
    QString bio;
    QString avatarUrl;
    bool online;
};

struct Contact {
    QString userId;
    QString username;
    QString displayName;
    QString alias;
    QString avatarUrl;
    bool isContact = false;
};

struct Gift {
    QString giftId;
    QString name;
    QString imageUrl;
    int quantity = 1;
    QString priceLabel;
};

struct GroupInfo {
    QString groupId;
    QString name;
    int memberCount = 0;
    bool isMutual = false;
};

struct WalletTransaction {
    QString id;
    QString description;
    QString type;
    double amountTenths = 0.0;
    qint64 createdAt = 0;
};

struct WalletOverview {
    double balanceTenths = 0.0;
    std::vector<WalletTransaction> transactions;
};

struct UserProfile {
    QString userId;
    QString username;
    QString displayName;
    QString handle;
    QString bio;
    QString avatarUrl;
    qint64 createdAt = 0;
    std::vector<Gift> gifts;
    std::vector<GroupInfo> groups;
    std::vector<GroupInfo> mutualGroups;
    WalletOverview wallet;
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

    // NEW: Reply tracking
    QString replyToId;  // Message ID this is replying to (empty = not a reply)
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
