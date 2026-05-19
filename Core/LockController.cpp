#include "LockController.h"
#include "TimeRuleModel.h"
#include "TimeRule.h"
#include <QQmlComponent>
#include <QQuickWindow>
#include <QScreen>
#include <QGuiApplication>
#include <QProcess>
#include <QSettings>
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
    QSettings settings;
    bool lastRunning = settings.value("ServiceRunning", false).toBool();
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
    QSettings settings;
    if (settings.value("EnableInputBlock", true).toBool())
        blockInput(true);
    if (settings.value("DisableTaskManager", true).toBool())
        disableTaskManager(true);
    if (settings.value("KillTaskmgr", true).toBool())
        QProcess::execute("taskkill", QStringList() << "/f" << "/im" << "taskmgr.exe");
}

void LockController::removeSystemRestrictions()
{
    QSettings settings;
    if (settings.value("EnableInputBlock", true).toBool())
        blockInput(false);
    if (settings.value("DisableTaskManager", true).toBool())
        disableTaskManager(false);
}

// ---------- 状态持久化 ----------
void LockController::saveServiceState()
{
    QSettings settings;
    settings.setValue("ServiceRunning", m_checking);
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
        QSettings settings;
        QString bgImage = settings.value("LockBackground", "").toString();
        m_lockWindow->setProperty("backgroundImage", bgImage);

        if (isLocked && !m_wasLocked) {
            QSettings settings;
            if (settings.value("AutoTimeSync", false).toBool())
                syncSystemTime();
        }
        m_wasLocked = isLocked;

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
void LockController::syncSystemTime()
{
#ifdef Q_OS_WIN
    // 使用 Windows 时间服务强制同步
    QProcess::startDetached("w32tm", QStringList() << "/resync");
#else
    // 非 Windows 暂不实现
#endif
}