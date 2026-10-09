#include "storage/storageworker.h"

#include <QThread>
#include <QUuid>
#include <utility>

#include "domain/searchstrategyfactory.h"
#include "storage/sqlitedraftrepository.h"
#include "storage/sqlitenoterepository.h"
#include "storage/sqlitetransactionmanager.h"

StorageWorker::StorageWorker(QString dbPath, QString searchStrategyId,
                             std::shared_ptr<CommandQueue> queue, QObject *parent)
    : QObject(parent),
      m_dbPath(std::move(dbPath)),
      m_searchStrategyId(std::move(searchStrategyId)),
      m_queue(std::move(queue))
{
}

StorageWorker::~StorageWorker()
{
    teardown();
}

void StorageWorker::init()
{
    // Unique name per worker: several workers (tests) may coexist in one process.
    const QString connectionName =
        QStringLiteral("notemanager_") + QUuid::createUuid().toString(QUuid::WithoutBraces);

    auto connection = std::make_unique<SqliteConnection>(m_dbPath, connectionName);
    QString error;
    if (!connection->open(&error)) {
        emit initialized(false, error);
        return;
    }

    auto noteRepo = std::make_shared<SqliteNoteRepository>(connectionName);
    auto draftRepo = std::make_shared<SqliteDraftRepository>(connectionName);

    m_connection = std::move(connection);
    m_transactions = std::make_unique<SqliteTransactionManager>(connectionName);
    m_notes = std::make_unique<NoteService>(noteRepo, createSearchStrategy(m_searchStrategyId));
    m_drafts = std::make_unique<DraftService>(draftRepo, noteRepo);
    m_ready = true;
    emit initialized(true, QString());
}

void StorageWorker::teardown()
{
    m_ready = false;
    m_drafts.reset();
    m_notes.reset();
    m_transactions.reset();
    m_connection.reset();
}

void StorageWorker::drain()
{
    while (std::optional<Command> command = m_queue->tryPop()) {
        if (std::holds_alternative<ShutdownCmd>(*command)) {
            // Release the database inside the thread that created it, then stop
            // the thread's event loop.
            teardown();
            QThread::currentThread()->quit();
            return;
        }
        if (!m_ready)
            continue; // initialization failed; nothing can be executed
        std::visit([this](const auto &cmd) { handle(cmd); }, *command);
    }
}

void StorageWorker::handle(const LoadAllCmd &cmd)
{
    const ListResult result = m_notes->list();
    if (!result.ok) {
        emit failed(cmd.requestId, CommandKind::List, result.error);
        return;
    }
    emit notesLoaded(cmd.requestId, result.notes);
}

void StorageWorker::handle(const SearchCmd &cmd)
{
    const ListResult result = m_notes->search(cmd.query, cmd.scope);
    if (!result.ok) {
        emit failed(cmd.requestId, CommandKind::List, result.error);
        return;
    }
    emit notesLoaded(cmd.requestId, result.notes);
}

void StorageWorker::handle(const SaveNoteCmd &cmd)
{
    // Note write and draft removal commit together: either the note is stored
    // and the draft is gone, or nothing changed.
    TransactionScope tx(*m_transactions);
    if (!tx.active()) {
        emit failed(cmd.requestId, CommandKind::SaveNote, QStringLiteral("Cannot start transaction"));
        return;
    }

    const SaveResult saved = m_notes->save(cmd.note);
    if (!saved.ok) {
        emit failed(cmd.requestId, CommandKind::SaveNote, saved.error);
        return;
    }

    QString error;
    if (!cmd.draftKey.isEmpty() && !m_drafts->removeStored(cmd.draftKey, &error)) {
        emit failed(cmd.requestId, CommandKind::SaveNote, error);
        return;
    }
    if (!tx.commit()) {
        emit failed(cmd.requestId, CommandKind::SaveNote, QStringLiteral("Commit failed"));
        return;
    }

    if (!cmd.draftKey.isEmpty())
        m_drafts->closeSession(cmd.draftKey);
    emit noteSaved(cmd.requestId, saved.note);
}

void StorageWorker::handle(const DeleteNoteCmd &cmd)
{
    QString error;
    if (!m_notes->remove(cmd.noteId, &error)) {
        emit failed(cmd.requestId, CommandKind::DeleteNote, error);
        return;
    }
    emit noteDeleted(cmd.requestId, cmd.noteId);
}

void StorageWorker::handle(const SaveDraftCmd &cmd)
{
    Draft draft = cmd.draft;
    QString error;
    if (!m_drafts->save(draft, &error)) {
        emit failed(cmd.requestId, CommandKind::SaveDraft, error);
        return;
    }
    if (draft.savedAt.isValid())
        emit draftSaved(draft.key, draft.savedAt);
}

void StorageWorker::handle(const DeleteDraftCmd &cmd)
{
    QString error;
    if (!m_drafts->discard(cmd.key, &error))
        emit failed(cmd.requestId, CommandKind::DeleteDraft, error);
}

void StorageWorker::handle(const LoadDraftsCmd &cmd)
{
    QVector<DraftInfo> drafts;
    QString error;
    if (!m_drafts->recover(drafts, &error)) {
        emit failed(cmd.requestId, CommandKind::LoadDrafts, error);
        return;
    }
    emit draftsLoaded(cmd.requestId, drafts);
}

void StorageWorker::handle(const GetForExportCmd &cmd)
{
    QString error;
    const std::optional<Note> note = m_notes->get(cmd.noteId, &error);
    if (!note) {
        emit failed(cmd.requestId, CommandKind::Export,
                    error.isEmpty() ? QStringLiteral("Note not found") : error);
        return;
    }
    emit noteForExport(cmd.requestId, *note);
}
