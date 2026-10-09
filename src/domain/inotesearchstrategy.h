#pragma once

#include <QString>
#include <QVector>

#include "domain/note.h"

enum class SearchScope {
    TitleOnly,       ///< Filter by title (main-window filter box).
    TitleAndContent  ///< Substring search in title and text.
};

/// Search algorithm contract. New algorithms (tags, full-text, fuzzy...) can be
/// added without touching repositories, services or UI.
class INoteSearchStrategy {
public:
    virtual ~INoteSearchStrategy() = default;

    virtual QVector<Note> filter(const QVector<Note> &notes, const QString &query,
                                 SearchScope scope) const = 0;
};
