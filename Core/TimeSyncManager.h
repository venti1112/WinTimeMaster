#ifndef TIMESYNCMANAGER_H
#define TIMESYNCMANAGER_H

#include <QObject>
#include <QString>
#include <QHostAddress>

class QTimer;
class QUdpSocket;
class QHostInfo;

class TimeSyncManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString server READ server WRITE setServer NOTIFY serverChanged)
    Q_PROPERTY(int intervalMinutes READ intervalMinutes WRITE setIntervalMinutes NOTIFY intervalMinutesChanged)
    Q_PROPERTY(QString lastSyncStatus READ lastSyncStatus NOTIFY lastSyncResult)
    Q_PROPERTY(QString lastSyncTime READ lastSyncTime NOTIFY lastSyncResult)
    Q_PROPERTY(bool lastSyncSuccess READ lastSyncSuccess NOTIFY lastSyncResult)
    Q_PROPERTY(bool syncing READ isSyncing NOTIFY syncingChanged)

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
    bool isSyncing() const;

    void reloadFromConfig();

public slots:
    bool syncNow();

signals:
    void enabledChanged(bool enabled);
    void serverChanged(const QString &server);
    void intervalMinutesChanged(int minutes);
    void lastSyncResult();
    void syncingChanged(bool syncing);

private slots:
    void onHostLookupFinished(const QHostInfo &hostInfo);
    void onSocketReadyRead();
    void onStageTimeout();

private:
    void rescheduleTimer();
    void recordResult(bool success, const QString &message);
    void startStageTimeout(int ms);
    void finishWithError(const QString &message);
    void finishWithSuccess();
    void resetPipeline();
    bool applyNtpResponse(const QByteArray &response, QString *errorOut);

    QTimer *m_timer = nullptr;
    bool m_enabled = false;
    QString m_server;
    int m_intervalMinutes = 60;

    QString m_lastStatus;
    QString m_lastTime;
    bool m_lastSuccess = false;

    bool m_syncing = false;
    int m_lookupId = -1;
    QUdpSocket *m_socket = nullptr;
    QTimer *m_stageTimer = nullptr;
    QHostAddress m_targetAddress;
};

#endif // TIMESYNCMANAGER_H
