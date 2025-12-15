#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "DataStructures.h"

class WebSocketClient : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketClient(QObject* parent = nullptr);

    void connectToServer();
    void login(const QString& username, const QString& password);
    void sendMessage(const QString& chatId, const QString& text, const QString& replyToId = QString());

    // NEW: Function to request older messages
    void fetchHistory(const QString& chatId, qint64 beforeTimestamp);

    // NEW: Send seen receipt
    void sendMessageSeen(const QString& chatId, const QString& messageId);

    // NEW: Edit message
    void editMessage(const QString& chatId, const QString& messageId, const QString& newText);

    void logout();

    bool isConnected() const;
    QString currentUserId() const { return m_currentUser.userId; }

signals:
    void connected();
    void disconnected();
    void loginSuccess(const User& user, const QString& token);
    void authFailed(const QString& message);
    void chatHistoryReceived(const std::vector<Chat>& chats);
    void messageReceived(const Message& msg);
    void userListUpdated(const std::vector<User>& users);
    void errorOccurred(const QString& msg);
    void newChatCreated(const Chat& chat);
    void messageSeenUpdate(const QString& chatId, const QString& messageId, const QString& userId);
    void messageUpdated(const QString& chatId, const QString& messageId, const QString& newContent, qint64 editedAt);

private slots:
    void onConnected();
    void onTextMessageReceived(QString message);
    void onSslErrors(const QList<QSslError>& errors);

private:
    QWebSocket m_webSocket;
    User m_currentUser;
    QString m_token;

    void handleLoginSuccess(const QJsonObject& data);
    void handleChatHistory(const QJsonObject& data);
    void handleMessage(const QJsonObject& data);
    void handleUserListUpdate(const QJsonObject& data);
    void handleNewChat(const QJsonObject& data);
    void handleMessageSeenUpdate(const QJsonObject& data);
    void handleMessageUpdated(const QJsonObject& data);
};

#endif // WEBSOCKETCLIENT_H