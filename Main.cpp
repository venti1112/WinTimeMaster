#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <QFont>
#include <QCommandLineParser>
#include <QLocalSocket>
#include <QSharedMemory>
#include <QSurfaceFormat>
#include <QStandardPaths>

#include "Core/ConfigManager.h"
#include "Core/LanguageManager.h"
#include "Core/SettingsController.h"
#include "Core/TrayManager.h"
#include "Core/LockController.h"
#include "Core/TimeSyncManager.h"
#include "Core/AboutController.h"
#include "Core/UpdateChecker.h"
#include "Core/RemoteConfigUpdater.h"

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

    QSurfaceFormat format;
    format.setAlphaBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);

    // ----- 单实例检测 -----
    QSharedMemory sharedMem("Global\\WinTimeMaster_SingleInstance");
    if (!sharedMem.create(1)) {
        // 已有实例运行，通过 IPC 通知旧实例显示窗口
        QLocalSocket socket;
        socket.connectToServer("WinTimeMaster_IPC");
        if (socket.waitForConnected(2000)) {
            socket.write("show");
            socket.waitForBytesWritten(2000);
        } else {
            qWarning() << "Could not connect to existing instance.";
        }
        return 0;
    }

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

    QQmlApplicationEngine engine;

    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/config.json";
    ConfigManager::initialize(configPath);

    LanguageManager *languageManager = new LanguageManager(&engine, &engine);
    engine.rootContext()->setContextProperty("LanguageSwitcher", languageManager);

    SettingsController *settingsCtrl = new SettingsController(&engine);
    engine.rootContext()->setContextProperty("SettingsController", settingsCtrl);

    TimeSyncManager *timeSyncMgr = new TimeSyncManager(&engine);
    engine.rootContext()->setContextProperty("TimeSyncManager", timeSyncMgr);

    QObject::connect(settingsCtrl, &SettingsController::settingsImported,
                     timeSyncMgr, &TimeSyncManager::reloadFromConfig);

    LockController *lockCtrl = new LockController(settingsCtrl->timeRuleModel(), &engine, &engine);
    engine.rootContext()->setContextProperty("LockController", lockCtrl);

    AboutController *aboutCtrl = new AboutController(&engine);
    engine.rootContext()->setContextProperty("AboutController", aboutCtrl);

    UpdateChecker *updateChecker = new UpdateChecker(&engine);
    engine.rootContext()->setContextProperty("UpdateChecker", updateChecker);

    RemoteConfigUpdater *remoteConfigUpdater = new RemoteConfigUpdater(&engine);
    engine.rootContext()->setContextProperty("RemoteConfigUpdater", remoteConfigUpdater);
    remoteConfigUpdater->reloadFromConfig();

    TrayManager *trayManager = new TrayManager(&engine, &engine);
    engine.rootContext()->setContextProperty("TrayManager", trayManager);
    trayManager->setLockController(lockCtrl);

    QObject::connect(languageManager, &LanguageManager::languageChanged,
                     trayManager, &TrayManager::updateMenuTexts);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("WinTimeMaster", "Settings");
    engine.loadFromModule("WinTimeMaster", "PasswordWindow");

    QObject *settingsWindow = nullptr;
    QObject *passwordWindow = nullptr;
    for (QObject *obj : engine.rootObjects()) {
        if (obj->objectName() == "passwordWindow") {
            passwordWindow = obj;
        } else if (obj->objectName().isEmpty() && !settingsWindow) {
            settingsWindow = obj;
        }
    }
    if (settingsWindow)
        engine.rootContext()->setContextProperty("SettingsWindow", settingsWindow);
    if (passwordWindow)
        engine.rootContext()->setContextProperty("AuthWindow", passwordWindow);

    if (!startMinimized) {
        trayManager->showSettingsWindow();
    }

    return app.exec();
}