#include <QtTest>

#include "domain/draftservice.h"
#include "domain/noteservice.h"
#include "domain/substringsearchstrategy.h"
#include "storage/sqlitedraftrepository.h"
#include "storage/sqlitenoterepository.h"
#include "testutils.h"

class TstServices : public QObject {
    Q_OBJECT
private slots:
    void saveAssignsIdAndTimestamps()
    {
        TempDb db;
        QVERIFY(db.open());
        NoteService svc(std::make_shared<SqliteNoteRepository>(db.name()),
                        std::make_shared<SubstringSearchStrategy>());

        const SaveResult created = svc.save(makeNote("  Hello  "));
        QVERIFY(created.ok);
        QVERIFY(created.note.id > 0);
        QCOMPARE(created.note.title, QStringLiteral("Hello"));
        QCOMPARE(created.note.createdAt, created.note.updatedAt);

        QTest::qWait(5);
        Note edited = created.note;
        edited.content = QStringLiteral("body");
        const SaveResult updated = svc.save(edited);
        QVERIFY(updated.ok);
        QCOMPARE(updated.note.createdAt, created.note.createdAt);
        QVERIFY(updated.note.updatedAt > created.note.updatedAt);
    }

    void invalidTitleIsRejected()
    {
        TempDb db;
        QVERIFY(db.open());
        NoteService svc(std::make_shared<SqliteNoteRepository>(db.name()),
                        std::make_shared<SubstringSearchStrategy>());
        const SaveResult r = svc.save(makeNote(""));
        QVERIFY(!r.ok);
        QCOMPARE(r.validation, NoteValidator::Error::TitleEmpty);
        QCOMPARE(svc.list().notes.size(), 0);
    }

    void updateOfMissingNoteFails()
    {
        TempDb db;
        QVERIFY(db.open());
        NoteService svc(std::make_shared<SqliteNoteRepository>(db.name()),
                        std::make_shared<SubstringSearchStrategy>());
        Note n = makeNote("ghost");
        n.id = 42;
        QVERIFY(!svc.save(n).ok);
    }

    void searchUsesStrategy()
    {
        TempDb db;
        QVERIFY(db.open());
        NoteService svc(std::make_shared<SqliteNoteRepository>(db.name()),
                        std::make_shared<SubstringSearchStrategy>());
        QVERIFY(svc.save(makeNote(QString::fromUtf8("Рецепты"), "borsch")).ok);
        QVERIFY(svc.save(makeNote("Work", "meeting")).ok);

        QCOMPARE(svc.search(QString::fromUtf8("РЕЦЕПТ"), SearchScope::TitleOnly).notes.size(), 1);
        QCOMPARE(svc.search("meet", SearchScope::TitleOnly).notes.size(), 0);
        QCOMPARE(svc.search("meet", SearchScope::TitleAndContent).notes.size(), 1);
    }

    void draftClassification()
    {
        Note current = makeNote("n");
        current.id = 1;
        current.updatedAt = QDateTime::currentDateTimeUtc();

        Draft d;
        QCOMPARE(DraftService::classify(d, std::nullopt), DraftStatus::NewNote);

        d.noteId = 1;
        d.baseUpdatedAt = current.updatedAt;
        QCOMPARE(DraftService::classify(d, current), DraftStatus::Clean);
        QCOMPARE(DraftService::classify(d, std::nullopt), DraftStatus::NoteDeleted);

        d.baseUpdatedAt = current.updatedAt.addSecs(-10);
        QCOMPARE(DraftService::classify(d, current), DraftStatus::Conflict);
    }

    void closedSessionIgnoresLateDraftWrites()
    {
        TempDb db;
        QVERIFY(db.open());
        auto notes = std::make_shared<SqliteNoteRepository>(db.name());
        auto drafts = std::make_shared<SqliteDraftRepository>(db.name());
        DraftService svc(drafts, notes);

        Draft d;
        d.key = QStringLiteral("session");
        d.title = QStringLiteral("t");
        QVERIFY(svc.save(d));
        QVERIFY(d.savedAt.isValid());
        QCOMPARE(drafts->getAll().size(), 1);

        QVERIFY(svc.discard(d.key));
        QCOMPARE(drafts->getAll().size(), 0);

        Draft late = d;
        QVERIFY(svc.save(late));
        QVERIFY(!late.savedAt.isValid());
        QCOMPARE(drafts->getAll().size(), 0);
    }

    void recoverReportsStatuses()
    {
        TempDb db;
        QVERIFY(db.open());
        auto notes = std::make_shared<SqliteNoteRepository>(db.name());
        auto drafts = std::make_shared<SqliteDraftRepository>(db.name());
        NoteService noteSvc(notes, std::make_shared<SubstringSearchStrategy>());
        DraftService svc(drafts, notes);

        const Note saved = noteSvc.save(makeNote("existing")).note;

        Draft clean;
        clean.key = "a";
        clean.noteId = saved.id;
        clean.baseUpdatedAt = saved.updatedAt;
        Draft fresh;
        fresh.key = "b";
        Draft orphan;
        orphan.key = "c";
        orphan.noteId = 999;
        QVERIFY(svc.save(clean));
        QVERIFY(svc.save(fresh));
        QVERIFY(svc.save(orphan));

        QVector<DraftInfo> infos;
        QVERIFY(svc.recover(infos));
        QCOMPARE(infos.size(), 3);
        QHash<QString, DraftStatus> byKey;
        for (const auto &i : infos)
            byKey[i.draft.key] = i.status;
        QCOMPARE(byKey["a"], DraftStatus::Clean);
        QCOMPARE(byKey["b"], DraftStatus::NewNote);
        QCOMPARE(byKey["c"], DraftStatus::NoteDeleted);
    }
};

QTEST_APPLESS_MAIN(TstServices)
#include "tst_services.moc"
