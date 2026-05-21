#include "TimeSyncManager.h"
#include "ConfigManager.h"

#include <QTimer>
#include <QUdpSocket>
#include <QHostInfo>
#include <QHostAddress>
#include <QDateTime>
#include <QDebug>
#include <windows.h>

namespace {
constexpr int kDefaultIntervalMinutes = 60;
constexpr int kMinIntervalMinutes = 1;
constexpr int kStageTimeoutMs = 5000;
const char *kDefaultServer = "time.windows.com";
constexpr quint64 kNtpUnixOffsetSeconds = 2208988800ULL;
}

TimeSyncManager::TimeSyncManager(QObject *parent) : QObject(parent) {
    m_timer = new QTimer(this);
    m_timer->setSingleShot(false);
    connect(m_timer, &QTimer::timeout, this, [this]() { syncNow(); });

    m_stageTimer = new QTimer(this);
    m_stageTimer->setSingleShot(true);
    connect(m_stageTimer, &QTimer::timeout, this, &TimeSyncManager::onStageTimeout);

    reloadFromConfig();
}

TimeSyncManager::~TimeSyncManager() {
    resetPipeline();
}

void TimeSyncManager::reloadFromConfig() {
    ConfigManager *cfg = ConfigManager::instance();
    if (!cfg) return;

    bool oldEnabled = m_enabled;
    QString oldServer = m_server;
    int oldInterval = m_intervalMinutes;

    m_enabled = cfg->readBool("AutoTimeSync", false);
    m_server = cfg->readString("TimeSyncServer", QString::fromLatin1(kDefaultServer));
    if (m_server.trimmed().isEmpty())  m_server = QString::fromLatin1(kDefaultServer);
    m_intervalMinutes = cfg->readInt("TimeSyncIntervalMinutes", kDefaultIntervalMinutes);
    if (m_intervalMinutes < kMinIntervalMinutes)  m_intervalMinutes = kMinIntervalMinutes;

    if (oldEnabled != m_enabled) emit enabledChanged(m_enabled);
    if (oldServer != m_server) emit serverChanged(m_server);
    if (oldInterval != m_intervalMinutes) emit intervalMinutesChanged(m_intervalMinutes);

    rescheduleTimer();

    if (m_enabled) QTimer::singleShot(0, this, [this]() { syncNow(); });
}

bool TimeSyncManager::isEnabled() const {
    return m_enabled;
}

void TimeSyncManager::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    if (ConfigManager::instance())
        ConfigManager::instance()->writeBool("AutoTimeSync", enabled);
    emit enabledChanged(enabled);
    rescheduleTimer();
    if (m_enabled)
        syncNow();
}

QString TimeSyncManager::server() const { return m_server; }

void TimeSyncManager::setServer(const QString &server)
{
    QString trimmed = server.trimmed();
    if (trimmed.isEmpty())
        trimmed = QString::fromLatin1(kDefaultServer);
    if (m_server == trimmed) return;
    m_server = trimmed;
    if (ConfigManager::instance())
        ConfigManager::instance()->writeString("TimeSyncServer", m_server);
    emit serverChanged(m_server);
}

int TimeSyncManager::intervalMinutes() const { return m_intervalMinutes; }

void TimeSyncManager::setIntervalMinutes(int minutes) {
    if (minutes < kMinIntervalMinutes)
        minutes = kMinIntervalMinutes;
    if (m_intervalMinutes == minutes) return;
    m_intervalMinutes = minutes;
    if (ConfigManager::instance())
        ConfigManager::instance()->writeInt("TimeSyncIntervalMinutes", minutes);
    emit intervalMinutesChanged(minutes);
    rescheduleTimer();
}

QString TimeSyncManager::lastSyncStatus() const { return m_lastStatus; }
QString TimeSyncManager::lastSyncTime() const { return m_lastTime; }
bool TimeSyncManager::lastSyncSuccess() const { return m_lastSuccess; }
bool TimeSyncManager::isSyncing() const { return m_syncing; }

void TimeSyncManager::rescheduleTimer() {
    if (!m_timer) return;
    if (!m_enabled) {
        m_timer->stop();
        return;
    }
    int intervalMs = m_intervalMinutes * 60 * 1000;
    if (intervalMs <= 0) intervalMs = kDefaultIntervalMinutes * 60 * 1000;
    m_timer->start(intervalMs);
}

void TimeSyncManager::recordResult(bool success, const QString &message) {
    m_lastSuccess = success;
    m_lastStatus = message;
    m_lastTime = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    emit lastSyncResult();
}

void TimeSyncManager::startStageTimeout(int ms) {
    m_stageTimer->start(ms);
}

