#include "SettingsController.h"
#include <QSettings>
#include <QCoreApplication>
#include <QProcess>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
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

// ---------- 开机自启 ----------
bool SettingsController::isAutostartEnabled() const
{
    return checkAutostartTaskExists();
}

void SettingsController::setAutostartEnabled(bool enabled)
{
    if (isAutostartEnabled() == enabled) return;
    updateAutostartTask(enabled);
    emit autostartEnabledChanged(enabled);
}

bool SettingsController::checkAutostartTaskExists() const
{
    QProcess proc;
    proc.start("schtasks", QStringList() << "/Query" << "/TN" << "WinTimeMaster");
    proc.waitForFinished(3000);
    return proc.exitCode() == 0;
}

void SettingsController::updateAutostartTask(bool enable)
{
    QString exePath = QCoreApplication::applicationFilePath();
    QString taskName = "WinTimeMaster";
    if (enable) {
        QProcess proc;
        proc.start("schtasks", QStringList()
                                   << "/Create" << "/F" << "/SC" << "ONLOGON"
                                   << "/RL" << "HIGHEST" << "/TN" << taskName
                                   << "/TR" << (QString("\"%1\" --autostart").arg(exePath)));
        proc.waitForFinished(5000);
        if (proc.exitCode() != 0)
            qWarning() << "Failed to create autostart task:" << proc.readAllStandardError();
    } else {
        QProcess proc;
        proc.start("schtasks", QStringList() << "/Delete" << "/F" << "/TN" << taskName);
        proc.waitForFinished(5000);
        if (proc.exitCode() != 0)
            qWarning() << "Failed to delete autostart task:" << proc.readAllStandardError();
    }
}

// ---------- 安全选项 ----------
bool SettingsController::isDisableTaskManager() const
{
    QSettings settings;
    return settings.value("DisableTaskManager", true).toBool();
}
void SettingsController::setDisableTaskManager(bool disable)
{
    if (isDisableTaskManager() == disable) return;
    QSettings settings;
    settings.setValue("DisableTaskManager", disable);
    emit disableTaskManagerChanged(disable);
}

bool SettingsController::isEnableInputBlock() const
{
    QSettings settings;
    return settings.value("EnableInputBlock", true).toBool();
}
void SettingsController::setEnableInputBlock(bool enable)
{
    if (isEnableInputBlock() == enable) return;
    QSettings settings;
    settings.setValue("EnableInputBlock", enable);
    emit enableInputBlockChanged(enable);
}

bool SettingsController::isKillTaskmgr() const
{
    QSettings settings;
    return settings.value("KillTaskmgr", true).toBool();
}
void SettingsController::setKillTaskmgr(bool kill)
{
    if (isKillTaskmgr() == kill) return;
    QSettings settings;
    settings.setValue("KillTaskmgr", kill);
    emit killTaskmgrChanged(kill);
}

bool SettingsController::isAutoTimeSync() const
{
    QSettings settings;
    return settings.value("AutoTimeSync", false).toBool();
}
void SettingsController::setAutoTimeSync(bool enabled)
{
    if (isAutoTimeSync() == enabled) return;
    QSettings settings;
    settings.setValue("AutoTimeSync", enabled);
    emit autoTimeSyncChanged(enabled);
}

QString SettingsController::lockScreenBackground() const
{
    QSettings settings;
    return settings.value("LockBackground", "#000000").toString();
}
void SettingsController::setLockScreenBackground(const QString &background)
{
    if (lockScreenBackground() == background) return;
    QSettings settings;
    settings.setValue("LockBackground", background);
    emit lockScreenBackgroundChanged(background);
}

// ---------- 导出/导入 ----------
QString SettingsController::exportSettings(const QString &filePath)
{
    QSettings settings;
    QStringList keys = settings.allKeys();
    QJsonObject root;
    for (const QString &key : keys)
        root.insert(key, QJsonValue::fromVariant(settings.value(key)));

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return tr("Cannot open file for writing");
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    return {};
}

QString SettingsController::importSettings(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return tr("Cannot open file for reading");

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError)
        return tr("JSON parse error: ") + error.errorString();
    if (!doc.isObject())
        return tr("Invalid settings file");

    QJsonObject root = doc.object();
    QSettings settings;
    settings.clear();
    for (auto it = root.begin(); it != root.end(); ++it)
        settings.setValue(it.key(), it.value().toVariant());
    settings.sync();

    emit settingsImported();
    return {};
}