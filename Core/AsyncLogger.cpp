#include "AsyncLogger.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QMetaObject>
#include <QStandardPaths>
#include <QThread>
#include <cstdio>
#include <cstdlib>
#include <windows.h>

QtMessageHandler AsyncLogger::s_previousHandler = nullptr;

AsyncLogger &AsyncLogger::instance() {
    static AsyncLogger inst;
    return inst;
}

AsyncLogger::AsyncLogger() {
    m_logLevel.storeRelease(static_cast<int>(QtDebugMsg));
    m_consoleOutput.storeRelease(0);
    m_running.storeRelease(0);
}

AsyncLogger::~AsyncLogger() {
    shutdown();
}

bool AsyncLogger::init(const QString &logDir, qint64 maxFileSize, int maxBackupFiles) {
    if (m_running.loadAcquire()) {
        return true;
    }

    m_logDir = logDir.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/logs")
        : logDir;
    m_maxFileSize = maxFileSize;
    m_maxBackupFiles = maxBackupFiles;

    if (!QDir().mkpath(m_logDir)) {
        return false;
    }

    openLogFile();
    if (!m_file.isOpen()) {
        return false;
    }

    m_running.storeRelease(1);

    m_writerThread = QThread::create([this]() { writerLoop(); });
    m_writerThread->setObjectName(QStringLiteral("AsyncLoggerWriter"));
    m_writerThread->start();

    s_previousHandler = qInstallMessageHandler(&AsyncLogger::messageHandler);

    return true;
}

void AsyncLogger::shutdown() {
    if (!m_running.loadAcquire()) {
        return;
    }

    qInstallMessageHandler(s_previousHandler);
    s_previousHandler = nullptr;

    {
        QMutexLocker locker(&m_mutex);
        m_running.storeRelease(0);
        m_condition.wakeAll();
    }

    if (m_writerThread) {
        m_writerThread->wait();
        delete m_writerThread;
        m_writerThread = nullptr;
    }

    closeLogFile();
}

void AsyncLogger::setLogLevel(QtMsgType level) {
    m_logLevel.storeRelease(static_cast<int>(level));
}

QtMsgType AsyncLogger::logLevel() const {
    return static_cast<QtMsgType>(m_logLevel.loadAcquire());
}

void AsyncLogger::setConsoleOutput(bool enabled) {
    m_consoleOutput.storeRelease(enabled ? 1 : 0);
}

void AsyncLogger::messageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg) {
    instance().enqueue(type, ctx, msg);

    if (type == QtFatalMsg) {
        // 不要在这里调用 shutdown()：若 fatal 来自 writer 线程，shutdown 会等待自己；
        // 若 fatal 来自非主线程，BlockingQueuedConnection 也会死锁主线程。
        // 改为直接同步写入 fatal 行并刷盘，对话框走非阻塞路径。
        instance().writeFatalSync(type, ctx, msg);
        showFatalDialog(msg);
        std::abort();
    }
}

void AsyncLogger::writeFatalSync(QtMsgType type, const QMessageLogContext &ctx, const QString &msg) {
    LogEntry entry;
    entry.type = type;
    entry.category = ctx.category ? QString::fromUtf8(ctx.category) : QString();
    entry.file = ctx.file ? QString::fromUtf8(ctx.file) : QString();
    entry.line = ctx.line;
    entry.function = ctx.function ? QString::fromUtf8(ctx.function) : QString();
    entry.message = msg;
    entry.timestamp = QDateTime::currentDateTime();
    entry.threadId = QThread::currentThreadId();

    QMutexLocker locker(&m_mutex);
    if (m_file.isOpen()) {
        m_stream << formatEntry(entry) << '\n';
        m_stream.flush();
    }
    if (m_consoleOutput.loadAcquire()) {
        const QByteArray bytes = formatEntry(entry).toLocal8Bit();
        std::fwrite(bytes.constData(), 1, bytes.size(), stderr);
        std::fputc('\n', stderr);
        std::fflush(stderr);
    }
}

