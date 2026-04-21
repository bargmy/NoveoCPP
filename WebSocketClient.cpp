#include "WebSocketClient.h"

#include "AppConfig.h"

#include <QNetworkRequest>
#include <QJsonValue>
#include <QJsonParseError>
#include <QSet>
#include <QSslError>
#include <QStringList>
#include <QVariant>

namespace {
QJsonObject parsePossiblyNestedJsonObjectString(const QString& input)
{
    QString raw = input.trimmed();
    for (int depth = 0; depth < 3 && !raw.isEmpty(); ++depth) {
        QJsonParseError error;
        const QJsonDocument parsed = QJsonDocument::fromJson(raw.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError) {
            return QJsonObject();
        }
        if (parsed.isObject()) {
            return parsed.object();
        }
        if (parsed.isArray()) {
            return QJsonObject();
        }
        if (parsed.isNull()) {
            return QJsonObject();
        }

        // Some backends double-encode JSON: first parse yields a JSON string.
        const QVariant variant = parsed.toVariant();
        if (variant.type() == QVariant::String) {
            const QString nested = variant.toString().trimmed();
            if (nested == raw) {
                return QJsonObject();
            }
            raw = nested;
            continue;
        }
        return QJsonObject();
    }
    return QJsonObject();
}
}

WebSocketClient::WebSocketClient(QObject* parent)
    : QObject(parent)
{
    connect(&m_webSocket, &QWebSocket::connected, this, &WebSocketClient::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &WebSocketClient::disconnected);
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketClient::onTextMessageReceived);
    connect(&m_webSocket, &QWebSocket::binaryMessageReceived, this, &WebSocketClient::onBinaryMessageReceived);
    connect(&m_webSocket, &QWebSocket::sslErrors, this, &WebSocketClient::onSslErrors);
}

void WebSocketClient::connectToServer()
{
    QNetworkRequest request(AppConfig::websocketUrl());
    request.setRawHeader("Origin", "https://noveo.ir");
    m_webSocket.open(request);
}

void WebSocketClient::disconnectFromServer()
{
    m_webSocket.close();
}

void WebSocketClient::login(const QString& username, const QString& password)
{
    sendJson(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("login_with_password")},
        {QStringLiteral("username"), username},
        {QStringLiteral("password"), password},
    });
}

void WebSocketClient::registerUser(const QString& username, const QString& password)
{
    sendJson(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("register")},
        {QStringLiteral("username"), username},
        {QStringLiteral("password"), password},
    });
}

void WebSocketClient::reconnectWithToken(const QString& userId, const QString& token)
{
    sendJson(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("reconnect")},
        {QStringLiteral("userId"), userId},
        {QStringLiteral("token"), token},
    });
}

void WebSocketClient::sendMessage(const QString& chatId, const QString& text, const QString& replyToId)
{
    QJsonObject content;
    content.insert(QStringLiteral("text"), text);
    content.insert(QStringLiteral("file"), QJsonValue::Null);
    content.insert(QStringLiteral("theme"), QJsonValue::Null);

    QString recipientId;
    if (chatId.startsWith(QStringLiteral("temp_"))) {
        recipientId = chatId.mid(5);
    } else if (chatId.contains('_')) {
        const QStringList ids = chatId.split('_', Qt::SkipEmptyParts);
        if (ids.size() == 2) {
            recipientId = (ids[0] == m_currentUser.userId) ? ids[1] : ids[0];
        }
    }
    sendMessagePayload(chatId, content, replyToId, recipientId);
}

void WebSocketClient::sendMessagePayload(const QString& chatId,
                                         const QJsonObject& content,
                                         const QString& replyToId,
                                         const QString& recipientId)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("type"), QStringLiteral("message"));
    payload.insert(QStringLiteral("content"), content);

    if (!recipientId.isEmpty()) {
        payload.insert(QStringLiteral("recipientId"), recipientId);
    } else {
        payload.insert(QStringLiteral("chatId"), chatId);
    }

    if (replyToId.isEmpty()) {
        payload.insert(QStringLiteral("replyToId"), QJsonValue::Null);
    } else {
        payload.insert(QStringLiteral("replyToId"), replyToId);
    }
    sendJson(payload);
}

void WebSocketClient::fetchHistory(const QString& chatId, qint64 beforeTimestamp)
{
    sendJson(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("get_history")},
        {QStringLiteral("chatId"), chatId},
        {QStringLiteral("before"), static_cast<double>(beforeTimestamp)},
    });
}

