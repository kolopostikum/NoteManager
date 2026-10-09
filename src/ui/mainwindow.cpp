#include "ui/mainwindow.h"

#include <QAction>
#include <QActionGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QRegExp>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QStatusBar>
#include <QTableView>
#include <QTextBrowser>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QUuid>

#include "app/languagemanager.h"
#include "domain/isettings.h"
#include "ui/notelistmodel.h"

namespace {

constexpr int kSearchDebounceMs = 250;

// Sort presets offered in the combo box: {column, ascending}.
struct SortPreset {
    int column;
    bool ascending;
};
constexpr SortPreset kSortPresets[] = {
    {NoteListModel::UpdatedColumn, false},
    {NoteListModel::UpdatedColumn, true},
    {NoteListModel::TitleColumn, true},
    {NoteListModel::TitleColumn, false},
};

} // namespace

MainWindow::MainWindow(NoteController &controller, ISettings &settings, LanguageManager &language,
                       QWidget *parent)
    : QMainWindow(parent), m_controller(controller), m_settings(settings), m_language(language)
{
    resize(960, 600);

    buildActions();
    buildUi();
    buildMenus();

    connect(&m_controller, &NoteController::ready, this, &MainWindow::onReady);
    connect(&m_controller, &NoteController::notesLoaded, this, &MainWindow::onNotesLoaded);
    connect(&m_controller, &NoteController::noteSaved, this, &MainWindow::onNoteSaved);
    connect(&m_controller, &NoteController::noteDeleted, this, &MainWindow::onNoteDeleted);
    connect(&m_controller, &NoteController::draftsLoaded, this, &MainWindow::onDraftsLoaded);
    connect(&m_controller, &NoteController::exportFinished, this, &MainWindow::onExportFinished);
    connect(&m_controller, &NoteController::failed, this, &MainWindow::onFailed);

    retranslate();
    updateActionStates();
}

// ---------------------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------------------

