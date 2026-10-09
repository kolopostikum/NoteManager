#pragma once

#include <QString>

#include "domain/idraftrepository.h"

class QSqlQuery;

/// SQLite implementation of IDraftRepository (table `drafts`).
class SqliteDraftRepository : public IDraftRepository {
public:
    explicit SqliteDraftRepository(QString connectionName);

    bool upsert(const Draft &draft) override;
    bool remove(const QString &key) override;
    QVector<Draft> getAll() override;

    QString lastError() const override { return m_lastError; }

private:
    bool fail(const QSqlQuery &query);

    QString m_connection;
    QString m_lastError;
};
