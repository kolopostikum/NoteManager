#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>

/// Plain domain entity. Timestamps are always kept in UTC.
struct Note {
    int id = 0;
    QString title;
    QString content;
    QString tags;
    QDateTime createdAt;
    QDateTime updatedAt;

    bool isNew() const { return id == 0; }
};

Q_DECLARE_METATYPE(Note)
