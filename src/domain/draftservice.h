#pragma once

#include <QSet>
#include <QString>
#include <QVector>
#include <memory>

#include "domain/draft.h"
#include "domain/idraftrepository.h"
#include "domain/inoterepository.h"

/// Draft life-cycle: storing autosaved drafts, discarding them, and classifying
/// leftovers after an abnormal termination.
class DraftService {
public:
    DraftService(std::shared_ptr<IDraftRepository> drafts, std::shared_ptr<INoteRepository> notes);

    /// Stores the draft and stamps savedAt. Writes for already closed editing
    /// sessions are ignored (savedAt stays null) so a late autosave can never
    /// resurrect a draft that was saved or discarded.
    bool save(Draft &draft, QString *error = nullptr);

    /// Removes the stored draft only; use inside a transaction together with the
    /// note write, then call closeSession() after a successful commit.
    bool removeStored(const QString &key, QString *error = nullptr);
    void closeSession(const QString &key);

    /// removeStored() + closeSession().
    bool discard(const QString &key, QString *error = nullptr);

    /// All stored drafts, each classified against the current stored note.
    bool recover(QVector<DraftInfo> &out, QString *error = nullptr);

    static DraftStatus classify(const Draft &draft, const std::optional<Note> &current);

private:
    std::shared_ptr<IDraftRepository> m_drafts;
    std::shared_ptr<INoteRepository> m_notes;
    QSet<QString> m_closedSessions;
};
