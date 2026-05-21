#ifndef PASSWORDHASHER_H
#define PASSWORDHASHER_H

#include <QString>

class PasswordHasher {
public:
    static QString hashPassword(const QString &password);
    static bool verifyPassword(const QString &password, const QString &hash);
};

#endif // PASSWORDHASHER_H
