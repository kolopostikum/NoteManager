#pragma once

#include <QString>
#include <QVector>
#include <memory>
#include <optional>

#include "domain/inotesearchstrategy.h"
#include "domain/inoterepository.h"
#include "domain/notevalidator.h"

struct ListResult {
    bool ok = false;
    QVector<Note> notes;
    QString error;
};

struct SaveResult {
    bool ok = false;
    Note note;
    QString error;
    NoteValidator::Error validation = NoteValidator::Error::None;
};

/// Application logic for notes: validation, timestamps, search.
class NoteService {
public:
    NoteService(std::shared_ptr<INoteRepository> repository,
                std::shared_ptr<INoteSearchStrategy> searchStrategy);

    ListResult list();
    ListResult search(const QString &query, SearchScope scope);
    std::optional<Note> get(int id, QString *error = nullptr);
    SaveResult save(Note note);
    bool remove(int id, QString *error = nullptr);

    void setSearchStrategy(std::shared_ptr<INoteSearchStrategy> strategy);

private:
    std::shared_ptr<INoteRepository> m_repository;
    std::shared_ptr<INoteSearchStrategy> m_searchStrategy;
};
