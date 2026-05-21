#include "LockController.h"
#include "TimeRuleModel.h"
#include "TimeRule.h"
#include "ConfigManager.h"
#include "PasswordHasher.h"
#include <QQmlComponent>
#include <QQuickWindow>
#include <QScreen>
#include <QGuiApplication>
#include <QProcess>
#include <QDebug>
#include <atomic>
#include <windows.h>
#include <winreg.h>

static std::atomic<LockController*> g_hookController{nullptr};

LockController::LockController(TimeRuleModel *model, QQmlApplicationEngine *engine, QObject *parent) : QObject(parent), m_model(model), m_engine(engine) {
    if (m_engine) {
        QQmlComponent component(m_engine, QUrl("qrc:/qt/qml/WinTimeMaster/UI/LockScreen.qml"));
        if (component.isError()) qWarning() << "Failed to load LockScreen.qml:" << component.errorString();
        else {
            QObject *obj = component.create();
            m_lockWindow = qobject_cast<QQuickWindow*>(obj);
            if (m_lockWindow) {
                m_lockWindow->setVisible(false);
                // 提前建立平台窗口，使 winId() 在首次安装钩子时即可返回有效 HWND
                m_lockWindow->create();
                m_lockHwnd = reinterpret_cast<HWND>(m_lockWindow->winId());
            } else {
                qWarning() << "LockScreen root is not a Window!";
                delete obj;
            }
        }
    }

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &LockController::checkLockRules);

    bool lastRunning = ConfigManager::instance()->readBool("ServiceRunning", false);
    if (lastRunning) startChecking();
}

LRESULT CALLBACK LockController::keyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    LockController *ctrl = g_hookController.load(std::memory_order_acquire);
    if (nCode == HC_ACTION && ctrl) {
        if (ctrl->m_emergencyPasswordActive.load(std::memory_order_acquire))
            return CallNextHookEx(nullptr, nCode, wParam, lParam);

        HWND fg = GetForegroundWindow();
        if (ctrl->m_lockHwnd && fg == ctrl->m_lockHwnd)
            return CallNextHookEx(nullptr, nCode, wParam, lParam);

        return 1;
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

LRESULT CALLBACK LockController::mouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    LockController *ctrl = g_hookController.load(std::memory_order_acquire);
    if (nCode == HC_ACTION && ctrl) {
        if (ctrl->m_emergencyPasswordActive.load(std::memory_order_acquire))
            return CallNextHookEx(nullptr, nCode, wParam, lParam);

        if (wParam == WM_MOUSEMOVE)
            return CallNextHookEx(nullptr, nCode, wParam, lParam);

        const MSLLHOOKSTRUCT *ms = reinterpret_cast<const MSLLHOOKSTRUCT*>(lParam);
        HWND target = WindowFromPoint(ms->pt);
        HWND top = target ? GetAncestor(target, GA_ROOT) : nullptr;
        if (top && ctrl->m_lockHwnd && top == ctrl->m_lockHwnd)
            return CallNextHookEx(nullptr, nCode, wParam, lParam);

        return 1;
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void LockController::beginEmergencyPassword() {
    m_emergencyPasswordActive.store(true, std::memory_order_release);
    blockInput(false);
}

void LockController::cancelEmergencyPassword() {
    m_emergencyPasswordActive.store(false, std::memory_order_release);
    if (m_checking && m_restrictionsActive) {
        if (ConfigManager::instance()->readBool("EnableInputBlock", true))
            blockInput(true);
    }
}

void LockController::blockInput(bool block) {
    if (block && m_emergencyPasswordActive.load(std::memory_order_acquire)) return;

    if (block) {
        g_hookController.store(this, std::memory_order_release);
        if (m_lockWindow && !m_lockHwnd)
            m_lockHwnd = reinterpret_cast<HWND>(m_lockWindow->winId());

        HMODULE hMod = GetModuleHandle(nullptr);
        if (!m_keyboardHook) {
            m_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardHookProc, hMod, 0);
            if (!m_keyboardHook) qWarning() << "Failed to install keyboard hook, error" << GetLastError();
        }
        if (!m_mouseHook) {
            m_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, mouseHookProc, hMod, 0);
            if (!m_mouseHook) qWarning() << "Failed to install mouse hook, error" << GetLastError();
        }
    } else {
        if (m_keyboardHook) {
            UnhookWindowsHookEx(m_keyboardHook);
            m_keyboardHook = nullptr;
        }
        if (m_mouseHook) {
            UnhookWindowsHookEx(m_mouseHook);
            m_mouseHook = nullptr;
        }
        g_hookController.store(nullptr, std::memory_order_release);
    }
}

