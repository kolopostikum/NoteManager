#pragma once

#include <QMetaType>
#include <QString>
#include <QtGlobal>
#include <variant>

#include "domain/draft.h"
#include "domain/inotesearchstrategy.h"
#include "domain/note.h"

/// Commands travelling from the UI-side controller to the storage worker. They
/// are plain value types; the variant gives exhaustive, type-safe dispatch.
struct LoadAllCmd {
    quint64 requestId = 0;
};

struct SearchCmd {
    quint64 requestId = 0;
    QString query;
    SearchScope scope = SearchScope::TitleAndContent;
};

/// Saves the note and, atomically, removes the draft of the editing session.
struct SaveNoteCmd {
    quint64 requestId = 0;
    Note note;
    QString draftKey;
};

struct DeleteNoteCmd {
    quint64 requestId = 0;
    int noteId = 0;
};

struct SaveDraftCmd {
    quint64 requestId = 0;
    Draft draft;
};

struct DeleteDraftCmd {
    quint64 requestId = 0;
    QString key;
};

struct LoadDraftsCmd {
    quint64 requestId = 0;
};

/// Reads the up-to-date note (after all previously queued writes) for export.
struct GetForExportCmd {
    quint64 requestId = 0;
    int noteId = 0;
};

struct ShutdownCmd {
};

using Command = std::variant<LoadAllCmd, SearchCmd, SaveNoteCmd, DeleteNoteCmd, SaveDraftCmd,
                             DeleteDraftCmd, LoadDraftsCmd, GetForExportCmd, ShutdownCmd>;

/// Identifies which operation a failure belongs to.
enum class CommandKind {
    List,
    SaveNote,
    DeleteNote,
    SaveDraft,
    DeleteDraft,
    LoadDrafts,
    Export
};

Q_DECLARE_METATYPE(CommandKind)