void TimeSyncManager::resetPipeline() {
    m_stageTimer->stop();
    if (m_lookupId != -1) {
        QHostInfo::abortHostLookup(m_lookupId);
        m_lookupId = -1;
    }
    if (m_socket) {
        m_socket->disconnect(this);
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    m_targetAddress.clear();
}

void TimeSyncManager::finishWithError(const QString &message) {
    qWarning() << "Time sync failed:" << message;
    resetPipeline();
    if (m_syncing) {
        m_syncing = false;
        emit syncingChanged(false);
    }
    recordResult(false, message);
}

void TimeSyncManager::finishWithSuccess() {
    resetPipeline();
    if (m_syncing) {
        m_syncing = false;
        emit syncingChanged(false);
    }
    recordResult(true, tr("Time synchronized successfully"));
}

void TimeSyncManager::onStageTimeout() {
    finishWithError(tr("NTP request timed out"));
}

bool TimeSyncManager::syncNow() {
    if (m_syncing) return false;

    if (m_server.trimmed().isEmpty()) {
        recordResult(false, tr("Server address is empty"));
        return false;
    }

    m_syncing = true;
    emit syncingChanged(true);

    m_lookupId = QHostInfo::lookupHost(m_server, this, &TimeSyncManager::onHostLookupFinished);
    startStageTimeout(kStageTimeoutMs);
    return true;
}

void TimeSyncManager::onHostLookupFinished(const QHostInfo &hostInfo) {
    m_lookupId = -1;
    m_stageTimer->stop();

    if (!m_syncing) return;

    if (hostInfo.error() != QHostInfo::NoError || hostInfo.addresses().isEmpty()) {
        finishWithError(tr("Failed to resolve server: %1").arg(hostInfo.errorString()));
        return;
    }

    m_targetAddress = QHostAddress();
    for (const QHostAddress &addr : hostInfo.addresses()) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
            m_targetAddress = addr;
            break;
        }
    }
    if (m_targetAddress.isNull()) m_targetAddress = hostInfo.addresses().first();

    m_socket = new QUdpSocket(this);
    connect(m_socket, &QUdpSocket::readyRead, this, &TimeSyncManager::onSocketReadyRead);

    QByteArray request(48, 0);
    request[0] = 0x1B;
    qint64 sent = m_socket->writeDatagram(request, m_targetAddress, 123);
    if (sent != request.size()) {
        finishWithError(tr("Failed to send NTP request: %1").arg(m_socket->errorString()));
        return;
    }

    startStageTimeout(kStageTimeoutMs);
}

void TimeSyncManager::onSocketReadyRead() {
    if (!m_socket) return;
    m_stageTimer->stop();

    QByteArray response(48, 0);
    qint64 received = m_socket->readDatagram(response.data(), response.size());
    if (received < 48) {
        finishWithError(tr("Invalid NTP response"));
        return;
    }

    QString err;
    if (!applyNtpResponse(response, &err)) {
        finishWithError(err);
        return;
    }

    finishWithSuccess();
}

bool TimeSyncManager::applyNtpResponse(const QByteArray &response, QString *errorOut) {
    auto setError = [&](const QString &msg) {
        if (errorOut) *errorOut = msg;
    };

    auto readUint32BE = [](const QByteArray &buf, int offset) -> quint32 {
        return (static_cast<quint32>(static_cast<quint8>(buf[offset])) << 24)
             | (static_cast<quint32>(static_cast<quint8>(buf[offset + 1])) << 16)
             | (static_cast<quint32>(static_cast<quint8>(buf[offset + 2])) << 8)
             |  static_cast<quint32>(static_cast<quint8>(buf[offset + 3]));
    };

    // LI（闰秒指示）= 字节 0 的高 2 位；3 表示服务器自身未同步 / 报警，不可信。
    const quint8 li = (static_cast<quint8>(response[0]) >> 6) & 0x3;
    if (li == 3) {
        setError(tr("NTP server is unsynchronized (alarm condition)"));
        return false;
    }
    // Stratum = 字节 1；0 = Kiss-of-Death（KoD），拒绝接受并应回退。
    const quint8 stratum = static_cast<quint8>(response[1]);
    if (stratum == 0) {
        setError(tr("NTP server returned kiss-of-death (stratum 0)"));
        return false;
    }

    quint32 secsSince1900 = readUint32BE(response, 40);
    quint32 fracSeconds = readUint32BE(response, 44);

    if (secsSince1900 == 0) {
        setError(tr("NTP server returned invalid timestamp"));
        return false;
    }

    qint64 unixSecs = static_cast<qint64>(secsSince1900) - static_cast<qint64>(kNtpUnixOffsetSeconds);
    int milliseconds = static_cast<int>((static_cast<quint64>(fracSeconds) * 1000ULL) >> 32);

    QDateTime utcNow = QDateTime::fromMSecsSinceEpoch(unixSecs * 1000LL + milliseconds, Qt::UTC);
    if (!utcNow.isValid()) {
        setError(tr("Invalid timestamp from NTP server"));
        return false;
    }

    SYSTEMTIME st;
    QDate date = utcNow.date();
    QTime time = utcNow.time();
    st.wYear = static_cast<WORD>(date.year());
    st.wMonth = static_cast<WORD>(date.month());
    st.wDay = static_cast<WORD>(date.day());
    st.wDayOfWeek = static_cast<WORD>(date.dayOfWeek() % 7);
    st.wHour = static_cast<WORD>(time.hour());
    st.wMinute = static_cast<WORD>(time.minute());
    st.wSecond = static_cast<WORD>(time.second());
    st.wMilliseconds = static_cast<WORD>(time.msec());

    if (!::SetSystemTime(&st)) {
        DWORD err = ::GetLastError();
        setError(tr("Failed to set system time (error %1). Administrator privileges required.")
                     .arg(static_cast<int>(err)));
        return false;
    }
    return true;
}
