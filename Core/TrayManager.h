#ifndef TRAYMANAGER_H
#define TRAYMANAGER_H

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QQmlApplicationEngine>

class LockController;

class TrayManager : public QObject
{
    Q_OBJECT

public:
    explicit TrayManager(QQmlApplicationEngine *engine, QObject *parent = nullptr);
    ~TrayManager() override = default;

    void setLockController(LockController *ctrl);

    Q_INVOKABLE void showSettingsWindow();
    Q_INVOKABLE void hideSettingsWindow();
    Q_INVOKABLE void showTrayIcon();

public slots:
    void updateMenuTexts();
    void updateTooltip();

private:
    void setupTrayIcon();
    void setupMenu();

    QQmlApplicationEngine *m_engine = nullptr;
    QSystemTrayIcon *m_trayIcon = nullptr;
    QMenu *m_trayMenu = nullptr;
    QAction *m_showAction = nullptr;
    QAction *m_startStopAction = nullptr;
    QAction *m_quitAction = nullptr;
    LockController *m_lockCtrl = nullptr;
};

#endif // TRAYMANAGER_H