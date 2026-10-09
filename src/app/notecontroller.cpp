#include "app/notecontroller.h"

#include <QFutureWatcher>
#include <QtConcurrent>
#include <utility>

NoteController::NoteController(QString dbPath, QString searchStrategyId, QObject *parent)
    : QObject(parent), m_dbPath(std::move(dbPath)), m_queue(std::make_shared<CommandQueue>())
{
    qRegisterMetaType<Note>("Note");
    qRegisterMetaType<QVector<Note>>("QVector<Note>");
    qRegisterMetaType<Draft>("Draft");
    qRegisterMetaType<DraftInfo>("DraftInfo");
    qRegisterMetaType<QVector<DraftInfo>>("QVector<DraftInfo>");
    qRegisterMetaType<CommandKind>("CommandKind");

    m_worker = std::make_unique<StorageWorker>(m_dbPath, std::move(searchStrategyId), m_queue);
    m_worker->moveToThread(&m_thread);

    connect(&m_thread, &QThread::started, m_worker.get(), &StorageWorker::init);
    connect(m_worker.get(), &StorageWorker::initialized, this, &NoteController::ready);
    connect(m_worker.get(), &StorageWorker::notesLoaded, this, &NoteController::onNotesLoaded);
    connect(m_worker.get(), &StorageWorker::noteSaved, this, &NoteController::noteSaved);
    connect(m_worker.get(), &StorageWorker::noteDeleted, this, &NoteController::noteDeleted);
    connect(m_worker.get(), &StorageWorker::draftSaved, this, &NoteController::draftSaved);
    connect(m_worker.get(), &StorageWorker::draftsLoaded, this,
            [this](quint64, const QVector<DraftInfo> &drafts) { emit draftsLoaded(drafts); });
    connect(m_worker.get(), &StorageWorker::noteForExport, this, &NoteController::onNoteForExport);
    connect(m_worker.get(), &StorageWorker::failed, this, &NoteController::onFailed);
}

NoteController::~NoteController()
{
    shutdown();
}

void NoteController::start()
{
    if (m_started)
        return;
    m_started = true;
    m_thread.start();
}

void NoteController::shutdown()
{
    if (!m_started || m_stopped)
        return;
    m_stopped = true;

    m_queue->push(ShutdownCmd{});
    QMetaObject::invokeMethod(m_worker.get(), "drain", Qt::QueuedConnection);
    m_thread.wait();
}

quint64 NoteController::post(Command command)
{
    if (m_stopped)
        return 0;
    m_queue->push(std::move(command));
    // Wake the worker; drain() is a no-op when the queue is already empty.
    QMetaObject::invokeMethod(m_worker.get(), "drain", Qt::QueuedConnection);
    return m_lastRequestId;
}

quint64 NoteController::loadAll()
{
    const quint64 id = nextId();
    m_latestListRequest = id;
    return post(LoadAllCmd{id});
}

quint64 NoteController::search(const QString &query, SearchScope scope)
{
    const quint64 id = nextId();
    m_latestListRequest = id;
    return post(SearchCmd{id, query, scope});
}

quint64 NoteController::saveNote(const Note &note, const QString &draftKey)
{
    return post(SaveNoteCmd{nextId(), note, draftKey});
}

quint64 NoteController::deleteNote(int noteId)
{
    return post(DeleteNoteCmd{nextId(), noteId});
}

quint64 NoteController::saveDraft(const Draft &draft)
{
    return post(SaveDraftCmd{nextId(), draft});
}

quint64 NoteController::deleteDraft(const QString &key)
{
    return post(DeleteDraftCmd{nextId(), key});
}

quint64 NoteController::loadDrafts()
{
    return post(LoadDraftsCmd{nextId()});
}

quint64 NoteController::exportNote(int noteId, const QString &path)
{
    const quint64 id = nextId();
    m_pendingExports.insert(id, path);
    return post(GetForExportCmd{id, noteId});
}

void NoteController::onNotesLoaded(quint64 requestId, const QVector<Note> &notes)
{
    // A newer list/search request supersedes this answer.
    if (requestId < m_latestListRequest)
        return;
    emit notesLoaded(notes);
}

void NoteController::onNoteForExport(quint64 requestId, const Note &note)
{
    const QString path = m_pendingExports.take(requestId);
    const ExportFormat format = NoteExporter::formatForPath(path);

    // File I/O runs in the global thread pool; the watcher lives in this
    // (UI) thread and is released by its parent once finished.
    auto *watcher = new QFutureWatcher<ExportResult>(this);
    connect(watcher, &QFutureWatcher<ExportResult>::finished, this, [this, watcher, requestId, path]() {
        const ExportResult result = watcher->result();
        watcher->deleteLater();
        emit exportFinished(requestId, result.ok, path, result.error);
    });
    watcher->setFuture(QtConcurrent::run(&NoteExporter::write, note, path, format));
}

void NoteController::onFailed(quint64 requestId, CommandKind kind, const QString &message)
{
    if (kind == CommandKind::Export) {
        const QString path = m_pendingExports.take(requestId);
        emit exportFinished(requestId, false, path, message);
        return;
    }
    emit failed(requestId, kind, message);
}
