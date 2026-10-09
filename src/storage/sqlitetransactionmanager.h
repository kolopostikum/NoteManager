#pragma once

#include <QString>

#include "domain/itransactionmanager.h"

class SqliteTransactionManager : public ITransactionManager {
public:
    explicit SqliteTransactionManager(QString connectionName);

    bool begin() override;
    bool commit() override;
    bool rollback() override;

private:
    QString m_connection;
};
