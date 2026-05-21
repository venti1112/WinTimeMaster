#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>

class QTimer;

class ConfigManager : public QObject {
    Q_OBJECT
public:
    static ConfigManager* instance();
    static void initialize(const QString &filePath);
    static void destroy();

    void loadFromFile();
    // 同步刷盘（取消任何 pending 的延迟保存）
    void saveToFile();
    // 与 saveToFile 等价的显式刷盘，命名更清晰；外部场景：导出前、进程退出前
    void flush();

    QString readString(const QString &key, const QString &defaultValue = {}) const;
    void writeString(const QString &key, const QString &value);

    bool readBool(const QString &key, bool defaultValue = false) const;
    void writeBool(const QString &key, bool value);

    int readInt(const QString &key, int defaultValue = 0) const;
    void writeInt(const QString &key, int value);

    QJsonArray readJsonArray(const QString &key) const;
    void writeJsonArray(const QString &key, const QJsonArray &arr);

    QVariant value(const QString &key) const;
    void setValue(const QString &key, const QVariant &value);

    QStringList allKeys() const;
    QJsonObject allData() const;
    void clearAll();

    QString configFilePath() const;

signals:
    // 任意键写入成功后触发；批量写入也会逐键发出，消费方可据此联动 UI 或缓存
    void valueChanged(const QString &key);

private:
    explicit ConfigManager(QObject *parent = nullptr);
    // 标脏并启动短延迟合并定时器；一次突发的多次写入仅产生一次磁盘 I/O
    void scheduleSave();

    static ConfigManager* s_instance;
    QString m_filePath;
    QJsonObject m_data;
    QTimer *m_saveTimer = nullptr;
    bool m_dirty = false;
};

#endif // CONFIGMANAGER_H