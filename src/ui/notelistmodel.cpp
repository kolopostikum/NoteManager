#include "ui/notelistmodel.h"

#include <QLocale>
#include <utility>

NoteListModel::NoteListModel(QObject *parent) : QAbstractTableModel(parent) {}

void NoteListModel::setNotes(QVector<Note> notes)
{
    beginResetModel();
    m_notes = std::move(notes);
    endResetModel();
}

int NoteListModel::rowOfId(int noteId) const
{
    for (int row = 0; row < m_notes.size(); ++row) {
        if (m_notes.at(row).id == noteId)
            return row;
    }
    return -1;
}

int NoteListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_notes.size();
}

int NoteListModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant NoteListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_notes.size())
        return {};
    const Note &note = m_notes.at(index.row());

    if (role == NoteRole)
        return QVariant::fromValue(note);

    if (role == SortRole) {
        switch (index.column()) {
        case TitleColumn:
            return note.title;
        case TagsColumn:
            return note.tags;
        case UpdatedColumn:
            return note.updatedAt;
        }
        return {};
    }

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case TitleColumn:
            return note.title;
        case TagsColumn:
            return note.tags;
        case UpdatedColumn:
            return QLocale().toString(note.updatedAt.toLocalTime(), QLocale::ShortFormat);
        }
    }
    if (role == Qt::ToolTipRole && index.column() == TitleColumn)
        return note.title;
    return {};
}

QVariant NoteListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case TitleColumn:
        return tr("Title");
    case TagsColumn:
        return tr("Tags");
    case UpdatedColumn:
        return tr("Modified");
    }
    return {};
}
