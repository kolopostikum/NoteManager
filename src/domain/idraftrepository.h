#pragma once

#include <QString>
#include <QVector>

#include "domain/draft.h"

class IDraftRepository {
public:
    virtual ~IDraftRepository() = default;

    /// Inserts or replaces the draft with the same key.
    virtual bool upsert(const Draft &draft) = 0;
    virtual bool remove(const QString &key) = 0;
    virtual QVector<Draft> getAll() = 0;

    virtual QString lastError() const = 0;
};
