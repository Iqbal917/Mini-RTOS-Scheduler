#include "mini_rtos/scheduler.h"

#include <algorithm>
#include <utility>

namespace mini_rtos {

void Scheduler::enqueueReady(TaskPtr task) {
    if (!task) {
        return;
    }

    if (task->state() == TaskState::Blocked) {
        task->setState(TaskState::Ready);
    }

    if (std::find(readyQueue_.begin(), readyQueue_.end(), task) == readyQueue_.end()) {
        readyQueue_.push_back(std::move(task));
    }
}

void Scheduler::enqueueBlocked(TaskPtr task) {
    if (!task) {
        return;
    }

    if (task->state() != TaskState::Blocked) {
        task->setState(TaskState::Blocked);
    }

    if (std::find(blockedQueue_.begin(), blockedQueue_.end(), task) == blockedQueue_.end()) {
        blockedQueue_.push_back(std::move(task));
    }
}

void Scheduler::removeTask(TaskPtr task) {
    if (!task) {
        return;
    }

    readyQueue_.erase(std::remove(readyQueue_.begin(), readyQueue_.end(), task), readyQueue_.end());
    blockedQueue_.erase(std::remove(blockedQueue_.begin(), blockedQueue_.end(), task), blockedQueue_.end());

    task->markTerminated();

    if (currentTask_ && currentTask_ == task) {
        currentTask_.reset();
    }
}

Scheduler::TaskPtr Scheduler::currentTask() const noexcept {
    return currentTask_;
}

const std::vector<Scheduler::TaskPtr>& Scheduler::readyQueue() const noexcept {
    return readyQueue_;
}

const std::vector<Scheduler::TaskPtr>& Scheduler::blockedQueue() const noexcept {
    return blockedQueue_;
}

std::size_t Scheduler::readyCount() const noexcept {
    return readyQueue_.size();
}

std::size_t Scheduler::blockedCount() const noexcept {
    return blockedQueue_.size();
}

void Scheduler::setCurrentTask(TaskPtr task) noexcept {
    currentTask_ = std::move(task);
}

void Scheduler::clearCurrentTask() noexcept {
    currentTask_.reset();
}

} // namespace mini_rtos
