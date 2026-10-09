#include "storage/sqlitetransactionmanager.h"

#include <QSqlDatabase>
#include <utility>

SqliteTransactionManager::SqliteTransactionManager(QString connectionName)
    : m_connection(std::move(connectionName))
{
}

bool SqliteTransactionManager::begin()
{
    return QSqlDatabase::database(m_connection).transaction();
}

bool SqliteTransactionManager::commit()
{
    return QSqlDatabase::database(m_connection).commit();
}

bool SqliteTransactionManager::rollback()
{
    return QSqlDatabase::database(m_connection).rollback();
}
