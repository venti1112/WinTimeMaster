#ifndef SETTINGSCONTROLLER_H
#define SETTINGSCONTROLLER_H

#include <QObject>
#include "TimeRuleModel.h"

class SettingsController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TimeRuleModel* timeRuleModel READ timeRuleModel CONSTANT)
    Q_PROPERTY(bool autostartEnabled READ isAutostartEnabled WRITE setAutostartEnabled NOTIFY autostartEnabledChanged)

public:
    explicit SettingsController(QObject *parent = nullptr);
    ~SettingsController() override = default;

    TimeRuleModel *timeRuleModel() const;

    bool isAutostartEnabled() const;
    Q_INVOKABLE void setAutostartEnabled(bool enabled);

signals:
    void autostartEnabledChanged(bool enabled);

private:
    void updateAutostartTask(bool enable);
    TimeRuleModel *m_model = nullptr;
};

#endif // SETTINGSCONTROLLER_H