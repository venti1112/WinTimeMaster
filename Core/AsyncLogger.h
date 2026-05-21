#ifndef ASYNCLOGGER_H
#define ASYNCLOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QQueue>
#include <QWaitCondition>
#include <QDateTime>
#include <QAtomicInt>
#include <QLoggingCategory>

class QThread;

class AsyncLogger {
public:
    static AsyncLogger &instance();

    bool init(const QString &logDir = QString(),
              qint64 maxFileSize = 10 * 1024 * 1024,
              int maxBackupFiles = 5);
    void shutdown();

    void setLogLevel(QtMsgType level);
    QtMsgType logLevel() const;
    void setConsoleOutput(bool enabled);

    AsyncLogger(const AsyncLogger &) = delete;
    AsyncLogger &operator=(const AsyncLogger &) = delete;

private:
    AsyncLogger();
    ~AsyncLogger();

    struct LogEntry {
        QtMsgType type;
        QString category;
        QString file;
        int line;
        QString function;
        QString message;
        QDateTime timestamp;
        Qt::HANDLE threadId;
    };

    static void messageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg);

    void enqueue(QtMsgType type, const QMessageLogContext &ctx, const QString &msg);
    void writeFatalSync(QtMsgType type, const QMessageLogContext &ctx, const QString &msg);
    void writerLoop();
    void writeEntry(const LogEntry &entry);
    QString formatEntry(const LogEntry &entry) const;
    static QString levelString(QtMsgType type);
    void openLogFile();
    void closeLogFile();
    void rotateLogs();
    void cleanOldBackups();
    static void showFatalDialog(const QString &msg);

    QString m_logDir;
    QString m_currentFile;
    QFile m_file;
    QTextStream m_stream;
    qint64 m_maxFileSize = 10 * 1024 * 1024;
    int m_maxBackupFiles = 5;
    QAtomicInt m_logLevel;
    QAtomicInt m_consoleOutput;

    QQueue<LogEntry> m_queue;
    QMutex m_mutex;
    QWaitCondition m_condition;
    QAtomicInt m_running;
    QThread *m_writerThread = nullptr;

    static QtMessageHandler s_previousHandler;
};

#endif // ASYNCLOGGER_H
