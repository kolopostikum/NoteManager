#pragma once

#include "domain/note.h"

/// Pure, synchronous validation rules. Returns codes (not texts) so the domain
/// stays free of any translation concern; the UI maps codes to messages.
class NoteValidator {
public:
    enum class Error { None, TitleEmpty, TitleTooLong };

    static constexpr int kMaxTitleLength = 100;

    static Error validateTitle(const QString &title);
    static Error validate(const Note &note);
};
