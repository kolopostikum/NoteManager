#include <QtTest>

#include "domain/notevalidator.h"

class TstValidator : public QObject {
    Q_OBJECT
private slots:
    void emptyTitleRejected()
    {
        QCOMPARE(NoteValidator::validateTitle(QString()), NoteValidator::Error::TitleEmpty);
        QCOMPARE(NoteValidator::validateTitle(QStringLiteral("   ")), NoteValidator::Error::TitleEmpty);
    }

    void boundaryLength()
    {
        QCOMPARE(NoteValidator::validateTitle(QString(100, QLatin1Char('a'))),
                 NoteValidator::Error::None);
        QCOMPARE(NoteValidator::validateTitle(QString(101, QLatin1Char('a'))),
                 NoteValidator::Error::TitleTooLong);
    }

    void surroundingWhitespaceIgnored()
    {
        const QString padded = QStringLiteral("  ") + QString(100, QLatin1Char('a')) + QStringLiteral("  ");
        QCOMPARE(NoteValidator::validateTitle(padded), NoteValidator::Error::None);
    }
};

QTEST_APPLESS_MAIN(TstValidator)
#include "tst_validator.moc"
