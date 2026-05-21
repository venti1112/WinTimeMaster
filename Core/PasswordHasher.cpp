#include "PasswordHasher.h"
#include <windows.h>
#include <ntstatus.h>
#include <bcrypt.h>
#include <QByteArray>
#include <QString>
#include <QStringList>

#pragma comment(lib, "bcrypt.lib")

namespace {
constexpr int kSaltLength = 16;
constexpr int kHashLength = 32;
constexpr ULONGLONG kDefaultIterations = 100000;

QByteArray generateSalt(int length) {
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    QByteArray salt(length, 0);

    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_RNG_ALGORITHM, nullptr, 0) != STATUS_SUCCESS)
        return {};

    NTSTATUS status = BCryptGenRandom(hAlg, reinterpret_cast<PUCHAR>(salt.data()), static_cast<ULONG>(length), 0);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    if (status != STATUS_SUCCESS) return {};
    return salt;
}

QByteArray pbkdf2Sha256(const QByteArray &password, const QByteArray &salt, ULONGLONG iterations, int outLength) {
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) != STATUS_SUCCESS)
        return {};

    QByteArray out(outLength, 0);
    NTSTATUS status = BCryptDeriveKeyPBKDF2(
        hAlg,
        reinterpret_cast<PUCHAR>(const_cast<char*>(password.constData())), static_cast<ULONG>(password.size()),
        reinterpret_cast<PUCHAR>(const_cast<char*>(salt.constData())), static_cast<ULONG>(salt.size()),
        iterations,
        reinterpret_cast<PUCHAR>(out.data()), static_cast<ULONG>(out.size()),
        0);

    BCryptCloseAlgorithmProvider(hAlg, 0);

    if (status != STATUS_SUCCESS) return {};
    return out;
}

bool constantTimeEquals(const QByteArray &a, const QByteArray &b) {
    if (a.size() != b.size()) return false;
    unsigned char diff = 0;
    for (int i = 0; i < a.size(); ++i)
        diff |= static_cast<unsigned char>(a[i]) ^ static_cast<unsigned char>(b[i]);
    return diff == 0;
}
}

QString PasswordHasher::hashPassword(const QString &password) {
    if (password.isEmpty()) return QString();

    QByteArray salt = generateSalt(kSaltLength);
    if (salt.isEmpty()) return QString();

    QByteArray hash = pbkdf2Sha256(password.toUtf8(), salt, kDefaultIterations, kHashLength);
    if (hash.isEmpty()) return QString();

    QByteArray result;
    result.append("$pbkdf2-sha256$");
    result.append(QByteArray::number(static_cast<qulonglong>(kDefaultIterations)));
    result.append("$");
    result.append(salt.toBase64());
    result.append("$");
    result.append(hash.toBase64());
    return QString::fromLatin1(result);
}

bool PasswordHasher::verifyPassword(const QString &password, const QString &storedHash) {
    if (storedHash.isEmpty()) return password.isEmpty();
    if (password.isEmpty()) return false;

    QStringList parts = storedHash.split('$');
    // 期望格式: "" / "pbkdf2-sha256" / iterations / salt_b64 / hash_b64
    if (parts.size() < 5) return false;
    if (parts[1] != QLatin1String("pbkdf2-sha256")) return false;

    bool ok = false;
    ULONGLONG iterations = parts[2].toULongLong(&ok);
    if (!ok || iterations == 0) return false;

    QByteArray salt = QByteArray::fromBase64(parts[3].toLatin1());
    QByteArray expectedHash = QByteArray::fromBase64(parts[4].toLatin1());
    if (salt.isEmpty() || expectedHash.isEmpty()) return false;

    QByteArray computedHash = pbkdf2Sha256(password.toUtf8(), salt, iterations, expectedHash.size());
    if (computedHash.isEmpty()) return false;

    return constantTimeEquals(computedHash, expectedHash);
}
