#include <QtTest>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QTimer>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QTableView>
#include <QTemporaryDir>
#include <QTextBrowser>

#include "app/languagemanager.h"
#include "app/notecontroller.h"
#include "app/qsettingsadapter.h"
#include "testutils.h"
#include "ui/mainwindow.h"
#include "ui/notedialog.h"

class TstUi : public QObject {
    Q_OBJECT
private slots:
    void dialogRejectsEmptyTitle()
    {
        NoteDialog::Init init;
        init.draftKey = QStringLiteral("k");
        NoteDialog dlg(init);
        dlg.show();

        auto *save = dlg.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save);
        QTest::mouseClick(save, Qt::LeftButton);
        QVERIFY(dlg.isVisible()); // validation blocked the dialog from closing
        QVERIFY(dlg.result() != QDialog::Accepted);
    }

    void dialogEnforcesTitleLength()
    {
        NoteDialog::Init init;
        NoteDialog dlg(init);
        auto *title = dlg.findChildren<QLineEdit *>().first();
        title->setText(QString(150, QLatin1Char('x')));
        QCOMPARE(title->text().size(), 100); // maxLength guard
    }

    void dialogAcceptsValidNote()
    {
        NoteDialog::Init init;
        init.draftKey = QStringLiteral("k");
        NoteDialog dlg(init);
        dlg.show();
        dlg.findChildren<QLineEdit *>().first()->setText(QStringLiteral("My note"));
        dlg.findChild<QPlainTextEdit *>()->setPlainText(QStringLiteral("line1\nline2"));

        QTest::mouseClick(dlg.findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save), Qt::LeftButton);
        QCOMPARE(dlg.result(), int(QDialog::Accepted));
        QCOMPARE(dlg.note().title, QStringLiteral("My note"));
        QCOMPARE(dlg.note().content, QStringLiteral("line1\nline2"));
    }

    void dialogDateFieldsAreReadOnly()
    {
        NoteDialog::Init init;
        init.note = makeNote("n");
        init.note.id = 1;
        init.note.createdAt = init.note.updatedAt = QDateTime::currentDateTimeUtc();
        NoteDialog dlg(init);
        const auto edits = dlg.findChildren<QLineEdit *>();
        // title, tags, created, updated
        QCOMPARE(edits.size(), 4);
        QVERIFY(!edits[0]->isReadOnly());
        QVERIFY(!edits[1]->isReadOnly());
        QVERIFY(edits[2]->isReadOnly());
        QVERIFY(edits[3]->isReadOnly());
    }

    void dialogAutosavesOnlyWhenModified()
    {
        NoteDialog::Init init;
        init.note = makeNote("n");
        init.note.id = 3;
        init.draftKey = QStringLiteral("sess");
        init.baseUpdatedAt = QDateTime::currentDateTimeUtc();
        NoteDialog dlg(init);
        dlg.setAutosaveInterval(40);
        QSignalSpy spy(&dlg, &NoteDialog::draftAutosave);

        QTest::qWait(150);
        QCOMPARE(spy.size(), 0); // pristine form: nothing to save

        dlg.findChild<QPlainTextEdit *>()->setPlainText(QStringLiteral("edited"));
        QTRY_COMPARE(spy.size(), 1);
        const Draft d = spy.first().at(0).value<Draft>();
        QCOMPARE(d.key, QStringLiteral("sess"));
        QCOMPARE(d.noteId, 3);
        QCOMPARE(d.content, QStringLiteral("edited"));
        QCOMPARE(d.baseUpdatedAt, init.baseUpdatedAt);

        QTest::qWait(150);
        QCOMPARE(spy.size(), 1); // no new edits -> no new draft
    }

    void crashedDraftIsOfferedAndRestored()
    {
        QTemporaryDir dir;
        const QString dbPath = dir.filePath("n.db");

        // "Previous run": an autosaved draft that was never saved as a note.
        {
            NoteController before(dbPath, QStringLiteral("substring"));
            QSignalSpy ready(&before, &NoteController::ready);
            before.start();
            QVERIFY(ready.wait(5000));
            Draft d;
            d.key = QStringLiteral("crashed-session");
            d.title = QStringLiteral("Lost idea");
            d.content = QStringLiteral("text that must not be lost");
            before.saveDraft(d);
            before.shutdown();
        }

        // New run: answer the recovery prompt, then save from the editor.
        bool sawPrompt = false;
        bool sawEditor = false;
        QTimer poller;
        poller.setInterval(30);
        connect(&poller, &QTimer::timeout, this, [&]() {
            QWidget *modal = QApplication::activeModalWidget();
            if (!modal)
                return;
            if (auto *dlg = qobject_cast<NoteDialog *>(modal)) {
                sawEditor = true;
                QCOMPARE(dlg->note().title, QStringLiteral("Lost idea"));
                QCOMPARE(dlg->note().content, QStringLiteral("text that must not be lost"));
                dlg->accept();
            } else if (auto *box = qobject_cast<QMessageBox *>(modal)) {
                sawPrompt = true;
                for (QAbstractButton *b : box->buttons()) {
                    if (b->text() == QLatin1String("Restore"))
                        b->click();
                }
            }
        });
        poller.start();

        QSettingsAdapter settings(dir.filePath("settings.ini"));
        LanguageManager language;
        NoteController controller(dbPath, QStringLiteral("substring"));
        MainWindow window(controller, settings, language);
        window.show();
        controller.start();

        auto *table = window.findChild<QTableView *>();
        QTRY_COMPARE(table->model()->rowCount(), 1);
        QVERIFY(sawPrompt);
        QVERIFY(sawEditor);
        poller.stop();

        // The draft was consumed by the save: nothing is offered next time.
        QSignalSpy drafts(&controller, &NoteController::draftsLoaded);
        controller.loadDrafts();
        QTRY_COMPARE(drafts.size(), 1);
        QCOMPARE(drafts.first().at(0).value<QVector<DraftInfo>>().size(), 0);
        controller.shutdown();
    }

    void mainWindowListSearchAndPreview()
    {
        QTemporaryDir dir;
        QSettingsAdapter settings(dir.filePath("settings.ini"));
        LanguageManager language;
        NoteController controller(dir.filePath("n.db"), QStringLiteral("substring"));
        MainWindow window(controller, settings, language);
        window.show();

        QSignalSpy saved(&controller, &NoteController::noteSaved);
        controller.start();

        controller.saveNote(makeNote("Groceries", "milk and bread"), QString());
        controller.saveNote(makeNote(QString::fromUtf8("Работа"), QString::fromUtf8("созвон с командой")), QString());
        QTRY_COMPARE(saved.size(), 2);

        auto *table = window.findChild<QTableView *>();
        QVERIFY(table);
        QTRY_COMPARE(table->model()->rowCount(), 2);

        // Case-insensitive Cyrillic search through the search box (debounced).
        auto *search = window.findChildren<QLineEdit *>().first();
        search->setText(QString::fromUtf8("РАБОТА"));
        QTRY_COMPARE(table->model()->rowCount(), 1);

        // Content search is on by default.
        search->setText(QStringLiteral("bread"));
        QTRY_COMPARE(table->model()->rowCount(), 1);

        search->clear();
        QTRY_COMPARE(table->model()->rowCount(), 2);

        // Selecting a row fills the preview.
        table->selectRow(0);
        auto *preview = window.findChild<QTextBrowser *>();
        QTRY_VERIFY(!preview->toPlainText().isEmpty());

        controller.shutdown();
    }
};

QTEST_MAIN(TstUi)
#include "tst_ui.moc"
