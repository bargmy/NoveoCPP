#include "WebSocketClient.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHostInfo>
#include <QSslSocket>
#include <QAbstractSocket>
#include <QNetworkProxy>

WebSocketClient::WebSocketClient(QObject* parent) : QObject(parent)
{
    connect(&m_webSocket, &QWebSocket::connected, this, &WebSocketClient::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, [this]() {
        emit debugLog(QString("WebSocket disconnected | state=%1 | closeCode=%2 | closeReason=%3")
                          .arg(socketStateToString(m_webSocket.state()))
                          .arg(static_cast<int>(m_webSocket.closeCode()))
                          .arg(m_webSocket.closeReason()));
        emit disconnected();
    });
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketClient::onTextMessageReceived);
    connect(&m_webSocket, &QWebSocket::sslErrors, this, &WebSocketClient::onSslErrors);
    connect(&m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &WebSocketClient::onSocketError);
    connect(&m_webSocket, &QWebSocket::stateChanged, this, &WebSocketClient::onStateChanged);
}

QString WebSocketClient::socketStateToString(QAbstractSocket::SocketState state) const {
    switch (state) {
    case QAbstractSocket::UnconnectedState: return "UnconnectedState";
    case QAbstractSocket::HostLookupState: return "HostLookupState";
    case QAbstractSocket::ConnectingState: return "ConnectingState";
    case QAbstractSocket::ConnectedState: return "ConnectedState";
    case QAbstractSocket::BoundState: return "BoundState";
    case QAbstractSocket::ListeningState: return "ListeningState";
    case QAbstractSocket::ClosingState: return "ClosingState";
    default: return QString("UnknownState(%1)").arg(static_cast<int>(state));
    }
}

QString WebSocketClient::socketErrorToString(QAbstractSocket::SocketError error) const {
    switch (error) {
    case QAbstractSocket::ConnectionRefusedError: return "ConnectionRefusedError";
    case QAbstractSocket::RemoteHostClosedError: return "RemoteHostClosedError";
    case QAbstractSocket::HostNotFoundError: return "HostNotFoundError";
    case QAbstractSocket::SocketAccessError: return "SocketAccessError";
    case QAbstractSocket::SocketResourceError: return "SocketResourceError";
    case QAbstractSocket::SocketTimeoutError: return "SocketTimeoutError";
    case QAbstractSocket::DatagramTooLargeError: return "DatagramTooLargeError";
    case QAbstractSocket::NetworkError: return "NetworkError";
    case QAbstractSocket::AddressInUseError: return "AddressInUseError";
    case QAbstractSocket::SocketAddressNotAvailableError: return "SocketAddressNotAvailableError";
    case QAbstractSocket::UnsupportedSocketOperationError: return "UnsupportedSocketOperationError";
    case QAbstractSocket::ProxyAuthenticationRequiredError: return "ProxyAuthenticationRequiredError";
    case QAbstractSocket::SslHandshakeFailedError: return "SslHandshakeFailedError";
    case QAbstractSocket::UnfinishedSocketOperationError: return "UnfinishedSocketOperationError";
    case QAbstractSocket::ProxyConnectionRefusedError: return "ProxyConnectionRefusedError";
    case QAbstractSocket::ProxyConnectionClosedError: return "ProxyConnectionClosedError";
    case QAbstractSocket::ProxyConnectionTimeoutError: return "ProxyConnectionTimeoutError";
    case QAbstractSocket::ProxyNotFoundError: return "ProxyNotFoundError";
    case QAbstractSocket::ProxyProtocolError: return "ProxyProtocolError";
    case QAbstractSocket::OperationError: return "OperationError";
    case QAbstractSocket::SslInternalError: return "SslInternalError";
    case QAbstractSocket::SslInvalidUserDataError: return "SslInvalidUserDataError";
    case QAbstractSocket::TemporaryError: return "TemporaryError";
    case QAbstractSocket::UnknownSocketError: return "UnknownSocketError";
    default: return QString("UnknownSocketError(%1)").arg(static_cast<int>(error));
    }
}

