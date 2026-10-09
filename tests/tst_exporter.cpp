#include <QtTest>
#include <QTemporaryDir>

#include "app/noteexporter.h"
#include "testutils.h"

class TstExporter : public QObject {
    Q_OBJECT
private slots:
    void formatFromSuffix()
    {
        QCOMPARE(NoteExporter::formatForPath("a.md"), ExportFormat::Markdown);
        QCOMPARE(NoteExporter::formatForPath("A.MD"), ExportFormat::Markdown);
        QCOMPARE(NoteExporter::formatForPath("a.txt"), ExportFormat::Txt);
        QCOMPARE(NoteExporter::formatForPath("noext"), ExportFormat::Txt);
    }

    void markdownRendering()
    {
        const QString text = NoteExporter::render(makeNote("Title", "Body", " a , b "), ExportFormat::Markdown);
        QVERIFY(text.startsWith(QStringLiteral("# Title\n")));
        QVERIFY(text.contains(QStringLiteral("`a`, `b`")));
        QVERIFY(text.endsWith(QStringLiteral("Body\n")));
    }

    void txtRendering()
    {
        const QString text = NoteExporter::render(makeNote("Title", "Body", "a"), ExportFormat::Txt);
        QVERIFY(text.startsWith(QStringLiteral("Title\n=====\n")));
        QVERIFY(text.contains(QStringLiteral("Tags: a")));
    }

    void writesUtf8File()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("ru.txt");
        const Note n = makeNote(QString::fromUtf8("Заголовок"), QString::fromUtf8("Тело заметки"));
        const ExportResult r = NoteExporter::write(n, path, ExportFormat::Txt);
        QVERIFY2(r.ok, qPrintable(r.error));

        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QVERIFY(QString::fromUtf8(f.readAll()).contains(QString::fromUtf8("Тело заметки")));
    }

    void failureIsReported()
    {
        const ExportResult r = NoteExporter::write(makeNote("x"), "/nonexistent-dir/x.txt", ExportFormat::Txt);
        QVERIFY(!r.ok);
        QVERIFY(!r.error.isEmpty());
    }
};

QTEST_APPLESS_MAIN(TstExporter)
#include "tst_exporter.moc"
