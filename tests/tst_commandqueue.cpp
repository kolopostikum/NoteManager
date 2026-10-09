#include <QtTest>

#include "storage/commandqueue.h"

namespace {
Command draft(const QString &key, const QString &title, quint64 id = 0)
{
    SaveDraftCmd c;
    c.requestId = id;
    c.draft.key = key;
    c.draft.title = title;
    return c;
}
} // namespace

class TstCommandQueue : public QObject {
    Q_OBJECT
private slots:
    void fifoOrder()
    {
        CommandQueue q;
        q.push(LoadAllCmd{1});
        q.push(DeleteNoteCmd{2, 5});
        q.push(LoadDraftsCmd{3});
        QCOMPARE(q.size(), std::size_t(3));
        QCOMPARE(std::get<LoadAllCmd>(*q.tryPop()).requestId, quint64(1));
        QCOMPARE(std::get<DeleteNoteCmd>(*q.tryPop()).noteId, 5);
        QVERIFY(std::holds_alternative<LoadDraftsCmd>(*q.tryPop()));
        QVERIFY(!q.tryPop().has_value());
    }

    void consecutiveDraftsOfSameSessionAreCoalesced()
    {
        CommandQueue q;
        q.push(draft("s", "one"));
        q.push(draft("s", "two"));
        q.push(draft("s", "three"));
        QCOMPARE(q.size(), std::size_t(1));
        QCOMPARE(std::get<SaveDraftCmd>(*q.tryPop()).draft.title, QStringLiteral("three"));
    }

    void differentSessionsAreNotCoalesced()
    {
        CommandQueue q;
        q.push(draft("a", "x"));
        q.push(draft("b", "y"));
        QCOMPARE(q.size(), std::size_t(2));
    }

    void draftAfterSaveIsNotMovedBeforeIt()
    {
        CommandQueue q;
        q.push(draft("s", "old"));
        SaveNoteCmd save;
        save.draftKey = QStringLiteral("s");
        q.push(save);
        q.push(draft("s", "late")); // must stay after SaveNote

        QCOMPARE(q.size(), std::size_t(3));
        QVERIFY(std::holds_alternative<SaveDraftCmd>(*q.tryPop()));
        QVERIFY(std::holds_alternative<SaveNoteCmd>(*q.tryPop()));
        QCOMPARE(std::get<SaveDraftCmd>(*q.tryPop()).draft.title, QStringLiteral("late"));
    }

    void unrelatedCommandsDoNotBlockCoalescing()
    {
        CommandQueue q;
        q.push(draft("s", "one"));
        q.push(LoadAllCmd{9});
        q.push(draft("s", "two"));
        QCOMPARE(q.size(), std::size_t(2));
        QCOMPARE(std::get<SaveDraftCmd>(*q.tryPop()).draft.title, QStringLiteral("two"));
    }
};

QTEST_APPLESS_MAIN(TstCommandQueue)
#include "tst_commandqueue.moc"