void WebSocketClient::connectToServer() {
    const QUrl url(QStringLiteral("wss://noveo.ir:8443/ws"));
    emit debugLog(QString("connectToServer() called | currentState=%1 | target=%2")
                      .arg(socketStateToString(m_webSocket.state()))
                      .arg(url.toString()));

    const QNetworkProxy proxy = m_webSocket.proxy();
    emit debugLog(QString("Proxy info | type=%1 | host=%2 | port=%3")
                      .arg(static_cast<int>(proxy.type()))
                      .arg(proxy.hostName())
                      .arg(proxy.port()));

    emit debugLog(QString("SSL support | supportsSsl=%1 | buildVersion=%2 | runtimeVersion=%3")
                      .arg(QSslSocket::supportsSsl() ? "true" : "false")
                      .arg(QSslSocket::sslLibraryBuildVersionString())
                      .arg(QSslSocket::sslLibraryVersionString()));

    QHostInfo hostInfo = QHostInfo::fromName(url.host());
    if (hostInfo.error() != QHostInfo::NoError) {
        emit debugLog(QString("DNS lookup failed | host=%1 | error=%2")
                          .arg(url.host())
                          .arg(hostInfo.errorString()));
    } else {
        QStringList addresses;
        for (const QHostAddress& address : hostInfo.addresses()) {
            addresses << address.toString();
        }
        emit debugLog(QString("DNS lookup success | host=%1 | addresses=%2")
                          .arg(url.host())
                          .arg(addresses.join(", ")));
    }

    if (m_webSocket.state() != QAbstractSocket::UnconnectedState) {
        emit debugLog(QString("Socket is not idle before open | state=%1 | aborting previous socket")
                          .arg(socketStateToString(m_webSocket.state())));
        m_webSocket.abort();
    }

    emit debugLog(QString("Opening websocket now | url=%1").arg(url.toString()));
    m_webSocket.open(url);
}

void WebSocketClient::onStateChanged(QAbstractSocket::SocketState state) {
    emit debugLog(QString("WebSocket state changed -> %1")
                      .arg(socketStateToString(state)));
}

void WebSocketClient::onSocketError(QAbstractSocket::SocketError error) {
    const QString message = QString("WebSocket error | code=%1 | name=%2 | text=%3")
                                .arg(static_cast<int>(error))
                                .arg(socketErrorToString(error))
                                .arg(m_webSocket.errorString());
    emit debugLog(message);
    emit errorOccurred(message);
}

void WebSocketClient::onSslErrors(const QList<QSslError>& errors) {
    QStringList sslMessages;
    for (const QSslError& error : errors) {
        sslMessages << error.errorString();
    }
    emit debugLog(QString("SSL errors encountered | count=%1 | details=%2")
                      .arg(errors.size())
                      .arg(sslMessages.join(" | ")));
    m_webSocket.ignoreSslErrors();
    emit debugLog("SSL errors were ignored by client");
}

void WebSocketClient::onConnected() {
    emit debugLog(QString("WebSocket connected | peer=%1:%2 | requestUrl=%3")
                      .arg(m_webSocket.peerName())
                      .arg(m_webSocket.peerPort())
                      .arg(m_webSocket.requestUrl().toString()));
    qDebug() << "WebSocket connected";
    emit connected();
}

bool WebSocketClient::isConnected() const {
    return m_webSocket.state() == QAbstractSocket::ConnectedState;
}

