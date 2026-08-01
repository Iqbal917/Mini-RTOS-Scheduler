#pragma once

#include "mini_rtos/scheduler.h"

namespace mini_rtos {

class PriorityScheduler : public Scheduler {
public:
    explicit PriorityScheduler(std::size_t timeQuantum = 1,
                               bool enablePriorityInheritance = true);

    void addTask(TaskPtr task) override;
    TaskPtr pickNextTask() override;
    void tick(std::size_t elapsedTicks = 1) override;

    bool priorityInheritanceEnabled() const noexcept;
    void setPriorityInheritanceEnabled(bool enabled) noexcept;

private:
    std::size_t timeQuantum_;
    std::size_t quantumCounter_;
    bool priorityInheritanceEnabled_;
};

} // namespace mini_rtos
