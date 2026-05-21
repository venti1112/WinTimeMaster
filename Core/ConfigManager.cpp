#include "ConfigManager.h"
#include <QCoreApplication>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QDir>
#include <QTimer>
#include <QDebug>

ConfigManager* ConfigManager::s_instance = nullptr;

ConfigManager::ConfigManager(QObject *parent) : QObject(parent) {
    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(200);
    connect(m_saveTimer, &QTimer::timeout, this, &ConfigManager::saveToFile);
}

ConfigManager* ConfigManager::instance() {
    return s_instance;
}

void ConfigManager::initialize(const QString &filePath) {
    if (s_instance) return;
    s_instance = new ConfigManager();
    s_instance->m_filePath = filePath;
    s_instance->loadFromFile();

    // 进程退出前保证任何 pending 的写入落盘
    if (auto *app = QCoreApplication::instance()) {
        QObject::connect(app, &QCoreApplication::aboutToQuit, s_instance, []() {
            if (s_instance) s_instance->flush();
        });
    }
}

void ConfigManager::destroy() {
    if (s_instance) s_instance->flush();
    delete s_instance;
    s_instance = nullptr;
}

void ConfigManager::loadFromFile() {
    // 取消任何待落盘的旧数据，避免 import 之后被它覆盖
    if (m_saveTimer) m_saveTimer->stop();
    m_dirty = false;

    QFile file(m_filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject()) m_data = doc.object();
        file.close();
    }
}

void ConfigManager::scheduleSave() {
    m_dirty = true;
    if (m_saveTimer) m_saveTimer->start();
}

void ConfigManager::flush() {
    if (m_saveTimer) m_saveTimer->stop();
    if (m_dirty) saveToFile();
}

void ConfigManager::saveToFile() {
    if (m_saveTimer) m_saveTimer->stop();
    m_dirty = false;

    QDir dir = QFileInfo(m_filePath).absoluteDir();
    if (!dir.exists()) dir.mkpath(".");

    QSaveFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot open config file for write:" << m_filePath << file.errorString();
        return;
    }
    QJsonDocument doc(m_data);
    if (file.write(doc.toJson(QJsonDocument::Indented)) == -1) {
        qWarning() << "Failed to write config:" << file.errorString();
        file.cancelWriting();
        return;
    }
    if (!file.commit())
        qWarning() << "Failed to commit config file:" << m_filePath << file.errorString();
}

QString ConfigManager::readString(const QString &key, const QString &defaultValue) const {
    return m_data.value(key).toString(defaultValue);
}

void ConfigManager::writeString(const QString &key, const QString &value) {
    m_data.insert(key, value);
    scheduleSave();
    emit valueChanged(key);
}

bool ConfigManager::readBool(const QString &key, bool defaultValue) const {
    if (m_data.contains(key)) return m_data.value(key).toBool(defaultValue);
    return defaultValue;
}

void ConfigManager::writeBool(const QString &key, bool value) {
    m_data.insert(key, QJsonValue(value));
    scheduleSave();
    emit valueChanged(key);
}

int ConfigManager::readInt(const QString &key, int defaultValue) const {
    return m_data.value(key).toInt(defaultValue);
}

void ConfigManager::writeInt(const QString &key, int value) {
    m_data.insert(key, value);
    scheduleSave();
    emit valueChanged(key);
}

QJsonArray ConfigManager::readJsonArray(const QString &key) const {
    QJsonValue val = m_data.value(key);
    if (val.isArray()) return val.toArray();
    return QJsonArray();
}

void ConfigManager::writeJsonArray(const QString &key, const QJsonArray &arr) {
    m_data.insert(key, arr);
    scheduleSave();
    emit valueChanged(key);
}

QVariant ConfigManager::value(const QString &key) const {
    return m_data.value(key).toVariant();
}

void ConfigManager::setValue(const QString &key, const QVariant &value) {
    m_data.insert(key, QJsonValue::fromVariant(value));
    scheduleSave();
    emit valueChanged(key);
}

QStringList ConfigManager::allKeys() const {
    return m_data.keys();
}

QJsonObject ConfigManager::allData() const {
    return m_data;
}

void ConfigManager::clearAll() {
    m_data = QJsonObject();
    scheduleSave();
    emit valueChanged(QString());
}

QString ConfigManager::configFilePath() const {
    return m_filePath;
}