#include "ConfigManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QDir>
#include <QDebug>

ConfigManager* ConfigManager::s_instance = nullptr;

ConfigManager::ConfigManager(QObject *parent) : QObject(parent) {}

ConfigManager* ConfigManager::instance() { return s_instance; }

void ConfigManager::initialize(const QString &filePath)
{
    if (s_instance) return;
    s_instance = new ConfigManager();
    s_instance->m_filePath = filePath;
    s_instance->loadFromFile();
}

void ConfigManager::destroy()
{
    delete s_instance;
    s_instance = nullptr;
}

void ConfigManager::loadFromFile()
{
    QFile file(m_filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject()) m_data = doc.object();
        file.close();
    }
}

void ConfigManager::saveToFile()
{
    QDir dir = QFileInfo(m_filePath).absoluteDir();
    if (!dir.exists()) dir.mkpath(".");

    QFile file(m_filePath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(m_data);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    } else {
        qWarning() << "Cannot write config file:" << m_filePath;
    }
}

QString ConfigManager::readString(const QString &key, const QString &defaultValue) const
{
    return m_data.value(key).toString(defaultValue);
}

void ConfigManager::writeString(const QString &key, const QString &value)
{
    m_data.insert(key, value);
    saveToFile();
}

bool ConfigManager::readBool(const QString &key, bool defaultValue) const
{
    if (m_data.contains(key))
        return m_data.value(key).toBool(defaultValue);
    return defaultValue;
}

void ConfigManager::writeBool(const QString &key, bool value)
{
    m_data.insert(key, QJsonValue(value));
    saveToFile();
}

int ConfigManager::readInt(const QString &key, int defaultValue) const
{
    return m_data.value(key).toInt(defaultValue);
}

void ConfigManager::writeInt(const QString &key, int value)
{
    m_data.insert(key, value);
    saveToFile();
}

QJsonArray ConfigManager::readJsonArray(const QString &key) const
{
    QJsonValue val = m_data.value(key);
    if (val.isArray()) return val.toArray();
    return QJsonArray();
}

void ConfigManager::writeJsonArray(const QString &key, const QJsonArray &arr)
{
    m_data.insert(key, arr);
    saveToFile();
}

QVariant ConfigManager::value(const QString &key) const
{
    return m_data.value(key).toVariant();
}

void ConfigManager::setValue(const QString &key, const QVariant &value)
{
    m_data.insert(key, QJsonValue::fromVariant(value));
    saveToFile();
}

QStringList ConfigManager::allKeys() const { return m_data.keys(); }

QJsonObject ConfigManager::allData() const { return m_data; }

void ConfigManager::clearAll()
{
    m_data = QJsonObject();
    saveToFile();
}

QString ConfigManager::configFilePath() const
{
    return m_filePath;
}