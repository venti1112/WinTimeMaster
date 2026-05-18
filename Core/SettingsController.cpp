#include "SettingsController.h"

SettingsController::SettingsController(QObject *parent)
    : QObject(parent)
    , m_model(new TimeRuleModel(this))
{
}

TimeRuleModel *SettingsController::timeRuleModel() const
{
    return m_model;
}