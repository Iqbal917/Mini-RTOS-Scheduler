#pragma once

#include "mini_rtos/common.h"
#include "mini_rtos/task.h"

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace mini_rtos {

class Mutex {
public:
    using WakeCallback = std::function<void(std::shared_ptr<Task>)>;

    Mutex() = default;

    bool tryLock(std::shared_ptr<Task> owner);
    void unlock();
    void addWaitingTask(std::shared_ptr<Task> task);
    void setWakeCallback(WakeCallback callback);
    bool isLocked() const noexcept;
    std::shared_ptr<Task> owner() const noexcept;

private:
    bool locked_ = false;
    std::shared_ptr<Task> owner_;
    std::vector<std::shared_ptr<Task>> waiters_;
    WakeCallback wakeCallback_;
};

} // namespace mini_rtos
