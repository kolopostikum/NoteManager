#pragma once

/// Unit-of-work contract: lets services group several repository calls into
/// one atomic change without knowing the storage technology.
class ITransactionManager {
public:
    virtual ~ITransactionManager() = default;

    virtual bool begin() = 0;
    virtual bool commit() = 0;
    virtual bool rollback() = 0;
};

/// RAII helper: rolls back automatically unless commit() succeeded.
class TransactionScope {
public:
    explicit TransactionScope(ITransactionManager &manager)
        : m_manager(manager), m_active(manager.begin())
    {
    }
    ~TransactionScope()
    {
        if (m_active)
            m_manager.rollback();
    }
    TransactionScope(const TransactionScope &) = delete;
    TransactionScope &operator=(const TransactionScope &) = delete;

    bool active() const { return m_active; }

    bool commit()
    {
        if (!m_active)
            return false;
        m_active = false;
        return m_manager.commit();
    }

private:
    ITransactionManager &m_manager;
    bool m_active;
};