void WebSocketClient::login(const QString& username, const QString& password) {
    if (!isConnected()) {
        emit debugLog(QString("WS OUT skipped: login_with_password because socket is not connected | state=%1")
                          .arg(socketStateToString(m_webSocket.state())));
        return;
    }
    QJsonObject payload;
    payload["type"] = "login_with_password";
    payload["username"] = username;
    payload["password"] = password;
    QJsonDocument doc(payload);
    emit debugLog(QString("WS OUT: login_with_password for user '%1'").arg(username));
    m_webSocket.sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void WebSocketClient::sendMessage(const QString& chatId, const QString& text, const QString& replyToId) {
    if (!isConnected()) {
        emit debugLog(QString("WS OUT skipped: message because socket is not connected | state=%1 | chatId=%2")
                          .arg(socketStateToString(m_webSocket.state()))
                          .arg(chatId));
        return;
    }

    QJsonObject content;
    content["text"] = text;
    content["file"] = QJsonValue::Null;
    content["theme"] = QJsonValue::Null;

    QJsonObject payload;
    payload["type"] = "message";

    if (chatId.contains("_") && !chatId.startsWith("temp")) {
        QStringList ids = chatId.split("_");
        if (ids.size() == 2) {
            QString recipientId = (ids[0] == m_currentUser.userId) ? ids[1] : ids[0];
            payload["recipientId"] = recipientId;
        }
        else {
            payload["chatId"] = chatId;
        }
    }
    else {
        payload["chatId"] = chatId;
    }

    payload["content"] = content;

    if (!replyToId.isEmpty()) {
        payload["replyToId"] = replyToId;
    }
    else {
        payload["replyToId"] = QJsonValue::Null;
    }

    QJsonDocument doc(payload);
    emit debugLog(QString("WS OUT: message to chat '%1' | length=%2 | replyToId=%3")
                      .arg(chatId)
                      .arg(text.size())
                      .arg(replyToId.isEmpty() ? QStringLiteral("<none>") : replyToId));
    m_webSocket.sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void WebSocketClient::fetchHistory(const QString& chatId, qint64 beforeTimestamp) {
    if (!isConnected()) {
        emit debugLog(QString("WS OUT skipped: get_history because socket is not connected | state=%1 | chatId=%2")
                          .arg(socketStateToString(m_webSocket.state()))
                          .arg(chatId));
        return;
    }

    QJsonObject payload;
    payload["type"] = "get_history";
    payload["chatId"] = chatId;
    payload["before"] = (double)beforeTimestamp;

    QJsonDocument doc(payload);
    emit debugLog(QString("WS OUT: get_history | chatId=%1 | before=%2")
                      .arg(chatId)
                      .arg(beforeTimestamp));
    m_webSocket.sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void WebSocketClient::onTextMessageReceived(QString message) {
    emit debugLog(QString("WS IN raw length=%1 | preview=%2")
                      .arg(message.size())
                      .arg(message.left(300)));
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        emit debugLog("WS IN parse failure: payload is not a JSON object");
        return;
    }

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();
    emit debugLog(QString("WS IN type=%1")
                      .arg(type.isEmpty() ? QStringLiteral("<missing>") : type));

    if (type == "login_success") {
        handleLoginSuccess(obj);
    }
    else if (type == "auth_failed") {
        emit debugLog(QString("Auth failed from server | message=%1").arg(obj["message"].toString()));
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
    else if (type == "message_updated") {
        handleMessageUpdated(obj);
    }
    else if (type == "message_deleted") {
        handleMessageDeleted(obj);
    }
    else if (type == "error") {
        emit debugLog(QString("Server error payload | message=%1").arg(obj["message"].toString()));
        emit errorOccurred(obj["message"].toString());
    }
    else {
        emit debugLog(QString("WS IN unhandled type=%1").arg(type));
    }
}

void WebSocketClient::handleLoginSuccess(const QJsonObject& data) {
    QJsonObject userObj = data["user"].toObject();
    m_currentUser.userId = userObj["userId"].toString();
    m_currentUser.username = userObj["username"].toString();
    m_currentUser.avatarUrl = userObj["avatarUrl"].toString();

    m_token = data["token"].toString();
    emit debugLog(QString("Login success | userId=%1 | username=%2 | tokenLength=%3")
                      .arg(m_currentUser.userId)
                      .arg(m_currentUser.username)
                      .arg(m_token.size()));
    emit loginSuccess(m_currentUser, m_token);
}

void WebSocketClient::handleChatHistory(const QJsonObject& data) {
    std::vector<Chat> chats;
    QJsonArray chatsArray = data["chats"].toArray();
    emit debugLog(QString("chat_history received | chats=%1").arg(chatsArray.size()));

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

                if (contentObj.contains("file") && !contentObj["file"].isNull()) {
                    QString captionText = contentObj["text"].toString();

                    if (!captionText.isEmpty()) {
                        msg.text = captionText + " [Image]";
                    }
                    else {
                        QJsonObject fileObj = contentObj["file"].toObject();
                        QString fileName = fileObj["name"].toString();
                        if (!fileName.isEmpty()) {
                            msg.text = "[" + fileName + "]";
                        }
                        else {
                            msg.text = "[Image]";
                        }
                    }
                }
                else {
                    msg.text = contentObj["text"].toString();
                }

                if (mObj.contains("editedAt")) {
                    msg.editedAt = (qint64)mObj["editedAt"].toDouble();
                }

                if (mObj.contains("replyToId")) {
                    msg.replyToId = mObj["replyToId"].toString();
                }

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

    if (contentObj.contains("file") && !contentObj["file"].isNull()) {
        QString captionText = contentObj["text"].toString();

        if (!captionText.isEmpty()) {
            msg.text = captionText + " [Image]";
        }
        else {
            QJsonObject fileObj = contentObj["file"].toObject();
            QString fileName = fileObj["name"].toString();
            if (!fileName.isEmpty()) {
                msg.text = "[" + fileName + "]";
            }
            else {
                msg.text = "[Image]";
            }
        }
    }
    else {
        msg.text = contentObj["text"].toString();
    }

    if (mObj.contains("editedAt")) {
        msg.editedAt = (qint64)mObj["editedAt"].toDouble();
    }

    if (mObj.contains("replyToId")) {
        msg.replyToId = mObj["replyToId"].toString();
    }

    if (mObj.contains("seenBy") && mObj["seenBy"].isArray()) {
        QJsonArray seenByArr = mObj["seenBy"].toArray();
        for (const auto& val : seenByArr) {
            msg.seenBy.append(val.toString());
        }
    }

    emit debugLog(QString("Message event received | chatId=%1 | messageId=%2 | senderId=%3")
                      .arg(msg.chatId)
                      .arg(msg.messageId)
                      .arg(msg.senderId));
    emit messageReceived(msg);
}

void WebSocketClient::handleUserListUpdate(const QJsonObject& data) {
    std::vector<User> users;
    QJsonArray usersArr = data["users"].toArray();
    emit debugLog(QString("user_list_update received | users=%1").arg(usersArr.size()));

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

    emit debugLog(QString("new_chat_info received | chatId=%1 | chatType=%2")
                      .arg(chat.chatId)
                      .arg(chat.chatType));
    emit newChatCreated(chat);
}

void WebSocketClient::sendMessageSeen(const QString& chatId, const QString& messageId) {
    if (!isConnected()) {
        emit debugLog(QString("WS OUT skipped: message_seen because socket is not connected | state=%1 | chatId=%2 | messageId=%3")
                          .arg(socketStateToString(m_webSocket.state()))
                          .arg(chatId)
                          .arg(messageId));
        return;
    }

    QJsonObject payload;
    payload["type"] = "message_seen";
    payload["chatId"] = chatId;
    payload["messageId"] = messageId;

    QJsonDocument doc(payload);
    emit debugLog(QString("WS OUT: message_seen | chatId=%1 | messageId=%2")
                      .arg(chatId)
                      .arg(messageId));
    m_webSocket.sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void WebSocketClient::handleMessageSeenUpdate(const QJsonObject& data) {
    QString chatId = data["chatId"].toString();
    QString messageId = data["messageId"].toString();
    QString userId = data["userId"].toString();

    emit debugLog(QString("message_seen_update received | chatId=%1 | messageId=%2 | userId=%3")
                      .arg(chatId)
                      .arg(messageId)
                      .arg(userId));
    emit messageSeenUpdate(chatId, messageId, userId);
}

void WebSocketClient::editMessage(const QString& chatId, const QString& messageId, const QString& newText) {
    if (!isConnected()) {
        emit debugLog(QString("WS OUT skipped: edit_message because socket is not connected | state=%1 | chatId=%2 | messageId=%3")
                          .arg(socketStateToString(m_webSocket.state()))
                          .arg(chatId)
                          .arg(messageId));
        return;
    }

    QJsonObject payload;
    payload["type"] = "edit_message";
    payload["chatId"] = chatId;
    payload["messageId"] = messageId;
    payload["newContent"] = newText;

    QJsonDocument doc(payload);
    emit debugLog(QString("WS OUT: edit_message | chatId=%1 | messageId=%2 | newLength=%3")
                      .arg(chatId)
                      .arg(messageId)
                      .arg(newText.size()));
    m_webSocket.sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void WebSocketClient::handleMessageUpdated(const QJsonObject& data) {
    QString chatId = data["chatId"].toString();
    QString messageId = data["messageId"].toString();
    qint64 editedAt = (qint64)data["editedAt"].toDouble();

    QString newContent;
    if (data["newContent"].isString()) {
        QJsonDocument cd = QJsonDocument::fromJson(data["newContent"].toString().toUtf8());
        QJsonObject contentObj = cd.object();
        newContent = contentObj["text"].toString();
    }
    else {
        QJsonObject contentObj = data["newContent"].toObject();
        newContent = contentObj["text"].toString();
    }

    emit debugLog(QString("message_updated received | chatId=%1 | messageId=%2 | editedAt=%3")
                      .arg(chatId)
                      .arg(messageId)
                      .arg(editedAt));
    emit messageUpdated(chatId, messageId, newContent, editedAt);
}

void WebSocketClient::deleteMessage(const QString& chatId, const QString& messageId) {
    if (!isConnected()) {
        emit debugLog(QString("WS OUT skipped: delete_message because socket is not connected | state=%1 | chatId=%2 | messageId=%3")
                          .arg(socketStateToString(m_webSocket.state()))
                          .arg(chatId)
                          .arg(messageId));
        return;
    }

    QJsonObject payload;
    payload["type"] = "delete_message";
    payload["chatId"] = chatId;
    payload["messageId"] = messageId;

    QJsonDocument doc(payload);
    emit debugLog(QString("WS OUT: delete_message | chatId=%1 | messageId=%2")
                      .arg(chatId)
                      .arg(messageId));
    m_webSocket.sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void WebSocketClient::handleMessageDeleted(const QJsonObject& data) {
    QString chatId = data["chatId"].toString();
    QString messageId = data["messageId"].toString();

    emit debugLog(QString("message_deleted received | chatId=%1 | messageId=%2")
                      .arg(chatId)
                      .arg(messageId));
    emit messageDeleted(chatId, messageId);
}

void WebSocketClient::logout() {
    emit debugLog(QString("logout() called | currentUserId=%1 | state=%2")
                      .arg(m_currentUser.userId)
                      .arg(socketStateToString(m_webSocket.state())));
    m_currentUser = User();
    m_token.clear();
    m_webSocket.close();
}
