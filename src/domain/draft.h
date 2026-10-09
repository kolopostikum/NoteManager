#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <optional>

#include "domain/note.h"

/// Unsaved editing state of one editing session. One draft per session key.
struct Draft {
    QString key;       ///< Editing-session token (UUID), primary key.
    int noteId = 0;    ///< 0 for a note that was never saved.
    QString title;
    QString content;
    QString tags;
    QDateTime baseUpdatedAt; ///< notes.updated_at the editing started from.
    QDateTime savedAt;       ///< When the draft was written.
};

enum class DraftStatus {
    NewNote,     ///< Draft of a note that does not exist yet.
    Clean,       ///< Underlying note is unchanged since the draft started.
    Conflict,    ///< Note was modified after the draft started.
    NoteDeleted  ///< Underlying note no longer exists.
};

/// A stored draft together with its relation to the current stored note.
struct DraftInfo {
    Draft draft;
    DraftStatus status = DraftStatus::NewNote;
    std::optional<Note> current;
};

Q_DECLARE_METATYPE(Draft)
Q_DECLARE_METATYPE(DraftInfo)
