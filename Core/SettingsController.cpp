#include "SettingsController.h"
#include <QSettings>
#include <QCoreApplication>
#include <QProcess>
#include <QDebug>

SettingsController::SettingsController(QObject *parent)
    : QObject(parent)
    , m_model(new TimeRuleModel(this))
{
}

TimeRuleModel *SettingsController::timeRuleModel() const
{
    return m_model;
}

bool SettingsController::isAutostartEnabled() const
{
    QSettings settings;
    return settings.value("AutoStart", false).toBool();
}

void SettingsController::setAutostartEnabled(bool enabled)
{
    if (isAutostartEnabled() == enabled)
        return;

    QSettings settings;
    settings.setValue("AutoStart", enabled);
    updateAutostartTask(enabled);
    emit autostartEnabledChanged(enabled);
}

void SettingsController::updateAutostartTask(bool enable)
{
    QString exePath = QCoreApplication::applicationFilePath();
    QString taskName = "WinTimeMaster";

    if (enable) {
        QString cmd = QString(
                          "schtasks /Create /F /SC ONLOGON /RL HIGHEST /TN \"%1\" "
                          "/TR \"\\\"%2\\\" --autostart\""
                          ).arg(taskName, exePath);
        qDebug() << "Creating autostart task:" << cmd;
        QProcess::execute(cmd);
    } else {
        QString cmd = QString("schtasks /Delete /F /TN \"%1\"").arg(taskName);
        qDebug() << "Removing autostart task:" << cmd;
        QProcess::execute(cmd);
    }
}