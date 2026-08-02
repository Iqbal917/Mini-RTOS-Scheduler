#pragma once

#include "mini_rtos/context_switch_log.h"
#include "mini_rtos/scheduler.h"

namespace mini_rtos {

/// Preemptive, static-priority scheduler. Unlike RoundRobinScheduler there is
/// no time quantum: the highest-priority ready task always runs, and it is
/// preempted the instant a strictly higher-priority task becomes ready.
/// Equal-priority tasks do NOT preempt one another (run-to-completion/yield
/// among peers), matching classic fixed-priority scheduling semantics.
class PriorityScheduler : public Scheduler {
public:
    explicit PriorityScheduler(bool enablePriorityInheritance = true);

    void addTask(TaskPtr task) override;
    TaskPtr pickNextTask() override;
    void tick(std::size_t elapsedTicks = 1) override;

    bool priorityInheritanceEnabled() const noexcept;
    void setPriorityInheritanceEnabled(bool enabled) noexcept;
    const ContextSwitchLog& log() const noexcept;

private:
    /// Returns the highest-priority task currently in the ready queue without
    /// removing it, or nullptr if the ready queue is empty.
    TaskPtr peekHighestPriorityReady() const;

    bool priorityInheritanceEnabled_;
    TickCount tickCount_;
    ContextSwitchLog log_;
    /// True when currentTask_ has just been dispatched and hasn't had its one
    /// execute() call yet (see RoundRobinScheduler for why this must persist
    /// across tick() calls rather than being a local variable).
    bool needsExecute_;
};

} // namespace mini_rtos