void WebSocketClient::sendMessageSeen(const QString& chatId, const QString& messageId)
{
    sendJson(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("message_seen")},
        {QStringLiteral("chatId"), chatId},
        {QStringLiteral("messageId"), messageId},
    });
}

void WebSocketClient::editMessage(const QString& chatId, const QString& messageId, const QString& newText)
{
    sendJson(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("edit_message")},
        {QStringLiteral("chatId"), chatId},
        {QStringLiteral("messageId"), messageId},
        {QStringLiteral("newContent"), newText},
    });
}

void WebSocketClient::deleteMessage(const QString& chatId, const QString& messageId)
{
    sendJson(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("delete_message")},
        {QStringLiteral("chatId"), chatId},
        {QStringLiteral("messageId"), messageId},
    });
}

void WebSocketClient::sendTyping(const QString& chatId)
{
    sendJson(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("typing")},
        {QStringLiteral("chatId"), chatId},
    });
}

void WebSocketClient::joinChannel(const QString& chatId)
{
    sendJson(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("join_channel")},
        {QStringLiteral("chatId"), chatId},
    });
}

void WebSocketClient::requestChannelByHandle(const QString& handle)
{
    sendJson(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("get_channel_by_handle")},
        {QStringLiteral("handle"), handle},
    });
}

void WebSocketClient::pinMessage(const QString& chatId, const QString& messageId)
{
    sendJson(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("pin_message")},
        {QStringLiteral("chatId"), chatId},
        {QStringLiteral("messageId"), messageId},
    });
}

void WebSocketClient::unpinMessage(const QString& chatId)
{
    sendJson(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("unpin_message")},
        {QStringLiteral("chatId"), chatId},
    });
}

void WebSocketClient::updateUsername(const QString& username)
{
    sendJson(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("update_username")},
        {QStringLiteral("username"), username},
    });
}

void WebSocketClient::changePassword(const QString& oldPassword, const QString& newPassword)
{
    sendJson(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("change_password")},
        {QStringLiteral("oldPassword"), oldPassword},
        {QStringLiteral("newPassword"), newPassword},
    });
}

void WebSocketClient::deleteAccount(const QString& password)
{
    sendJson(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("delete_account")},
        {QStringLiteral("password"), password},
    });
}

void WebSocketClient::voiceStart(const QString& chatId)
{
    sendJson(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("voice_start")},
        {QStringLiteral("chatId"), chatId},
    });
}

void WebSocketClient::voiceJoin(const QString& chatId)
{
    sendJson(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("voice_join")},
        {QStringLiteral("chatId"), chatId},
    });
}

void WebSocketClient::voiceLeave(const QString& chatId)
{
    sendJson(QJsonObject{
        {QStringLiteral("type"), QStringLiteral("voice_leave")},
        {QStringLiteral("chatId"), chatId},
    });
}

void WebSocketClient::sendBinaryAudio(const QByteArray& audioPayload)
{
    if (!isConnected() || audioPayload.isEmpty()) {
        return;
    }
    m_webSocket.sendBinaryMessage(audioPayload);
}

void WebSocketClient::logout()
{
    m_currentUser = User();
    m_token.clear();
    m_tokenExpiresAt = 0;
    m_webSocket.close();
}

bool WebSocketClient::isConnected() const
{
    return m_webSocket.state() == QAbstractSocket::ConnectedState;
}

void WebSocketClient::onConnected()
{
    emit connected();
}

