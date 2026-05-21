#ifndef TIMERULE_H
#define TIMERULE_H

#include <QObject>
#include <QTime>
#include <QDate>
#include <QString>

class TimeRule : public QObject {
    Q_OBJECT

    Q_PROPERTY(int id READ id CONSTANT)
    Q_PROPERTY(QTime startTime READ startTime WRITE setStartTime NOTIFY startTimeChanged)
    Q_PROPERTY(QTime endTime READ endTime WRITE setEndTime NOTIFY endTimeChanged)
    Q_PROPERTY(RepeatMode repeatMode READ repeatMode WRITE setRepeatMode NOTIFY repeatModeChanged)
    Q_PROPERTY(int weekDays READ weekDays WRITE setWeekDays NOTIFY weekDaysChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QDate onceDate READ onceDate WRITE setOnceDate NOTIFY onceDateChanged)

public:
    enum RepeatMode {
        Once = 0,
        Daily,
        WeekDays
    };
    Q_ENUM(RepeatMode)

    explicit TimeRule(int id, QObject *parent = nullptr);

    int id() const;
    QTime startTime() const;
    QTime endTime() const;
    RepeatMode repeatMode() const;
    int weekDays() const;
    bool enabled() const;
    QDate onceDate() const;

    void setStartTime(const QTime &time);
    void setEndTime(const QTime &time);
    void setRepeatMode(RepeatMode mode);
    void setWeekDays(int days);
    void setEnabled(bool enabled);
    void setOnceDate(const QDate &date);

signals:
    void startTimeChanged();
    void endTimeChanged();
    void repeatModeChanged();
    void weekDaysChanged();
    void enabledChanged();
    void onceDateChanged();

private:
    int m_id;
    QTime m_startTime;
    QTime m_endTime;
    RepeatMode m_repeatMode = Daily;
    int m_weekDays = 0;
    bool m_enabled = true;
    QDate m_onceDate;
};

#endif // TIMERULE_H