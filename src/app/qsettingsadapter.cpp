#include "app/qsettingsadapter.h"

#include <QLocale>

QSettingsAdapter::QSettingsAdapter() = default;

QSettingsAdapter::QSettingsAdapter(const QString &iniPath)
    : m_settings(iniPath, QSettings::IniFormat)
{
}

QString QSettingsAdapter::language() const
{
    const QString fallback = QLocale::system().language() == QLocale::Russian
                                 ? QStringLiteral("ru")
                                 : QStringLiteral("en");
    return m_settings.value(QStringLiteral("ui/language"), fallback).toString();
}

void QSettingsAdapter::setLanguage(const QString &language)
{
    m_settings.setValue(QStringLiteral("ui/language"), language);
}

int QSettingsAdapter::sortColumn() const
{
    return m_settings.value(QStringLiteral("ui/sortColumn"), 2).toInt();
}

bool QSettingsAdapter::sortAscending() const
{
    return m_settings.value(QStringLiteral("ui/sortAscending"), false).toBool();
}

void QSettingsAdapter::setSort(int column, bool ascending)
{
    m_settings.setValue(QStringLiteral("ui/sortColumn"), column);
    m_settings.setValue(QStringLiteral("ui/sortAscending"), ascending);
}

QString QSettingsAdapter::searchStrategy() const
{
    return m_settings.value(QStringLiteral("search/strategy"), QStringLiteral("substring"))
        .toString();
}

bool QSettingsAdapter::searchInContent() const
{
    return m_settings.value(QStringLiteral("search/inContent"), true).toBool();
}

void QSettingsAdapter::setSearchInContent(bool enabled)
{
    m_settings.setValue(QStringLiteral("search/inContent"), enabled);
}