void WebSocketClient::onTextMessageReceived(const QString& message)
{
    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        return;
    }

    const QJsonObject obj = doc.object();
    const QString type = obj.value(QStringLiteral("type")).toString();

    if (type == QStringLiteral("login_success")) {
        handleLoginSuccess(obj);
    } else if (type == QStringLiteral("auth_failed")) {
        emit authFailed(obj.value(QStringLiteral("message")).toString());
    } else if (type == QStringLiteral("chat_history")) {
        handleChatHistory(obj);
    } else if (type == QStringLiteral("message")) {
        handleMessage(obj);
    } else if (type == QStringLiteral("user_list_update")) {
        handleUserListUpdate(obj);
    } else if (type == QStringLiteral("new_chat_info")) {
        handleNewChat(obj);
    } else if (type == QStringLiteral("message_seen_update")) {
        handleMessageSeenUpdate(obj);
    } else if (type == QStringLiteral("message_updated")) {
        handleMessageUpdated(obj);
    } else if (type == QStringLiteral("message_deleted")) {
        handleMessageDeleted(obj);
    } else if (type == QStringLiteral("presence_update")) {
        handlePresenceUpdate(obj);
    } else if (type == QStringLiteral("typing")) {
        handleTyping(obj);
    } else if (type == QStringLiteral("channel_info")) {
        handleChannelInfo(obj);
    } else if (type == QStringLiteral("member_joined")) {
        handleMemberJoined(obj);
    } else if (type == QStringLiteral("message_pinned")) {
        handleMessagePinned(obj);
    } else if (type == QStringLiteral("message_unpinned")) {
        handleMessageUnpinned(obj);
    } else if (type == QStringLiteral("password_changed")) {
        handlePasswordChanged(obj);
    } else if (type == QStringLiteral("voice_chat_update")) {
        handleVoiceChatUpdate(obj);
    } else if (type == QStringLiteral("incoming_call")) {
        handleIncomingCall(obj);
    } else if (type == QStringLiteral("user_updated")) {
        handleUserUpdated(obj);
    } else if (type == QStringLiteral("error")) {
        emit errorOccurred(obj.value(QStringLiteral("message")).toString());
    }
}

void WebSocketClient::onBinaryMessageReceived(const QByteArray& message)
{
    if (message.isEmpty()) {
        return;
    }

    const quint8 userIdLength = static_cast<quint8>(message.at(0));
    if (message.size() < 1 + userIdLength) {
        return;
    }

    const QString userId = QString::fromUtf8(message.mid(1, userIdLength));
    const QByteArray audioPayload = message.mid(1 + userIdLength);
    if (userId.isEmpty() || audioPayload.isEmpty()) {
        return;
    }
    emit binaryAudioReceived(userId, audioPayload);
}

void WebSocketClient::onSslErrors(const QList<QSslError>& errors)
{
    Q_UNUSED(errors);
    m_webSocket.ignoreSslErrors();
}

void WebSocketClient::sendJson(const QJsonObject& payload)
{
    if (!isConnected()) {
        return;
    }
    m_webSocket.sendTextMessage(QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact)));
}

void WebSocketClient::handleLoginSuccess(const QJsonObject& data)
{
    m_currentUser = parseUserObject(data.value(QStringLiteral("user")).toObject());
    m_token = data.value(QStringLiteral("token")).toString();
    m_tokenExpiresAt = static_cast<qint64>(data.value(QStringLiteral("expiresAt")).toDouble());
    emit loginSuccess(m_currentUser, m_token, m_tokenExpiresAt);
}

void WebSocketClient::handleChatHistory(const QJsonObject& data)
{
    std::vector<Chat> chats;
    const QJsonArray chatsArray = data.value(QStringLiteral("chats")).toArray();
    chats.reserve(static_cast<size_t>(chatsArray.size()));

    for (const QJsonValue& value : chatsArray) {
        chats.push_back(parseChatObject(value.toObject()));
    }
    emit chatHistoryReceived(chats);

    if (data.contains(QStringLiteral("activeVoiceChats"))) {
        emit voiceChatUpdated(parseVoiceParticipantsMap(data.value(QStringLiteral("activeVoiceChats")).toObject()));
    }
}

void WebSocketClient::handleMessage(const QJsonObject& data)
{
    emit messageReceived(parseMessageObject(data));
}

void WebSocketClient::handleUserListUpdate(const QJsonObject& data)
{
    std::vector<User> users;
    const QJsonArray usersArr = data.value(QStringLiteral("users")).toArray();
    users.reserve(static_cast<size_t>(usersArr.size()));

    QSet<QString> onlineUsers;
    const QJsonArray onlineArr = data.value(QStringLiteral("online")).toArray();
    for (const QJsonValue& onlineVal : onlineArr) {
        onlineUsers.insert(onlineVal.toString());
    }

    for (const QJsonValue& userVal : usersArr) {
        User user = parseUserObject(userVal.toObject());
        if (!onlineUsers.isEmpty()) {
            user.online = onlineUsers.contains(user.userId);
        }
        users.push_back(user);
    }
    emit userListUpdated(users);
}

void WebSocketClient::handleNewChat(const QJsonObject& data)
{
    emit newChatCreated(parseChatObject(data.value(QStringLiteral("chat")).toObject()));
}

void WebSocketClient::handleMessageSeenUpdate(const QJsonObject& data)
{
    emit messageSeenUpdate(data.value(QStringLiteral("chatId")).toString(),
                           data.value(QStringLiteral("messageId")).toString(),
                           data.value(QStringLiteral("userId")).toString());
}

