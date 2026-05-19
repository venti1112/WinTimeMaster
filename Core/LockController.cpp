#include "LockController.h"
#include "TimeRuleModel.h"
#include "TimeRule.h"
#include "ConfigManager.h"
#include <QQmlComponent>
#include <QQuickWindow>
#include <QScreen>
#include <QGuiApplication>
#include <QProcess>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <winreg.h>
#endif

LockController::LockController(TimeRuleModel *model, QQmlApplicationEngine *engine,
                               QObject *parent)
    : QObject(parent), m_model(model), m_engine(engine)
{
    if (m_engine) {
        QQmlComponent component(m_engine,
                                QUrl("qrc:/qt/qml/WinTimeMaster/UI/LockScreen.qml"));
        if (component.isError()) {
            qWarning() << "Failed to load LockScreen.qml:" << component.errorString();
        } else {
            QObject *obj = component.create();
            m_lockWindow = qobject_cast<QQuickWindow*>(obj);
            if (m_lockWindow) {
                m_lockWindow->setVisible(false);
            } else {
                qWarning() << "LockScreen root is not a Window!";
                delete obj;
            }
        }
    }

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &LockController::checkLockRules);

    // 恢复上次服务状态
    bool lastRunning = ConfigManager::instance()->readBool("ServiceRunning", false);
    if (lastRunning) {
        startChecking();
    }
}

// ---------- 系统限制 ----------
void LockController::blockInput(bool block)
{
#ifdef Q_OS_WIN
    ::BlockInput(block ? TRUE : FALSE);
#endif
}

void LockController::disableTaskManager(bool disable)
{
#ifdef Q_OS_WIN
    HKEY hKey;
    LONG result = RegCreateKeyEx(HKEY_CURRENT_USER,
                                 L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System",
                                 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL);
    if (result == ERROR_SUCCESS) {
        DWORD value = disable ? 1 : 0;
        RegSetValueEx(hKey, L"DisableTaskMgr", 0, REG_DWORD,
                      (BYTE*)&value, sizeof(value));
        RegCloseKey(hKey);
    }
#endif
}

void LockController::applySystemRestrictions()
{
    ConfigManager *cfg = ConfigManager::instance();
    if (cfg->readBool("EnableInputBlock", true))
        blockInput(true);
    if (cfg->readBool("DisableTaskManager", true))
        disableTaskManager(true);
    if (cfg->readBool("KillTaskmgr", true))
        QProcess::execute("taskkill", QStringList() << "/f" << "/im" << "taskmgr.exe");
}

void LockController::removeSystemRestrictions()
{
    ConfigManager *cfg = ConfigManager::instance();
    if (cfg->readBool("EnableInputBlock", true))
        blockInput(false);
    if (cfg->readBool("DisableTaskManager", true))
        disableTaskManager(false);
}

// ---------- 状态持久化 ----------
void LockController::saveServiceState()
{
    ConfigManager::instance()->writeBool("ServiceRunning", m_checking);
}

bool LockController::isChecking() const
{
    return m_checking;
}

void LockController::startChecking()
{
    if (m_checking) return;
    m_checking = true;
    m_timer->start();
    checkLockRules();
    saveServiceState();
    emit checkingChanged(true);
}

void LockController::stopChecking()
{
    if (!m_checking) return;
    m_checking = false;
    m_timer->stop();
    removeSystemRestrictions();
    if (m_lockWindow)
        m_lockWindow->hide();
    saveServiceState();
    emit checkingChanged(false);
}

void LockController::toggleChecking()
{
    if (m_checking) stopChecking();
    else startChecking();
}

