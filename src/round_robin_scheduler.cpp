#include "mini_rtos/round_robin_scheduler.h"

#include <algorithm>
#include <utility>

namespace mini_rtos {

RoundRobinScheduler::RoundRobinScheduler(std::size_t timeQuantum)
    : timeQuantum_(timeQuantum), quantumCounter_(0), tickCount_(0) {
}

void RoundRobinScheduler::addTask(TaskPtr task) {
    if (!task) {
        return;
    }

    task->setYieldCallback([this, task]() {
        if (currentTask_ == task) {
            currentTask_->setState(TaskState::Ready);
            enqueueReady(task);
            currentTask_.reset();
        }
    });

    enqueueReady(task);
}

Scheduler::TaskPtr RoundRobinScheduler::pickNextTask() {
    if (readyQueue_.empty()) {
        return nullptr;
    }

    TaskPtr next = readyQueue_.front();
    readyQueue_.erase(readyQueue_.begin());
    return next;
}

void RoundRobinScheduler::tick(std::size_t elapsedTicks) {
    if (elapsedTicks == 0) {
        return;
    }

    ++tickCount_;

    if (!currentTask_) {
        currentTask_ = pickNextTask();
    }

    if (!currentTask_) {
        return;
    }

    currentTask_->setState(TaskState::Running);
    currentTask_->decrementBurstTime(elapsedTicks);
    quantumCounter_ += elapsedTicks;

    if (currentTask_->remainingBurstTime() == 0) {
        currentTask_->markTerminated();
        currentTask_->setState(TaskState::Terminated);
        auto previous = currentTask_;
        currentTask_.reset();

        if (previous) {
            log_.record({tickCount_, previous->name(), "<done>", "completed"});
        }

        currentTask_ = pickNextTask();
        if (currentTask_) {
            currentTask_->setState(TaskState::Running);
            log_.record({tickCount_, "<none>", currentTask_->name(), "task_started"});
        }
        quantumCounter_ = 0;
        return;
    }

    if (quantumCounter_ >= timeQuantum_) {
        auto previous = currentTask_;
        previous->setState(TaskState::Ready);
        enqueueReady(previous);
        currentTask_.reset();

        currentTask_ = pickNextTask();
        if (currentTask_) {
            currentTask_->setState(TaskState::Running);
            log_.record({tickCount_, previous->name(), currentTask_->name(), "quantum_expired"});
        }

        quantumCounter_ = 0;
        return;
    }

    auto result = currentTask_->execute();
    if (result == Task::ExecutionResult::Yielded) {
        auto previous = currentTask_;
        previous->setState(TaskState::Ready);
        enqueueReady(previous);
        currentTask_.reset();

        currentTask_ = pickNextTask();
        if (currentTask_) {
            currentTask_->setState(TaskState::Running);
            log_.record({tickCount_, previous->name(), currentTask_->name(), "yielded"});
        }
    }
}

std::size_t RoundRobinScheduler::timeQuantum() const noexcept {
    return timeQuantum_;
}

void RoundRobinScheduler::setTimeQuantum(std::size_t quantum) noexcept {
    timeQuantum_ = quantum;
}

const ContextSwitchLog& RoundRobinScheduler::log() const noexcept {
    return log_;
}

} // namespace mini_rtos
