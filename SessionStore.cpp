#include "SessionStore.h"

#include <QDateTime>
#include <QSettings>

namespace {
constexpr const char* kOrg = "Noveo";
constexpr const char* kApp = "MessengerClient";
constexpr const char* kUserIdKey = "session/userId";
constexpr const char* kTokenKey = "session/token";
constexpr const char* kExpiresAtKey = "session/expiresAt";
constexpr const char* kLegacyUserKey = "username";
constexpr const char* kLegacyPasswordKey = "password";
} // namespace

bool SessionData::isPresent() const
{
    return !userId.isEmpty() && !token.isEmpty() && expiresAt > 0;
}

SessionData SessionStore::load()
{
    QSettings settings(kOrg, kApp);

    SessionData session;
    session.userId = settings.value(kUserIdKey).toString();
    session.token = settings.value(kTokenKey).toString();
    session.expiresAt = settings.value(kExpiresAtKey).toLongLong();

    // Hard migrate away from plaintext password persistence.
    settings.remove(kLegacyPasswordKey);
    settings.remove(kLegacyUserKey);

    if (!session.isPresent()) {
        return SessionData();
    }
    if (isExpired(session)) {
        clear();
        return SessionData();
    }
    return session;
}

void SessionStore::save(const SessionData& session)
{
    if (!session.isPresent()) {
        clear();
        return;
    }

    QSettings settings(kOrg, kApp);
    settings.setValue(kUserIdKey, session.userId);
    settings.setValue(kTokenKey, session.token);
    settings.setValue(kExpiresAtKey, session.expiresAt);
    settings.remove(kLegacyPasswordKey);
    settings.remove(kLegacyUserKey);
}

void SessionStore::clear()
{
    QSettings settings(kOrg, kApp);
    settings.remove(kUserIdKey);
    settings.remove(kTokenKey);
    settings.remove(kExpiresAtKey);
    settings.remove(kLegacyPasswordKey);
    settings.remove(kLegacyUserKey);
}

bool SessionStore::isExpired(const SessionData& session)
{
    if (!session.isPresent()) {
        return true;
    }
    return QDateTime::currentSecsSinceEpoch() >= session.expiresAt;
}
