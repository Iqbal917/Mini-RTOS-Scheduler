#pragma once

#include "mini_rtos/common.h"
#include "mini_rtos/task.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

namespace mini_rtos {

/// A counting semaphore with a real wait list, so it can participate in
/// scheduler-driven blocking rather than just tracking a bare integer.
class Semaphore {
public:
    using WakeCallback = std::function<void(std::shared_ptr<Task>)>;

    explicit Semaphore(std::size_t initialCount = 0);

    /// Attempts to acquire a permit for 'requester'. Returns true and
    /// decrements the count if a permit was available immediately. Returns
    /// false and adds 'requester' to the wait list otherwise — the caller
    /// (scheduler) is responsible for transitioning 'requester' to Blocked.
    bool wait(std::shared_ptr<Task> requester);

    /// Releases a permit. If a task is waiting, the highest-priority waiter
    /// is handed the permit directly (count_ is unaffected in that case) and
    /// wakeCallback_ fires so the scheduler can wake it. Otherwise count_ is
    /// incremented for a future caller.
    void signal();

    void setWakeCallback(WakeCallback callback);
    std::size_t count() const noexcept;
    const std::vector<std::shared_ptr<Task>>& waiters() const noexcept;

private:
    std::size_t count_;
    std::vector<std::shared_ptr<Task>> waiters_;
    WakeCallback wakeCallback_;
};

} // namespace mini_rtos
