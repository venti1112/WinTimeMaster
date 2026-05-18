#include "LanguageManager.h"

#include <QCoreApplication>
#include <QLocale>
#include <QSettings>
#include <QDebug>

LanguageManager::LanguageManager(QQmlApplicationEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
    m_translator = new QTranslator(this);

    // 从设置或系统语言初始化
    QSettings settings;
    QString savedLang = settings.value("Language").toString();
    m_currentLanguage = savedLang.isEmpty() ? QLocale::system().name() : savedLang;

    loadTranslation(m_currentLanguage);
}

QString LanguageManager::currentLanguage() const
{
    return m_currentLanguage;
}

void LanguageManager::switchLanguage(const QString &languageCode)
{
    if (m_currentLanguage == languageCode)
        return;

    QSettings settings;
    settings.setValue("Language", languageCode);

    loadTranslation(languageCode);
    emit languageChanged();

    if (m_engine) {
        m_engine->retranslate(); // 强制刷新 QML 翻译
    }
}

void LanguageManager::loadTranslation(const QString &languageCode)
{
    // 移除旧的翻译器
    qApp->removeTranslator(m_translator);

    // 尝试加载完整的语言代码，例如 zh_CN
    QString qmFilePath = QString(":/i18n/%1.qm").arg(languageCode);
    bool loaded = m_translator->load(qmFilePath);

    if (!loaded) {
        // 回退到单纯的语言名，例如 zh
        QString langOnly = languageCode.section('_', 0, 0);
        qmFilePath = QString(":/i18n/%1.qm").arg(langOnly);
        loaded = m_translator->load(qmFilePath);
    }

    if (loaded) {
        qApp->installTranslator(m_translator);
        m_currentLanguage = languageCode;
        qDebug() << "Loaded translation:" << qmFilePath;
    } else {
        qWarning() << "Failed to load translation for:" << languageCode;
    }
}