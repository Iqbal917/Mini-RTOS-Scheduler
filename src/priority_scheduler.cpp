#include "mini_rtos/priority_scheduler.h"

#include <algorithm>

namespace mini_rtos {

PriorityScheduler::PriorityScheduler(bool enablePriorityInheritance)
    : priorityInheritanceEnabled_(enablePriorityInheritance), tickCount_(0), needsExecute_(false) {
}

void PriorityScheduler::addTask(TaskPtr task) {
    if (!task) {
        return;
    }

    task->setYieldCallback([this, task]() {
        if (currentTask_ == task) {
            task->setState(TaskState::Ready);
            enqueueReady(task);
            currentTask_.reset();
        }
    });

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

Scheduler::TaskPtr PriorityScheduler::peekHighestPriorityReady() const {
    if (readyQueue_.empty()) {
        return nullptr;
    }

    auto highest = readyQueue_.begin();
    for (auto it = readyQueue_.begin() + 1; it != readyQueue_.end(); ++it) {
        if ((*it)->priority() > (*highest)->priority()) {
            highest = it;
        }
    }
    return *highest;
}

void PriorityScheduler::tick(std::size_t elapsedTicks) {
    if (elapsedTicks == 0) {
        return;
    }

    ++tickCount_;

    if (!currentTask_) {
        currentTask_ = pickNextTask();
        if (!currentTask_) {
            return; // nothing ready — scheduler idle this tick
        }
        currentTask_->setState(TaskState::Running);
        needsExecute_ = true;
    }

    // Preemption check: a strictly higher-priority ready task always
    // preempts immediately — no quantum involved in pure priority
    // scheduling. This happens even before running the current task's
    // dispatch-time work, matching real preemptive-priority semantics
    // (the higher-priority task should not have to wait even one tick).
    TaskPtr contender = peekHighestPriorityReady();
    if (contender && contender->priority() > currentTask_->priority()) {
        TaskPtr preempted = currentTask_;
        preempted->setState(TaskState::Ready);
        enqueueReady(preempted);
        currentTask_.reset();

        currentTask_ = pickNextTask();
        currentTask_->setState(TaskState::Running);
        needsExecute_ = true;
        log_.record({tickCount_, preempted->name(), currentTask_->name(), "preempted_by_higher_priority"});
        // Fall through: the newly-dispatched higher-priority task still gets
        // its own execute() call below, on this same tick, since preemption
        // should not cost it a tick of latency.
    }

    TaskPtr dispatched = currentTask_;

    if (needsExecute_) {
        needsExecute_ = false;
        auto result = dispatched->execute();

        if (result == Task::ExecutionResult::Yielded) {
            currentTask_ = pickNextTask();
            if (currentTask_) {
                currentTask_->setState(TaskState::Running);
                needsExecute_ = true;
            }
            log_.record({tickCount_, dispatched->name(),
                          currentTask_ ? currentTask_->name() : "<none>", "yielded"});
            return;
        }

        if (result == Task::ExecutionResult::Blocked) {
            // The task already moved itself to the blocked queue (typically
            // via entry_ calling the scheduler's enqueueBlocked() after
            // failing to acquire a mutex/semaphore). Just release the CPU.
            currentTask_.reset();
            log_.record({tickCount_, dispatched->name(), "<blocked>", "blocked_on_resource"});

            currentTask_ = pickNextTask();
            if (currentTask_) {
                currentTask_->setState(TaskState::Running);
                needsExecute_ = true;
                log_.record({tickCount_, "<none>", currentTask_->name(), "task_started"});
            }
            return;
        }

        if (dispatched->isTerminated()) {
            currentTask_.reset();
            log_.record({tickCount_, dispatched->name(), "<done>", "completed"});

            currentTask_ = pickNextTask();
            if (currentTask_) {
                currentTask_->setState(TaskState::Running);
                needsExecute_ = true;
                log_.record({tickCount_, "<none>", currentTask_->name(), "task_started"});
            }
            return;
        }
        // ExecutionResult::Ran — fall through to burst bookkeeping below.
    }

    dispatched->decrementBurstTime(elapsedTicks);

    if (dispatched->remainingBurstTime() == 0) {
        dispatched->markTerminated();
        currentTask_.reset();
        log_.record({tickCount_, dispatched->name(), "<done>", "completed"});

        currentTask_ = pickNextTask();
        if (currentTask_) {
            currentTask_->setState(TaskState::Running);
            needsExecute_ = true;
            log_.record({tickCount_, "<none>", currentTask_->name(), "task_started"});
        }
    }
}

bool PriorityScheduler::priorityInheritanceEnabled() const noexcept {
    return priorityInheritanceEnabled_;
}

void PriorityScheduler::setPriorityInheritanceEnabled(bool enabled) noexcept {
    priorityInheritanceEnabled_ = enabled;
}

const ContextSwitchLog& PriorityScheduler::log() const noexcept {
    return log_;
}

} // namespace mini_rtos
