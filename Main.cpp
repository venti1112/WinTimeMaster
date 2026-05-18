#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <QFont>
#include <QCommandLineParser>

#include "Core/LanguageManager.h"
#include "Core/SettingsController.h"
#include "Core/TrayManager.h"
#include "Core/LockController.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

static bool isAdmin()
{
#ifdef Q_OS_WIN
    HANDLE hToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return false;
    TOKEN_ELEVATION elevation;
    DWORD size = sizeof(TOKEN_ELEVATION);
    bool result = false;
    if (GetTokenInformation(hToken, TokenElevation, &elevation, size, &size))
        result = (elevation.TokenIsElevated != 0);
    CloseHandle(hToken);
    return result;
#else
    return true;
#endif
}

static void restartAsAdmin()
{
#ifdef Q_OS_WIN
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    ShellExecuteW(NULL, L"runas", exePath, NULL, NULL, SW_SHOW);
#endif
}

int main(int argc, char *argv[])
{
    if (!isAdmin()) {
        restartAsAdmin();
        return 0;
    }

    QApplication app(argc, argv);

    // 命令行解析
    QCommandLineParser parser;
    QCommandLineOption autostartOption("autostart", "Start minimized to tray.");
    parser.addOption(autostartOption);
    parser.process(app);
    bool startMinimized = parser.isSet(autostartOption);

    QCoreApplication::setApplicationName("WinTimeMaster");
    QCoreApplication::setApplicationVersion(DISPLAY_VERSION);
    QCoreApplication::setOrganizationName("Venti1112");
    QCoreApplication::setOrganizationDomain("ventichat.com");
    QGuiApplication::setApplicationDisplayName("WinTimeMaster");
    QGuiApplication::setDesktopFileName("WinTimeMaster");
    app.setWindowIcon(QIcon(":/icon.ico"));

    QQuickStyle::setStyle("FluentWinUI3");

    QFont globalFont("Microsoft YaHei UI", 10);
    globalFont.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(globalFont);

    QQmlApplicationEngine engine;

    LanguageManager *languageManager = new LanguageManager(&engine, &engine);
    engine.rootContext()->setContextProperty("LanguageSwitcher", languageManager);

    SettingsController *settingsCtrl = new SettingsController(&engine);
    engine.rootContext()->setContextProperty("SettingsController", settingsCtrl);

    LockController *lockCtrl = new LockController(settingsCtrl->timeRuleModel(), &engine, &engine);
    engine.rootContext()->setContextProperty("LockController", lockCtrl);

    TrayManager *trayManager = new TrayManager(&engine, &engine);
    engine.rootContext()->setContextProperty("TrayManager", trayManager);
    trayManager->setLockController(lockCtrl);

    QObject::connect(languageManager, &LanguageManager::languageChanged,
                     trayManager, &TrayManager::updateMenuTexts);

    // 将启动模式传给 QML
    engine.rootContext()->setContextProperty("startMinimized", startMinimized);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("WinTimeMaster", "Settings");

    return app.exec();
}