void MainWindow::buildActions()
{
    m_addAction = new QAction(QIcon(QStringLiteral(":/icons/add.svg")), QString(), this);
    m_addAction->setShortcut(QKeySequence::New); // Ctrl+N
    connect(m_addAction, &QAction::triggered, this, &MainWindow::addNote);

    m_editAction = new QAction(QIcon(QStringLiteral(":/icons/edit.svg")), QString(), this);
    m_editAction->setShortcut(Qt::Key_F2);
    connect(m_editAction, &QAction::triggered, this, &MainWindow::editSelected);

    m_deleteAction = new QAction(QIcon(QStringLiteral(":/icons/delete.svg")), QString(), this);
    m_deleteAction->setShortcut(QKeySequence::Delete); // Delete
    // Active only while the list (or a child) has focus, so pressing Delete in
    // the search box edits text instead of removing a note.
    m_deleteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_deleteAction, &QAction::triggered, this, &MainWindow::deleteSelected);

    m_refreshAction = new QAction(QIcon(QStringLiteral(":/icons/refresh.svg")), QString(), this);
    m_refreshAction->setShortcut(QKeySequence::Refresh); // F5
    connect(m_refreshAction, &QAction::triggered, this, &MainWindow::reload);

    m_exportAction = new QAction(QIcon(QStringLiteral(":/icons/export.svg")), QString(), this);
    m_exportAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_E));
    connect(m_exportAction, &QAction::triggered, this, &MainWindow::exportSelected);

    m_quitAction = new QAction(this);
    m_quitAction->setShortcut(QKeySequence::Quit);
    connect(m_quitAction, &QAction::triggered, this, &QWidget::close);
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);

    // Search / sort row
    auto *searchRow = new QHBoxLayout;
    m_searchEdit = new QLineEdit(central);
    m_searchEdit->setClearButtonEnabled(true);
    m_searchInContent = new QCheckBox(central);
    m_searchInContent->setChecked(m_settings.searchInContent());
    m_sortLabel = new QLabel(central);
    m_sortCombo = new QComboBox(central);
    for (int i = 0; i < 4; ++i)
        m_sortCombo->addItem(QString());
    searchRow->addWidget(m_searchEdit, 1);
    searchRow->addWidget(m_searchInContent);
    searchRow->addWidget(m_sortLabel);
    searchRow->addWidget(m_sortCombo);
    root->addLayout(searchRow);

    // Buttons row
    auto *buttonRow = new QHBoxLayout;
    for (QAction *action : {m_addAction, m_editAction, m_deleteAction, m_refreshAction, m_exportAction}) {
        auto *button = new QToolButton(central);
        button->setDefaultAction(action);
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        buttonRow->addWidget(button);
    }
    buttonRow->addStretch(1);
    root->addLayout(buttonRow);

    // List + preview
    m_model = new NoteListModel(this);
    m_proxy = new QSortFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setSortRole(NoteListModel::SortRole);
    m_proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setSortLocaleAware(true);

    m_table = new QTableView(central);
    m_table->setModel(m_proxy);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setSectionResizeMode(NoteListModel::TitleColumn, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(NoteListModel::TagsColumn,
                                                      QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(NoteListModel::UpdatedColumn,
                                                      QHeaderView::ResizeToContents);
    m_table->setSortingEnabled(true);
    m_table->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_table->addAction(m_editAction);
    m_table->addAction(m_deleteAction);
    m_table->addAction(m_exportAction);

    m_preview = new QTextBrowser(central);
    m_preview->setOpenLinks(false);
    m_preview->setOpenExternalLinks(false);

    auto *splitter = new QSplitter(Qt::Horizontal, central);
    splitter->addWidget(m_table);
    splitter->addWidget(m_preview);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    root->addWidget(splitter, 1);

    setCentralWidget(central);

    m_countLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_countLabel);

    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(kSearchDebounceMs);

    // Wiring
    connect(m_searchTimer, &QTimer::timeout, this, &MainWindow::reload);
    connect(m_searchEdit, &QLineEdit::textChanged, m_searchTimer, qOverload<>(&QTimer::start));
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &MainWindow::reload);
    connect(m_searchInContent, &QCheckBox::toggled, this, [this](bool on) {
        m_settings.setSearchInContent(on);
        reload();
    });
    connect(m_sortCombo, qOverload<int>(&QComboBox::activated), this, &MainWindow::onSortComboChanged);
    connect(m_table->horizontalHeader(), &QHeaderView::sortIndicatorChanged, this,
            &MainWindow::onHeaderSortChanged);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &MainWindow::onSelectionChanged);
    connect(m_table, &QTableView::activated, this, &MainWindow::editSelected);
    connect(m_table, &QTableView::doubleClicked, this, &MainWindow::editSelected);

    // Restore the saved sort order.
    const int column = m_settings.sortColumn();
    applySort(column >= 0 && column < NoteListModel::ColumnCount ? column : NoteListModel::UpdatedColumn,
              m_settings.sortAscending() ? Qt::AscendingOrder : Qt::DescendingOrder);
}

void MainWindow::buildMenus()
{
    m_fileMenu = menuBar()->addMenu(QString());
    m_fileMenu->addAction(m_addAction);
    m_fileMenu->addAction(m_exportAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_quitAction);

    m_languageMenu = menuBar()->addMenu(QString());
    m_languageGroup = new QActionGroup(this);
    m_languageGroup->setExclusive(true);
    struct Entry {
        const char *code;
        const char *name;
    };
    for (const Entry &entry : {Entry{"en", "English"}, Entry{"ru", "\xD0\xA0\xD1\x83\xD1\x81\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9"}}) {
        QAction *action = m_languageMenu->addAction(QString::fromUtf8(entry.name));
        action->setCheckable(true);
        action->setData(QString::fromLatin1(entry.code));
        action->setChecked(m_language.current() == QLatin1String(entry.code));
        m_languageGroup->addAction(action);
    }
    connect(m_languageGroup, &QActionGroup::triggered, this, &MainWindow::onLanguageChosen);
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        retranslate();
    QMainWindow::changeEvent(event);
}

