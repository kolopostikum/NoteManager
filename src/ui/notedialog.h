#pragma once

#include <QDateTime>
#include <QDialog>
#include <QString>

#include "domain/draft.h"
#include "domain/note.h"

class QCloseEvent;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QTimer;

/// Modeless top-level editor for one note. It validates input synchronously
/// (pure rules, no storage access), tracks changes and periodically emits the
/// current state as a draft; persisting is done by whoever listens to the signals.
class NoteDialog : public QDialog {
    Q_OBJECT
public:
    struct Init {
        Note note;                 ///< Content to show (for a restored draft: the draft text).
        QString draftKey;          ///< Editing-session token.
        QDateTime baseUpdatedAt;   ///< notes.updated_at the editing started from.
        bool restored = false;     ///< Opened from a recovered draft: counts as modified.
    };

    /// Always create with parent == nullptr so the window is not transient for MainWindow.
    explicit NoteDialog(const Init &init, QWidget *parent = nullptr);

    /// Note assembled from the form fields (id and created date are preserved).
    Note note() const;
    Draft draft() const;
    const QString &draftKey() const { return m_draftKey; }
    int noteId() const { return m_original.id; }

    void setAutosaveInterval(int milliseconds);
    /// Called when the storage layer confirmed that the draft was written.
    void showDraftSaved(const QDateTime &when);

    bool isModified() const;

    /// The note was deleted while this editor was open: keep the text as a new note.
    void detachAsNewNote();

    /// Ask about unsaved changes if needed, then emit save/discard and close.
    /// Returns false if the user chose to keep editing.
    bool tryClose();

signals:
    /// Emitted every autosave interval if there were edits since the last time.
    void draftAutosave(const Draft &draft);
    /// Validated note ready to persist; the dialog will close immediately after.
    void saveRequested(const Note &note, const QString &draftKey);
    /// User discarded the session; the dialog will close immediately after.
    void discardRequested(const QString &draftKey);

public slots:
    void accept() override;
    void reject() override;

protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void retranslate();
    void updateValidation();
    void onEdited();
    void onAutosaveTimer();
    bool emitSaveIfValid();
    void emitDiscardAndClose();

    Note m_original;
    QString m_draftKey;
    QDateTime m_baseUpdatedAt;
    bool m_restored = false;
    bool m_dirtySinceAutosave = false;
    bool m_hasDraftStatus = false;
    bool m_closing = false;
    QDateTime m_lastDraftTime;

    QLineEdit *m_titleEdit = nullptr;
    QLineEdit *m_tagsEdit = nullptr;
    QPlainTextEdit *m_contentEdit = nullptr;
    QLineEdit *m_createdEdit = nullptr;
    QLineEdit *m_updatedEdit = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_tagsLabel = nullptr;
    QLabel *m_contentLabel = nullptr;
    QLabel *m_createdLabel = nullptr;
    QLabel *m_updatedLabel = nullptr;
    QLabel *m_errorLabel = nullptr;
    QLabel *m_draftLabel = nullptr;
    QDialogButtonBox *m_buttons = nullptr;
    QTimer *m_autosaveTimer = nullptr;
};
