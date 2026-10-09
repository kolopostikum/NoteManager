#include <QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>

#include "storage/sqlitedraftrepository.h"
#include "storage/sqlitenoterepository.h"
#include "storage/sqlitetransactionmanager.h"
#include "testutils.h"

class TstRepository : public QObject {
    Q_OBJECT
private slots:
    void schemaIsCreatedOnFirstOpen()
    {
        TempDb db;
        QVERIFY(db.open());
        QVERIFY(QFileInfo::exists(db.path));

        QSqlQuery q(QSqlDatabase::database(db.name()));
        QVERIFY(q.exec(QStringLiteral("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name")));
        QStringList tables;
        while (q.next())
            tables << q.value(0).toString();
        QVERIFY(tables.contains(QStringLiteral("notes")));
        QVERIFY(tables.contains(QStringLiteral("drafts")));
    }

    void crud()
    {
        TempDb db;
        QVERIFY(db.open());
        SqliteNoteRepository repo(db.name());

        Note n = makeNote("Title", "Body", "a,b");
        n.createdAt = n.updatedAt = QDateTime::currentDateTimeUtc();
        QVERIFY(repo.add(n));
        const int id = repo.lastInsertId();
        QVERIFY(id > 0);

        auto loaded = repo.getById(id);
        QVERIFY(loaded.has_value());
        QCOMPARE(loaded->title, QStringLiteral("Title"));
        QCOMPARE(loaded->tags, QStringLiteral("a,b"));
        QCOMPARE(loaded->createdAt.toMSecsSinceEpoch(), n.createdAt.toMSecsSinceEpoch());

        loaded->title = QStringLiteral("Changed");
        QVERIFY(repo.update(*loaded));
        QCOMPARE(repo.getById(id)->title, QStringLiteral("Changed"));
        QCOMPARE(repo.getAll().size(), 1);

        QVERIFY(repo.remove(id));
        QVERIFY(!repo.getById(id).has_value());
        QVERIFY(!repo.remove(id));
        QVERIFY(!repo.lastError().isEmpty());
    }

    void sqlInjectionIsInert()
    {
        TempDb db;
        QVERIFY(db.open());
        SqliteNoteRepository repo(db.name());

        Note n = makeNote(QStringLiteral("x'); DROP TABLE notes; --"), QStringLiteral("' OR '1'='1"));
        n.createdAt = n.updatedAt = QDateTime::currentDateTimeUtc();
        QVERIFY(repo.add(n));
        const auto all = repo.getAll();
        QCOMPARE(all.size(), 1);
        QCOMPARE(all[0].title, n.title);
    }

    void unicodeRoundTrip()
    {
        TempDb db;
        QVERIFY(db.open());
        SqliteNoteRepository repo(db.name());

        Note n = makeNote(QString::fromUtf8("Привет, мир"), QString::fromUtf8("Многострочный\nтекст"));
        n.createdAt = n.updatedAt = QDateTime::currentDateTimeUtc();
        QVERIFY(repo.add(n));
        QCOMPARE(repo.getAll()[0].content, n.content);
    }

    void transactionRollback()
    {
        TempDb db;
        QVERIFY(db.open());
        SqliteNoteRepository repo(db.name());
        SqliteTransactionManager tx(db.name());

        Note n = makeNote("T");
        n.createdAt = n.updatedAt = QDateTime::currentDateTimeUtc();
        {
            TransactionScope scope(tx);
            QVERIFY(scope.active());
            QVERIFY(repo.add(n));
        } // no commit -> rollback
        QCOMPARE(repo.getAll().size(), 0);

        TransactionScope scope(tx);
        QVERIFY(repo.add(n));
        QVERIFY(scope.commit());
        QCOMPARE(repo.getAll().size(), 1);
    }

    void draftsUpsertAndRemove()
    {
        TempDb db;
        QVERIFY(db.open());
        SqliteDraftRepository repo(db.name());

        Draft d;
        d.key = QStringLiteral("k1");
        d.title = QStringLiteral("first");
        d.savedAt = QDateTime::currentDateTimeUtc();
        QVERIFY(repo.upsert(d));
        d.title = QStringLiteral("second");
        d.noteId = 7;
        d.baseUpdatedAt = QDateTime::currentDateTimeUtc();
        QVERIFY(repo.upsert(d));

        const auto all = repo.getAll();
        QCOMPARE(all.size(), 1);
        QCOMPARE(all[0].title, QStringLiteral("second"));
        QCOMPARE(all[0].noteId, 7);
        QVERIFY(all[0].baseUpdatedAt.isValid());

        QVERIFY(repo.remove(QStringLiteral("k1")));
        QCOMPARE(repo.getAll().size(), 0);
    }
};

QTEST_APPLESS_MAIN(TstRepository)
#include "tst_repository.moc"
