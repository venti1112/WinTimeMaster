#include "LanguageManager.h"

#include <QCoreApplication>
#include <QLocale>
#include <QSettings>
#include <QMap>
#include <QDebug>

LanguageManager::LanguageManager(QQmlApplicationEngine *engine, QObject *parent)
    : QObject(parent), m_engine(engine)
{
    m_translator = new QTranslator(this);

    QSettings settings;
    QString savedLang = settings.value("Language").toString();

    bool loaded = false;
    if (!savedLang.isEmpty()) {
        loaded = loadTranslation(savedLang);
    }

    if (!loaded) {
        // 如果保存的语言不可用（或不存在），回退到系统语言自动识别
        detectAndLoadLanguage();
    }

    // 极端情况保护：如果仍未加载任何语言，强制加载英语
    if (m_currentLanguage.isEmpty()) {
        loadTranslation("en_US");
    }
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

    if (m_engine)
        m_engine->retranslate();
}

bool LanguageManager::loadTranslation(const QString &languageCode)
{
    qApp->removeTranslator(m_translator);

    // 首先尝试完整代码（如 "zh_CN"）
    QString qmPath = QString(":/i18n/%1.qm").arg(languageCode);
    bool loaded = m_translator->load(qmPath);

    // 如果完整代码失败，尝试纯语言部分（如 "zh"）
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
    } else {
        qWarning() << "Failed to load translation for:" << languageCode;
    }
    return loaded;
}

QString LanguageManager::resolveLanguageCode(const QString &localeName) const
{
    // 第一步：如果精确匹配就直接返回（loadTranslation 会再次尝试，但这里可以快速返回）
    // 但我们仍要处理基础语言回退和中文特殊映射，所以交给 loadTranslation 不够，因为
    // 我们需要将 zh_HK 映射到 zh_TW 而不是尝试加载 zh_HK 或 zh，因为 zh.qm 不存在。
    // 因此我们在此处做智能映射。

    const QString lang = localeName.section('_', 0, 0);
    const QString region = localeName.section('_', 1, 1);

    // 中文特殊处理
    if (lang == "zh") {
        // 根据地区或脚本决定使用简体还是繁体
        // 支持常见地区：CN, SG, Hans -> 简体；TW, HK, MO, Hant -> 繁体
        if (region == "CN" || region == "SG" || region == "HANS") {
            return "zh_CN";
        }
        if (region == "TW" || region == "HK" || region == "MO" || region == "HANT") {
            return "zh_TW";
        }
        // 其他未知中文区域，默认使用简体中文
        return "zh_CN";
    }

    // 英语：统一使用 en_US
    if (lang == "en") {
        return "en_US";
    }

    // 韩语：统一使用 ko
    if (lang == "ko") {
        return "ko";
    }

    // 日语：统一使用 ja_JP
    if (lang == "ja") {
        return "ja_JP";
    }

    // 对于其他语言，如果我们的翻译文件只支持基础语言代码（如 ko），则直接返回基础代码。
    // 但我们没有 fr 等文件，所以会返回空，由外层回退到英语。
    // 若将来增加其他语言翻译，可在此处按需扩展。

    // 默认：返回 localeName 本身，让 loadTranslation 尝试加载它和基础语言代码。
    // 如果都不行，外层会回退英语。
    return localeName;
}

void LanguageManager::detectAndLoadLanguage()
{
    const QString systemLocale = QLocale::system().name();  // e.g. "zh_HK"
    QString bestLang = resolveLanguageCode(systemLocale);

    if (!bestLang.isEmpty()) {
        if (!loadTranslation(bestLang)) {
            // 如果映射后的语言仍然加载失败，尝试基础语言代码
            QString langOnly = bestLang.section('_', 0, 0);
            if (langOnly != bestLang && !loadTranslation(langOnly)) {
                // 全部失败，最终回退英语
                loadTranslation("en_US");
            }
        }
    } else {
        // 无法解析语言，直接回退英语
        loadTranslation("en_US");
    }
}