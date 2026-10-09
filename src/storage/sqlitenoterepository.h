#pragma once

#include <QString>

#include "domain/inoterepository.h"

class QSqlQuery;

/// SQLite implementation of INoteRepository. Every statement is parameterized
/// (prepare + bindValue). The repository stores only the connection name and
/// looks the connection up per call, so it never keeps a handle alive.
class SqliteNoteRepository : public INoteRepository {
public:
    explicit SqliteNoteRepository(QString connectionName);

    QVector<Note> getAll() override;
    std::optional<Note> getById(int id) override;
    bool add(const Note &note) override;
    bool update(const Note &note) override;
    bool remove(int id) override;

    int lastInsertId() const override { return m_lastInsertId; }
    QString lastError() const override { return m_lastError; }

private:
    static Note readNote(const QSqlQuery &query);
    void bindNote(QSqlQuery &query, const Note &note) const;
    bool fail(const QSqlQuery &query);

    QString m_connection;
    QString m_lastError;
    int m_lastInsertId = 0;
};
