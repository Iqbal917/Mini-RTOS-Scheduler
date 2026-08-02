#pragma once

#include "mini_rtos/context_switch_log.h"
#include "mini_rtos/scheduler.h"

#include <unordered_map>

namespace mini_rtos {

/// Preemptive Rate-Monotonic Scheduler: each periodic task's priority is
/// implicitly its period — the SHORTER the period, the higher the effective
/// priority (classic RMS assignment). Non-periodic tasks are treated as
/// having an infinite period (lowest priority) and only run when nothing
/// periodic is ready.
///
/// Simplification: 1 tick == 1 millisecond, so a task's period().count() is
/// used directly as a tick interval between releases.
class RateMonotonicScheduler : public Scheduler {
public:
    RateMonotonicScheduler();

    void addTask(TaskPtr task) override;
    TaskPtr pickNextTask() override;
    void tick(std::size_t elapsedTicks = 1) override;

    const ContextSwitchLog& log() const noexcept;

private:
    TaskPtr peekHighestPriorityReady() const;
    /// Returns true if 'a' has strictly higher RMS priority (shorter period,
    /// or periodic beats non-periodic) than 'b'.
    static bool hasHigherRmsPriority(const TaskPtr& a, const TaskPtr& b);

    std::vector<TaskPtr> allTasks_;
    std::unordered_map<TaskId, TickCount> nextRelease_;
    TickCount tickCount_;
    ContextSwitchLog log_;
    bool needsExecute_;
};

} // namespace mini_rtos