LockController::~LockController() {
    blockInput(false);
}

bool LockController::emergencyExit(const QString &password) {
    QString storedHash = ConfigManager::instance()->readString("PasswordHash", QString());
    if (!storedHash.isEmpty()) {
        if (!PasswordHasher::verifyPassword(password, storedHash))
            return false;
    }

    m_timer->stop();
    m_checking = false;
    m_emergencyPasswordActive.store(false, std::memory_order_release);
    removeSystemRestrictions();
    if (m_lockWindow) m_lockWindow->hide();
    saveServiceState();
    emit checkingChanged(false);
    QTimer::singleShot(0, QCoreApplication::instance(), &QCoreApplication::quit);
    return true;
}

void LockController::disableTaskManager(bool disable) {
    HKEY hKey;
    LONG result = RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL);
    if (result == ERROR_SUCCESS) {
        DWORD value = disable ? 1 : 0;
        RegSetValueEx(hKey, L"DisableTaskMgr", 0, REG_DWORD, (BYTE*)&value, sizeof(value));
        RegCloseKey(hKey);
    }
}

void LockController::applySystemRestrictions() {
    ConfigManager *cfg = ConfigManager::instance();

    if (!m_restrictionsActive) {
        if (cfg->readBool("EnableInputBlock", true)) {
            blockInput(true);
            m_inputBlockInstalled = true;
        }
        if (cfg->readBool("DisableTaskManager", true)) {
            disableTaskManager(true);
            m_taskMgrDisabled = true;
        }
        m_restrictionsActive = true;
    }

    if (cfg->readBool("KillTaskmgr", true))
        QProcess::startDetached("taskkill", QStringList() << "/f" << "/im" << "taskmgr.exe");
}

void LockController::removeSystemRestrictions() {
    if (!m_restrictionsActive) return;
    // 以实际安装状态为准撤销 —— 在 apply 之后用户可能改了 config，但已经安装的钩子/注册表项仍需还原
    if (m_inputBlockInstalled) {
        blockInput(false);
        m_inputBlockInstalled = false;
    }
    if (m_taskMgrDisabled) {
        disableTaskManager(false);
        m_taskMgrDisabled = false;
    }
    m_restrictionsActive = false;
}

void LockController::saveServiceState() {
    ConfigManager::instance()->writeBool("ServiceRunning", m_checking);
}

bool LockController::isChecking() const {
    return m_checking;
}

void LockController::startChecking() {
    if (m_checking) return;
    m_checking = true;
    m_timer->start();
    checkLockRules();
    saveServiceState();
    qWarning("Service started");
    emit checkingChanged(true);
}

void LockController::stopChecking() {
    if (!m_checking) return;
    m_checking = false;
    m_timer->stop();
    removeSystemRestrictions();
    if (m_lockWindow) m_lockWindow->hide();
    saveServiceState();
    qWarning("Service stopped");
    emit checkingChanged(false);
}

void LockController::toggleChecking() {
    if (m_checking) stopChecking();
    else startChecking();
}

