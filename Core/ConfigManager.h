#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>

class ConfigManager : public QObject
{
    Q_OBJECT
public:
    static ConfigManager* instance();
    static void initialize(const QString &filePath);
    static void destroy();

    void loadFromFile();
    void saveToFile();

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

private:
    explicit ConfigManager(QObject *parent = nullptr);
    static ConfigManager* s_instance;
    QString m_filePath;
    QJsonObject m_data;
};

#endif // CONFIGMANAGER_H