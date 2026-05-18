#include "TrayManager.h"
#include "LockController.h"
#include <QApplication>
#include <QStyle>

TrayManager::TrayManager(QQmlApplicationEngine *engine, QObject *parent)
    : QObject(parent), m_engine(engine)
{
    setupTrayIcon();
    setupMenu();
}

void TrayManager::setupTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/icon.ico"));
    m_trayIcon->setToolTip(tr("WinTimeMaster"));

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            showSettingsWindow();
        }
    });

    m_trayIcon->show();
}

void TrayManager::setupMenu()
{
    m_trayMenu = new QMenu();

    m_showAction = m_trayMenu->addAction(tr("Show Settings"));
    connect(m_showAction, &QAction::triggered, this, &TrayManager::showSettingsWindow);

    // 启动/停止检查动作稍后由 setLockController 添加
    m_startStopAction = nullptr;

    m_quitAction = m_trayMenu->addAction(tr("Quit"));
    connect(m_quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(m_trayMenu);
}

void TrayManager::setLockController(LockController *ctrl)
{
    if (!ctrl) return;

    // 如果已存在则先移除
    if (m_startStopAction) {
        m_trayMenu->removeAction(m_startStopAction);
        delete m_startStopAction;
        m_startStopAction = nullptr;
    }

    // 插入在“显示设置”和“退出”之间
    m_startStopAction = new QAction(tr("Start Checking"), this);
    m_trayMenu->insertAction(m_quitAction, m_startStopAction);

    connect(m_startStopAction, &QAction::triggered, ctrl, &LockController::toggleChecking);
    connect(ctrl, &LockController::checkingChanged, this, [this](bool checking) {
        if (m_startStopAction)
            m_startStopAction->setText(checking ? tr("Stop Checking") : tr("Start Checking"));
    });
}

void TrayManager::showSettingsWindow()
{
    if (m_engine) {
        QObject *rootObj = m_engine->rootObjects().value(0);
        if (auto window = qobject_cast<QWindow*>(rootObj)) {
            window->show();
            window->raise();
            window->requestActivate();
        }
    }
}

void TrayManager::hideSettingsWindow()
{
    if (m_engine) {
        QObject *rootObj = m_engine->rootObjects().value(0);
        if (auto window = qobject_cast<QWindow*>(rootObj)) {
            window->hide();
        }
    }
}

void TrayManager::showTrayIcon()
{
    if (m_trayIcon)
        m_trayIcon->show();
}

void TrayManager::updateMenuTexts()
{
    if (m_showAction)
        m_showAction->setText(tr("Show Settings"));
    if (m_quitAction)
        m_quitAction->setText(tr("Quit"));
    // 注意：m_startStopAction 文本由 checkingChanged 信号自动更新，此处无需处理
}