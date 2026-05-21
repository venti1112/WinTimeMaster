#include "UpdateChecker.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDesktopServices>
#include <QUrl>

static const QString kGitHubApiUrl = "https://api.github.com/repos/venti1112/WinTimeMaster/releases/latest";

UpdateChecker::UpdateChecker(QObject *parent) : QObject(parent) {
    connect(&m_networkManager, &QNetworkAccessManager::finished, this, &UpdateChecker::onReplyFinished);
}

bool UpdateChecker::isChecking() const {
    return m_checking;
}

QString UpdateChecker::latestVersion() const {
    return m_latestVersion;
}

QString UpdateChecker::downloadUrl() const {
    return m_downloadUrl;
}

QString UpdateChecker::updateMessage() const {
    return m_updateMessage;
}

void UpdateChecker::checkForUpdates() {
    if (m_checking) return;

    m_checking = true;
    emit checkingChanged(true);

    QNetworkRequest request((QUrl(kGitHubApiUrl)));
    request.setHeader(QNetworkRequest::UserAgentHeader, "WinTimeMaster-UpdateChecker");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    m_networkManager.get(request);
}

void UpdateChecker::openDownloadUrl() {
    if (!m_downloadUrl.isEmpty()) QDesktopServices::openUrl(QUrl(m_downloadUrl));
}

void UpdateChecker::onReplyFinished(QNetworkReply *reply) {
    m_checking = false;
    emit checkingChanged(false);

    if (reply->error() != QNetworkReply::NoError) {
        QString error = reply->errorString();
        emit checkFailed(error);
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        emit checkFailed(tr("Failed to parse update info"));
        return;
    }

    QJsonObject json = doc.object();
    QString tagName = json.value("tag_name").toString();
    QString htmlUrl = json.value("html_url").toString();
    QString body = json.value("body").toString();

    if (tagName.isEmpty()) {
        emit checkFailed(tr("Invalid update info"));
        return;
    }

    m_latestVersion = tagName;
    m_downloadUrl = htmlUrl;
    m_updateMessage = body;

    emit latestVersionChanged(m_latestVersion);
    emit downloadUrlChanged(m_downloadUrl);
    emit updateMessageChanged(m_updateMessage);

    QString currentVersion = QCoreApplication::applicationVersion();
    if (tagName != currentVersion) emit updateAvailable(tagName, htmlUrl, body);
    else emit noUpdateAvailable();
}
