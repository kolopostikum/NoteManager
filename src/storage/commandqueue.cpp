#include "storage/commandqueue.h"

#include <QMutexLocker>
#include <utility>

namespace {

struct SessionKeyVisitor {
    QString operator()(const SaveDraftCmd &c) const { return c.draft.key; }
    QString operator()(const DeleteDraftCmd &c) const { return c.key; }
    QString operator()(const SaveNoteCmd &c) const { return c.draftKey; }
    template <typename Other>
    QString operator()(const Other &) const
    {
        return QString();
    }
};

/// Editing-session key a command refers to (null for commands that do not).
QString sessionKeyOf(const Command &command)
{
    return std::visit(SessionKeyVisitor{}, command);
}

} // namespace

void CommandQueue::push(Command command)
{
    QMutexLocker locker(&m_mutex);

    const QString key = sessionKeyOf(command);
    if (std::holds_alternative<SaveDraftCmd>(command) && !key.isEmpty()) {
        // Walk back to the latest command touching the same session. Only if it
        // is a pending draft write can it safely be replaced in place.
        for (auto it = m_items.rbegin(); it != m_items.rend(); ++it) {
            if (sessionKeyOf(*it) != key)
                continue;
            if (std::holds_alternative<SaveDraftCmd>(*it))
                *it = std::move(command);
            else
                m_items.push_back(std::move(command));
            return;
        }
    }
    m_items.push_back(std::move(command));
}

std::optional<Command> CommandQueue::tryPop()
{
    QMutexLocker locker(&m_mutex);
    if (m_items.empty())
        return std::nullopt;
    Command command = std::move(m_items.front());
    m_items.pop_front();
    return command;
}

std::size_t CommandQueue::size() const
{
    QMutexLocker locker(&m_mutex);
    return m_items.size();
}
