#pragma once

#include <QString>

#include "domain/note.h"

enum class ExportFormat { Txt, Markdown };

struct ExportResult {
    bool ok = false;
    QString error;
};

/// Renders and writes a note snapshot to a file. All functions are pure or
/// touch only their arguments, so they are safe to run on a worker thread.
class NoteExporter {
public:
    /// Picks the format from the file suffix (".md"/".markdown" -> Markdown).
    static ExportFormat formatForPath(const QString &path);

    static QString render(const Note &note, ExportFormat format);

    /// Atomic write (QSaveFile): the target is replaced only on success.
    static ExportResult write(const Note &note, const QString &path, ExportFormat format);
};
