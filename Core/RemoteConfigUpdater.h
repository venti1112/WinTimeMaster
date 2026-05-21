#ifndef REMOTECONFIGUPDATER_H
#define REMOTECONFIGUPDATER_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>

class QTimer;
class QNetworkReply;

class RemoteConfigUpdater : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString remoteUrl READ remoteUrl WRITE setRemoteUrl NOTIFY remoteUrlChanged)
    Q_PROPERTY(int intervalMinutes READ intervalMinutes WRITE setIntervalMinutes NOTIFY intervalMinutesChanged)
    Q_PROPERTY(QString lastSyncStatus READ lastSyncStatus NOTIFY lastSyncResult)
    Q_PROPERTY(QString lastSyncTime READ lastSyncTime NOTIFY lastSyncResult)
    Q_PROPERTY(bool lastSyncSuccess READ lastSyncSuccess NOTIFY lastSyncResult)

public:
    explicit RemoteConfigUpdater(QObject *parent = nullptr);
    ~RemoteConfigUpdater() override;

    bool isEnabled() const;
    Q_INVOKABLE void setEnabled(bool enabled);

    QString remoteUrl() const;
    Q_INVOKABLE void setRemoteUrl(const QString &url);

    int intervalMinutes() const;
    Q_INVOKABLE void setIntervalMinutes(int minutes);

    QString lastSyncStatus() const;
    QString lastSyncTime() const;
    bool lastSyncSuccess() const;

    Q_INVOKABLE void reloadFromConfig();

public slots:
    void syncNow();

signals:
    void enabledChanged(bool enabled);
    void remoteUrlChanged(const QString &url);
    void intervalMinutesChanged(int minutes);
    void lastSyncResult();
    void configUpdated();

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    void rescheduleTimer();
    void recordResult(bool success, const QString &message);

    QTimer *m_timer = nullptr;
    QNetworkAccessManager m_networkManager;
    bool m_enabled = false;
    QString m_remoteUrl;
    int m_intervalMinutes = 60;

    QString m_lastStatus;
    QString m_lastTime;
    bool m_lastSuccess = false;
};

#endif // REMOTECONFIGUPDATER_H
