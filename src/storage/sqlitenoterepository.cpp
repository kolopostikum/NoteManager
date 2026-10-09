#include "storage/sqlitenoterepository.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <utility>

namespace {

QString toDb(const QDateTime &dt)
{
    return dt.toUTC().toString(Qt::ISODateWithMs);
}

/// A null QString would be stored as SQL NULL and violate NOT NULL columns.
QString nonNull(const QString &text)
{
    return text.isNull() ? QStringLiteral("") : text;
}

QDateTime fromDb(const QString &text)
{
    return QDateTime::fromString(text, Qt::ISODateWithMs);
}

constexpr auto kSelectColumns = "SELECT id, title, content, tags, created_at, updated_at FROM notes";

} // namespace

SqliteNoteRepository::SqliteNoteRepository(QString connectionName)
    : m_connection(std::move(connectionName))
{
}

Note SqliteNoteRepository::readNote(const QSqlQuery &query)
{
    Note note;
    note.id = query.value(0).toInt();
    note.title = query.value(1).toString();
    note.content = query.value(2).toString();
    note.tags = query.value(3).toString();
    note.createdAt = fromDb(query.value(4).toString());
    note.updatedAt = fromDb(query.value(5).toString());
    return note;
}

void SqliteNoteRepository::bindNote(QSqlQuery &query, const Note &note) const
{
    query.bindValue(QStringLiteral(":title"), nonNull(note.title));
    query.bindValue(QStringLiteral(":content"), nonNull(note.content));
    query.bindValue(QStringLiteral(":tags"), nonNull(note.tags));
    query.bindValue(QStringLiteral(":created_at"), toDb(note.createdAt));
    query.bindValue(QStringLiteral(":updated_at"), toDb(note.updatedAt));
}

bool SqliteNoteRepository::fail(const QSqlQuery &query)
{
    m_lastError = query.lastError().text();
    if (m_lastError.isEmpty())
        m_lastError = QStringLiteral("Unknown database error");
    return false;
}

QVector<Note> SqliteNoteRepository::getAll()
{
    m_lastError.clear();
    QVector<Note> notes;

    QSqlQuery query(QSqlDatabase::database(m_connection));
    if (!query.prepare(QString::fromLatin1(kSelectColumns) +
                       QStringLiteral(" ORDER BY updated_at DESC, id DESC"))
        || !query.exec()) {
        fail(query);
        return notes;
    }
    while (query.next())
        notes.append(readNote(query));
    return notes;
}

std::optional<Note> SqliteNoteRepository::getById(int id)
{
    m_lastError.clear();

    QSqlQuery query(QSqlDatabase::database(m_connection));
    if (!query.prepare(QString::fromLatin1(kSelectColumns) + QStringLiteral(" WHERE id = :id"))) {
        fail(query);
        return std::nullopt;
    }
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec()) {
        fail(query);
        return std::nullopt;
    }
    if (!query.next())
        return std::nullopt;
    return readNote(query);
}

bool SqliteNoteRepository::add(const Note &note)
{
    m_lastError.clear();

    QSqlQuery query(QSqlDatabase::database(m_connection));
    if (!query.prepare(QStringLiteral(
            "INSERT INTO notes (title, content, tags, created_at, updated_at) "
            "VALUES (:title, :content, :tags, :created_at, :updated_at)")))
        return fail(query);
    bindNote(query, note);
    if (!query.exec())
        return fail(query);

    m_lastInsertId = query.lastInsertId().toInt();
    return true;
}

bool SqliteNoteRepository::update(const Note &note)
{
    m_lastError.clear();

    QSqlQuery query(QSqlDatabase::database(m_connection));
    if (!query.prepare(QStringLiteral(
            "UPDATE notes SET title = :title, content = :content, tags = :tags, "
            "created_at = :created_at, updated_at = :updated_at WHERE id = :id")))
        return fail(query);
    bindNote(query, note);
    query.bindValue(QStringLiteral(":id"), note.id);
    if (!query.exec())
        return fail(query);
    if (query.numRowsAffected() < 1) {
        m_lastError = QStringLiteral("Note %1 not found").arg(note.id);
        return false;
    }
    return true;
}

bool SqliteNoteRepository::remove(int id)
{
    m_lastError.clear();

    QSqlQuery query(QSqlDatabase::database(m_connection));
    if (!query.prepare(QStringLiteral("DELETE FROM notes WHERE id = :id")))
        return fail(query);
    query.bindValue(QStringLiteral(":id"), id);
    if (!query.exec())
        return fail(query);
    if (query.numRowsAffected() < 1) {
        m_lastError = QStringLiteral("Note %1 not found").arg(id);
        return false;
    }
    return true;
}