void AsyncLogger::showFatalDialog(const QString &msg) {
    QCoreApplication *app = QCoreApplication::instance();

    const QString title = QCoreApplication::translate("AsyncLogger", "Fatal Error");
    const QString body = QCoreApplication::translate(
        "AsyncLogger",
        "A fatal error occurred and the application will exit:\n\n%1").arg(msg);

    if (app && QThread::currentThread() == app->thread()) {
        // 在主线程：可以直接弹 QMessageBox。
        QMessageBox::critical(nullptr, title, body);
        return;
    }
    // 非主线程：避免 BlockingQueuedConnection（主线程可能也被阻塞，会死锁）。
    // 改用 Win32 原生 MessageBoxW，它不依赖 Qt 事件循环，可在任意线程安全调用。
    MessageBoxW(nullptr,
                reinterpret_cast<LPCWSTR>(body.utf16()),
                reinterpret_cast<LPCWSTR>(title.utf16()),
                MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
}

void AsyncLogger::enqueue(QtMsgType type, const QMessageLogContext &ctx, const QString &msg) {
    if (!m_running.loadAcquire()) {
        return;
    }
    if (static_cast<int>(type) < m_logLevel.loadAcquire()) {
        return;
    }

    LogEntry entry;
    entry.type = type;
    entry.category = ctx.category ? QString::fromUtf8(ctx.category) : QString();
    entry.file = ctx.file ? QString::fromUtf8(ctx.file) : QString();
    entry.line = ctx.line;
    entry.function = ctx.function ? QString::fromUtf8(ctx.function) : QString();
    entry.message = msg;
    entry.timestamp = QDateTime::currentDateTime();
    entry.threadId = QThread::currentThreadId();

    QMutexLocker locker(&m_mutex);
    m_queue.enqueue(std::move(entry));
    m_condition.wakeOne();
}

void AsyncLogger::writerLoop() {
    while (true) {
        QQueue<LogEntry> batch;
        {
            QMutexLocker locker(&m_mutex);
            while (m_queue.isEmpty() && m_running.loadAcquire()) {
                m_condition.wait(&m_mutex);
            }
            batch.swap(m_queue);
            if (batch.isEmpty() && !m_running.loadAcquire()) {
                break;
            }
        }

        while (!batch.isEmpty()) {
            writeEntry(batch.dequeue());
        }

        if (m_file.isOpen()) {
            m_stream.flush();
            if (m_file.size() >= m_maxFileSize) {
                rotateLogs();
            }
        }
    }
}

void AsyncLogger::writeEntry(const LogEntry &entry) {
    const QString line = formatEntry(entry);

    if (m_file.isOpen()) {
        m_stream << line << '\n';
    }

    if (m_consoleOutput.loadAcquire()) {
        FILE *out = (entry.type >= QtWarningMsg) ? stderr : stdout;
        const QByteArray bytes = line.toLocal8Bit();
        std::fwrite(bytes.constData(), 1, bytes.size(), out);
        std::fputc('\n', out);
    }
}

QString AsyncLogger::formatEntry(const LogEntry &entry) const {
    QString category;
    if (!entry.category.isEmpty() && entry.category != QLatin1String("default")) {
        category = QStringLiteral("[%1] ").arg(entry.category);
    }

    return QStringLiteral("[%1] [%2] [TID:0x%3] %4%5")
        .arg(entry.timestamp.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")),
             levelString(entry.type))
        .arg(reinterpret_cast<quintptr>(entry.threadId), 0, 16)
        .arg(category, entry.message);
}

QString AsyncLogger::levelString(QtMsgType type) {
    switch (type) {
        case QtDebugMsg:    return QStringLiteral("DEBUG");
        case QtInfoMsg:     return QStringLiteral("INFO");
        case QtWarningMsg:  return QStringLiteral("WARN");
        case QtCriticalMsg: return QStringLiteral("ERROR");
        case QtFatalMsg:    return QStringLiteral("FATAL");
    }
    return QStringLiteral("?????");
}

void AsyncLogger::openLogFile() {
    const QString date = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd"));
    m_currentFile = QStringLiteral("%1/app_%2.log").arg(m_logDir, date);

    m_file.setFileName(m_currentFile);
    if (m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_stream.setDevice(&m_file);
        m_stream.setEncoding(QStringConverter::Utf8);
    }
}

void AsyncLogger::closeLogFile() {
    if (m_file.isOpen()) {
        m_stream.flush();
        m_stream.setDevice(nullptr);
        m_file.close();
    }
}

void AsyncLogger::rotateLogs() {
    closeLogFile();

    QFileInfo info(m_currentFile);
    const QString base = info.completeBaseName();
    const QString dir = info.absolutePath();

    int idx = 1;
    QString rotated;
    do {
        rotated = QStringLiteral("%1/%2.%3.log").arg(dir, base).arg(idx++);
    } while (QFile::exists(rotated));

    QFile::rename(m_currentFile, rotated);

    cleanOldBackups();
    openLogFile();
}

void AsyncLogger::cleanOldBackups() {
    QDir dir(m_logDir);
    QStringList logs = dir.entryList({QStringLiteral("*.log")}, QDir::Files, QDir::Time);
    while (logs.size() > m_maxBackupFiles) {
        QFile::remove(dir.filePath(logs.takeLast()));
    }
}
