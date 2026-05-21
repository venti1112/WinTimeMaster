#include "SettingsController.h"
#include "ConfigManager.h"
#include "PasswordHasher.h"
#include <QCoreApplication>
#include <QProcess>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QDebug>

SettingsController::SettingsController(QObject *parent) : QObject(parent), m_model(new TimeRuleModel(this)) {}

TimeRuleModel *SettingsController::timeRuleModel() const {
    return m_model;
}

bool SettingsController::isAutostartEnabled() const {
    return checkAutostartTaskExists();
}
void SettingsController::setAutostartEnabled(bool enabled) {
    if (isAutostartEnabled() == enabled) return;
    updateAutostartTask(enabled);
    emit autostartEnabledChanged(enabled);
}

bool SettingsController::checkAutostartTaskExists() const {
    QProcess proc;
    proc.start("schtasks", QStringList() << "/Query" << "/TN" << "WinTimeMaster");
    proc.waitForFinished(3000);
    return proc.exitCode() == 0;
}
void SettingsController::updateAutostartTask(bool enable) {
    QString exePath = QCoreApplication::applicationFilePath();
    QString taskName = "WinTimeMaster";
    if (enable) {
        QProcess proc;
        proc.start("schtasks", QStringList() << "/Create" << "/F" << "/SC" << "ONLOGON" << "/RL" << "HIGHEST" << "/TN" << taskName << "/TR" << (QString("\"%1\" --autostart").arg(exePath)));
        proc.waitForFinished(5000);
        if (proc.exitCode() != 0) qWarning() << "Failed to create autostart task:" << proc.readAllStandardError();
    }
    else {
        QProcess proc;
        proc.start("schtasks", QStringList() << "/Delete" << "/F" << "/TN" << taskName);
        proc.waitForFinished(5000);
        if (proc.exitCode() != 0) qWarning() << "Failed to delete autostart task:" << proc.readAllStandardError();
    }
}

bool SettingsController::isDisableTaskManager() const {
    return ConfigManager::instance()->readBool("DisableTaskManager", true);
}
void SettingsController::setDisableTaskManager(bool disable) {
    if (isDisableTaskManager() == disable) return;
    ConfigManager::instance()->writeBool("DisableTaskManager", disable);
    emit disableTaskManagerChanged(disable);
}

bool SettingsController::isEnableInputBlock() const {
    return ConfigManager::instance()->readBool("EnableInputBlock", true);
}
void SettingsController::setEnableInputBlock(bool enable) {
    if (isEnableInputBlock() == enable) return;
    ConfigManager::instance()->writeBool("EnableInputBlock", enable);
    emit enableInputBlockChanged(enable);
}

bool SettingsController::isKillTaskmgr() const {
    return ConfigManager::instance()->readBool("KillTaskmgr", true);
}
void SettingsController::setKillTaskmgr(bool kill) {
    if (isKillTaskmgr() == kill) return;
    ConfigManager::instance()->writeBool("KillTaskmgr", kill);
    emit killTaskmgrChanged(kill);
}

QString SettingsController::lockScreenBackground() const {
    return ConfigManager::instance()->readString("LockBackground", "#000000");
}
void SettingsController::setLockScreenBackground(const QString &background) {
    if (lockScreenBackground() == background) return;
    ConfigManager::instance()->writeString("LockBackground", background);
    emit lockScreenBackgroundChanged(background);
}

bool SettingsController::lockBackgroundVideoSound() const {
    return ConfigManager::instance()->readBool("LockBackgroundVideoSound", false);
}
void SettingsController::setLockBackgroundVideoSound(bool enabled) {
    if (lockBackgroundVideoSound() == enabled) return;
    ConfigManager::instance()->writeBool("LockBackgroundVideoSound", enabled);
    emit lockBackgroundVideoSoundChanged(enabled);
}

QString SettingsController::lockPromptColor() const {
    return ConfigManager::instance()->readString("LockPromptColor", "#ffffffff");
}
void SettingsController::setLockPromptColor(const QString &color) {
    if (lockPromptColor() == color) return;
    ConfigManager::instance()->writeString("LockPromptColor", color);
    emit lockPromptColorChanged(color);
}

QString SettingsController::lockCurrentTimeColor() const {
    return ConfigManager::instance()->readString("LockCurrentTimeColor", "#ffd3d3d3");
}
void SettingsController::setLockCurrentTimeColor(const QString &color) {
    if (lockCurrentTimeColor() == color) return;
    ConfigManager::instance()->writeString("LockCurrentTimeColor", color);
    emit lockCurrentTimeColorChanged(color);
}

QString SettingsController::lockUnlockTimeColor() const {
    return ConfigManager::instance()->readString("LockUnlockTimeColor", "#ffd3d3d3");
}
void SettingsController::setLockUnlockTimeColor(const QString &color) {
    if (lockUnlockTimeColor() == color) return;
    ConfigManager::instance()->writeString("LockUnlockTimeColor", color);
    emit lockUnlockTimeColorChanged(color);
}

QString SettingsController::lockRemainingTimeColor() const {
    return ConfigManager::instance()->readString("LockRemainingTimeColor", "#ffff6347");
}
void SettingsController::setLockRemainingTimeColor(const QString &color) {
    if (lockRemainingTimeColor() == color) return;
    ConfigManager::instance()->writeString("LockRemainingTimeColor", color);
    emit lockRemainingTimeColorChanged(color);
}

QString SettingsController::lockScreenTextPosition() const {
    return ConfigManager::instance()->readString("LockTextPosition", "center");
}
void SettingsController::setLockScreenTextPosition(const QString &position) {
    if (lockScreenTextPosition() == position) return;
    ConfigManager::instance()->writeString("LockTextPosition", position);
    emit lockScreenTextPositionChanged(position);
}

