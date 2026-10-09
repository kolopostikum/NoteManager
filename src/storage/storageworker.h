#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QVector>
#include <memory>

#include "domain/draft.h"
#include "domain/draftservice.h"
#include "domain/itransactionmanager.h"
#include "domain/note.h"
#include "domain/noteservice.h"
#include "storage/commandqueue.h"
#include "storage/commands.h"
#include "storage/sqliteconnection.h"

/// Executes queued commands on a dedicated thread. It owns the SQLite
/// connection, the repositories and the services, all created inside the worker
/// thread (Qt SQL connections are thread-affine). Results are reported through
/// signals, which are delivered to the UI thread as queued events.
class StorageWorker : public QObject {
    Q_OBJECT
public:
    StorageWorker(QString dbPath, QString searchStrategyId, std::shared_ptr<CommandQueue> queue,
                  QObject *parent = nullptr);
    ~StorageWorker() override;

public slots:
    /// Opens the database and builds the object graph. Runs in the worker thread.
    void init();
    /// Processes queued commands until the queue is empty or Shutdown arrives.
    void drain();

signals:
    void initialized(bool ok, const QString &error);
    void notesLoaded(quint64 requestId, const QVector<Note> &notes);
    void noteSaved(quint64 requestId, const Note &note);
    void noteDeleted(quint64 requestId, int noteId);
    void draftSaved(const QString &key, const QDateTime &savedAt);
    void draftsLoaded(quint64 requestId, const QVector<DraftInfo> &drafts);
    void noteForExport(quint64 requestId, const Note &note);
    void failed(quint64 requestId, CommandKind kind, const QString &message);

private:
    void handle(const LoadAllCmd &cmd);
    void handle(const SearchCmd &cmd);
    void handle(const SaveNoteCmd &cmd);
    void handle(const DeleteNoteCmd &cmd);
    void handle(const SaveDraftCmd &cmd);
    void handle(const DeleteDraftCmd &cmd);
    void handle(const LoadDraftsCmd &cmd);
    void handle(const GetForExportCmd &cmd);
    void handle(const ShutdownCmd &) {}

    void teardown();

    QString m_dbPath;
    QString m_searchStrategyId;
    std::shared_ptr<CommandQueue> m_queue;
    bool m_ready = false;

    // Destruction order matters: services/repositories first, connection last.
    std::unique_ptr<SqliteConnection> m_connection;
    std::unique_ptr<ITransactionManager> m_transactions;
    std::unique_ptr<NoteService> m_notes;
    std::unique_ptr<DraftService> m_drafts;
};