void WebSocketClient::handleMessageUpdated(const QJsonObject& data)
{
    const QJsonObject content = parseContentObject(data.value(QStringLiteral("newContent")));
    emit messageUpdated(data.value(QStringLiteral("chatId")).toString(),
                        data.value(QStringLiteral("messageId")).toString(),
                        extractMessageText(content),
                        static_cast<qint64>(data.value(QStringLiteral("editedAt")).toDouble()));
}

void WebSocketClient::handleMessageDeleted(const QJsonObject& data)
{
    emit messageDeleted(data.value(QStringLiteral("chatId")).toString(),
                        data.value(QStringLiteral("messageId")).toString());
}

void WebSocketClient::handlePresenceUpdate(const QJsonObject& data)
{
    emit presenceUpdated(data.value(QStringLiteral("userId")).toString(),
                         data.value(QStringLiteral("online")).toBool());
}

void WebSocketClient::handleTyping(const QJsonObject& data)
{
    emit typingReceived(data.value(QStringLiteral("chatId")).toString(),
                        data.value(QStringLiteral("senderId")).toString());
}

void WebSocketClient::handleChannelInfo(const QJsonObject& data)
{
    emit channelInfoReceived(parseChatObject(data.value(QStringLiteral("channel")).toObject()));
}

void WebSocketClient::handleMemberJoined(const QJsonObject& data)
{
    QStringList members;
    const QJsonArray membersArr = data.value(QStringLiteral("members")).toArray();
    for (const QJsonValue& member : membersArr) {
        members.push_back(member.toString());
    }
    emit memberJoined(data.value(QStringLiteral("chatId")).toString(), members);
}

void WebSocketClient::handleMessagePinned(const QJsonObject& data)
{
    emit messagePinned(data.value(QStringLiteral("chatId")).toString(),
                       parseMessageObject(data.value(QStringLiteral("message")).toObject()));
}

void WebSocketClient::handleMessageUnpinned(const QJsonObject& data)
{
    emit messageUnpinned(data.value(QStringLiteral("chatId")).toString());
}

void WebSocketClient::handlePasswordChanged(const QJsonObject& data)
{
    const QString token = data.value(QStringLiteral("token")).toString();
    const qint64 expiresAt = static_cast<qint64>(data.value(QStringLiteral("expiresAt")).toDouble());
    const QString warning = data.value(QStringLiteral("warning")).toString();
    if (!token.isEmpty()) {
        m_token = token;
    }
    if (expiresAt > 0) {
        m_tokenExpiresAt = expiresAt;
    }
    emit passwordChanged(m_token, m_tokenExpiresAt, warning);
}

void WebSocketClient::handleVoiceChatUpdate(const QJsonObject& data)
{
    emit voiceChatUpdated(parseVoiceParticipantsMap(data.value(QStringLiteral("activeVoiceChats")).toObject()));
}

void WebSocketClient::handleIncomingCall(const QJsonObject& data)
{
    emit incomingCall(data.value(QStringLiteral("chatId")).toString(),
                      data.value(QStringLiteral("callerId")).toString(),
                      data.value(QStringLiteral("chatName")).toString(),
                      data.value(QStringLiteral("callerName")).toString(),
                      data.value(QStringLiteral("callerAvatar")).toString());
}

void WebSocketClient::handleUserUpdated(const QJsonObject& data)
{
    emit userUpdated(parseUserObject(data));
}

QJsonObject WebSocketClient::parseContentObject(const QJsonValue& contentValue)
{
    if (contentValue.isObject()) {
        return contentValue.toObject();
    }
    if (contentValue.isString()) {
        const QString raw = contentValue.toString();
        const QJsonObject parsedObj = parsePossiblyNestedJsonObjectString(raw);
        if (!parsedObj.isEmpty()) {
            return parsedObj;
        }
        QJsonObject fallback;
        fallback.insert(QStringLiteral("text"), raw);
        return fallback;
    }
    return QJsonObject();
}

