#include "LockController.h"
#include "TimeRuleModel.h"
#include "TimeRule.h"
#include <QQmlComponent>
#include <QQuickWindow>
#include <QDebug>

LockController::LockController(TimeRuleModel *model, QQmlApplicationEngine *engine,
                               QObject *parent)
    : QObject(parent), m_model(model), m_engine(engine)
{
    // 创建锁屏窗口（独立 QML Window）
    if (m_engine) {
        QQmlComponent component(m_engine,
                                QUrl("qrc:/qt/qml/WinTimeMaster/UI/LockScreen.qml"));
        if (component.isError()) {
            qWarning() << "Failed to load LockScreen.qml:" << component.errorString();
        } else {
            QObject *obj = component.create();
            m_lockWindow = qobject_cast<QQuickWindow*>(obj);
            if (m_lockWindow) {
                // 确保初始状态为隐藏
                m_lockWindow->setVisibility(QWindow::Hidden);
            } else {
                qWarning() << "LockScreen root object is not a Window!";
                delete obj;
            }
        }
    }

    // 创建定时器，每秒检查一次
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &LockController::checkLockRules);
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
    checkLockRules();          // 立即检查一次
    emit checkingChanged(true);
}

void LockController::stopChecking()
{
    if (!m_checking) return;
    m_checking = false;
    m_timer->stop();
    if (m_lockWindow) {
        m_lockWindow->setVisibility(QWindow::Hidden);
    }
    emit checkingChanged(false);
}

void LockController::toggleChecking()
{
    if (m_checking)
        stopChecking();
    else
        startChecking();
}

// ---------- 核心检查逻辑 ----------
void LockController::checkLockRules()
{
    if (!m_lockWindow || !m_model) return;

    QDateTime now = QDateTime::currentDateTime();
    QDate today = now.date();
    int dayOfWeek = today.dayOfWeek();   // 1=Monday, 7=Sunday
    QTime currentTime = now.time();

    bool isLocked = false;
    QDateTime latestUnlock;   // 最晚的解锁时间

    const QList<TimeRule*> rules = m_model->rules();
    for (const TimeRule *rule : rules) {
        if (!rule->enabled()) continue;

        QTime startTime = rule->startTime();
        QTime endTime = rule->endTime();
        if (!startTime.isValid() || !endTime.isValid()) continue;

        QPair<QDateTime, QDateTime> interval;
        bool active = false;

        // 根据基准日期生成锁定区间 [start, end)
        auto makeInterval = [&](const QDate &baseDate) -> QPair<QDateTime, QDateTime> {
            QDateTime start(baseDate, startTime);
            QDateTime end;
            if (endTime <= startTime) {
                // 跨天：结束于次日
                end = QDateTime(baseDate.addDays(1), endTime);
            } else {
                end = QDateTime(baseDate, endTime);
            }
            return {start, end};
        };

        switch (rule->repeatMode()) {
        case TimeRule::Once:
        case TimeRule::Daily: {
            // 对于 Once，暂按 Daily 处理（未实现一次性失效逻辑）
            auto [s, e] = makeInterval(today);
            if (now >= s && now < e) {
                active = true;
                interval = {s, e};
            }
            break;
        }
        case TimeRule::WeekDays: {
            int weekMask = rule->weekDays();
            // 先检查昨天（用于处理跨天到今天的规则）
            QDate yesterday = today.addDays(-1);
            if ((1 << (yesterday.dayOfWeek() - 1)) & weekMask) {
                auto [s, e] = makeInterval(yesterday);
                if (now >= s && now < e) {
                    active = true;
                    interval = {s, e};
                }
            }
            // 如果昨天区间未命中，再检查今天
            if (!active && ((1 << (dayOfWeek - 1)) & weekMask)) {
                auto [s, e] = makeInterval(today);
                if (now >= s && now < e) {
                    active = true;
                    interval = {s, e};
                }
            }
            break;
        }
        default:
            break;
        }

        if (active) {
            isLocked = true;
            // 如果尚未记录或此规则的结束时间更晚，则更新
            if (!latestUnlock.isValid() || interval.second > latestUnlock)
                latestUnlock = interval.second;
        }
    }

    // 更新锁屏窗口
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

        // 全屏显示锁屏
        m_lockWindow->setVisibility(QWindow::FullScreen);
        m_lockWindow->raise();
        m_lockWindow->requestActivate();
    } else {
        // 没有活跃规则，隐藏锁屏
        m_lockWindow->setVisibility(QWindow::Hidden);
    }
}