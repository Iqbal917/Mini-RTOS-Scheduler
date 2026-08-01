#include "mini_rtos/priority_scheduler.h"

namespace mini_rtos {

PriorityScheduler::PriorityScheduler(std::size_t timeQuantum, bool enablePriorityInheritance)
    : timeQuantum_(timeQuantum), quantumCounter_(0), priorityInheritanceEnabled_(enablePriorityInheritance) {
}

void PriorityScheduler::addTask(TaskPtr task) {
    if (!task) {
        return;
    }
    enqueueReady(task);
}

Scheduler::TaskPtr PriorityScheduler::pickNextTask() {
    if (readyQueue_.empty()) {
        return nullptr;
    }

    auto highest = readyQueue_.begin();
    for (auto it = readyQueue_.begin() + 1; it != readyQueue_.end(); ++it) {
        if ((*it)->priority() > (*highest)->priority()) {
            highest = it;
        }
    }

    TaskPtr selected = *highest;
    readyQueue_.erase(highest);
    return selected;
}

void PriorityScheduler::tick(std::size_t elapsedTicks) {
    (void)elapsedTicks;
}

bool PriorityScheduler::priorityInheritanceEnabled() const noexcept {
    return priorityInheritanceEnabled_;
}

void PriorityScheduler::setPriorityInheritanceEnabled(bool enabled) noexcept {
    priorityInheritanceEnabled_ = enabled;
}

} // namespace mini_rtos
