#pragma once

#include <QString>

/// User preferences contract (language, sorting, search options).
class ISettings {
public:
    virtual ~ISettings() = default;

    virtual QString language() const = 0;
    virtual void setLanguage(const QString &language) = 0;

    virtual int sortColumn() const = 0;
    virtual bool sortAscending() const = 0;
    virtual void setSort(int column, bool ascending) = 0;

    virtual QString searchStrategy() const = 0;
    virtual bool searchInContent() const = 0;
    virtual void setSearchInContent(bool enabled) = 0;
};
