#include "TimeRule.h"

TimeRule::TimeRule(int id, QObject *parent) : QObject(parent), m_id(id) {}

int TimeRule::id() const {
    return m_id;
}

QTime TimeRule::startTime() const {
    return m_startTime;
}

QTime TimeRule::endTime() const {
    return m_endTime;
}

TimeRule::RepeatMode TimeRule::repeatMode() const {
    return m_repeatMode;
}

int TimeRule::weekDays() const {
    return m_weekDays;
}

bool TimeRule::enabled() const {
    return m_enabled;
}

QDate TimeRule::onceDate() const {
    return m_onceDate;
}

void TimeRule::setStartTime(const QTime &time) {
    if (m_startTime != time) {
        m_startTime = time;
        emit startTimeChanged();
    }
}

void TimeRule::setEndTime(const QTime &time) {
    if (m_endTime != time) {
        m_endTime = time;
        emit endTimeChanged();
    }
}

void TimeRule::setRepeatMode(RepeatMode mode) {
    if (m_repeatMode != mode) {
        m_repeatMode = mode;
        if (m_enabled && mode == Once && m_onceDate.isValid()) {
            m_onceDate = QDate();
            emit onceDateChanged();
        }
        emit repeatModeChanged();
    }
}

void TimeRule::setWeekDays(int days) {
    if (m_weekDays != days) {
        m_weekDays = days;
        emit weekDaysChanged();
    }
}

void TimeRule::setEnabled(bool enabled) {
    if (m_enabled != enabled) {
        m_enabled = enabled;
        if (enabled && m_repeatMode == Once && m_onceDate.isValid()) {
            m_onceDate = QDate();
            emit onceDateChanged();
        }
        emit enabledChanged();
    }
}

void TimeRule::setOnceDate(const QDate &date) {
    if (m_onceDate != date) {
        m_onceDate = date;
        emit onceDateChanged();
    }
}