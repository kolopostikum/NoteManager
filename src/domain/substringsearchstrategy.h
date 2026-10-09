#pragma once

#include "domain/inotesearchstrategy.h"

/// Case-insensitive, Unicode-aware substring match performed in memory.
/// SQLite's LIKE/lower() only fold ASCII, so Cyrillic and other scripts would
/// not match case-insensitively if the search were done in SQL.
class SubstringSearchStrategy : public INoteSearchStrategy {
public:
    QVector<Note> filter(const QVector<Note> &notes, const QString &query,
                         SearchScope scope) const override;
};
