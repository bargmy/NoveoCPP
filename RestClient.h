#ifndef RESTCLIENT_H
#define RESTCLIENT_H

#include <QByteArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QStringList>

class RestClient : public QObject
{
    Q_OBJECT
public:
    explicit RestClient(QObject* parent = nullptr);

    void setAuthContext(const QString& userId, const QString& token);
    void clearAuthContext();

    QNetworkReply* postJson(const QString& path, const QJsonObject& body, bool includeAuth = true);
    QNetworkReply* uploadBytes(const QString& path,
                               const QByteArray& data,
                               const QString& fileName,
                               const QString& mimeType,
                               const QString& formField = QStringLiteral("file"),
                               bool includeAuth = true);

    QNetworkReply* uploadFile(const QString& path,
                              const QString& localPath,
                              const QString& formField = QStringLiteral("file"),
                              bool includeAuth = true);

    // Typed API wrappers used by parity plan.
    QNetworkReply* uploadChatFile(const QByteArray& data, const QString& fileName, const QString& mimeType);
    QNetworkReply* uploadAvatar(const QByteArray& data, const QString& fileName, const QString& mimeType);
    QNetworkReply* createGroup(const QString& name, const QStringList& members);
    QNetworkReply* createChannel(const QString& name,
                                 const QString& handle,
                                 const QByteArray& avatarData = QByteArray(),
                                 const QString& avatarFileName = QString(),
                                 const QString& avatarMimeType = QString());
    QNetworkReply* chatSettingsAction(const QString& action, const QString& chatId, const QJsonObject& extra = QJsonObject());
    QNetworkReply* updatePrivacy(bool blockGroupInvites);

private:
    QNetworkRequest buildRequest(const QString& path, bool includeAuth) const;
    QString resolveUrl(const QString& path) const;

    QNetworkAccessManager m_network;
    QString m_apiBaseUrl;
    QString m_userId;
    QString m_token;
};

#endif // RESTCLIENT_H
