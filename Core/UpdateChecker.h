#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class UpdateChecker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool checking READ isChecking NOTIFY checkingChanged)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY latestVersionChanged)
    Q_PROPERTY(QString downloadUrl READ downloadUrl NOTIFY downloadUrlChanged)
    Q_PROPERTY(QString updateMessage READ updateMessage NOTIFY updateMessageChanged)

public:
    explicit UpdateChecker(QObject *parent = nullptr);

    bool isChecking() const;
    QString latestVersion() const;
    QString downloadUrl() const;
    QString updateMessage() const;

    Q_INVOKABLE void checkForUpdates();
    Q_INVOKABLE void openDownloadUrl();

signals:
    void checkingChanged(bool checking);
    void latestVersionChanged(const QString &version);
    void downloadUrlChanged(const QString &url);
    void updateMessageChanged(const QString &message);
    void updateAvailable(const QString &version, const QString &url, const QString &message);
    void noUpdateAvailable();
    void checkFailed(const QString &error);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager m_networkManager;
    bool m_checking = false;
    QString m_latestVersion;
    QString m_downloadUrl;
    QString m_updateMessage;
};

#endif // UPDATECHECKER_H
