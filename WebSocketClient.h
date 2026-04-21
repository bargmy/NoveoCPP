#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QSslError>
#include <QUrl>
#include <QWebSocket>

#include "DataStructures.h"

class WebSocketClient : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketClient(QObject* parent = nullptr);

    void connectToServer();
    void disconnectFromServer();

    void login(const QString& username, const QString& password);
    void registerUser(const QString& username, const QString& password);
    void reconnectWithToken(const QString& userId, const QString& token);

    // Compatibility send helper used by old UI.
    void sendMessage(const QString& chatId, const QString& text, const QString& replyToId = QString());
    // Full payload message send used by parity features.
    void sendMessagePayload(const QString& chatId,
                            const QJsonObject& content,
                            const QString& replyToId = QString(),
                            const QString& recipientId = QString());

    void fetchHistory(const QString& chatId, qint64 beforeTimestamp);
    void sendMessageSeen(const QString& chatId, const QString& messageId);
    void editMessage(const QString& chatId, const QString& messageId, const QString& newText);
    void deleteMessage(const QString& chatId, const QString& messageId);

    void sendTyping(const QString& chatId);
    void joinChannel(const QString& chatId);
    void requestChannelByHandle(const QString& handle);
    void pinMessage(const QString& chatId, const QString& messageId);
    void unpinMessage(const QString& chatId);

    void updateUsername(const QString& username);
    void changePassword(const QString& oldPassword, const QString& newPassword);
    void deleteAccount(const QString& password);

    void voiceStart(const QString& chatId);
    void voiceJoin(const QString& chatId);
    void voiceLeave(const QString& chatId);
    void sendBinaryAudio(const QByteArray& audioPayload);

    void logout();

    bool isConnected() const;
    QString currentUserId() const { return m_currentUser.userId; }
    QString currentToken() const { return m_token; }
    qint64 tokenExpiresAt() const { return m_tokenExpiresAt; }

signals:
    void connected();
    void disconnected();

    void loginSuccess(const User& user, const QString& token, qint64 expiresAt);
    void authFailed(const QString& message);

    void chatHistoryReceived(const std::vector<Chat>& chats);
    void messageReceived(const Message& msg);
    void userListUpdated(const std::vector<User>& users);
    void errorOccurred(const QString& msg);
    void newChatCreated(const Chat& chat);

    void messageSeenUpdate(const QString& chatId, const QString& messageId, const QString& userId);
    void messageUpdated(const QString& chatId, const QString& messageId, const QString& newContent, qint64 editedAt);
    void messageDeleted(const QString& chatId, const QString& messageId);

    void presenceUpdated(const QString& userId, bool online);
    void typingReceived(const QString& chatId, const QString& senderId);
    void channelInfoReceived(const Chat& channel);
    void memberJoined(const QString& chatId, const QStringList& members);
    void messagePinned(const QString& chatId, const Message& message);
    void messageUnpinned(const QString& chatId);
    void passwordChanged(const QString& token, qint64 expiresAt, const QString& warning);
    void voiceChatUpdated(const VoiceChatParticipants& participantsByChat);
    void incomingCall(const QString& chatId,
                      const QString& callerId,
                      const QString& chatName,
                      const QString& callerName,
                      const QString& callerAvatar);
    void userUpdated(const User& user);

    // PCM voice packet from websocket binary frames.
    void binaryAudioReceived(const QString& userId, const QByteArray& pcm16Payload);

private slots:
    void onConnected();
    void onTextMessageReceived(const QString& message);
    void onBinaryMessageReceived(const QByteArray& message);
    void onSslErrors(const QList<QSslError>& errors);

private:
    void sendJson(const QJsonObject& payload);

    void handleLoginSuccess(const QJsonObject& data);
    void handleChatHistory(const QJsonObject& data);
    void handleMessage(const QJsonObject& data);
    void handleUserListUpdate(const QJsonObject& data);
    void handleNewChat(const QJsonObject& data);
    void handleMessageSeenUpdate(const QJsonObject& data);
    void handleMessageUpdated(const QJsonObject& data);
    void handleMessageDeleted(const QJsonObject& data);
    void handlePresenceUpdate(const QJsonObject& data);
    void handleTyping(const QJsonObject& data);
    void handleChannelInfo(const QJsonObject& data);
    void handleMemberJoined(const QJsonObject& data);
    void handleMessagePinned(const QJsonObject& data);
    void handleMessageUnpinned(const QJsonObject& data);
    void handlePasswordChanged(const QJsonObject& data);
    void handleVoiceChatUpdate(const QJsonObject& data);
    void handleIncomingCall(const QJsonObject& data);
    void handleUserUpdated(const QJsonObject& data);

    static QJsonObject parseContentObject(const QJsonValue& contentValue);
    static Message parseMessageObject(const QJsonObject& obj);
    static User parseUserObject(const QJsonObject& obj);
    static Chat parseChatObject(const QJsonObject& obj);
    static QString extractMessageText(const QJsonObject& contentObj);
    static FileAttachment parseFileAttachment(const QJsonValue& value);
    static ForwardedInfo parseForwardedInfo(const QJsonValue& value);
    static VoiceChatParticipants parseVoiceParticipantsMap(const QJsonObject& value);

    QWebSocket m_webSocket;
    User m_currentUser;
    QString m_token;
    qint64 m_tokenExpiresAt = 0;
};

#endif // WEBSOCKETCLIENT_H
