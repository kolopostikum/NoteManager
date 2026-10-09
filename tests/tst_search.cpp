#include <QtTest>

#include "domain/substringsearchstrategy.h"
#include "testutils.h"

class TstSearch : public QObject {
    Q_OBJECT
private slots:
    void emptyQueryReturnsEverything()
    {
        const QVector<Note> notes = {makeNote("a"), makeNote("b")};
        SubstringSearchStrategy s;
        QCOMPARE(s.filter(notes, QString(), SearchScope::TitleOnly).size(), 2);
        QCOMPARE(s.filter(notes, QStringLiteral("  "), SearchScope::TitleOnly).size(), 2);
    }

    void titleOnlyIgnoresContent()
    {
        const QVector<Note> notes = {makeNote("Shopping", "milk"), makeNote("Work", "shopping list")};
        SubstringSearchStrategy s;
        const auto byTitle = s.filter(notes, QStringLiteral("shop"), SearchScope::TitleOnly);
        QCOMPARE(byTitle.size(), 1);
        QCOMPARE(byTitle[0].title, QStringLiteral("Shopping"));
        QCOMPARE(s.filter(notes, QStringLiteral("shop"), SearchScope::TitleAndContent).size(), 2);
    }

    void cyrillicIsCaseInsensitive()
    {
        const QVector<Note> notes = {makeNote(QString::fromUtf8("Заметка"), QString::fromUtf8("Текст ПРО Кошек"))};
        SubstringSearchStrategy s;
        QCOMPARE(s.filter(notes, QString::fromUtf8("заметка"), SearchScope::TitleOnly).size(), 1);
        QCOMPARE(s.filter(notes, QString::fromUtf8("про кошек"), SearchScope::TitleAndContent).size(), 1);
        QCOMPARE(s.filter(notes, QString::fromUtf8("собак"), SearchScope::TitleAndContent).size(), 0);
    }
};

QTEST_APPLESS_MAIN(TstSearch)
#include "tst_search.moc"