void LockController::checkLockRules() {
    if (!m_lockWindow || !m_model) return;

    QDateTime now = QDateTime::currentDateTime();
    QDate today = now.date();
    int dayOfWeek = today.dayOfWeek();
    QTime currentTime = now.time();

    bool isLocked = false;
    QDateTime latestUnlock;

    const QList<TimeRule*> rules = m_model->rules();
    for (TimeRule *rule : rules) {
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
            case TimeRule::Once: {
                QDate onceDate = rule->onceDate();

                // 首次评估：根据当前时间锚定具体日期
                if (!onceDate.isValid()) {
                    // 跨午夜规则的"昨天"开始的窗口可能仍在进行中，优先归属昨天
                    if (endTime <= startTime) {
                        QDateTime yEnd(today, endTime);
                        if (now < yEnd) onceDate = today.addDays(-1);
                    }
                    if (!onceDate.isValid()) {
                        auto [tStart, tEnd] = makeInterval(today);
                        onceDate = (now < tEnd) ? today : today.addDays(1);
                    }
                    rule->setOnceDate(onceDate);
                }

                auto [s, e] = makeInterval(onceDate);
                if (now >= s && now < e) {
                    active = true;
                    interval = {s, e};
                } else if (now >= e) {
                    // 窗口已过，自动禁用并清掉过时锚点
                    rule->setOnceDate(QDate());
                    rule->setEnabled(false);
                }
                break;
            }
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
            if (!latestUnlock.isValid() || interval.second > latestUnlock) latestUnlock = interval.second;
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
        QString remainingStr = QString("%1:%2:%3").arg(h, 2, 10, QChar('0')).arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0'));

        m_lockWindow->setProperty("currentTime", currentTimeStr);
        m_lockWindow->setProperty("unlockTime", unlockTimeStr);
        m_lockWindow->setProperty("remainingTime", remainingStr);

        ConfigManager *cfgInst = ConfigManager::instance();
        QString bgImage = cfgInst->readString("LockBackground", "");
        m_lockWindow->setProperty("backgroundImage", bgImage);

        m_lockWindow->setProperty("promptColor", cfgInst->readString("LockPromptColor", "#ffffffff"));
        m_lockWindow->setProperty("currentTimeColor", cfgInst->readString("LockCurrentTimeColor", "#ffd3d3d3"));
        m_lockWindow->setProperty("unlockTimeColor", cfgInst->readString("LockUnlockTimeColor", "#ffd3d3d3"));
        m_lockWindow->setProperty("remainingTimeColor", cfgInst->readString("LockRemainingTimeColor", "#ffff6347"));
        m_lockWindow->setProperty("textPosition", cfgInst->readString("LockTextPosition", "center"));
        m_lockWindow->setProperty("promptText", cfgInst->readString("LockPromptText", QString()));
        m_lockWindow->setProperty("hideCurrentTime", cfgInst->readBool("HideCurrentTime", false));
        m_lockWindow->setProperty("hideUnlockTime", cfgInst->readBool("HideUnlockTime", false));
        m_lockWindow->setProperty("hideRemainingTime", cfgInst->readBool("HideRemainingTime", false));
        m_lockWindow->setProperty("emergencyExitEnabled", cfgInst->readBool("EmergencyExitEnabled", false));
        m_lockWindow->setProperty("emergencyExitClickCount",  cfgInst->readInt("EmergencyExitClickCount", 3));
        m_lockWindow->setProperty("backgroundVideoSound", cfgInst->readBool("LockBackgroundVideoSound", false));

        applySystemRestrictions();

        if (!m_lockWindow->isVisible()) {
            QRect totalRect;
            const auto screens = QGuiApplication::screens();
            for (QScreen *screen : screens) totalRect = totalRect.united(screen->geometry());
            m_lockWindow->setGeometry(totalRect);
            m_lockWindow->show();
            m_lockWindow->raise();
            m_lockWindow->requestActivate();
        }
    }
    else if (m_lockWindow->isVisible()) {
        removeSystemRestrictions();
        m_lockWindow->hide();
    }
}
