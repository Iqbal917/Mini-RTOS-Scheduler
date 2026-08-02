#pragma once

#include "mini_rtos/context_switch_log.h"
#include "mini_rtos/scheduler.h"

namespace mini_rtos {

class RoundRobinScheduler : public Scheduler {
public:
    explicit RoundRobinScheduler(std::size_t timeQuantum = 1);

    void addTask(TaskPtr task) override;
    TaskPtr pickNextTask() override;
    void tick(std::size_t elapsedTicks = 1) override;

    std::size_t timeQuantum() const noexcept;
    void setTimeQuantum(std::size_t quantum) noexcept;
    const ContextSwitchLog& log() const noexcept;

private:
    std::size_t timeQuantum_;
    std::size_t quantumCounter_;
    TickCount tickCount_;
    ContextSwitchLog log_;
    /// True when currentTask_ has just been dispatched and hasn't had its one
    /// execute() call yet. This must be a persistent member, not a local
    /// variable in tick() — a task can become current mid-tick() (e.g. right
    /// after another task yields), and its own execute() call must happen on
    /// the *next* tick(), not be skipped or run twice.
    bool needsExecute_;
};

} // namespace mini_rtos
