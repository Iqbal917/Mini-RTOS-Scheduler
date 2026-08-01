#pragma once

#include "mini_rtos/common.h"
#include "mini_rtos/task.h"

#include <memory>
#include <vector>

namespace mini_rtos {

class Scheduler {
public:
    using TaskPtr = std::shared_ptr<Task>;

    virtual ~Scheduler() = default;

    virtual void addTask(TaskPtr task) = 0;
    virtual TaskPtr pickNextTask() = 0;
    virtual void tick(std::size_t elapsedTicks = 1) = 0;

    void enqueueReady(TaskPtr task);
    void enqueueBlocked(TaskPtr task);
    /// Removes a task from all scheduler queues and marks it terminated.
    /// This keeps the task state consistent even when it is no longer tracked.
    void removeTask(TaskPtr task);

    TaskPtr currentTask() const noexcept;
    const std::vector<TaskPtr>& readyQueue() const noexcept;
    const std::vector<TaskPtr>& blockedQueue() const noexcept;
    std::size_t readyCount() const noexcept;
    std::size_t blockedCount() const noexcept;

protected:
    void setCurrentTask(TaskPtr task) noexcept;
    void clearCurrentTask() noexcept;

    std::vector<TaskPtr> readyQueue_;
    std::vector<TaskPtr> blockedQueue_;
    TaskPtr currentTask_;
};

} // namespace mini_rtos
