#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QIcon>
#include <QFont>
#include "Core/LanguageManager.h"
#include "Core/SettingsController.h"
#include "Core/TrayManager.h"
#include "Core/LockController.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName("WinTimeMaster");
    QCoreApplication::setApplicationVersion(DISPLAY_VERSION);
    QCoreApplication::setOrganizationName("Venti1112");
    QCoreApplication::setOrganizationDomain("ventichat.com");
    QGuiApplication::setApplicationDisplayName("WinTimeMaster");
    QGuiApplication::setDesktopFileName("WinTimeMaster");
    app.setWindowIcon(QIcon(":/icon.ico"));

    QQuickStyle::setStyle("FluentWinUI3");

    // 全局字体
    QFont globalFont("Microsoft YaHei", 10);
    globalFont.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(globalFont);

    QQmlApplicationEngine engine;

    // 语言管理器
    LanguageManager *languageManager = new LanguageManager(&engine, &engine);
    engine.rootContext()->setContextProperty("LanguageSwitcher", languageManager);

    // 设置控制器（时间规则模型）
    SettingsController *settingsCtrl = new SettingsController(&engine);
    engine.rootContext()->setContextProperty("SettingsController", settingsCtrl);

    // 锁屏控制器
    LockController *lockCtrl = new LockController(settingsCtrl->timeRuleModel(), &engine, &engine);
    engine.rootContext()->setContextProperty("LockController", lockCtrl);

    // 托盘管理器
    TrayManager *trayManager = new TrayManager(&engine, &engine);
    engine.rootContext()->setContextProperty("TrayManager", trayManager);

    // 注入锁屏控制器到托盘，以便添加启停菜单项
    trayManager->setLockController(lockCtrl);

    // 语言切换时更新托盘菜单
    QObject::connect(languageManager, &LanguageManager::languageChanged,
                     trayManager, &TrayManager::updateMenuTexts);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("WinTimeMaster", "Settings");

    return app.exec();
}