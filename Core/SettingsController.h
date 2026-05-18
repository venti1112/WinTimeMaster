#ifndef SETTINGSCONTROLLER_H
#define SETTINGSCONTROLLER_H

#include <QObject>
#include "TimeRuleModel.h"

class SettingsController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TimeRuleModel* timeRuleModel READ timeRuleModel CONSTANT)

public:
    explicit SettingsController(QObject *parent = nullptr);
    ~SettingsController() override = default;

    TimeRuleModel *timeRuleModel() const;

private:
    TimeRuleModel *m_model = nullptr;
};

#endif // SETTINGSCONTROLLER_H