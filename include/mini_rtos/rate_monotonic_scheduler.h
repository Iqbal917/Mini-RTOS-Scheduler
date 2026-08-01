#pragma once

#include "mini_rtos/scheduler.h"

namespace mini_rtos {

class RateMonotonicScheduler : public Scheduler {
public:
    explicit RateMonotonicScheduler(std::size_t timeQuantum = 1);

    void addTask(TaskPtr task) override;
    TaskPtr pickNextTask() override;
    void tick(std::size_t elapsedTicks = 1) override;

    std::size_t timeQuantum() const noexcept;
    void setTimeQuantum(std::size_t quantum) noexcept;

private:
    std::size_t timeQuantum_;
    std::size_t quantumCounter_;
};

} // namespace mini_rtos