void MainWindow::retranslate()
{
    setWindowTitle(tr("NoteManager"));

    m_addAction->setText(tr("Add"));
    m_addAction->setToolTip(tr("Create a new note (%1)").arg(m_addAction->shortcut().toString(QKeySequence::NativeText)));
    m_editAction->setText(tr("Edit"));
    m_editAction->setToolTip(tr("Edit the selected note (%1)").arg(m_editAction->shortcut().toString(QKeySequence::NativeText)));
    m_deleteAction->setText(tr("Delete"));
    m_deleteAction->setToolTip(tr("Delete the selected note (%1)").arg(m_deleteAction->shortcut().toString(QKeySequence::NativeText)));
    m_refreshAction->setText(tr("Refresh"));
    m_refreshAction->setToolTip(tr("Reload the list (%1)").arg(m_refreshAction->shortcut().toString(QKeySequence::NativeText)));
    m_exportAction->setText(tr("Export..."));
    m_exportAction->setToolTip(tr("Export the selected note to .txt or .md (%1)").arg(m_exportAction->shortcut().toString(QKeySequence::NativeText)));
    m_quitAction->setText(tr("Quit"));

    m_fileMenu->setTitle(tr("&File"));
    m_languageMenu->setTitle(tr("&Language"));

    m_searchEdit->setPlaceholderText(tr("Search notes..."));
    m_searchInContent->setText(tr("Search in text"));
    m_sortLabel->setText(tr("Sort:"));
    m_sortCombo->setItemText(0, tr("Modified (newest first)"));
    m_sortCombo->setItemText(1, tr("Modified (oldest first)"));
    m_sortCombo->setItemText(2, tr("Title (A-Z)"));
    m_sortCombo->setItemText(3, tr("Title (Z-A)"));
    m_preview->setPlaceholderText(tr("Select a note to preview it"));

    m_model->retranslate();
    m_countLabel->setText(tr("Notes: %1").arg(m_model->rowCount()));
    updatePreview();
}

// ---------------------------------------------------------------------------
// Sorting
// ---------------------------------------------------------------------------

void MainWindow::applySort(int column, Qt::SortOrder order)
{
    m_table->sortByColumn(column, order);

    int presetIndex = -1;
    for (int i = 0; i < 4; ++i) {
        if (kSortPresets[i].column == column
            && kSortPresets[i].ascending == (order == Qt::AscendingOrder))
            presetIndex = i;
    }
    QSignalBlocker blocker(m_sortCombo);
    m_sortCombo->setCurrentIndex(presetIndex);
}

void MainWindow::onSortComboChanged(int index)
{
    if (index < 0 || index >= 4)
        return;
    const SortPreset &preset = kSortPresets[index];
    applySort(preset.column, preset.ascending ? Qt::AscendingOrder : Qt::DescendingOrder);
}

void MainWindow::onHeaderSortChanged(int column, Qt::SortOrder order)
{
    m_settings.setSort(column, order == Qt::AscendingOrder);

    int presetIndex = -1;
    for (int i = 0; i < 4; ++i) {
        if (kSortPresets[i].column == column
            && kSortPresets[i].ascending == (order == Qt::AscendingOrder))
            presetIndex = i;
    }
    QSignalBlocker blocker(m_sortCombo);
    m_sortCombo->setCurrentIndex(presetIndex);
}

// ---------------------------------------------------------------------------
// Controller results
// ---------------------------------------------------------------------------

void MainWindow::onReady(bool ok, const QString &error)
{
    if (!ok) {
        QMessageBox::critical(this, tr("Database error"),
                              tr("Cannot open the notes database:\n%1").arg(error));
        QTimer::singleShot(0, this, &QWidget::close);
        return;
    }
    reload();
    m_controller.loadDrafts(); // unsaved work left over after a crash
}

void MainWindow::reload()
{
    m_searchTimer->stop();
    m_controller.search(m_searchEdit->text(),
                        m_searchInContent->isChecked() ? SearchScope::TitleAndContent
                                                       : SearchScope::TitleOnly);
}

void MainWindow::onNotesLoaded(const QVector<Note> &notes)
{
    int keepId = m_pendingSelectId;
    m_pendingSelectId = 0;
    if (keepId == 0) {
        if (const auto note = currentNote())
            keepId = note->id;
    }

    m_model->setNotes(notes);
    m_countLabel->setText(tr("Notes: %1").arg(notes.size()));
    if (keepId != 0)
        selectNoteById(keepId);
    onSelectionChanged();
}

void MainWindow::selectNoteById(int noteId)
{
    const int sourceRow = m_model->rowOfId(noteId);
    if (sourceRow < 0)
        return;
    const QModelIndex index = m_proxy->mapFromSource(m_model->index(sourceRow, 0));
    m_table->selectRow(index.row());
    m_table->scrollTo(index);
}

