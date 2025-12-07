#ifndef DATASTRUCTURES_H
#define DATASTRUCTURES_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <vector>

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
