#include "LanguageManager.h"
#include "ConfigManager.h"

#include <QCoreApplication>
#include <QLocale>
#include <QMap>
#include <QDebug>

LanguageManager::LanguageManager(QQmlApplicationEngine *engine, QObject *parent) : QObject(parent), m_engine(engine) {
    m_translator = new QTranslator(this);

    QString savedLang = ConfigManager::instance()->readString("Language");

    bool loaded = false;
    if (!savedLang.isEmpty()) loaded = loadTranslation(savedLang);
    if (!loaded) detectAndLoadLanguage();
    if (m_currentLanguage.isEmpty()) loadTranslation("en_US");
}

QString LanguageManager::currentLanguage() const {
    return m_currentLanguage;
}

void LanguageManager::switchLanguage(const QString &languageCode) {
    if (m_currentLanguage == languageCode) return;

    ConfigManager::instance()->writeString("Language", languageCode);

    loadTranslation(languageCode);
    emit languageChanged();

    if (m_engine) m_engine->retranslate();
}

bool LanguageManager::loadTranslation(const QString &languageCode) {
    qApp->removeTranslator(m_translator);

    QString qmPath = QString(":/i18n/%1.qm").arg(languageCode);
    bool loaded = m_translator->load(qmPath);

    if (!loaded) {
        QString langOnly = languageCode.section('_', 0, 0);
        if (langOnly != languageCode) {
            qmPath = QString(":/i18n/%1.qm").arg(langOnly);
            loaded = m_translator->load(qmPath);
        }
    }

    if (loaded) {
        qApp->installTranslator(m_translator);
        m_currentLanguage = languageCode;
        qDebug() << "Loaded translation:" << qmPath;
    }
    else qWarning() << "Failed to load translation for:" << languageCode;
    return loaded;
}

QString LanguageManager::resolveLanguageCode(const QString &localeName) const {
    const QString lang = localeName.section('_', 0, 0);
    const QString region = localeName.section('_', 1, 1);

    if (lang == "zh") {
        if (region == "CN" || region == "SG" || region == "HANS") return "zh_CN";
        else if (region == "TW" || region == "HK" || region == "MO" || region == "HANT") return "zh_TW";
        else return "zh_CN";
    }
    else if (lang == "en") return "en_US";
    else if (lang == "ko") return "ko";
    else if (lang == "ja") return "ja_JP";
    else return "en_US";
}

void LanguageManager::detectAndLoadLanguage() {
    const QString systemLocale = QLocale::system().name();
    QString bestLang = resolveLanguageCode(systemLocale);

    if (!bestLang.isEmpty()) {
        if (!loadTranslation(bestLang)) {
            QString langOnly = bestLang.section('_', 0, 0);
            if (langOnly != bestLang && !loadTranslation(langOnly)) loadTranslation("en_US");
        }
    }
    else loadTranslation("en_US");
}

void LanguageManager::reloadLanguage() {
    QString lang = ConfigManager::instance()->readString("Language");
    if (lang.isEmpty()) lang = QLocale::system().name();
    switchLanguage(lang);
}