#pragma once

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QString>
#include <QThread>
#include <QVector>
#include <memory>

#include "app/noteexporter.h"
#include "domain/draft.h"
#include "domain/inotesearchstrategy.h"
#include "domain/note.h"
#include "storage/commandqueue.h"
#include "storage/commands.h"
#include "storage/storageworker.h"

/// Thin asynchronous client used by the UI. Every request is turned into a
/// command, placed in an ordered queue and executed by the storage worker
/// thread; results come back as signals. The UI never touches the database.
class NoteController : public QObject {
    Q_OBJECT
public:
    NoteController(QString dbPath, QString searchStrategyId, QObject *parent = nullptr);
    ~NoteController() override;

    /// Starts the worker thread; emits ready() when the database is available.
    void start();
    /// Processes every queued command, then stops the worker. Blocking, meant for exit.
    void shutdown();

    // Each request returns an id that is echoed by the matching result signal.
    quint64 loadAll();
    quint64 search(const QString &query, SearchScope scope);
    quint64 saveNote(const Note &note, const QString &draftKey);
    quint64 deleteNote(int noteId);
    quint64 saveDraft(const Draft &draft);
    quint64 deleteDraft(const QString &key);
    quint64 loadDrafts();
    quint64 exportNote(int noteId, const QString &path);

signals:
    void ready(bool ok, const QString &error);
    /// Only the result of the most recent list/search request is delivered.
    void notesLoaded(const QVector<Note> &notes);
    void noteSaved(quint64 requestId, const Note &note);
    void noteDeleted(quint64 requestId, int noteId);
    void draftSaved(const QString &key, const QDateTime &savedAt);
    void draftsLoaded(const QVector<DraftInfo> &drafts);
    void exportFinished(quint64 requestId, bool ok, const QString &path, const QString &error);
    void failed(quint64 requestId, CommandKind kind, const QString &message);

private slots:
    void onNotesLoaded(quint64 requestId, const QVector<Note> &notes);
    void onNoteForExport(quint64 requestId, const Note &note);
    void onFailed(quint64 requestId, CommandKind kind, const QString &message);

private:
    quint64 post(Command command);
    quint64 nextId() { return ++m_lastRequestId; }

    QString m_dbPath;
    std::shared_ptr<CommandQueue> m_queue;
    QThread m_thread;
    std::unique_ptr<StorageWorker> m_worker;
    quint64 m_lastRequestId = 0;
    quint64 m_latestListRequest = 0;
    bool m_started = false;
    bool m_stopped = false;
    QHash<quint64, QString> m_pendingExports;
};
