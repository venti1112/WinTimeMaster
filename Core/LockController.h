#ifndef LOCKCONTROLLER_H
#define LOCKCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QQuickWindow>
#include <QQmlApplicationEngine>
#include "TimeRuleModel.h"

class LockController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool checking READ isChecking NOTIFY checkingChanged)

public:
    explicit LockController(TimeRuleModel *model, QQmlApplicationEngine *engine,
                            QObject *parent = nullptr);
    ~LockController() override = default;

    bool isChecking() const;

public slots:
    void startChecking();
    void stopChecking();
    void toggleChecking();

signals:
    void checkingChanged(bool checking);

private slots:
    void checkLockRules();

private:
    void updateLockWindow();

    TimeRuleModel *m_model = nullptr;
    QQuickWindow *m_lockWindow = nullptr;
    QTimer *m_timer = nullptr;
    bool m_checking = false;
    QQmlApplicationEngine *m_engine = nullptr;   // 用于创建锁屏窗口
};

#endif // LOCKCONTROLLER_H