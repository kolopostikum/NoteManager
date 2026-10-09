#pragma once

#include <QString>
#include <QTemporaryDir>

#include "domain/note.h"
#include "storage/sqliteconnection.h"

inline Note makeNote(const QString &title, const QString &content = QString(),
                     const QString &tags = QString())
{
    Note n;
    n.title = title;
    n.content = content;
    n.tags = tags;
    return n;
}

/// Temporary on-disk database with its own connection.
struct TempDb {
    QTemporaryDir dir;
    QString path = dir.filePath(QStringLiteral("test.db"));
    SqliteConnection connection{path, QStringLiteral("test_conn")};

    bool open(QString *error = nullptr) { return dir.isValid() && connection.open(error); }
    QString name() const { return connection.name(); }
};
