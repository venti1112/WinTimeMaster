#ifndef TIMESYNCMANAGER_H
#define TIMESYNCMANAGER_H

#include <QObject>
#include <QString>

class QTimer;

class TimeSyncManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString server READ server WRITE setServer NOTIFY serverChanged)
    Q_PROPERTY(int intervalMinutes READ intervalMinutes WRITE setIntervalMinutes NOTIFY intervalMinutesChanged)
    Q_PROPERTY(QString lastSyncStatus READ lastSyncStatus NOTIFY lastSyncResult)
    Q_PROPERTY(QString lastSyncTime READ lastSyncTime NOTIFY lastSyncResult)
    Q_PROPERTY(bool lastSyncSuccess READ lastSyncSuccess NOTIFY lastSyncResult)

public:
    explicit TimeSyncManager(QObject *parent = nullptr);
    ~TimeSyncManager() override;

    bool isEnabled() const;
    Q_INVOKABLE void setEnabled(bool enabled);

    QString server() const;
    Q_INVOKABLE void setServer(const QString &server);

    int intervalMinutes() const;
    Q_INVOKABLE void setIntervalMinutes(int minutes);

    QString lastSyncStatus() const;
    QString lastSyncTime() const;
    bool lastSyncSuccess() const;

    void reloadFromConfig();

public slots:
    bool syncNow();

signals:
    void enabledChanged(bool enabled);
    void serverChanged(const QString &server);
    void intervalMinutesChanged(int minutes);
    void lastSyncResult();

private:
    void rescheduleTimer();
    bool fetchAndApplyNtp(const QString &server, QString *errorOut);
    void recordResult(bool success, const QString &message);

    QTimer *m_timer = nullptr;
    bool m_enabled = false;
    QString m_server;
    int m_intervalMinutes = 60;

    QString m_lastStatus;
    QString m_lastTime;
    bool m_lastSuccess = false;
};

#endif // TIMESYNCMANAGER_H
