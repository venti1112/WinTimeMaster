#include "TimeRule.h"

TimeRule::TimeRule(int id, QObject *parent)
    : QObject(parent), m_id(id) {}

int TimeRule::id() const { return m_id; }
QTime TimeRule::startTime() const { return m_startTime; }
QTime TimeRule::endTime() const { return m_endTime; }
TimeRule::RepeatMode TimeRule::repeatMode() const { return m_repeatMode; }
int TimeRule::weekDays() const { return m_weekDays; }
bool TimeRule::enabled() const { return m_enabled; }

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
        emit enabledChanged();
    }
}