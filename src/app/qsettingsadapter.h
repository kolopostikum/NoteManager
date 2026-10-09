#pragma once

#include <QSettings>

#include "domain/isettings.h"

/// ISettings backed by QSettings (INI file in the user's config directory).
class QSettingsAdapter : public ISettings {
public:
    QSettingsAdapter();
    /// For tests: use an explicit settings file.
    explicit QSettingsAdapter(const QString &iniPath);

    QString language() const override;
    void setLanguage(const QString &language) override;

    int sortColumn() const override;
    bool sortAscending() const override;
    void setSort(int column, bool ascending) override;

    QString searchStrategy() const override;
    bool searchInContent() const override;
    void setSearchInContent(bool enabled) override;

private:
    mutable QSettings m_settings;
};
