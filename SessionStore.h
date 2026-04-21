#ifndef SESSIONSTORE_H
#define SESSIONSTORE_H

#include <QString>

struct SessionData {
    QString userId;
    QString token;
    qint64 expiresAt = 0; // Unix seconds

    bool isPresent() const;
};

class SessionStore
{
public:
    static SessionData load();
    static void save(const SessionData& session);
    static void clear();
    static bool isExpired(const SessionData& session);
};

#endif // SESSIONSTORE_H
