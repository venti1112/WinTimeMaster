#ifndef SETTINGSCONTROLLER_H
#define SETTINGSCONTROLLER_H

#include <QObject>
#include "TimeRuleModel.h"

class SettingsController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TimeRuleModel* timeRuleModel READ timeRuleModel CONSTANT)
    Q_PROPERTY(bool autostartEnabled READ isAutostartEnabled WRITE setAutostartEnabled NOTIFY autostartEnabledChanged)
    Q_PROPERTY(bool disableTaskManager READ isDisableTaskManager WRITE setDisableTaskManager NOTIFY disableTaskManagerChanged)
    Q_PROPERTY(bool enableInputBlock READ isEnableInputBlock WRITE setEnableInputBlock NOTIFY enableInputBlockChanged)
    Q_PROPERTY(bool killTaskmgr READ isKillTaskmgr WRITE setKillTaskmgr NOTIFY killTaskmgrChanged)
    Q_PROPERTY(bool autoTimeSync READ isAutoTimeSync WRITE setAutoTimeSync NOTIFY autoTimeSyncChanged)
    Q_PROPERTY(QString lockScreenBackground READ lockScreenBackground WRITE setLockScreenBackground NOTIFY lockScreenBackgroundChanged)

public:
    explicit SettingsController(QObject *parent = nullptr);
    ~SettingsController() override = default;

    TimeRuleModel *timeRuleModel() const;

    bool isAutostartEnabled() const;
    Q_INVOKABLE void setAutostartEnabled(bool enabled);

    bool isDisableTaskManager() const;
    Q_INVOKABLE void setDisableTaskManager(bool disable);
    bool isEnableInputBlock() const;
    Q_INVOKABLE void setEnableInputBlock(bool enable);
    bool isKillTaskmgr() const;
    Q_INVOKABLE void setKillTaskmgr(bool kill);
    bool isAutoTimeSync() const;
    Q_INVOKABLE void setAutoTimeSync(bool enabled);
    QString lockScreenBackground() const;
    Q_INVOKABLE void setLockScreenBackground(const QString &background);

    Q_INVOKABLE QString exportSettings(const QString &filePath);
    Q_INVOKABLE QString importSettings(const QString &filePath);

signals:
    void autostartEnabledChanged(bool enabled);
    void disableTaskManagerChanged(bool disable);
    void enableInputBlockChanged(bool enable);
    void killTaskmgrChanged(bool kill);
    void autoTimeSyncChanged(bool enabled);
    void lockScreenBackgroundChanged(const QString &background);
    void settingsImported();

private:
    void updateAutostartTask(bool enable);
    bool checkAutostartTaskExists() const;
    TimeRuleModel *m_model = nullptr;
};

#endif // SETTINGSCONTROLLER_H