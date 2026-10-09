#include "domain/noteservice.h"

#include <utility>

NoteService::NoteService(std::shared_ptr<INoteRepository> repository,
                         std::shared_ptr<INoteSearchStrategy> searchStrategy)
    : m_repository(std::move(repository)), m_searchStrategy(std::move(searchStrategy))
{
}

ListResult NoteService::list()
{
    ListResult result;
    result.notes = m_repository->getAll();
    result.error = m_repository->lastError();
    result.ok = result.error.isEmpty();
    return result;
}

ListResult NoteService::search(const QString &query, SearchScope scope)
{
    ListResult result = list();
    if (result.ok)
        result.notes = m_searchStrategy->filter(result.notes, query, scope);
    return result;
}

std::optional<Note> NoteService::get(int id, QString *error)
{
    std::optional<Note> note = m_repository->getById(id);
    if (error)
        *error = m_repository->lastError();
    return note;
}

SaveResult NoteService::save(Note note)
{
    SaveResult result;

    result.validation = NoteValidator::validate(note);
    if (result.validation != NoteValidator::Error::None) {
        result.error = QStringLiteral("Invalid note title");
        return result;
    }
    note.title = note.title.trimmed();

    const QDateTime now = QDateTime::currentDateTimeUtc();
    if (note.isNew()) {
        note.createdAt = now;
        note.updatedAt = now;
        if (!m_repository->add(note)) {
            result.error = m_repository->lastError();
            return result;
        }
        note.id = m_repository->lastInsertId();
    } else {
        const std::optional<Note> existing = m_repository->getById(note.id);
        if (!existing) {
            result.error = m_repository->lastError().isEmpty()
                               ? QStringLiteral("Note %1 not found").arg(note.id)
                               : m_repository->lastError();
            return result;
        }
        note.createdAt = existing->createdAt;
        note.updatedAt = now;
        if (!m_repository->update(note)) {
            result.error = m_repository->lastError();
            return result;
        }
    }

    result.ok = true;
    result.note = note;
    return result;
}

bool NoteService::remove(int id, QString *error)
{
    const bool ok = m_repository->remove(id);
    if (!ok && error)
        *error = m_repository->lastError();
    return ok;
}

void NoteService::setSearchStrategy(std::shared_ptr<INoteSearchStrategy> strategy)
{
    m_searchStrategy = std::move(strategy);
}
