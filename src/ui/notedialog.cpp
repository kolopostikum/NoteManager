#include "ui/notedialog.h"

#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include "domain/notevalidator.h"

namespace {

constexpr int kDefaultAutosaveMs = 30 * 1000;

QString formatDate(const QDateTime &dt)
{
    if (!dt.isValid())
        return QStringLiteral("-");
    return QLocale().toString(dt.toLocalTime(), QLocale::LongFormat);
}

} // namespace

NoteDialog::NoteDialog(const Init &init, QWidget *parent)
    : QDialog(parent),
      m_original(init.note),
      m_draftKey(init.draftKey),
      m_baseUpdatedAt(init.baseUpdatedAt),
      m_restored(init.restored)
{
    // Independent top-level window (not glued to MainWindow via WM_TRANSIENT_FOR).
    setWindowFlags(Qt::Window);
    setModal(false);
    resize(560, 520);

    m_titleEdit = new QLineEdit(m_original.title, this);
    m_titleEdit->setMaxLength(NoteValidator::kMaxTitleLength);
    m_tagsEdit = new QLineEdit(m_original.tags, this);
    m_contentEdit = new QPlainTextEdit(this);
    m_contentEdit->setPlainText(m_original.content);

    m_createdEdit = new QLineEdit(formatDate(m_original.createdAt), this);
    m_createdEdit->setReadOnly(true);
    m_updatedEdit = new QLineEdit(formatDate(m_original.updatedAt), this);
    m_updatedEdit->setReadOnly(true);

    m_titleLabel = new QLabel(this);
    m_tagsLabel = new QLabel(this);
    m_contentLabel = new QLabel(this);
    m_createdLabel = new QLabel(this);
    m_updatedLabel = new QLabel(this);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setStyleSheet(QStringLiteral("color: #c0392b;"));
    m_draftLabel = new QLabel(this);
    m_draftLabel->setStyleSheet(QStringLiteral("color: gray;"));

    m_buttons = new QDialogButtonBox(this);
    QPushButton *save = m_buttons->addButton(QDialogButtonBox::Save);
    save->setShortcut(QKeySequence::Save); // Ctrl+S
    m_buttons->addButton(QDialogButtonBox::Cancel);
    connect(m_buttons, &QDialogButtonBox::accepted, this, &NoteDialog::accept);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &NoteDialog::reject);

    auto *form = new QFormLayout;
    form->addRow(m_titleLabel, m_titleEdit);
    form->addRow(QString(), m_errorLabel);
    form->addRow(m_tagsLabel, m_tagsEdit);
    form->addRow(m_contentLabel, m_contentEdit);
    form->addRow(m_createdLabel, m_createdEdit);
    form->addRow(m_updatedLabel, m_updatedEdit);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    auto *bottom = new QHBoxLayout;
    bottom->addWidget(m_draftLabel, 1);
    bottom->addWidget(m_buttons);
    layout->addLayout(bottom);

    m_autosaveTimer = new QTimer(this);
    m_autosaveTimer->setInterval(kDefaultAutosaveMs);
    connect(m_autosaveTimer, &QTimer::timeout, this, &NoteDialog::onAutosaveTimer);
    m_autosaveTimer->start();

    connect(m_titleEdit, &QLineEdit::textChanged, this, &NoteDialog::onEdited);
    connect(m_tagsEdit, &QLineEdit::textChanged, this, &NoteDialog::onEdited);
    connect(m_contentEdit, &QPlainTextEdit::textChanged, this, &NoteDialog::onEdited);

    m_dirtySinceAutosave = m_restored;
    retranslate();
    updateValidation();
    m_titleEdit->setFocus();
}

void NoteDialog::retranslate()
{
    setWindowTitle(m_original.isNew() ? tr("New note") : tr("Edit note"));
    m_titleLabel->setText(tr("Title:"));
    m_tagsLabel->setText(tr("Tags:"));
    m_contentLabel->setText(tr("Text:"));
    m_createdLabel->setText(tr("Created:"));
    m_updatedLabel->setText(tr("Modified:"));
    m_tagsEdit->setPlaceholderText(tr("comma, separated"));
    m_titleEdit->setPlaceholderText(tr("Required, up to %1 characters").arg(NoteValidator::kMaxTitleLength));
    m_buttons->button(QDialogButtonBox::Save)->setText(tr("Save"));
    m_buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    if (m_hasDraftStatus)
        showDraftSaved(m_lastDraftTime);
    updateValidation();
}

void NoteDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        retranslate();
    QDialog::changeEvent(event);
}

