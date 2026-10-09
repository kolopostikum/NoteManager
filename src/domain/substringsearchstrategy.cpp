#include "domain/substringsearchstrategy.h"

QVector<Note> SubstringSearchStrategy::filter(const QVector<Note> &notes, const QString &query,
                                              SearchScope scope) const
{
    const QString needle = query.trimmed().toCaseFolded();
    if (needle.isEmpty())
        return notes;

    QVector<Note> result;
    for (const Note &note : notes) {
        bool match = note.title.toCaseFolded().contains(needle);
        if (!match && scope == SearchScope::TitleAndContent)
            match = note.content.toCaseFolded().contains(needle);
        if (match)
            result.append(note);
    }
    return result;
}
