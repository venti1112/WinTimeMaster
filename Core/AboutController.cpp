#include "AboutController.h"
#include <QCoreApplication>
#include <QtGlobal>

AboutController::AboutController(QObject *parent) : QObject(parent) {}

QString AboutController::appVersion() const {
    return QCoreApplication::applicationVersion();
}

QString AboutController::qtVersion() const {
    return QT_VERSION_STR;
}

QString AboutController::compiler() const {
#if defined(Q_CC_MSVC)
    return QStringLiteral("MSVC %1").arg(_MSC_VER);
#elif defined(Q_CC_GNU)
    return QStringLiteral("GCC %1.%2.%3").arg(__GNUC__).arg(__GNUC_MINOR__).arg(__GNUC_PATCHLEVEL__);
#elif defined(Q_CC_CLANG)
    return QStringLiteral("Clang %1.%2.%3").arg(__clang_major__).arg(__clang_minor__).arg(__clang_patchlevel__);
#else
    return QStringLiteral("Unknown Compiler");
#endif
}

QString AboutController::buildType() const {
#ifdef QT_DEBUG
    return QStringLiteral("Debug");
#else
    return QStringLiteral("Release");
#endif
}

QString AboutController::developers() const {
    return QStringLiteral("Venti1112");
}

QString AboutController::githubUrl() const
{
    return QStringLiteral("https://github.com/venti1112/WinTimeMaster");
}