Note NoteDialog::note() const
{
    Note n = m_original;
    n.title = m_titleEdit->text();
    n.tags = m_tagsEdit->text();
    n.content = m_contentEdit->toPlainText();
    return n;
}

Draft NoteDialog::draft() const
{
    Draft d;
    d.key = m_draftKey;
    d.noteId = m_original.id;
    d.title = m_titleEdit->text();
    d.tags = m_tagsEdit->text();
    d.content = m_contentEdit->toPlainText();
    d.baseUpdatedAt = m_baseUpdatedAt;
    return d;
}

bool NoteDialog::isModified() const
{
    if (m_restored)
        return true;
    return m_titleEdit->text() != m_original.title || m_tagsEdit->text() != m_original.tags
           || m_contentEdit->toPlainText() != m_original.content;
}

void NoteDialog::setAutosaveInterval(int milliseconds)
{
    m_autosaveTimer->setInterval(milliseconds);
    m_autosaveTimer->start();
}

void NoteDialog::showDraftSaved(const QDateTime &when)
{
    m_hasDraftStatus = true;
    m_lastDraftTime = when;
    m_draftLabel->setText(
        tr("Draft saved at %1").arg(QLocale().toString(when.toLocalTime().time(), QLocale::ShortFormat)));
}

void NoteDialog::detachAsNewNote()
{
    m_original.id = 0;
    m_original.createdAt = QDateTime();
    m_original.updatedAt = QDateTime();
    m_baseUpdatedAt = QDateTime();
    m_restored = true;
    m_createdEdit->setText(formatDate(QDateTime()));
    m_updatedEdit->setText(formatDate(QDateTime()));
    retranslate();
}

void NoteDialog::onEdited()
{
    m_dirtySinceAutosave = true;
    updateValidation();
}

void NoteDialog::updateValidation()
{
    QString message;
    switch (NoteValidator::validateTitle(m_titleEdit->text())) {
    case NoteValidator::Error::None:
        break;
    case NoteValidator::Error::TitleEmpty:
        if (isModified() || !m_original.isNew())
            message = tr("Title is required.");
        break;
    case NoteValidator::Error::TitleTooLong:
        message = tr("Title must not exceed %1 characters.").arg(NoteValidator::kMaxTitleLength);
        break;
    }
    m_errorLabel->setText(message);
}

void NoteDialog::onAutosaveTimer()
{
    if (!m_dirtySinceAutosave)
        return;
    const bool nothingTyped = m_original.isNew() && m_titleEdit->text().isEmpty()
                              && m_tagsEdit->text().isEmpty() && m_contentEdit->toPlainText().isEmpty();
    m_dirtySinceAutosave = false;
    if (nothingTyped)
        return;
    emit draftAutosave(draft());
}

bool NoteDialog::emitSaveIfValid()
{
    const NoteValidator::Error error = NoteValidator::validateTitle(m_titleEdit->text());
    if (error != NoteValidator::Error::None) {
        m_errorLabel->setText(error == NoteValidator::Error::TitleEmpty
                                  ? tr("Title is required.")
                                  : tr("Title must not exceed %1 characters.")
                                        .arg(NoteValidator::kMaxTitleLength));
        m_titleEdit->setFocus();
        return false;
    }
    m_autosaveTimer->stop();
    m_closing = true;
    emit saveRequested(note(), m_draftKey);
    return true;
}

void NoteDialog::emitDiscardAndClose()
{
    m_autosaveTimer->stop();
    m_closing = true;
    emit discardRequested(m_draftKey);
}

void NoteDialog::accept()
{
    if (!emitSaveIfValid())
        return;
    done(QDialog::Accepted);
}

void NoteDialog::reject()
{
    if (!tryClose())
        return;
    done(QDialog::Rejected);
}

bool NoteDialog::tryClose()
{
    if (m_closing)
        return true;

    if (isModified()) {
        QMessageBox box(QMessageBox::Question, tr("Unsaved changes"),
                        tr("The note has unsaved changes."), QMessageBox::NoButton, this);
        QPushButton *saveButton = box.addButton(tr("Save"), QMessageBox::AcceptRole);
        QPushButton *discardButton = box.addButton(tr("Discard"), QMessageBox::DestructiveRole);
        box.addButton(tr("Keep editing"), QMessageBox::RejectRole);
        box.exec();

        if (box.clickedButton() == saveButton)
            return emitSaveIfValid();
        if (box.clickedButton() != discardButton)
            return false;
    }

    emitDiscardAndClose();
    return true;
}

void NoteDialog::closeEvent(QCloseEvent *event)
{
    if (m_closing) {
        event->accept();
        return;
    }
    if (!tryClose()) {
        event->ignore();
        return;
    }
    event->accept();
}
