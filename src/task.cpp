#include "mini_rtos/task.h"

#include <algorithm>
#include <utility>

namespace mini_rtos {

Task::Task(TaskId id,
           std::string name,
           Priority priority,
           std::size_t stackSize,
           EntryFunction entry,
           std::size_t burstTime,
           std::optional<std::chrono::milliseconds> period)
    : id_(id),
      name_(std::move(name)),
      priority_(priority),
      state_(TaskState::Ready),
      stackPointer_(stackSize),
      stackSize_(stackSize),
      entry_(std::move(entry)),
      yieldCallback_(),
      initialBurstTime_(std::max<std::size_t>(1, burstTime)),
      remainingBurstTime_(std::max<std::size_t>(1, burstTime)),
      period_(std::move(period)) {
}

TaskId Task::id() const noexcept {
    return id_;
}

const std::string& Task::name() const noexcept {
    return name_;
}

Priority Task::priority() const noexcept {
    return priority_;
}

TaskState Task::state() const noexcept {
    return state_;
}

std::size_t Task::stackPointer() const noexcept {
    return stackPointer_;
}

std::size_t Task::stackSize() const noexcept {
    return stackSize_;
}

std::size_t Task::remainingBurstTime() const noexcept {
    return remainingBurstTime_;
}

std::size_t Task::initialBurstTime() const noexcept {
    return initialBurstTime_;
}

const std::optional<std::chrono::milliseconds>& Task::period() const noexcept {
    return period_;
}

bool Task::isPeriodic() const noexcept {
    return period_.has_value();
}

bool Task::isBlocked() const noexcept {
    return state_ == TaskState::Blocked;
}

bool Task::isTerminated() const noexcept {
    return state_ == TaskState::Terminated;
}

bool Task::canTransitionTo(TaskState nextState) const noexcept {
    switch (state_) {
        case TaskState::Ready:
            return nextState == TaskState::Running || nextState == TaskState::Blocked ||
                   nextState == TaskState::Terminated;
        case TaskState::Running:
            return nextState == TaskState::Ready || nextState == TaskState::Blocked ||
                   nextState == TaskState::Terminated;
        case TaskState::Blocked:
            return nextState == TaskState::Ready || nextState == TaskState::Terminated;
        case TaskState::Terminated:
            return false;
    }

    return false;
}

void Task::setPriority(Priority priority) noexcept {
    priority_ = priority;
}

void Task::setState(TaskState newState) {
    if (canTransitionTo(newState)) {
        state_ = newState;
    }
}

void Task::setStackPointer(std::size_t stackPointer) noexcept {
    stackPointer_ = stackPointer;
}

void Task::setBurstTime(std::size_t burstTime) noexcept {
    initialBurstTime_ = burstTime;
    remainingBurstTime_ = burstTime;
}

void Task::decrementBurstTime(std::size_t amount) noexcept {
    if (remainingBurstTime_ >= amount) {
        remainingBurstTime_ -= amount;
    } else {
        remainingBurstTime_ = 0;
    }
}

void Task::resetBurstTime() noexcept {
    remainingBurstTime_ = initialBurstTime_;
}

void Task::markTerminated() {
    setState(TaskState::Terminated);
}

Task::ExecutionResult Task::execute() {
    if (state_ == TaskState::Terminated) {
        return ExecutionResult::Completed;
    }

    if (!canTransitionTo(TaskState::Running)) {
        return ExecutionResult::Completed;
    }

    setState(TaskState::Running);

    if (!entry_) {
        markTerminated();
        return ExecutionResult::Completed;
    }

    entry_(*this, [this]() { this->yield(); });

    if (state_ == TaskState::Running) {
        markTerminated();
        return ExecutionResult::Completed;
    }

    if (state_ == TaskState::Ready) {
        return ExecutionResult::Yielded;
    }

    return ExecutionResult::Completed;
}

void Task::yield() {
    if (state_ == TaskState::Running) {
        setState(TaskState::Ready);
    }

    if (yieldCallback_) {
        yieldCallback_();
    }
}

void Task::setYieldCallback(YieldCallback callback) noexcept {
    yieldCallback_ = std::move(callback);
}

} // namespace mini_rtos
