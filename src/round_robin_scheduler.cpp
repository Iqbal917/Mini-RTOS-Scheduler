#include "mini_rtos/round_robin_scheduler.h"

#include <algorithm>
#include <utility>

namespace mini_rtos {

RoundRobinScheduler::RoundRobinScheduler(std::size_t timeQuantum)
    : timeQuantum_(timeQuantum), quantumCounter_(0), tickCount_(0), needsExecute_(false) {
}

void RoundRobinScheduler::addTask(TaskPtr task) {
    if (!task) {
        return;
    }

    // When a task's entry_ calls yield(), this fires synchronously (still
    // inside the execute() call) and immediately releases the CPU: move the
    // task back to Ready and clear currentTask_ right away.
    task->setYieldCallback([this, task]() {
        if (currentTask_ == task) {
            task->setState(TaskState::Ready);
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
        if (!currentTask_) {
            return; // nothing ready — scheduler idle this tick
        }
        currentTask_->setState(TaskState::Running);
        needsExecute_ = true;
    }

    TaskPtr dispatched = currentTask_;

    // Run the task's unit of work exactly once, on the first tick after it
    // is dispatched. Task::execute() runs entry_ synchronously to completion
    // (or to its yield point) in a single call — it cannot be paused and
    // resumed mid-function — so this must fire exactly once per dispatch,
    // never on later ticks of the same dispatch, and never be skipped.
    if (needsExecute_) {
        needsExecute_ = false;
        auto result = dispatched->execute();

        if (result == Task::ExecutionResult::Yielded) {
            // The yield callback already moved 'dispatched' to Ready and
            // cleared currentTask_. This was a voluntary early release, not
            // a quantum expiry, so the next task starts with a full quantum.
            currentTask_ = pickNextTask();
            if (currentTask_) {
                currentTask_->setState(TaskState::Running);
                needsExecute_ = true;
            }
            quantumCounter_ = 0;
            log_.record({tickCount_, dispatched->name(),
                          currentTask_ ? currentTask_->name() : "<none>", "yielded"});
            return;
        }

        if (dispatched->isTerminated()) {
            // entry_ ran to completion without ever yielding — the task is
            // considered fully done in this single dispatch, regardless of
            // any remaining nominal burst time.
            currentTask_.reset();
            log_.record({tickCount_, dispatched->name(), "<done>", "completed"});

            currentTask_ = pickNextTask();
            if (currentTask_) {
                currentTask_->setState(TaskState::Running);
                needsExecute_ = true;
                log_.record({tickCount_, "<none>", currentTask_->name(), "task_started"});
            }
            quantumCounter_ = 0;
            return;
        }
        // Otherwise entry_ returned without terminating or yielding (e.g. an
        // empty entry_) — fall through to normal burst/quantum bookkeeping.
    }

    // The task continues occupying the CPU. No further entry_ invocation
    // happens here — this purely accounts for simulated CPU time already
    // spent on the work performed at dispatch, and governs when a real
    // preemption (quantum expiry or burst exhaustion) should occur.
    dispatched->decrementBurstTime(elapsedTicks);
    quantumCounter_ += elapsedTicks;

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
        quantumCounter_ = 0;
        return;
    }

    if (quantumCounter_ >= timeQuantum_) {
        dispatched->setState(TaskState::Ready);
        enqueueReady(dispatched);
        currentTask_.reset();

        currentTask_ = pickNextTask();
        if (currentTask_) {
            currentTask_->setState(TaskState::Running);
            needsExecute_ = true;
            log_.record({tickCount_, dispatched->name(), currentTask_->name(), "quantum_expired"});
        }
        quantumCounter_ = 0;
        return;
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
