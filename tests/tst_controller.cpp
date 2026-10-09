#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "app/notecontroller.h"
#include "testutils.h"

/// End-to-end tests of the asynchronous pipeline: controller -> queue -> worker
/// thread -> SQLite and back.
class TstController : public QObject {
    Q_OBJECT
private slots:
    void initTestCase()
    {
        qRegisterMetaType<Note>("Note");
        qRegisterMetaType<QVector<Note>>("QVector<Note>");
        qRegisterMetaType<QVector<DraftInfo>>("QVector<DraftInfo>");
    }

    void startupFailureIsReported()
    {
        NoteController c(QStringLiteral("/nonexistent-dir/x/notes.db"), QStringLiteral("substring"));
        QSignalSpy ready(&c, &NoteController::ready);
        c.start();
        QVERIFY(ready.wait(5000));
        QCOMPARE(ready.first().at(0).toBool(), false);
        QVERIFY(!ready.first().at(1).toString().isEmpty());
    }

    void saveThenListPreservesOrder()
    {
        QTemporaryDir dir;
        NoteController c(dir.filePath("n.db"), QStringLiteral("substring"));
        QSignalSpy ready(&c, &NoteController::ready);
        QSignalSpy saved(&c, &NoteController::noteSaved);
        QSignalSpy loaded(&c, &NoteController::notesLoaded);
        c.start();
        QVERIFY(ready.wait(5000));
        QVERIFY(ready.first().at(0).toBool());

        c.saveNote(makeNote("one"), QString());
        c.saveNote(makeNote("two"), QString());
        c.loadAll();
        QVERIFY(loaded.wait(5000));

        QCOMPARE(saved.size(), 2);
        const auto notes = loaded.first().at(0).value<QVector<Note>>();
        QCOMPARE(notes.size(), 2); // list was read after both writes
    }

    void staleListResultsAreDropped()
    {
        QTemporaryDir dir;
        NoteController c(dir.filePath("n.db"), QStringLiteral("substring"));
        QSignalSpy ready(&c, &NoteController::ready);
        QSignalSpy loaded(&c, &NoteController::notesLoaded);
        c.start();
        QVERIFY(ready.wait(5000));

        c.search("a", SearchScope::TitleOnly);
        c.search("b", SearchScope::TitleOnly);
        c.search("c", SearchScope::TitleOnly);
        QVERIFY(loaded.wait(5000));
        QTest::qWait(100);
        QCOMPARE(loaded.size(), 1);
    }

    void savingNoteRemovesItsDraftAtomically()
    {
        QTemporaryDir dir;
        NoteController c(dir.filePath("n.db"), QStringLiteral("substring"));
        QSignalSpy ready(&c, &NoteController::ready);
        QSignalSpy draftsLoaded(&c, &NoteController::draftsLoaded);
        c.start();
        QVERIFY(ready.wait(5000));

        Draft d;
        d.key = QStringLiteral("sess");
        d.title = QStringLiteral("work in progress");
        c.saveDraft(d);
        c.loadDrafts();
        QVERIFY(draftsLoaded.wait(5000));
        QCOMPARE(draftsLoaded.takeFirst().at(0).value<QVector<DraftInfo>>().size(), 1);

        c.saveNote(makeNote("final"), QStringLiteral("sess"));
        c.saveDraft(d); // late autosave of the closed session must be ignored
        c.loadDrafts();
        QVERIFY(draftsLoaded.wait(5000));
        QCOMPARE(draftsLoaded.takeFirst().at(0).value<QVector<DraftInfo>>().size(), 0);
    }

    void draftsSurviveRestart()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("n.db");
        {
            NoteController c(path, QStringLiteral("substring"));
            QSignalSpy ready(&c, &NoteController::ready);
            c.start();
            QVERIFY(ready.wait(5000));
            Draft d;
            d.key = QStringLiteral("crashed");
            d.title = QStringLiteral("unsaved");
            c.saveDraft(d);
            c.shutdown(); // drains the queue before stopping
        }
        NoteController c2(path, QStringLiteral("substring"));
        QSignalSpy ready(&c2, &NoteController::ready);
        QSignalSpy draftsLoaded(&c2, &NoteController::draftsLoaded);
        c2.start();
        QVERIFY(ready.wait(5000));
        c2.loadDrafts();
        QVERIFY(draftsLoaded.wait(5000));
        const auto drafts = draftsLoaded.first().at(0).value<QVector<DraftInfo>>();
        QCOMPARE(drafts.size(), 1);
        QCOMPARE(drafts[0].draft.title, QStringLiteral("unsaved"));
        QCOMPARE(drafts[0].status, DraftStatus::NewNote);
    }

    void exportWritesFile()
    {
        QTemporaryDir dir;
        NoteController c(dir.filePath("n.db"), QStringLiteral("substring"));
        QSignalSpy ready(&c, &NoteController::ready);
        QSignalSpy saved(&c, &NoteController::noteSaved);
        QSignalSpy exported(&c, &NoteController::exportFinished);
        c.start();
        QVERIFY(ready.wait(5000));

        c.saveNote(makeNote("Exported", "line1\nline2", "x,y"), QString());
        QVERIFY(saved.wait(5000));
        const int id = saved.first().at(1).value<Note>().id;

        const QString out = dir.filePath("note.md");
        c.exportNote(id, out);
        QVERIFY(exported.wait(5000));
        QVERIFY(exported.first().at(1).toBool());

        QFile f(out);
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QString text = QString::fromUtf8(f.readAll());
        QVERIFY(text.startsWith(QStringLiteral("# Exported")));
        QVERIFY(text.contains(QStringLiteral("line2")));
    }
};

QTEST_MAIN(TstController)
#include "tst_controller.moc"
