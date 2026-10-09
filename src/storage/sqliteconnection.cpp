#include "storage/sqliteconnection.h"

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <utility>

SqliteConnection::SqliteConnection(QString path, QString connectionName)
    : m_path(std::move(path)), m_name(std::move(connectionName))
{
}

SqliteConnection::~SqliteConnection()
{
    {
        // The handle must go out of scope before removeDatabase(), otherwise Qt
        // warns that the connection is still in use.
        QSqlDatabase db = QSqlDatabase::database(m_name, false);
        if (db.isValid() && db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(m_name);
}

bool SqliteConnection::open(QString *error)
{
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_name);
        db.setDatabaseName(m_path);
        if (!db.open()) {
            if (error)
                *error = db.lastError().text();
            return false;
        }

        QSqlQuery pragma(db);
        const QStringList pragmas = {
            QStringLiteral("PRAGMA journal_mode=WAL"),
            QStringLiteral("PRAGMA synchronous=NORMAL"),
            QStringLiteral("PRAGMA busy_timeout=5000"),
        };
        for (const QString &sql : pragmas) {
            if (!pragma.exec(sql)) {
                if (error)
                    *error = pragma.lastError().text();
                return false;
            }
        }
    }
    return createSchema(error);
}

bool SqliteConnection::createSchema(QString *error)
{
    QSqlDatabase db = QSqlDatabase::database(m_name);
    QSqlQuery query(db);

    const QStringList statements = {
        QStringLiteral("CREATE TABLE IF NOT EXISTS notes ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "title TEXT NOT NULL,"
                       "content TEXT,"
                       "tags TEXT,"
                       "created_at DATETIME NOT NULL,"
                       "updated_at DATETIME NOT NULL)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_notes_updated_at ON notes(updated_at)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS drafts ("
                       "draft_key TEXT PRIMARY KEY,"
                       "note_id INTEGER,"
                       "title TEXT NOT NULL,"
                       "content TEXT,"
                       "tags TEXT,"
                       "base_updated_at TEXT,"
                       "saved_at TEXT NOT NULL)"),
    };
    for (const QString &sql : statements) {
        if (!query.exec(sql)) {
            if (error)
                *error = query.lastError().text();
            return false;
        }
    }
    return true;
}