void MainWindow::onNoteSaved(quint64, const Note &note)
{
    statusBar()->showMessage(tr("Note \"%1\" saved").arg(note.title), 4000);
    m_pendingSelectId = note.id;
    reload();
}

void MainWindow::onNoteDeleted(quint64, int)
{
    statusBar()->showMessage(tr("Note deleted"), 4000);
    reload();
}

void MainWindow::onExportFinished(quint64, bool ok, const QString &path, const QString &error)
{
    if (ok) {
        statusBar()->showMessage(tr("Exported to %1").arg(QDir::toNativeSeparators(path)), 6000);
    } else {
        QMessageBox::warning(this, tr("Export failed"),
                             tr("Cannot export the note to %1:\n%2")
                                 .arg(QDir::toNativeSeparators(path), error));
    }
}

void MainWindow::onFailed(quint64, CommandKind kind, const QString &message)
{
    switch (kind) {
    case CommandKind::SaveDraft:
    case CommandKind::DeleteDraft:
        // Background housekeeping: do not interrupt the user with a modal box.
        statusBar()->showMessage(tr("Draft operation failed: %1").arg(message), 8000);
        break;
    case CommandKind::SaveNote:
        QMessageBox::critical(this, tr("Save failed"),
                              tr("The note could not be saved:\n%1\n\nYour text is kept as a draft "
                                 "and will be offered for recovery.")
                                  .arg(message));
        m_controller.loadDrafts();
        break;
    default:
        QMessageBox::critical(this, tr("Error"), message);
        break;
    }
}

// ---------------------------------------------------------------------------
// User actions
// ---------------------------------------------------------------------------

std::optional<Note> MainWindow::currentNote() const
{
    const QModelIndex current = m_table->selectionModel()->currentIndex();
    if (!current.isValid() || !m_table->selectionModel()->isRowSelected(current.row(), QModelIndex()))
        return std::nullopt;
    const QModelIndex source = m_proxy->mapToSource(current);
    if (!source.isValid())
        return std::nullopt;
    return m_model->noteAt(source.row());
}

void MainWindow::onSelectionChanged()
{
    updateActionStates();
    updatePreview();
}

void MainWindow::updateActionStates()
{
    const bool hasSelection = currentNote().has_value();
    m_editAction->setEnabled(hasSelection);
    m_deleteAction->setEnabled(hasSelection);
    m_exportAction->setEnabled(hasSelection);
}

void MainWindow::updatePreview()
{
    const std::optional<Note> note = currentNote();
    if (!note) {
        m_preview->clear();
        return;
    }
    QString markdown = QStringLiteral("# ") + note->title + QStringLiteral("\n\n");
    if (!note->tags.trimmed().isEmpty())
        markdown += tr("*Tags: %1*").arg(note->tags.trimmed()) + QStringLiteral("\n\n");
    markdown += note->content;
    m_preview->setMarkdown(markdown);
}

void MainWindow::addNote()
{
    NoteDialog::Init init;
    init.draftKey = QUuid::createUuid().toString(QUuid::WithoutBraces);
    runEditor(init);
}

void MainWindow::editSelected()
{
    const std::optional<Note> note = currentNote();
    if (!note)
        return;
    NoteDialog::Init init;
    init.note = *note;
    init.baseUpdatedAt = note->updatedAt;
    init.draftKey = QUuid::createUuid().toString(QUuid::WithoutBraces);
    runEditor(init);
}

void MainWindow::runEditor(const NoteDialog::Init &init)
{
    NoteDialog dialog(init, this);
    const QString key = init.draftKey;

    connect(&dialog, &NoteDialog::draftAutosave, &m_controller, &NoteController::saveDraft);
    connect(&m_controller, &NoteController::draftSaved, &dialog,
            [&dialog, key](const QString &savedKey, const QDateTime &when) {
                if (savedKey == key)
                    dialog.showDraftSaved(when);
            });

    if (dialog.exec() == QDialog::Accepted) {
        // Persist the draft first so the text survives even if saving fails.
        m_controller.saveDraft(dialog.draft());
        m_controller.saveNote(dialog.note(), key);
    } else {
        m_controller.deleteDraft(key);
    }
}

