#include "domain/notevalidator.h"

NoteValidator::Error NoteValidator::validateTitle(const QString &title)
{
    const QString trimmed = title.trimmed();
    if (trimmed.isEmpty())
        return Error::TitleEmpty;
    if (trimmed.size() > kMaxTitleLength)
        return Error::TitleTooLong;
    return Error::None;
}

NoteValidator::Error NoteValidator::validate(const Note &note)
{
    return validateTitle(note.title);
}
