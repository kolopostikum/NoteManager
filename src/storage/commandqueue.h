#pragma once

#include <QMutex>
#include <deque>
#include <optional>

#include "storage/commands.h"

/// Thread-safe FIFO that keeps all storage operations in the order the user
/// issued them. Consecutive autosave drafts of one editing session are
/// coalesced (latest wins) so a stream of edits never floods the queue.
class CommandQueue {
public:
    void push(Command command);
    std::optional<Command> tryPop();
    std::size_t size() const;

private:
    mutable QMutex m_mutex;
    std::deque<Command> m_items;
};
