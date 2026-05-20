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
    Q_PROPERTY(QString lockScreenBackground READ lockScreenBackground WRITE setLockScreenBackground NOTIFY lockScreenBackgroundChanged)
    Q_PROPERTY(QString lockPromptColor READ lockPromptColor WRITE setLockPromptColor NOTIFY lockPromptColorChanged)
    Q_PROPERTY(QString lockCurrentTimeColor READ lockCurrentTimeColor WRITE setLockCurrentTimeColor NOTIFY lockCurrentTimeColorChanged)
    Q_PROPERTY(QString lockUnlockTimeColor READ lockUnlockTimeColor WRITE setLockUnlockTimeColor NOTIFY lockUnlockTimeColorChanged)
    Q_PROPERTY(QString lockRemainingTimeColor READ lockRemainingTimeColor WRITE setLockRemainingTimeColor NOTIFY lockRemainingTimeColorChanged)
    Q_PROPERTY(QString lockScreenTextPosition READ lockScreenTextPosition WRITE setLockScreenTextPosition NOTIFY lockScreenTextPositionChanged)
    Q_PROPERTY(QString lockScreenPromptText READ lockScreenPromptText WRITE setLockScreenPromptText NOTIFY lockScreenPromptTextChanged)
    Q_PROPERTY(bool hideCurrentTime READ hideCurrentTime WRITE setHideCurrentTime NOTIFY hideCurrentTimeChanged)
    Q_PROPERTY(bool hideUnlockTime READ hideUnlockTime WRITE setHideUnlockTime NOTIFY hideUnlockTimeChanged)
    Q_PROPERTY(bool hideRemainingTime READ hideRemainingTime WRITE setHideRemainingTime NOTIFY hideRemainingTimeChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)

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
    QString lockScreenBackground() const;
    Q_INVOKABLE void setLockScreenBackground(const QString &background);

    QString lockPromptColor() const;
    Q_INVOKABLE void setLockPromptColor(const QString &color);
    QString lockCurrentTimeColor() const;
    Q_INVOKABLE void setLockCurrentTimeColor(const QString &color);
    QString lockUnlockTimeColor() const;
    Q_INVOKABLE void setLockUnlockTimeColor(const QString &color);
    QString lockRemainingTimeColor() const;
    Q_INVOKABLE void setLockRemainingTimeColor(const QString &color);

    QString lockScreenTextPosition() const;
    Q_INVOKABLE void setLockScreenTextPosition(const QString &position);

    QString lockScreenPromptText() const;
    Q_INVOKABLE void setLockScreenPromptText(const QString &text);

    bool hideCurrentTime() const;
    Q_INVOKABLE void setHideCurrentTime(bool hide);
    bool hideUnlockTime() const;
    Q_INVOKABLE void setHideUnlockTime(bool hide);
    bool hideRemainingTime() const;
    Q_INVOKABLE void setHideRemainingTime(bool hide);

    QString password() const;
    Q_INVOKABLE void setPassword(const QString &newPassword);
    Q_INVOKABLE bool verifyPassword(const QString &input) const;
    Q_INVOKABLE void showPasswordError();

    Q_INVOKABLE QString exportSettings(const QString &filePath);
    Q_INVOKABLE QString importSettings(const QString &filePath);

signals:
    void autostartEnabledChanged(bool enabled);
    void disableTaskManagerChanged(bool disable);
    void enableInputBlockChanged(bool enable);
    void killTaskmgrChanged(bool kill);
    void lockScreenBackgroundChanged(const QString &background);
    void lockPromptColorChanged(const QString &color);
    void lockCurrentTimeColorChanged(const QString &color);
    void lockUnlockTimeColorChanged(const QString &color);
    void lockRemainingTimeColorChanged(const QString &color);
    void lockScreenTextPositionChanged(const QString &position);
    void lockScreenPromptTextChanged(const QString &text);
    void hideCurrentTimeChanged(bool hide);
    void hideUnlockTimeChanged(bool hide);
    void hideRemainingTimeChanged(bool hide);
    void passwordChanged(const QString &password);
    void settingsImported();

private:
    void updateAutostartTask(bool enable);
    bool checkAutostartTaskExists() const;
    TimeRuleModel *m_model = nullptr;
};

#endif // SETTINGSCONTROLLER_H