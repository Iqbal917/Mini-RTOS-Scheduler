#include "mini_rtos/rate_monotonic_scheduler.h"

#include <algorithm>
#include <limits>

namespace mini_rtos {

RateMonotonicScheduler::RateMonotonicScheduler() : tickCount_(0), needsExecute_(false) {
}

void RateMonotonicScheduler::addTask(TaskPtr task) {
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

    allTasks_.push_back(task);

    // First instance releases immediately at t=0; the next release is one
    // full period later.
    if (task->isPeriodic()) {
        nextRelease_[task->id()] = static_cast<TickCount>(task->period()->count());
    }

    enqueueReady(task);
}

bool RateMonotonicScheduler::hasHigherRmsPriority(const TaskPtr& a, const TaskPtr& b) {
    // Shorter period = higher priority. Non-periodic tasks are treated as
    // having an effectively infinite period (lowest priority).
    auto periodOf = [](const TaskPtr& t) -> TickCount {
        return t->isPeriodic() ? static_cast<TickCount>(t->period()->count())
                                : std::numeric_limits<TickCount>::max();
    };
    return periodOf(a) < periodOf(b);
}

Scheduler::TaskPtr RateMonotonicScheduler::pickNextTask() {
    if (readyQueue_.empty()) {
        return nullptr;
    }

    auto highest = readyQueue_.begin();
    for (auto it = readyQueue_.begin() + 1; it != readyQueue_.end(); ++it) {
        if (hasHigherRmsPriority(*it, *highest)) {
            highest = it;
        }
    }

    TaskPtr selected = *highest;
    readyQueue_.erase(highest);
    return selected;
}

Scheduler::TaskPtr RateMonotonicScheduler::peekHighestPriorityReady() const {
    if (readyQueue_.empty()) {
        return nullptr;
    }

    auto highest = readyQueue_.begin();
    for (auto it = readyQueue_.begin() + 1; it != readyQueue_.end(); ++it) {
        if (hasHigherRmsPriority(*it, *highest)) {
            highest = it;
        }
    }
    return *highest;
}

void RateMonotonicScheduler::tick(std::size_t elapsedTicks) {
    if (elapsedTicks == 0) {
        return;
    }

    ++tickCount_;

    // Release periodic tasks whose period boundary has arrived. A task is
    // only re-released if it isn't already ready/running (i.e. it finished
    // its previous instance) — a task that's still mid-burst when its next
    // period arrives has overrun its deadline; this simple model just skips
    // the missed release rather than modeling deadline-miss handling.
    for (auto& task : allTasks_) {
        if (!task->isPeriodic()) {
            continue;
        }
        auto it = nextRelease_.find(task->id());
        if (it == nextRelease_.end() || tickCount_ < it->second) {
            continue;
        }
        bool alreadyActive = (currentTask_ == task) ||
                              (std::find(readyQueue_.begin(), readyQueue_.end(), task) != readyQueue_.end());
        if (!alreadyActive) {
            task->resetBurstTime();
            task->setState(TaskState::Ready);
            enqueueReady(task);
        }
        it->second += static_cast<TickCount>(task->period()->count());
    }

    if (!currentTask_) {
        currentTask_ = pickNextTask();
        if (!currentTask_) {
            return; // nothing ready — scheduler idle this tick
        }
        currentTask_->setState(TaskState::Running);
        needsExecute_ = true;
    }

    // Preemption: a task with a shorter period (higher RMS priority) always
    // preempts immediately, same as PriorityScheduler.
    TaskPtr contender = peekHighestPriorityReady();
    if (contender && hasHigherRmsPriority(contender, currentTask_)) {
        TaskPtr preempted = currentTask_;
        preempted->setState(TaskState::Ready);
        enqueueReady(preempted);
        currentTask_.reset();

        currentTask_ = pickNextTask();
        currentTask_->setState(TaskState::Running);
        needsExecute_ = true;
        log_.record({tickCount_, preempted->name(), currentTask_->name(), "preempted_by_shorter_period"});
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
        // A periodic task's "completion" here just means it finished this
        // instance's burst; it stays alive and will be re-released at its
        // next period boundary (handled at the top of tick()). Non-periodic
        // tasks terminate for good.
        if (dispatched->isPeriodic()) {
            dispatched->setState(TaskState::Blocked); // idle until next release
        } else {
            dispatched->markTerminated();
        }
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

const ContextSwitchLog& RateMonotonicScheduler::log() const noexcept {
    return log_;
}

} // namespace mini_rtos