Message WebSocketClient::parseMessageObject(const QJsonObject& obj)
{
    Message msg;
    msg.messageId = obj.value(QStringLiteral("messageId")).toString();
    msg.chatId = obj.value(QStringLiteral("chatId")).toString();
    msg.senderId = obj.value(QStringLiteral("senderId")).toString();
    msg.senderName = obj.value(QStringLiteral("senderName")).toString();
    if (msg.senderName.isEmpty()) {
        msg.senderName = obj.value(QStringLiteral("username")).toString();
    }
    msg.senderAvatarUrl = obj.value(QStringLiteral("senderAvatarUrl")).toString();
    if (msg.senderAvatarUrl.isEmpty()) {
        msg.senderAvatarUrl = obj.value(QStringLiteral("senderAvatar")).toString();
    }
    if (msg.senderAvatarUrl.isEmpty()) {
        msg.senderAvatarUrl = obj.value(QStringLiteral("avatarUrl")).toString();
    }
    const QJsonObject senderObj = obj.value(QStringLiteral("sender")).toObject();
    if (!senderObj.isEmpty()) {
        if (msg.senderId.isEmpty()) {
            msg.senderId = senderObj.value(QStringLiteral("userId")).toString();
        }
        if (msg.senderName.isEmpty()) {
            msg.senderName = senderObj.value(QStringLiteral("username")).toString();
        }
        if (msg.senderName.isEmpty()) {
            msg.senderName = senderObj.value(QStringLiteral("name")).toString();
        }
        if (msg.senderAvatarUrl.isEmpty()) {
            msg.senderAvatarUrl = senderObj.value(QStringLiteral("avatarUrl")).toString();
        }
    }
    msg.timestamp = static_cast<qint64>(obj.value(QStringLiteral("timestamp")).toDouble());
    msg.editedAt = static_cast<qint64>(obj.value(QStringLiteral("editedAt")).toDouble());
    msg.replyToId = obj.value(QStringLiteral("replyToId")).toString();

    const QJsonObject contentObj = parseContentObject(obj.value(QStringLiteral("content")));
    msg.rawContent = QString::fromUtf8(QJsonDocument(contentObj).toJson(QJsonDocument::Compact));
    msg.text = extractMessageText(contentObj);
    msg.file = parseFileAttachment(contentObj.value(QStringLiteral("file")));
    msg.theme = contentObj.value(QStringLiteral("theme")).toString();
    msg.forwardedInfo = parseForwardedInfo(contentObj.value(QStringLiteral("forwardedInfo")));

    // Legacy/fallback payload support: content fields may be at top level.
    if (msg.text.isEmpty() && obj.value(QStringLiteral("text")).isString()) {
        msg.text = obj.value(QStringLiteral("text")).toString();
    }
    if (msg.file.isNull() && obj.contains(QStringLiteral("file"))) {
        msg.file = parseFileAttachment(obj.value(QStringLiteral("file")));
    }
    if (msg.theme.isEmpty() && obj.value(QStringLiteral("theme")).isString()) {
        msg.theme = obj.value(QStringLiteral("theme")).toString();
    }

    // Recovery path: text itself may be a serialized content object.
    const QString trimmedText = msg.text.trimmed();
    if (!trimmedText.isEmpty() && trimmedText.startsWith('{') && trimmedText.endsWith('}')) {
        const QJsonObject nestedContent = parsePossiblyNestedJsonObjectString(trimmedText);
        if (!nestedContent.isEmpty() &&
            (nestedContent.contains(QStringLiteral("text")) ||
             nestedContent.contains(QStringLiteral("file")) ||
             nestedContent.contains(QStringLiteral("theme")))) {
            msg.rawContent = QString::fromUtf8(QJsonDocument(nestedContent).toJson(QJsonDocument::Compact));
            msg.text = extractMessageText(nestedContent);
            msg.file = parseFileAttachment(nestedContent.value(QStringLiteral("file")));
            msg.theme = nestedContent.value(QStringLiteral("theme")).toString();
            msg.forwardedInfo = parseForwardedInfo(nestedContent.value(QStringLiteral("forwardedInfo")));
        }
    }

    if (obj.contains(QStringLiteral("seenBy")) && obj.value(QStringLiteral("seenBy")).isArray()) {
        const QJsonArray seenByArr = obj.value(QStringLiteral("seenBy")).toArray();
        for (const QJsonValue& val : seenByArr) {
            msg.seenBy.append(val.toString());
        }
    }

    if (msg.text.isEmpty() && !msg.file.isNull()) {
        msg.text = msg.file.name.isEmpty() ? QStringLiteral("[Attachment]") : QStringLiteral("[%1]").arg(msg.file.name);
    }
    return msg;
}

