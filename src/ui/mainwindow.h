#pragma once

#include <QHash>
#include <QMainWindow>
#include <QSet>
#include <QVector>
#include <optional>

#include "app/notecontroller.h"
#include "domain/draft.h"
#include "domain/note.h"
#include "ui/notedialog.h"

class ISettings;
class LanguageManager;
class NoteListModel;
class QAction;
class QActionGroup;
class QCheckBox;
class QCloseEvent;
class QComboBox;
class QLabel;
class QLineEdit;
class QMenu;
class QSortFilterProxyModel;
class QTableView;
class QTextBrowser;
class QTimer;
class QToolButton;

/// Main window: a thin view. It renders the notes delivered by the controller
/// and forwards user intents back to it; it never touches storage directly.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(NoteController &controller, ISettings &settings, LanguageManager &language,
               QWidget *parent = nullptr);

protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onReady(bool ok, const QString &error);
    void onNotesLoaded(const QVector<Note> &notes);
    void onNoteSaved(quint64 requestId, const Note &note);
    void onNoteDeleted(quint64 requestId, int noteId);
    void onDraftsLoaded(const QVector<DraftInfo> &drafts);
    void onExportFinished(quint64 requestId, bool ok, const QString &path, const QString &error);
    void onFailed(quint64 requestId, CommandKind kind, const QString &message);

    void addNote();
    void editSelected();
    void deleteSelected();
    void exportSelected();
    void reload();

    void onSelectionChanged();
    void onSortComboChanged(int index);
    void onHeaderSortChanged(int column, Qt::SortOrder order);
    void onLanguageChosen(QAction *action);

private:
    void buildUi();
    void buildActions();
    void buildMenus();
    void retranslate();
    void updatePreview();
    void updateActionStates();
    void applySort(int column, Qt::SortOrder order);
    void selectNoteById(int noteId);
    void openEditor(const NoteDialog::Init &init);
    void unregisterEditor(NoteDialog *dialog);
    QList<NoteDialog *> openEditors() const;
    std::optional<Note> currentNote() const;

    NoteController &m_controller;
    ISettings &m_settings;
    LanguageManager &m_language;

    NoteListModel *m_model = nullptr;
    QSortFilterProxyModel *m_proxy = nullptr;
    QTableView *m_table = nullptr;
    QTextBrowser *m_preview = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QCheckBox *m_searchInContent = nullptr;
    QComboBox *m_sortCombo = nullptr;
    QLabel *m_sortLabel = nullptr;
    QLabel *m_countLabel = nullptr;
    QTimer *m_searchTimer = nullptr;

    QAction *m_addAction = nullptr;
    QAction *m_editAction = nullptr;
    QAction *m_deleteAction = nullptr;
    QAction *m_refreshAction = nullptr;
    QAction *m_exportAction = nullptr;
    QAction *m_quitAction = nullptr;
    QActionGroup *m_languageGroup = nullptr;
    QMenu *m_fileMenu = nullptr;
    QMenu *m_languageMenu = nullptr;

    /// Editors for existing notes (noteId > 0). Reopening activates the entry.
    QHash<int, NoteDialog *> m_editorsByNoteId;
    /// Editors for brand-new notes (id == 0), keyed by draft session.
    QHash<QString, NoteDialog *> m_editorsByDraftKey;

    int m_pendingSelectId = 0;
    bool m_recovering = false;
};
