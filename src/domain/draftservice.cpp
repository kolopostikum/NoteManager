#include "domain/draftservice.h"

#include <utility>

DraftService::DraftService(std::shared_ptr<IDraftRepository> drafts,
                           std::shared_ptr<INoteRepository> notes)
    : m_drafts(std::move(drafts)), m_notes(std::move(notes))
{
}

bool DraftService::save(Draft &draft, QString *error)
{
    if (m_closedSessions.contains(draft.key)) {
        draft.savedAt = QDateTime();
        return true;
    }
    draft.savedAt = QDateTime::currentDateTimeUtc();
    const bool ok = m_drafts->upsert(draft);
    if (!ok) {
        draft.savedAt = QDateTime();
        if (error)
            *error = m_drafts->lastError();
    }
    return ok;
}

bool DraftService::removeStored(const QString &key, QString *error)
{
    const bool ok = m_drafts->remove(key);
    if (!ok && error)
        *error = m_drafts->lastError();
    return ok;
}

void DraftService::closeSession(const QString &key)
{
    m_closedSessions.insert(key);
}

bool DraftService::discard(const QString &key, QString *error)
{
    const bool ok = removeStored(key, error);
    if (ok)
        closeSession(key);
    return ok;
}

bool DraftService::recover(QVector<DraftInfo> &out, QString *error)
{
    out.clear();
    const QVector<Draft> drafts = m_drafts->getAll();
    if (!m_drafts->lastError().isEmpty()) {
        if (error)
            *error = m_drafts->lastError();
        return false;
    }

    for (const Draft &draft : drafts) {
        DraftInfo info;
        info.draft = draft;
        if (draft.noteId != 0) {
            info.current = m_notes->getById(draft.noteId);
            if (!m_notes->lastError().isEmpty()) {
                if (error)
                    *error = m_notes->lastError();
                return false;
            }
        }
        info.status = classify(draft, info.current);
        out.append(info);
    }
    return true;
}

DraftStatus DraftService::classify(const Draft &draft, const std::optional<Note> &current)
{
    if (draft.noteId == 0)
        return DraftStatus::NewNote;
    if (!current)
        return DraftStatus::NoteDeleted;
    if (current->updatedAt.toMSecsSinceEpoch() == draft.baseUpdatedAt.toMSecsSinceEpoch())
        return DraftStatus::Clean;
    return DraftStatus::Conflict;
}
