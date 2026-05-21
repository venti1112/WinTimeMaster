#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <QCommandLineParser>
#include <QLocalSocket>
#include <QSurfaceFormat>
#include <QStandardPaths>
#include <QDebug>
#include <windows.h>
#include <shellapi.h>

#include "Core/AsyncLogger.h"
#include "Core/ConfigManager.h"
#include "Core/LanguageManager.h"
#include "Core/SettingsController.h"
#include "Core/TrayManager.h"
#include "Core/LockController.h"
#include "Core/TimeSyncManager.h"
#include "Core/AboutController.h"
#include "Core/UpdateChecker.h"
#include "Core/RemoteConfigUpdater.h"

static bool isAdmin() {
    qInfo("Permissions checking");
    HANDLE hToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) qFatal("The system environment is abnormal and permissions cannot be checked.");
    TOKEN_ELEVATION elevation;
    DWORD size = sizeof(TOKEN_ELEVATION);
    bool result = false;
    if (GetTokenInformation(hToken, TokenElevation, &elevation, size, &size)) result = (elevation.TokenIsElevated != 0);
    CloseHandle(hToken);
    return result;
}

static bool restartAsAdmin(int argc, char *argv[]) {
    qInfo("Attempting privilege escalation");
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    QStringList args;
    for (int i = 1; i < argc; ++i)
        args << QString::fromLocal8Bit(argv[i]);
    QString joined = args.join(' ');
    std::wstring wargs = joined.toStdWString();

    HINSTANCE result = ShellExecuteW(NULL, L"runas", exePath, wargs.empty() ? nullptr : wargs.c_str(), NULL, SW_SHOW);
    if ((INT_PTR)result > 32) {
        qInfo("Elevated process launched successfully.");
        return true;
    } else {
        DWORD error = GetLastError();
        qFatal() << "Failed to elevate privileges. ShellExecute error:" << error;
        return false;
    }
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName("WinTimeMaster");
    QCoreApplication::setApplicationVersion(DISPLAY_VERSION);
    QCoreApplication::setOrganizationName("Venti1112");
    QCoreApplication::setOrganizationDomain("ventichat.com");
    QGuiApplication::setApplicationDisplayName("WinTimeMaster");
    QGuiApplication::setDesktopFileName("WinTimeMaster");

    AsyncLogger::instance().init();
    AsyncLogger::instance().setLogLevel(QtDebugMsg);
    AsyncLogger::instance().setConsoleOutput(true);

    qInfo("Program starting");
    qInfo("Log system initialization complete");

    if (!isAdmin()) {
        qInfo("Not administrator privileges, privilege escalation is imminent.");
        if (!restartAsAdmin(argc, argv)) {
            return 1;
        }
        qInfo("A high-privilege process has been started and is about to exit.");
        return 0;
    }

    qInfo("Permission check passed; administrator privileges granted.");

    HANDLE instanceMutex = ::CreateMutexW(nullptr, FALSE, L"WinTimeMaster_SingleInstance");
    DWORD lastError = ::GetLastError();
    if (instanceMutex == nullptr || lastError == ERROR_ALREADY_EXISTS) {
        QLocalSocket socket;
        socket.connectToServer("WinTimeMaster_IPC");
        if (socket.waitForConnected(2000)) {
            socket.write("show");
            socket.waitForBytesWritten(2000);
        } else {
            qWarning() << "Could not connect to existing instance.";
        }
        if (instanceMutex) ::CloseHandle(instanceMutex);
        return 0;
    }

    QCommandLineParser parser;
    QCommandLineOption autostartOption("autostart", "Start minimized to tray.");
    parser.addOption(autostartOption);
    parser.process(app);
    bool startMinimized = parser.isSet(autostartOption);

    QSurfaceFormat format;
    format.setAlphaBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);

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
        const QString name = obj->objectName();
        if (name == "passwordWindow")
            passwordWindow = obj;
        else if (name == "settingsWindow")
            settingsWindow = obj;
    }
    if (settingsWindow)
        engine.rootContext()->setContextProperty("SettingsWindow", settingsWindow);
    if (passwordWindow)
        engine.rootContext()->setContextProperty("AuthWindow", passwordWindow);

    trayManager->setSettingsWindow(settingsWindow);

    if (!startMinimized) {
        trayManager->showSettingsWindow();
    }
    qInfo("Program initialization complete!");

    int ret = app.exec();
    qInfo("Program Exit");
    AsyncLogger::instance().shutdown();
    return ret;
}