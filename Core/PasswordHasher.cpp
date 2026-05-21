#include "PasswordHasher.h"
#include <windows.h>
#include <ntstatus.h>
#include <bcrypt.h>
#include <QByteArray>
#include <QString>
#include <QStringList>

#pragma comment(lib, "bcrypt.lib")

static QByteArray generateSalt(int length = 16) {
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    QByteArray salt(length, 0);

    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_RNG_ALGORITHM, nullptr, 0) == STATUS_SUCCESS) {
        BCryptGenRandom(hAlg, reinterpret_cast<PUCHAR>(salt.data()), length, 0);
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }
    return salt;
}

static QByteArray hashWithBCrypt(const QByteArray &password, const QByteArray &salt, int rounds = 10000) {
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    QByteArray hash(32, 0);
    DWORD cbHash = 0;

    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != STATUS_SUCCESS) return {};

    DWORD cbObject = 0, cbData = 0;
    BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&cbObject), sizeof(DWORD), &cbData, 0);
    QByteArray hashObject(cbObject, 0);

    BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&cbHash), sizeof(DWORD), &cbData, 0);
    hash.resize(cbHash);

    QByteArray input = salt + password;
    BCryptCreateHash(hAlg, &hHash, reinterpret_cast<PUCHAR>(hashObject.data()), hashObject.size(), nullptr, 0, 0);
    BCryptHashData(hHash, reinterpret_cast<PUCHAR>(input.data()), input.size(), 0);
    BCryptFinishHash(hHash, reinterpret_cast<PUCHAR>(hash.data()), hash.size(), 0);
    BCryptDestroyHash(hHash);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    for (int i = 1; i < rounds; ++i) {
        BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
        BCryptCreateHash(hAlg, &hHash, reinterpret_cast<PUCHAR>(hashObject.data()), hashObject.size(), nullptr, 0, 0);
        BCryptHashData(hHash, reinterpret_cast<PUCHAR>(hash.data()), hash.size(), 0);
        BCryptFinishHash(hHash, reinterpret_cast<PUCHAR>(hash.data()), hash.size(), 0);
        BCryptDestroyHash(hHash);
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }

    return hash;
}

QString PasswordHasher::hashPassword(const QString &password) {
    if (password.isEmpty()) return QString();

    QByteArray salt = generateSalt(16);
    QByteArray hash = hashWithBCrypt(password.toUtf8(), salt);
    if (hash.isEmpty()) return QString();

    QByteArray result;
    result.append("$pbkdf2$");
    result.append(salt.toBase64());
    result.append("$");
    result.append(hash.toBase64());
    return QString::fromLatin1(result);
}

bool PasswordHasher::verifyPassword(const QString &password, const QString &storedHash) {
    if (storedHash.isEmpty()) return password.isEmpty();
    if (password.isEmpty()) return false;

    QStringList parts = storedHash.split('$');
    if (parts.size() < 4) return false;

    QByteArray salt = QByteArray::fromBase64(parts[2].toLatin1());
    QByteArray expectedHash = QByteArray::fromBase64(parts[3].toLatin1());
    QByteArray computedHash = hashWithBCrypt(password.toUtf8(), salt);

    return computedHash == expectedHash;
}
