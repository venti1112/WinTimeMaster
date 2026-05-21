#ifndef LOCKCONTROLLER_H
#define LOCKCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QPair>
#include <QQmlApplicationEngine>
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
    Q_INVOKABLE void emergencyExit();
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

    TimeRuleModel *m_model = nullptr;
    QQuickWindow *m_lockWindow = nullptr;
    QTimer *m_timer = nullptr;
    bool m_checking = false;
    bool m_emergencyPasswordActive = false;
    QQmlApplicationEngine *m_engine = nullptr;
    HHOOK m_keyboardHook = nullptr;
};

#endif // LOCKCONTROLLER_H