QString SettingsController::lockScreenPromptText() const {
    return ConfigManager::instance()->readString("LockPromptText", QString());
}
void SettingsController::setLockScreenPromptText(const QString &text) {
    if (lockScreenPromptText() == text) return;
    ConfigManager::instance()->writeString("LockPromptText", text);
    emit lockScreenPromptTextChanged(text);
}

bool SettingsController::hideCurrentTime() const {
    return ConfigManager::instance()->readBool("HideCurrentTime", false);
}
void SettingsController::setHideCurrentTime(bool hide) {
    if (hideCurrentTime() == hide) return;
    ConfigManager::instance()->writeBool("HideCurrentTime", hide);
    emit hideCurrentTimeChanged(hide);
}

bool SettingsController::hideUnlockTime() const {
    return ConfigManager::instance()->readBool("HideUnlockTime", false);
}
void SettingsController::setHideUnlockTime(bool hide) {
    if (hideUnlockTime() == hide) return;
    ConfigManager::instance()->writeBool("HideUnlockTime", hide);
    emit hideUnlockTimeChanged(hide);
}

bool SettingsController::hideRemainingTime() const
{
    return ConfigManager::instance()->readBool("HideRemainingTime", false);
}
void SettingsController::setHideRemainingTime(bool hide)
{
    if (hideRemainingTime() == hide) return;
    ConfigManager::instance()->writeBool("HideRemainingTime", hide);
    emit hideRemainingTimeChanged(hide);
}

bool SettingsController::isEmergencyExitEnabled() const
{
    return ConfigManager::instance()->readBool("EmergencyExitEnabled", false);
}
void SettingsController::setEmergencyExitEnabled(bool enabled)
{
    if (isEmergencyExitEnabled() == enabled) return;
    ConfigManager::instance()->writeBool("EmergencyExitEnabled", enabled);
    emit emergencyExitEnabledChanged(enabled);
}

int SettingsController::emergencyExitClickCount() const
{
    return ConfigManager::instance()->readInt("EmergencyExitClickCount", 3);
}
void SettingsController::setEmergencyExitClickCount(int count)
{
    if (emergencyExitClickCount() == count) return;
    ConfigManager::instance()->writeInt("EmergencyExitClickCount", count);
    emit emergencyExitClickCountChanged(count);
}

QString SettingsController::password() const
{
    return ConfigManager::instance()->readString("PasswordHash", QString());
}
void SettingsController::setPassword(const QString &newPassword)
{
    QString oldHash = password();
    if (newPassword.isEmpty()) {
        if (oldHash.isEmpty()) return;
        ConfigManager::instance()->writeString("PasswordHash", QString());
        emit passwordChanged(QString());
    } else {
        QString newHash = PasswordHasher::hashPassword(newPassword);
        if (oldHash == newHash) return;
        ConfigManager::instance()->writeString("PasswordHash", newHash);
        emit passwordChanged(newHash);
    }
}

bool SettingsController::verifyPassword(const QString &input) const
{
    return PasswordHasher::verifyPassword(input, password());
}

void SettingsController::showPasswordError()
{
    qWarning("The user entered an incorrect password!");
    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Password Error"));
    msgBox.setText(tr("Incorrect password"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();
}

QString SettingsController::exportSettings(const QString &filePath)
{
    ConfigManager *cfg = ConfigManager::instance();

    cfg->saveToFile();

    QString srcPath = cfg->configFilePath();

    if (QFile::exists(filePath) && (!QFile::remove(filePath))) return tr("Cannot overwrite existing file");

    if (!QFile::copy(srcPath, filePath)) return tr("Cannot copy config file");

    return {};
}

QString SettingsController::importSettings(const QString &filePath)
{
    QFile src(filePath);
    if (!src.open(QIODevice::ReadOnly))
        return tr("Cannot open import file: %1").arg(src.errorString());

    QByteArray bytes = src.readAll();
    src.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError)
        return tr("Invalid JSON: %1").arg(parseError.errorString());
    if (!doc.isObject())
        return tr("Settings file must contain a JSON object");

    QJsonObject obj = doc.object();
    static const QStringList kKnownKeys = {
        "AutoTimeSync", "TimeSyncServer", "TimeSyncIntervalMinutes",
        "AutoStart", "DisableTaskManager", "EnableInputBlock", "KillTaskmgr",
        "EmergencyExitEnabled", "EmergencyExitClickCount",
        "LockBackground", "LockBackgroundVideoSound",
        "LockPromptColor", "LockCurrentTimeColor",
        "LockUnlockTimeColor", "LockRemainingTimeColor",
        "LockTextPosition", "LockPromptText",
        "HideCurrentTime", "HideUnlockTime", "HideRemainingTime",
        "PasswordHash", "ServiceRunning", "TimeRules",
        "RemoteConfigEnabled", "RemoteConfigUrl", "RemoteConfigIntervalMinutes",
        "Language"
    };
    bool looksLikeOurs = false;
    for (const QString &k : kKnownKeys) {
        if (obj.contains(k)) { looksLikeOurs = true; break; }
    }
    if (!looksLikeOurs)
        return tr("Not a valid WinTimeMaster settings file");

    ConfigManager *cfg = ConfigManager::instance();
    QString destPath = cfg->configFilePath();

    QSaveFile out(destPath);
    if (!out.open(QIODevice::WriteOnly))
        return tr("Cannot write to config: %1").arg(out.errorString());
    if (out.write(bytes) == -1) {
        out.cancelWriting();
        return tr("Failed to write config: %1").arg(out.errorString());
    }
    if (!out.commit())
        return tr("Failed to commit config: %1").arg(out.errorString());

    cfg->loadFromFile();

    emit settingsImported();
    return {};
}