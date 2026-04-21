#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QUrl>
#include <QString>
#include <QByteArray>
#include <QtGlobal>

namespace AppConfig {

inline QString apiBaseUrl() {
    const QByteArray fromEnv = qgetenv("NOVEO_API_BASE_URL");
    if (!fromEnv.isEmpty()) {
        return QString::fromUtf8(fromEnv);
    }
    return QStringLiteral("https://noveo.ir:8443");
}

inline QUrl websocketUrl() {
    const QByteArray wsEnv = qgetenv("NOVEO_WS_URL");
    if (!wsEnv.isEmpty()) {
        return QUrl(QString::fromUtf8(wsEnv));
    }

    QUrl api(apiBaseUrl());
    api.setScheme(api.scheme() == QStringLiteral("https") ? QStringLiteral("wss") : QStringLiteral("ws"));
    api.setPath(QStringLiteral("/ws"));
    return api;
}

} // namespace AppConfig

#endif // APPCONFIG_H
