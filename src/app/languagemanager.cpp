#include "app/languagemanager.h"

#include <QCoreApplication>

LanguageManager::LanguageManager(QObject *parent) : QObject(parent) {}

QStringList LanguageManager::availableLanguages()
{
    return {QStringLiteral("en"), QStringLiteral("ru")};
}

bool LanguageManager::apply(const QString &language)
{
    if (!availableLanguages().contains(language))
        return false;

    QCoreApplication::removeTranslator(&m_translator);
    if (language == QLatin1String("en")) {
        // Source strings are English: no catalogue needed.
        m_current = language;
        return true;
    }

    if (!m_translator.load(QStringLiteral(":/i18n/notemanager_%1.qm").arg(language)))
        return false;
    QCoreApplication::installTranslator(&m_translator);
    m_current = language;
    return true;
}
