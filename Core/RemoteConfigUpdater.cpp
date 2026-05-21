#include "RemoteConfigUpdater.h"
#include "ConfigManager.h"
#include <QTimer>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

RemoteConfigUpdater::RemoteConfigUpdater(QObject *parent) : QObject(parent){
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &RemoteConfigUpdater::syncNow);
    connect(&m_networkManager, &QNetworkAccessManager::finished, this, &RemoteConfigUpdater::onReplyFinished);
}

RemoteConfigUpdater::~RemoteConfigUpdater() {
    m_timer->stop();
}

bool RemoteConfigUpdater::isEnabled() const {
    return m_enabled;
}

void RemoteConfigUpdater::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    emit enabledChanged(enabled);
    rescheduleTimer();
}

QString RemoteConfigUpdater::remoteUrl() const {
    return m_remoteUrl;
}

void RemoteConfigUpdater::setRemoteUrl(const QString &url) {
    if (m_remoteUrl == url) return;
    m_remoteUrl = url;
    emit remoteUrlChanged(url);
    ConfigManager::instance()->writeString("RemoteConfigUrl", url);
}

int RemoteConfigUpdater::intervalMinutes() const {
    return m_intervalMinutes;
}

void RemoteConfigUpdater::setIntervalMinutes(int minutes) {
    if (m_intervalMinutes == minutes) return;
    m_intervalMinutes = minutes;
    emit intervalMinutesChanged(minutes);
    ConfigManager::instance()->writeInt("RemoteConfigIntervalMinutes", minutes);
    rescheduleTimer();
}

QString RemoteConfigUpdater::lastSyncStatus() const {
    return m_lastStatus;
}

QString RemoteConfigUpdater::lastSyncTime() const {
    return m_lastTime;
}

bool RemoteConfigUpdater::lastSyncSuccess() const {
    return m_lastSuccess;
}

void RemoteConfigUpdater::reloadFromConfig() {
    m_enabled = ConfigManager::instance()->readBool("RemoteConfigEnabled", false);
    m_remoteUrl = ConfigManager::instance()->readString("RemoteConfigUrl", QString());
    m_intervalMinutes = ConfigManager::instance()->readInt("RemoteConfigIntervalMinutes", 60);
    emit enabledChanged(m_enabled);
    emit remoteUrlChanged(m_remoteUrl);
    emit intervalMinutesChanged(m_intervalMinutes);
    rescheduleTimer();
}

void RemoteConfigUpdater::syncNow() {
    if (m_remoteUrl.isEmpty()) {
        recordResult(false, tr("Remote URL is empty"));
        return;
    }

    QNetworkRequest request((QUrl(m_remoteUrl)));
    request.setHeader(QNetworkRequest::UserAgentHeader, "WinTimeMaster-RemoteConfig");
    m_networkManager.get(request);
}

void RemoteConfigUpdater::onReplyFinished(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        recordResult(false, reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        recordResult(false, tr("Invalid JSON: %1").arg(parseError.errorString()));
        return;
    }

    if (!doc.isObject()) {
        recordResult(false, tr("JSON root is not an object"));
        return;
    }

    ConfigManager *cfg = ConfigManager::instance();
    QJsonObject json = doc.object();

    for (auto it = json.begin(); it != json.end(); ++it) {
        const QString &key = it.key();
        const QJsonValue &value = it.value();

        if (value.isBool()) cfg->writeBool(key, value.toBool());
        else if (value.isDouble()) cfg->writeInt(key, value.toInt());
        else if (value.isString()) cfg->writeString(key, value.toString());
        else if (value.isArray()) cfg->writeJsonArray(key, value.toArray());
    }

    cfg->saveToFile();
    recordResult(true, tr("Config updated successfully"));
    emit configUpdated();
}

void RemoteConfigUpdater::rescheduleTimer() {
    m_timer->stop();
    if (m_enabled && m_intervalMinutes > 0 && !m_remoteUrl.isEmpty()) m_timer->start(m_intervalMinutes * 60 * 1000);
}

void RemoteConfigUpdater::recordResult(bool success, const QString &message) {
    m_lastSuccess = success;
    m_lastStatus = message;
    m_lastTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    emit lastSyncResult();
}
