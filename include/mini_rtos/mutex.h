#pragma once

#include "mini_rtos/common.h"
#include "mini_rtos/task.h"

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace mini_rtos {

/// A mutex with priority inheritance to prevent unbounded priority inversion:
/// if a higher-priority task blocks waiting for a mutex held by a
/// lower-priority task, the holder's priority is temporarily boosted to
/// match, so a MEDIUM-priority task can't indefinitely starve the holder out
/// of the CPU while the high-priority task waits.
class Mutex {
public:
    /// Called when unlock() hands the mutex to a waiting task, so the
    /// scheduler can move that task from its blocked queue back to ready.
    using WakeCallback = std::function<void(std::shared_ptr<Task>)>;

    explicit Mutex(bool priorityInheritanceEnabled = true);

    /// Attempts to acquire the mutex for 'requester'.
    /// Returns true if acquired immediately. Returns false if the mutex is
    /// already held — in that case 'requester' is added to the wait list,
    /// and (if priority inheritance is enabled and requester outranks the
    /// current owner) the owner's priority is boosted immediately.
    /// The caller (scheduler) is responsible for transitioning 'requester'
    /// to Blocked when this returns false.
    bool lock(std::shared_ptr<Task> requester);

    /// Releases the mutex. If the owner's priority was boosted, it is
    /// restored to its base priority first. If tasks are waiting, the
    /// highest-priority waiter becomes the new owner and wakeCallback_ is
    /// invoked with it so the scheduler can wake it.
    void unlock();

    void setWakeCallback(WakeCallback callback);
    bool isLocked() const noexcept;
    std::shared_ptr<Task> owner() const noexcept;
    const std::vector<std::shared_ptr<Task>>& waiters() const noexcept;
    bool priorityInheritanceEnabled() const noexcept;

private:
    bool locked_ = false;
    bool priorityInheritanceEnabled_;
    std::shared_ptr<Task> owner_;
    std::vector<std::shared_ptr<Task>> waiters_;
    WakeCallback wakeCallback_;
};

} // namespace mini_rtos
