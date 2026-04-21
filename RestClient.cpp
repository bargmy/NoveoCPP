#include "RestClient.h"

#include "AppConfig.h"

#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMimeDatabase>
#include <QNetworkRequest>
#include <QUrl>

RestClient::RestClient(QObject* parent)
    : QObject(parent),
      m_apiBaseUrl(AppConfig::apiBaseUrl())
{
}

void RestClient::setAuthContext(const QString& userId, const QString& token)
{
    m_userId = userId;
    m_token = token;
}

void RestClient::clearAuthContext()
{
    m_userId.clear();
    m_token.clear();
}

QNetworkReply* RestClient::postJson(const QString& path, const QJsonObject& body, bool includeAuth)
{
    QNetworkRequest request = buildRequest(path, includeAuth);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    const QByteArray payload = QJsonDocument(body).toJson(QJsonDocument::Compact);
    return m_network.post(request, payload);
}

QNetworkReply* RestClient::uploadBytes(const QString& path,
                                       const QByteArray& data,
                                       const QString& fileName,
                                       const QString& mimeType,
                                       const QString& formField,
                                       bool includeAuth)
{
    QNetworkRequest request = buildRequest(path, includeAuth);
    auto* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart filePart;
    const QString disposition = QStringLiteral("form-data; name=\"%1\"; filename=\"%2\"")
                                    .arg(formField, fileName.isEmpty() ? QStringLiteral("upload.bin") : fileName);
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, disposition);
    if (!mimeType.isEmpty()) {
        filePart.setHeader(QNetworkRequest::ContentTypeHeader, mimeType);
    }
    filePart.setBody(data);
    multiPart->append(filePart);

    QNetworkReply* reply = m_network.post(request, multiPart);
    multiPart->setParent(reply);
    return reply;
}

QNetworkReply* RestClient::uploadFile(const QString& path, const QString& localPath, const QString& formField, bool includeAuth)
{
    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return nullptr;
    }

    const QByteArray payload = file.readAll();
    const QString fileName = QFileInfo(file).fileName();
    const QMimeDatabase db;
    const QString mimeType = db.mimeTypeForFile(fileName).name();
    return uploadBytes(path, payload, fileName, mimeType, formField, includeAuth);
}

QNetworkReply* RestClient::uploadChatFile(const QByteArray& data, const QString& fileName, const QString& mimeType)
{
    return uploadBytes(QStringLiteral("/upload/file"), data, fileName, mimeType);
}

QNetworkReply* RestClient::uploadAvatar(const QByteArray& data, const QString& fileName, const QString& mimeType)
{
    return uploadBytes(QStringLiteral("/upload/avatar"), data, fileName, mimeType);
}

QNetworkReply* RestClient::createGroup(const QString& name, const QStringList& members)
{
    QJsonArray membersJson;
    for (const QString& member : members) {
        membersJson.append(member);
    }

    QJsonObject body;
    body.insert(QStringLiteral("name"), name);
    body.insert(QStringLiteral("members"), membersJson);
    return postJson(QStringLiteral("/create_group"), body, true);
}

QNetworkReply* RestClient::createChannel(const QString& name,
                                         const QString& handle,
                                         const QByteArray& avatarData,
                                         const QString& avatarFileName,
                                         const QString& avatarMimeType)
{
    QNetworkRequest request = buildRequest(QStringLiteral("/create_channel"), true);
    auto* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart namePart;
    namePart.setHeader(QNetworkRequest::ContentDispositionHeader, QStringLiteral("form-data; name=\"name\""));
    namePart.setBody(name.toUtf8());
    multiPart->append(namePart);

    QHttpPart handlePart;
    handlePart.setHeader(QNetworkRequest::ContentDispositionHeader, QStringLiteral("form-data; name=\"handle\""));
    handlePart.setBody(handle.toUtf8());
    multiPart->append(handlePart);

    if (!avatarData.isEmpty()) {
        QHttpPart avatarPart;
        const QString disposition = QStringLiteral("form-data; name=\"avatar\"; filename=\"%1\"")
                                        .arg(avatarFileName.isEmpty() ? QStringLiteral("avatar.png") : avatarFileName);
        avatarPart.setHeader(QNetworkRequest::ContentDispositionHeader, disposition);
        if (!avatarMimeType.isEmpty()) {
            avatarPart.setHeader(QNetworkRequest::ContentTypeHeader, avatarMimeType);
        }
        avatarPart.setBody(avatarData);
        multiPart->append(avatarPart);
    }

    QNetworkReply* reply = m_network.post(request, multiPart);
    multiPart->setParent(reply);
    return reply;
}

QNetworkReply* RestClient::chatSettingsAction(const QString& action, const QString& chatId, const QJsonObject& extra)
{
    QJsonObject body = extra;
    body.insert(QStringLiteral("action"), action);
    body.insert(QStringLiteral("chatId"), chatId);
    return postJson(QStringLiteral("/chat/settings"), body, true);
}

QNetworkReply* RestClient::updatePrivacy(bool blockGroupInvites)
{
    QJsonObject body;
    body.insert(QStringLiteral("blockGroupInvites"), blockGroupInvites);
    return postJson(QStringLiteral("/user/privacy"), body, true);
}

QNetworkRequest RestClient::buildRequest(const QString& path, bool includeAuth) const
{
    QNetworkRequest request(QUrl(resolveUrl(path)));
    request.setRawHeader("Accept", "application/json");
    if (includeAuth) {
        if (!m_userId.isEmpty()) {
            request.setRawHeader("X-User-ID", m_userId.toUtf8());
        }
        if (!m_token.isEmpty()) {
            request.setRawHeader("X-Auth-Token", m_token.toUtf8());
        }
    }
    return request;
}

QString RestClient::resolveUrl(const QString& path) const
{
    if (path.startsWith(QStringLiteral("http://")) || path.startsWith(QStringLiteral("https://"))) {
        return path;
    }

    if (path.startsWith('/')) {
        return m_apiBaseUrl + path;
    }
    return m_apiBaseUrl + QStringLiteral("/") + path;
}