void MainWindow::deleteSelected()
{
    const std::optional<Note> note = currentNote();
    if (!note)
        return;

    QMessageBox box(QMessageBox::Question, tr("Delete note"),
                    tr("Delete the note \"%1\"?").arg(note->title), QMessageBox::NoButton, this);
    QPushButton *yes = box.addButton(tr("Delete"), QMessageBox::DestructiveRole);
    box.addButton(tr("Cancel"), QMessageBox::RejectRole);
    box.exec();
    if (box.clickedButton() == yes)
        m_controller.deleteNote(note->id);
}

void MainWindow::exportSelected()
{
    const std::optional<Note> note = currentNote();
    if (!note)
        return;

    QString suggested = note->title;
    suggested.replace(QRegExp(QStringLiteral("[\\\\/:*?\"<>|]")), QStringLiteral("_"));
    QString selectedFilter;
    QString path = QFileDialog::getSaveFileName(
        this, tr("Export note"), QDir::home().filePath(suggested + QStringLiteral(".txt")),
        tr("Text files (*.txt);;Markdown files (*.md)"), &selectedFilter);
    if (path.isEmpty())
        return;
    if (QFileInfo(path).suffix().isEmpty())
        path += selectedFilter.contains(QStringLiteral("*.md")) ? QStringLiteral(".md")
                                                                 : QStringLiteral(".txt");

    statusBar()->showMessage(tr("Exporting..."), 3000);
    m_controller.exportNote(note->id, path);
}

void MainWindow::onLanguageChosen(QAction *action)
{
    const QString code = action->data().toString();
    if (m_language.apply(code))
        m_settings.setLanguage(code);
}

// ---------------------------------------------------------------------------
// Draft recovery
// ---------------------------------------------------------------------------

void MainWindow::onDraftsLoaded(const QVector<DraftInfo> &drafts)
{
    if (m_recovering || drafts.isEmpty())
        return;
    m_recovering = true;

    for (const DraftInfo &info : drafts) {
        const Draft &draft = info.draft;
        const QString title = draft.title.isEmpty() ? tr("(untitled)") : draft.title;
        const QString when = draft.savedAt.isValid()
                                 ? QLocale().toString(draft.savedAt.toLocalTime(), QLocale::ShortFormat)
                                 : QStringLiteral("-");

        QString text;
        switch (info.status) {
        case DraftStatus::NewNote:
            text = tr("An unsaved new note \"%1\" was found (draft from %2).\nRestore it?");
            break;
        case DraftStatus::Clean:
            text = tr("Unsaved changes to the note \"%1\" were found (draft from %2).\nRestore them?");
            break;
        case DraftStatus::Conflict:
            text = tr("Unsaved changes to the note \"%1\" were found (draft from %2), but the note "
                      "was modified after the draft was started.\nRestoring will overwrite the "
                      "saved version. Restore the draft?");
            break;
        case DraftStatus::NoteDeleted:
            text = tr("An unsaved draft of the deleted note \"%1\" was found (draft from %2).\n"
                      "Restore it as a new note?");
            break;
        }

        QMessageBox box(QMessageBox::Question, tr("Recover unsaved changes"), text.arg(title, when),
                        QMessageBox::NoButton, this);
        QPushButton *restore = box.addButton(tr("Restore"), QMessageBox::AcceptRole);
        QPushButton *discard = box.addButton(tr("Discard"), QMessageBox::DestructiveRole);
        box.addButton(tr("Decide later"), QMessageBox::RejectRole);
        box.exec();

        if (box.clickedButton() == discard) {
            m_controller.deleteDraft(draft.key);
        } else if (box.clickedButton() == restore) {
            NoteDialog::Init init;
            init.draftKey = draft.key;
            init.restored = true;
            init.note.title = draft.title;
            init.note.content = draft.content;
            init.note.tags = draft.tags;
            if (info.status != DraftStatus::NoteDeleted) {
                init.note.id = draft.noteId;
                init.baseUpdatedAt = draft.baseUpdatedAt;
                if (info.current) {
                    init.note.createdAt = info.current->createdAt;
                    init.note.updatedAt = info.current->updatedAt;
                }
            }
            runEditor(init);
        }
    }
    m_recovering = false;
}
