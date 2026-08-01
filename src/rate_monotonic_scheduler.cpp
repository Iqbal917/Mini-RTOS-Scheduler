#include "mini_rtos/rate_monotonic_scheduler.h"

namespace mini_rtos {

RateMonotonicScheduler::RateMonotonicScheduler(std::size_t timeQuantum)
    : timeQuantum_(timeQuantum), quantumCounter_(0) {
}

void RateMonotonicScheduler::addTask(TaskPtr task) {
    if (!task) {
        return;
    }
    enqueueReady(task);
}

Scheduler::TaskPtr RateMonotonicScheduler::pickNextTask() {
    if (readyQueue_.empty()) {
        return nullptr;
    }

    auto highest = readyQueue_.begin();
    for (auto it = readyQueue_.begin() + 1; it != readyQueue_.end(); ++it) {
        auto leftPeriod = (*highest)->period().value_or(std::chrono::milliseconds(0));
        auto rightPeriod = (*it)->period().value_or(std::chrono::milliseconds(0));
        if (rightPeriod < leftPeriod) {
            highest = it;
        }
    }

    TaskPtr selected = *highest;
    readyQueue_.erase(highest);
    return selected;
}

void RateMonotonicScheduler::tick(std::size_t elapsedTicks) {
    (void)elapsedTicks;
}

std::size_t RateMonotonicScheduler::timeQuantum() const noexcept {
    return timeQuantum_;
}

void RateMonotonicScheduler::setTimeQuantum(std::size_t quantum) noexcept {
    timeQuantum_ = quantum;
}

} // namespace mini_rtos
