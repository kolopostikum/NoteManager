#pragma once

#include <QString>

/// Owns a named QSqlDatabase connection: opens it, applies pragmas, creates the
/// schema, and on destruction closes and unregisters the connection. A Qt SQL
/// connection must only be used from the thread that created it.
class SqliteConnection {
public:
    SqliteConnection(QString path, QString connectionName);
    ~SqliteConnection();
    SqliteConnection(const SqliteConnection &) = delete;
    SqliteConnection &operator=(const SqliteConnection &) = delete;

    bool open(QString *error = nullptr);
    const QString &name() const { return m_name; }

private:
    bool createSchema(QString *error);

    QString m_path;
    QString m_name;
};
