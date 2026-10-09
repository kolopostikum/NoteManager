#include "storage/sqlitedraftrepository.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <utility>

namespace {

QVariant toDb(const QDateTime &dt)
{
    if (!dt.isValid())
        return QVariant(QVariant::String); // NULL
    return dt.toUTC().toString(Qt::ISODateWithMs);
}

/// A null QString would be stored as SQL NULL and violate NOT NULL columns.
QString nonNull(const QString &text)
{
    return text.isNull() ? QStringLiteral("") : text;
}

QDateTime fromDb(const QVariant &value)
{
    if (value.isNull())
        return QDateTime();
    return QDateTime::fromString(value.toString(), Qt::ISODateWithMs);
}

} // namespace

SqliteDraftRepository::SqliteDraftRepository(QString connectionName)
    : m_connection(std::move(connectionName))
{
}

bool SqliteDraftRepository::fail(const QSqlQuery &query)
{
    m_lastError = query.lastError().text();
    if (m_lastError.isEmpty())
        m_lastError = QStringLiteral("Unknown database error");
    return false;
}

bool SqliteDraftRepository::upsert(const Draft &draft)
{
    m_lastError.clear();

    QSqlQuery query(QSqlDatabase::database(m_connection));
    if (!query.prepare(QStringLiteral(
            "INSERT OR REPLACE INTO drafts "
            "(draft_key, note_id, title, content, tags, base_updated_at, saved_at) "
            "VALUES (:key, :note_id, :title, :content, :tags, :base, :saved)")))
        return fail(query);

    query.bindValue(QStringLiteral(":key"), draft.key);
    query.bindValue(QStringLiteral(":note_id"),
                    draft.noteId == 0 ? QVariant(QVariant::Int) : QVariant(draft.noteId));
    query.bindValue(QStringLiteral(":title"), nonNull(draft.title));
    query.bindValue(QStringLiteral(":content"), nonNull(draft.content));
    query.bindValue(QStringLiteral(":tags"), nonNull(draft.tags));
    query.bindValue(QStringLiteral(":base"), toDb(draft.baseUpdatedAt));
    query.bindValue(QStringLiteral(":saved"), toDb(draft.savedAt).toString());
    return query.exec() ? true : fail(query);
}

bool SqliteDraftRepository::remove(const QString &key)
{
    m_lastError.clear();

    QSqlQuery query(QSqlDatabase::database(m_connection));
    if (!query.prepare(QStringLiteral("DELETE FROM drafts WHERE draft_key = :key")))
        return fail(query);
    query.bindValue(QStringLiteral(":key"), key);
    return query.exec() ? true : fail(query);
}

QVector<Draft> SqliteDraftRepository::getAll()
{
    m_lastError.clear();
    QVector<Draft> drafts;

    QSqlQuery query(QSqlDatabase::database(m_connection));
    if (!query.prepare(QStringLiteral(
            "SELECT draft_key, note_id, title, content, tags, base_updated_at, saved_at "
            "FROM drafts ORDER BY saved_at"))
        || !query.exec()) {
        fail(query);
        return drafts;
    }
    while (query.next()) {
        Draft d;
        d.key = query.value(0).toString();
        d.noteId = query.value(1).isNull() ? 0 : query.value(1).toInt();
        d.title = query.value(2).toString();
        d.content = query.value(3).toString();
        d.tags = query.value(4).toString();
        d.baseUpdatedAt = fromDb(query.value(5));
        d.savedAt = fromDb(query.value(6));
        drafts.append(d);
    }
    return drafts;
}
