#ifndef LOCKCONTROLLER_H
#define LOCKCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QPair>
#include <QQmlApplicationEngine>
#include <atomic>
#include <windows.h>

class TimeRuleModel;
class QQuickWindow;

class LockController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool checking READ isChecking NOTIFY checkingChanged)

public:
    explicit LockController(TimeRuleModel *model, QQmlApplicationEngine *engine,
                            QObject *parent = nullptr);
    ~LockController() override;

    bool isChecking() const;
    Q_INVOKABLE bool emergencyExit(const QString &password = QString());
    Q_INVOKABLE void beginEmergencyPassword();
    Q_INVOKABLE void cancelEmergencyPassword();

public slots:
    void startChecking();
    void stopChecking();
    void toggleChecking();

signals:
    void checkingChanged(bool checking);

private slots:
    void checkLockRules();

private:
    void saveServiceState();
    void applySystemRestrictions();
    void removeSystemRestrictions();
    void blockInput(bool block);
    void disableTaskManager(bool disable);

    static LRESULT CALLBACK keyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK mouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);

    TimeRuleModel *m_model = nullptr;
    QQuickWindow *m_lockWindow = nullptr;
    QTimer *m_timer = nullptr;
    bool m_checking = false;
    std::atomic_bool m_emergencyPasswordActive{false};
    bool m_restrictionsActive = false;
    // 钩子与注册表已实际安装/写入的状态 —— 与 config 解耦，避免在 apply 与 remove 之间配置变化导致泄漏
    bool m_inputBlockInstalled = false;
    bool m_taskMgrDisabled = false;
    QQmlApplicationEngine *m_engine = nullptr;
    HHOOK m_keyboardHook = nullptr;
    HHOOK m_mouseHook = nullptr;
    HWND m_lockHwnd = nullptr;
};

#endif // LOCKCONTROLLER_H