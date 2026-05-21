#include "TimeSyncManager.h"
#include "ConfigManager.h"

#include <QTimer>
#include <QUdpSocket>
#include <QHostInfo>
#include <QHostAddress>
#include <QDateTime>
#include <QEventLoop>
#include <QDebug>
#include <windows.h>

namespace {
constexpr int kDefaultIntervalMinutes = 60;
constexpr int kMinIntervalMinutes = 1;
const char *kDefaultServer = "time.windows.com";
constexpr quint64 kNtpUnixOffsetSeconds = 2208988800ULL;
}

TimeSyncManager::TimeSyncManager(QObject *parent) : QObject(parent) {
    m_timer = new QTimer(this);
    m_timer->setSingleShot(false);
    connect(m_timer, &QTimer::timeout, this, [this]() { syncNow(); });

    reloadFromConfig();
}

TimeSyncManager::~TimeSyncManager() = default;

void TimeSyncManager::reloadFromConfig() {
    ConfigManager *cfg = ConfigManager::instance();
    if (!cfg) return;

    m_enabled = cfg->readBool("AutoTimeSync", false);
    m_server = cfg->readString("TimeSyncServer", QString::fromLatin1(kDefaultServer));
    if (m_server.trimmed().isEmpty())  m_server = QString::fromLatin1(kDefaultServer);
    m_intervalMinutes = cfg->readInt("TimeSyncIntervalMinutes", kDefaultIntervalMinutes);
    if (m_intervalMinutes < kMinIntervalMinutes)  m_intervalMinutes = kMinIntervalMinutes;

    emit enabledChanged(m_enabled);
    emit serverChanged(m_server);
    emit intervalMinutesChanged(m_intervalMinutes);

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

bool TimeSyncManager::syncNow() {
    QString errorMsg;
    bool ok = fetchAndApplyNtp(m_server, &errorMsg);
    if (ok) recordResult(true, tr("Time synchronized successfully"));
    else {
        qWarning() << "Time sync failed:" << errorMsg;
        recordResult(false, errorMsg);
    }
    return ok;
}

bool TimeSyncManager::fetchAndApplyNtp(const QString &server, QString *errorOut) {
    auto setError = [&](const QString &msg) {
        if (errorOut) *errorOut = msg;
    };

    if (server.trimmed().isEmpty()) {
        setError(tr("Server address is empty"));
        return false;
    }

    QHostInfo hostInfo = QHostInfo::fromName(server);
    if (hostInfo.error() != QHostInfo::NoError || hostInfo.addresses().isEmpty()) {
        setError(tr("Failed to resolve server: %1").arg(hostInfo.errorString()));
        return false;
    }
    QHostAddress address;
    for (const QHostAddress &addr : hostInfo.addresses()) {
        if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
            address = addr;
            break;
        }
    }
    if (address.isNull()) address = hostInfo.addresses().first();

    QUdpSocket socket;
    QByteArray request(48, 0);
    request[0] = 0x1B;

    qint64 sent = socket.writeDatagram(request, address, 123);
    if (sent != request.size()) {
        setError(tr("Failed to send NTP request: %1").arg(socket.errorString()));
        return false;
    }

    if (!socket.waitForReadyRead(5000)) {
        setError(tr("NTP request timed out"));
        return false;
    }

    QByteArray response(48, 0);
    qint64 received = socket.readDatagram(response.data(), response.size());
    if (received < 48) {
        setError(tr("Invalid NTP response"));
        return false;
    }

    auto readUint32BE = [](const QByteArray &buf, int offset) -> quint32 {
        return (static_cast<quint32>(static_cast<quint8>(buf[offset])) << 24) | (static_cast<quint32>(static_cast<quint8>(buf[offset + 1])) << 16) | (static_cast<quint32>(static_cast<quint8>(buf[offset + 2])) << 8) | (static_cast<quint32>(static_cast<quint8>(buf[offset + 3])));
    };

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
    st.wDayOfWeek = static_cast<WORD>(date.dayOfWeek() % 7); // Win: Sun=0..Sat=6
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
