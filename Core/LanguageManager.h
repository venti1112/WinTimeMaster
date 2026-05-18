#ifndef LANGUAGEMANAGER_H
#define LANGUAGEMANAGER_H

#include <QObject>
#include <QTranslator>
#include <QQmlApplicationEngine>

class LanguageManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentLanguage READ currentLanguage NOTIFY languageChanged)

public:
    explicit LanguageManager(QQmlApplicationEngine *engine, QObject *parent = nullptr);
    ~LanguageManager() override = default;

    QString currentLanguage() const;

    Q_INVOKABLE void switchLanguage(const QString &languageCode);

signals:
    void languageChanged();

private:
    void loadTranslation(const QString &languageCode);

    QQmlApplicationEngine *m_engine = nullptr;
    QTranslator *m_translator = nullptr;
    QString m_currentLanguage;
};

#endif // LANGUAGEMANAGER_H