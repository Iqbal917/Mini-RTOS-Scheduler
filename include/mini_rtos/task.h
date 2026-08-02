#pragma once

#include "mini_rtos/common.h"

#include <chrono>
#include <functional>
#include <optional>

namespace mini_rtos {

class Task {
public:
    /// Callback invoked when a task reaches a cooperative yield point.
    using YieldCallback = std::function<void()>;

    /// Entry function for a task. The task can call the provided yield callback
    /// to voluntarily give control back to the scheduler.
    using EntryFunction = std::function<void(Task&, YieldCallback)>;

    enum class ExecutionResult {
        /// entry_ (or the scheduler) explicitly terminated the task.
        Completed,
        /// entry_ called the yield callback — the task voluntarily released
        /// the CPU before its dispatch-time work would otherwise be judged done.
        Yielded,
        /// entry_ returned normally without yielding or explicitly
        /// terminating. The task remains Running; it is NOT considered
        /// finished just because entry_'s body finished — remaining burst
        /// time still governs when the scheduler treats it as complete.
        /// This distinguishes "this dispatch's printed/visible work is done"
        /// from "this task has no more CPU time to consume".
        Ran,
        /// entry_ blocked the task (e.g. waiting on a mutex/semaphore) by
        /// calling markBlocked-equivalent logic itself, typically via the
        /// scheduler's enqueueBlocked(). The scheduler must not treat this
        /// task as still runnable.
        Blocked
    };

    Task(TaskId id,
         std::string name,
         Priority priority,
         std::size_t stackSize,
         EntryFunction entry,
         std::size_t burstTime = 5,
         std::optional<std::chrono::milliseconds> period = std::nullopt);

    virtual ~Task() = default;

    TaskId id() const noexcept;
    const std::string& name() const noexcept;
    Priority priority() const noexcept;
    TaskState state() const noexcept;
    std::size_t stackPointer() const noexcept;
    std::size_t stackSize() const noexcept;
    std::size_t remainingBurstTime() const noexcept;
    std::size_t initialBurstTime() const noexcept;
    const std::optional<std::chrono::milliseconds>& period() const noexcept;

    bool isPeriodic() const noexcept;
    bool isBlocked() const noexcept;
    bool isTerminated() const noexcept;
    bool canTransitionTo(TaskState nextState) const noexcept;

    void setPriority(Priority priority) noexcept;
    /// Returns the task's original (non-inherited) priority.
    Priority basePriority() const noexcept;
    /// Temporarily raises the task's effective priority for priority inheritance.
    /// Only applies if newPriority represents a higher priority than the current
    /// effective priority (per this project's convention: higher numeric value
    /// = higher priority). No-op otherwise.
    void boostPriority(Priority newPriority) noexcept;
    /// Restores the task's effective priority to its original base priority.
    void restorePriority() noexcept;
    /// True if the task's effective priority currently differs from its base
    /// priority (i.e. it is holding an inherited boost).
    bool isPriorityBoosted() const noexcept;
    void setState(TaskState newState);
    void setStackPointer(std::size_t stackPointer) noexcept;
    /// Sets the task's initial and remaining burst time budget.
    void setBurstTime(std::size_t burstTime) noexcept;
    void decrementBurstTime(std::size_t amount = 1) noexcept;
    void resetBurstTime() noexcept;
    void markTerminated();
    ExecutionResult execute();
    void yield();
    void setYieldCallback(YieldCallback callback) noexcept;

private:
    TaskId id_;
    std::string name_;
    Priority priority_;
    Priority basePriority_;
    TaskState state_;
    std::size_t stackPointer_;
    std::size_t stackSize_;
    EntryFunction entry_;
    YieldCallback yieldCallback_;
    std::size_t initialBurstTime_;
    std::size_t remainingBurstTime_;
    std::optional<std::chrono::milliseconds> period_;
};

} // namespace mini_rtos
