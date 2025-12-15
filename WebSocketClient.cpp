#include "WebSocketClient.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

WebSocketClient::WebSocketClient(QObject* parent) : QObject(parent)
{
    connect(&m_webSocket, &QWebSocket::connected, this, &WebSocketClient::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &WebSocketClient::disconnected);
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketClient::onTextMessageReceived);
    connect(&m_webSocket, &QWebSocket::sslErrors, this, &WebSocketClient::onSslErrors);
}

void WebSocketClient::connectToServer() {
    // URL from server config
    m_webSocket.open(QUrl(QStringLiteral("wss://api.pcpapc172.ir:8443/ws")));
}

void WebSocketClient::onSslErrors(const QList<QSslError>& errors) {
    Q_UNUSED(errors);
    m_webSocket.ignoreSslErrors();
}

void WebSocketClient::onConnected() {
    qDebug() << "WebSocket connected";
    emit connected();
}

bool WebSocketClient::isConnected() const {
    return m_webSocket.state() == QAbstractSocket::ConnectedState;
}

void WebSocketClient::login(const QString& username, const QString& password) {
    if (!isConnected()) return;
    QJsonObject payload;
    payload["type"] = "login_with_password";
    payload["username"] = username;
    payload["password"] = password;
    QJsonDocument doc(payload);
    m_webSocket.sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void WebSocketClient::sendMessage(const QString& chatId, const QString& text, const QString& replyToId) {
    if (!isConnected()) return;

    QJsonObject content;
    content["text"] = text;
    content["file"] = QJsonValue::Null;
    content["theme"] = QJsonValue::Null;

    QJsonObject payload;
    payload["type"] = "message";

    // NEW: Check if this is a new contact chat (format: user1_user2)
    if (chatId.contains("_") && !chatId.startsWith("temp")) {
        QStringList ids = chatId.split("_");
        if (ids.size() == 2) {
            // Extract the OTHER user's ID
            QString recipientId = (ids[0] == m_currentUser.userId) ? ids[1] : ids[0];
            payload["recipientId"] = recipientId;  // Server will create chat!
        }
        else {
            payload["chatId"] = chatId;
        }
    }
    else {
        payload["chatId"] = chatId;  // For existing chats
    }

    payload["content"] = content;

    if (!replyToId.isEmpty()) {
        payload["replyToId"] = replyToId;
    }
    else {
        payload["replyToId"] = QJsonValue::Null;
    }

    QJsonDocument doc(payload);
    m_webSocket.sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

// NEW: Implementation of fetchHistory
void WebSocketClient::fetchHistory(const QString& chatId, qint64 beforeTimestamp) {
    if (!isConnected()) return;

    QJsonObject payload;
    payload["type"] = "get_history"; // Assumed server event type
    payload["chatId"] = chatId;
    payload["before"] = (double)beforeTimestamp; // Send timestamp as double/number

    QJsonDocument doc(payload);
    m_webSocket.sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void WebSocketClient::onTextMessageReceived(QString message) {
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    if (type == "login_success") {
        handleLoginSuccess(obj);
    }
    else if (type == "auth_failed") {
        emit authFailed(obj["message"].toString());
    }
    else if (type == "chat_history") {
        handleChatHistory(obj);
    }
    else if (type == "message") {
        handleMessage(obj);
    }
    else if (type == "user_list_update") {
        handleUserListUpdate(obj);
    }
    else if (type == "new_chat_info") {
        handleNewChat(obj);
    }
    else if (type == "message_seen_update") {
        handleMessageSeenUpdate(obj);
    }
    else if (type == "error") {
        emit errorOccurred(obj["message"].toString());
    }
}

void WebSocketClient::handleLoginSuccess(const QJsonObject& data) {
    QJsonObject userObj = data["user"].toObject();
    m_currentUser.userId = userObj["userId"].toString();
    m_currentUser.username = userObj["username"].toString();
    m_currentUser.avatarUrl = userObj["avatarUrl"].toString();

    m_token = data["token"].toString();
    emit loginSuccess(m_currentUser, m_token);
}

void WebSocketClient::handleChatHistory(const QJsonObject& data) {
    std::vector<Chat> chats;
    QJsonArray chatsArray = data["chats"].toArray();

    for (const auto& val : chatsArray) {
        QJsonObject cObj = val.toObject();
        Chat chat;
        chat.chatId = cObj["chatId"].toString();
        chat.chatName = cObj["chatName"].toString();
        chat.chatType = cObj["chatType"].toString();
        chat.ownerId = cObj["ownerId"].toString();
        chat.avatarUrl = cObj["avatarUrl"].toString();

        QJsonArray membersArr = cObj["members"].toArray();
        for (const auto& m : membersArr) chat.members.append(m.toString());

        if (cObj.contains("messages")) {
            QJsonArray msgsArr = cObj["messages"].toArray();
            for (const auto& mVal : msgsArr) {
                QJsonObject mObj = mVal.toObject();
                Message msg;
                msg.messageId = mObj["messageId"].toString();
                msg.chatId = mObj["chatId"].toString();
                msg.senderId = mObj["senderId"].toString();
                msg.timestamp = (qint64)mObj["timestamp"].toDouble();

                QJsonObject contentObj;
                if (mObj["content"].isString()) {
                    QJsonDocument cd = QJsonDocument::fromJson(mObj["content"].toString().toUtf8());
                    contentObj = cd.object();
                }
                else {
                    contentObj = mObj["content"].toObject();
                }
                msg.text = contentObj["text"].toString();

                // NEW: Parse seenBy if present
                if (mObj.contains("seenBy") && mObj["seenBy"].isArray()) {
                    QJsonArray seenByArr = mObj["seenBy"].toArray();
                    for (const auto& val : seenByArr) {
                        msg.seenBy.append(val.toString());
                    }
                }

                chat.messages.push_back(msg);
            }
        }
        chats.push_back(chat);
    }
    emit chatHistoryReceived(chats);
}

void WebSocketClient::handleMessage(const QJsonObject& mObj) {
    Message msg;
    msg.messageId = mObj["messageId"].toString();
    msg.chatId = mObj["chatId"].toString();
    msg.senderId = mObj["senderId"].toString();
    msg.timestamp = (qint64)mObj["timestamp"].toDouble();

    QJsonObject contentObj;
    if (mObj["content"].isString()) {
        QJsonDocument cd = QJsonDocument::fromJson(mObj["content"].toString().toUtf8());
        contentObj = cd.object();
    }
    else {
        contentObj = mObj["content"].toObject();
    }
    msg.text = contentObj["text"].toString();

    // NEW: Parse seenBy if present
    if (mObj.contains("seenBy") && mObj["seenBy"].isArray()) {
        QJsonArray seenByArr = mObj["seenBy"].toArray();
        for (const auto& val : seenByArr) {
            msg.seenBy.append(val.toString());
        }
    }

    emit messageReceived(msg);
}

void WebSocketClient::handleUserListUpdate(const QJsonObject& data) {
    std::vector<User> users;
    QJsonArray usersArr = data["users"].toArray();

    for (const auto& uVal : usersArr) {
        QJsonObject uObj = uVal.toObject();
        User u;
        u.userId = uObj["userId"].toString();
        u.username = uObj["username"].toString();
        u.avatarUrl = uObj["avatarUrl"].toString();
        users.push_back(u);
    }
    emit userListUpdated(users);
}

void WebSocketClient::handleNewChat(const QJsonObject& data) {
    QJsonObject cObj = data["chat"].toObject();
    Chat chat;
    chat.chatId = cObj["chatId"].toString();
    chat.chatName = cObj["chatName"].toString();
    chat.chatType = cObj["chatType"].toString();
    chat.avatarUrl = cObj["avatarUrl"].toString();

    QJsonArray membersArr = cObj["members"].toArray();
    for (const auto& m : membersArr) chat.members.append(m.toString());

    emit newChatCreated(chat);
}

void WebSocketClient::sendMessageSeen(const QString& chatId, const QString& messageId) {
    if (!isConnected()) return;

    QJsonObject payload;
    payload["type"] = "message_seen";
    payload["chatId"] = chatId;
    payload["messageId"] = messageId;

    QJsonDocument doc(payload);
    m_webSocket.sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void WebSocketClient::handleMessageSeenUpdate(const QJsonObject& data) {
    QString chatId = data["chatId"].toString();
    QString messageId = data["messageId"].toString();
    QString userId = data["userId"].toString();

    emit messageSeenUpdate(chatId, messageId, userId);
}

void WebSocketClient::logout() {
    m_currentUser = User();
    m_token.clear();
    m_webSocket.close();
}