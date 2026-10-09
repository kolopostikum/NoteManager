#include "app/noteexporter.h"

#include <QFileInfo>
#include <QSaveFile>
#include <QStringList>
#include <QTextCodec>
#include <QTextStream>

namespace {

QString formatDate(const QDateTime &dt)
{
    return dt.isValid() ? dt.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
                        : QString();
}

QStringList splitTags(const QString &tags)
{
    QStringList result;
    const QStringList parts = tags.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        const QString tag = part.trimmed();
        if (!tag.isEmpty())
            result.append(tag);
    }
    return result;
}

} // namespace

ExportFormat NoteExporter::formatForPath(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == QLatin1String("md") || suffix == QLatin1String("markdown"))
        return ExportFormat::Markdown;
    return ExportFormat::Txt;
}

QString NoteExporter::render(const Note &note, ExportFormat format)
{
    const QStringList tags = splitTags(note.tags);
    QString out;

    if (format == ExportFormat::Markdown) {
        out += QStringLiteral("# ") + note.title + QStringLiteral("\n\n");
        if (!tags.isEmpty()) {
            QStringList quoted;
            for (const QString &tag : tags)
                quoted.append(QLatin1Char('`') + tag + QLatin1Char('`'));
            out += QStringLiteral("**Tags:** ") + quoted.join(QStringLiteral(", ")) +
                   QStringLiteral("\n\n");
        }
        out += QStringLiteral("_Created: %1 | Updated: %2_\n\n")
                   .arg(formatDate(note.createdAt), formatDate(note.updatedAt));
        out += note.content;
    } else {
        out += note.title + QStringLiteral("\n");
        out += QString(note.title.size(), QLatin1Char('=')) + QStringLiteral("\n");
        if (!tags.isEmpty())
            out += QStringLiteral("Tags: ") + tags.join(QStringLiteral(", ")) + QStringLiteral("\n");
        out += QStringLiteral("Created: %1\nUpdated: %2\n\n")
                   .arg(formatDate(note.createdAt), formatDate(note.updatedAt));
        out += note.content;
    }

    if (!out.endsWith(QLatin1Char('\n')))
        out += QLatin1Char('\n');
    return out;
}

ExportResult NoteExporter::write(const Note &note, const QString &path, ExportFormat format)
{
    ExportResult result;

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.error = file.errorString();
        return result;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream << render(note, format);
    stream.flush();

    if (!file.commit()) {
        result.error = file.errorString();
        return result;
    }
    result.ok = true;
    return result;
}