// ---------- 核心检查逻辑 ----------
void LockController::checkLockRules()
{
    if (!m_lockWindow || !m_model) return;

    QDateTime now = QDateTime::currentDateTime();
    QDate today = now.date();
    int dayOfWeek = today.dayOfWeek();
    QTime currentTime = now.time();

    bool isLocked = false;
    QDateTime latestUnlock;

    const QList<TimeRule*> rules = m_model->rules();
    for (const TimeRule *rule : rules) {
        if (!rule->enabled()) continue;

        QTime startTime = rule->startTime();
        QTime endTime = rule->endTime();
        if (!startTime.isValid() || !endTime.isValid()) continue;

        auto makeInterval = [&](const QDate &baseDate) -> QPair<QDateTime, QDateTime> {
            QDateTime start(baseDate, startTime);
            QDateTime end;
            if (endTime <= startTime)
                end = QDateTime(baseDate.addDays(1), endTime);
            else
                end = QDateTime(baseDate, endTime);
            return {start, end};
        };

        QPair<QDateTime, QDateTime> interval;
        bool active = false;

        switch (rule->repeatMode()) {
        case TimeRule::Once:
        case TimeRule::Daily: {
            auto [s, e] = makeInterval(today);
            if (now >= s && now < e) {
                active = true;
                interval = {s, e};
            }
            break;
        }
        case TimeRule::WeekDays: {
            int weekMask = rule->weekDays();
            QDate yesterday = today.addDays(-1);
            if ((1 << (yesterday.dayOfWeek() - 1)) & weekMask) {
                auto [s, e] = makeInterval(yesterday);
                if (now >= s && now < e) {
                    active = true;
                    interval = {s, e};
                }
            }
            if (!active && ((1 << (dayOfWeek - 1)) & weekMask)) {
                auto [s, e] = makeInterval(today);
                if (now >= s && now < e) {
                    active = true;
                    interval = {s, e};
                }
            }
            break;
        }
        default: break;
        }

        if (active) {
            isLocked = true;
            if (!latestUnlock.isValid() || interval.second > latestUnlock)
                latestUnlock = interval.second;
        }
    }

    if (isLocked && latestUnlock.isValid()) {
        QString currentTimeStr = now.toString("HH:mm:ss");
        QString unlockTimeStr = latestUnlock.toString("HH:mm:ss");
        qint64 secs = now.secsTo(latestUnlock);
        if (secs < 0) secs = 0;
        int h = secs / 3600;
        int m = (secs % 3600) / 60;
        int s = secs % 60;
        QString remainingStr = QString("%1:%2:%3")
                                   .arg(h, 2, 10, QChar('0'))
                                   .arg(m, 2, 10, QChar('0'))
                                   .arg(s, 2, 10, QChar('0'));

        m_lockWindow->setProperty("currentTime", currentTimeStr);
        m_lockWindow->setProperty("unlockTime", unlockTimeStr);
        m_lockWindow->setProperty("remainingTime", remainingStr);

        // 同步背景图片
        ConfigManager *cfgInst = ConfigManager::instance();
        QString bgImage = cfgInst->readString("LockBackground", "");
        m_lockWindow->setProperty("backgroundImage", bgImage);

        // 同步文字颜色 / 位置 / 提示词 / 隐藏开关
        m_lockWindow->setProperty("promptColor",
                                  cfgInst->readString("LockPromptColor", "#ffffffff"));
        m_lockWindow->setProperty("currentTimeColor",
                                  cfgInst->readString("LockCurrentTimeColor", "#ffd3d3d3"));
        m_lockWindow->setProperty("unlockTimeColor",
                                  cfgInst->readString("LockUnlockTimeColor", "#ffd3d3d3"));
        m_lockWindow->setProperty("remainingTimeColor",
                                  cfgInst->readString("LockRemainingTimeColor", "#ffff6347"));
        m_lockWindow->setProperty("textPosition",
                                  cfgInst->readString("LockTextPosition", "center"));
        m_lockWindow->setProperty("promptText",
                                  cfgInst->readString("LockPromptText", QString()));
        m_lockWindow->setProperty("hideCurrentTime",
                                  cfgInst->readBool("HideCurrentTime", false));
        m_lockWindow->setProperty("hideUnlockTime",
                                  cfgInst->readBool("HideUnlockTime", false));
        m_lockWindow->setProperty("hideRemainingTime",
                                  cfgInst->readBool("HideRemainingTime", false));

        applySystemRestrictions();

        if (!m_lockWindow->isVisible()) {
            QRect totalRect;
            const auto screens = QGuiApplication::screens();
            for (QScreen *screen : screens)
                totalRect = totalRect.united(screen->geometry());
            m_lockWindow->setGeometry(totalRect);
            m_lockWindow->show();
            m_lockWindow->raise();
            m_lockWindow->requestActivate();
        }
    } else {
        if (m_lockWindow->isVisible()) {
            removeSystemRestrictions();
            m_lockWindow->hide();
        }
    }
}
