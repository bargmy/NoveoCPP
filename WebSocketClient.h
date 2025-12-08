#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include "DataStructures.h"

class WebSocketClient : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketClient(QObject *parent = nullptr);

    // Core Connection Methods
    void connectToServer();
    void login(const QString &username, const QString &password);
    void sendMessage(const QString &chatId, const QString &text, const QString &replyToId = QString());
    
    // New Feature Methods
    void sendTyping(const QString &chatId);
    void deleteMessage(const QString &chatId, const QString &messageId);

    bool isConnected() const;
    QString currentUserId() const { return m_currentUser.userId; }

signals:
    // Connection & Auth Signals
    void connected();
    void disconnected();
    void loginSuccess(const User &user, const QString &token);
    void authFailed(const QString &message);
    void errorOccurred(const QString &msg);

    // Data Signals
    void chatHistoryReceived(const std::vector<Chat> &chats);
    void messageReceived(const Message &msg);
    void userListUpdated(const std::vector<User> &users);
    void newChatCreated(const Chat &chat);
    
    // New Feature Signals
    void userTyping(const QString &chatId, const QString &username);
    void userPresenceChanged(const QString &userId, bool online);
    void messageDeleted(const QString &chatId, const QString &messageId);

private slots:
    void onConnected();
    void onTextMessageReceived(QString message);
    void onSslErrors(const QList<QSslError> &errors);

private:
    QWebSocket m_webSocket;
    User m_currentUser;
    QString m_token;

    // Handlers for specific packet types
    void handleLoginSuccess(const QJsonObject &data);
    void handleChatHistory(const QJsonObject &data);
    void handleMessage(const QJsonObject &data);
    void handleUserListUpdate(const QJsonObject &data);
    void handleNewChat(const QJsonObject &data);
};

#endif // WEBSOCKETCLIENT_H