User WebSocketClient::parseUserObject(const QJsonObject& obj)
{
    User user;
    user.userId = obj.value(QStringLiteral("userId")).toString();
    user.username = obj.value(QStringLiteral("username")).toString();
    user.avatarUrl = obj.value(QStringLiteral("avatarUrl")).toString();
    user.online = obj.value(QStringLiteral("online")).toBool(false);
    user.blockGroupInvites = obj.value(QStringLiteral("blockGroupInvites")).toBool(false);
    return user;
}

Chat WebSocketClient::parseChatObject(const QJsonObject& obj)
{
    Chat chat;
    chat.chatId = obj.value(QStringLiteral("chatId")).toString();
    chat.chatName = obj.value(QStringLiteral("chatName")).toString();
    if (chat.chatName.isEmpty()) {
        chat.chatName = obj.value(QStringLiteral("name")).toString();
    }
    chat.chatType = obj.value(QStringLiteral("chatType")).toString();
    chat.ownerId = obj.value(QStringLiteral("ownerId")).toString();
    chat.handle = obj.value(QStringLiteral("handle")).toString();
    chat.avatarUrl = obj.value(QStringLiteral("avatarUrl")).toString();
    chat.unreadCount = obj.value(QStringLiteral("unreadCount")).toInt(0);
    chat.isVerified = obj.value(QStringLiteral("isVerified")).toBool(false);
    chat.createdAt = static_cast<qint64>(obj.value(QStringLiteral("created_at")).toDouble());
    if (chat.createdAt <= 0) {
        chat.createdAt = static_cast<qint64>(obj.value(QStringLiteral("createdAt")).toDouble());
    }

    const QJsonArray members = obj.value(QStringLiteral("members")).toArray();
    for (const QJsonValue& member : members) {
        chat.members.append(member.toString());
    }

    const QJsonArray messages = obj.value(QStringLiteral("messages")).toArray();
    chat.messages.reserve(static_cast<size_t>(messages.size()));
    for (const QJsonValue& value : messages) {
        Message msg = parseMessageObject(value.toObject());
        if (msg.chatId.isEmpty()) {
            msg.chatId = chat.chatId;
        }
        chat.messages.push_back(msg);
    }

    if (obj.contains(QStringLiteral("pinnedMessage")) && obj.value(QStringLiteral("pinnedMessage")).isObject()) {
        chat.hasPinnedMessage = true;
        chat.pinnedMessage = parseMessageObject(obj.value(QStringLiteral("pinnedMessage")).toObject());
        if (chat.pinnedMessage.chatId.isEmpty()) {
            chat.pinnedMessage.chatId = chat.chatId;
        }
    }
    return chat;
}

QString WebSocketClient::extractMessageText(const QJsonObject& contentObj)
{
    const QJsonValue textValue = contentObj.value(QStringLiteral("text"));
    if (textValue.isString()) {
        return textValue.toString();
    }
    return QString();
}

FileAttachment WebSocketClient::parseFileAttachment(const QJsonValue& value)
{
    FileAttachment file;
    if (value.isObject()) {
        const QJsonObject obj = value.toObject();
        file.url = obj.value(QStringLiteral("url")).toString();
        file.name = obj.value(QStringLiteral("name")).toString();
        file.type = obj.value(QStringLiteral("type")).toString();
        file.size = static_cast<qint64>(obj.value(QStringLiteral("size")).toDouble());
    } else if (value.isString()) {
        file.url = value.toString();
    }
    return file;
}

ForwardedInfo WebSocketClient::parseForwardedInfo(const QJsonValue& value)
{
    ForwardedInfo info;
    if (!value.isObject()) {
        return info;
    }
    const QJsonObject obj = value.toObject();
    info.from = obj.value(QStringLiteral("from")).toString();
    info.originalTs = static_cast<qint64>(obj.value(QStringLiteral("originalTs")).toDouble());
    return info;
}

VoiceChatParticipants WebSocketClient::parseVoiceParticipantsMap(const QJsonObject& value)
{
    VoiceChatParticipants result;
    for (auto it = value.constBegin(); it != value.constEnd(); ++it) {
        QStringList participants;
        if (it.value().isObject()) {
            const QJsonArray arr = it.value().toObject().value(QStringLiteral("participants")).toArray();
            for (const QJsonValue& participant : arr) {
                participants.push_back(participant.toString());
            }
        } else if (it.value().isArray()) {
            const QJsonArray arr = it.value().toArray();
            for (const QJsonValue& participant : arr) {
                participants.push_back(participant.toString());
            }
        }
        result.insert(it.key(), participants);
    }
    return result;
}
