#include "TrayManager.h"
#include "LockController.h"
#include <QApplication>
#include <QWindow>
#include <QStyle>
#include <QLocalSocket>
#include <QDebug>

TrayManager::TrayManager(QQmlApplicationEngine *engine, QObject *parent)
    : QObject(parent), m_engine(engine)
{
    setupTrayIcon();
    setupMenu();
    setupIpcServer();
}

void TrayManager::setupTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/icon.ico"));

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            showSettingsWindow();
        }
    });

    m_trayIcon->show();
    updateTooltip();
}

void TrayManager::setupMenu()
{
    m_trayMenu = new QMenu();

    m_showAction = m_trayMenu->addAction(tr("Show Settings"));
    connect(m_showAction, &QAction::triggered, this, &TrayManager::showSettingsWindow);

    // 启动/停止服务动作稍后由 setLockController 添加
    m_startStopAction = nullptr;

    m_quitAction = m_trayMenu->addAction(tr("Quit"));
    connect(m_quitAction, &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(m_trayMenu);
}

void TrayManager::setLockController(LockController *ctrl)
{
    m_lockCtrl = ctrl;
    if (!ctrl) return;

    if (m_startStopAction) {
        m_trayMenu->removeAction(m_startStopAction);
        delete m_startStopAction;
        m_startStopAction = nullptr;
    }

    m_startStopAction = new QAction(tr("Start Service"), this);  // 改为 Service
    m_trayMenu->insertAction(m_quitAction, m_startStopAction);

    connect(m_startStopAction, &QAction::triggered, ctrl, &LockController::toggleChecking);
    connect(ctrl, &LockController::checkingChanged, this, [this](bool checking) {
        if (m_startStopAction)
            m_startStopAction->setText(checking ? tr("Stop Service") : tr("Start Service"));
        updateTooltip();   // 状态变化更新提示
    });

    updateTooltip();
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
    if (m_startStopAction && m_lockCtrl) {
        m_startStopAction->setText(m_lockCtrl->isChecking() ? tr("Stop Service") : tr("Start Service"));
    }
    updateTooltip();   // 语言切换时更新提示
}

void TrayManager::updateTooltip()
{
    if (!m_trayIcon) return;
    QString status = m_lockCtrl && m_lockCtrl->isChecking()
                         ? tr("Service Running")
                         : tr("Service Stopped");
    m_trayIcon->setToolTip(tr("WinTimeMaster") + " - " + status);
}
void TrayManager::setupIpcServer()
{
    // 清理可能因上次崩溃残留的命名
    QLocalServer::removeServer("WinTimeMaster_IPC");
    m_ipcServer = new QLocalServer(this);
    if (!m_ipcServer->listen("WinTimeMaster_IPC")) {
        qWarning() << "Failed to start IPC server:" << m_ipcServer->errorString();
        return;
    }

    connect(m_ipcServer, &QLocalServer::newConnection, this, [this]() {
        QLocalSocket *client = m_ipcServer->nextPendingConnection();
        if (client) {
            // 只需要知道有消息即显示，可读取一下清空缓冲区
            client->waitForReadyRead(1000);
            client->readAll();
            showSettingsWindow();
            client->deleteLater();
        }
    });
}