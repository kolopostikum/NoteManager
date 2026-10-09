#pragma once

#include <QAbstractTableModel>
#include <QVector>
#include <optional>

#include "domain/note.h"

/// Read-only table model over the latest snapshot of notes received from the
/// controller. It contains no storage or business logic.
class NoteListModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column { TitleColumn = 0, TagsColumn, UpdatedColumn, ColumnCount };
    enum Role { NoteRole = Qt::UserRole + 1, SortRole };

    explicit NoteListModel(QObject *parent = nullptr);

    void setNotes(QVector<Note> notes);

    const Note &noteAt(int row) const { return m_notes.at(row); }
    int rowOfId(int noteId) const;
    /// Refreshes column captions after a language change.
    void retranslate() { emit headerDataChanged(Qt::Horizontal, 0, ColumnCount - 1); }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

private:
    QVector<Note> m_notes;
};
