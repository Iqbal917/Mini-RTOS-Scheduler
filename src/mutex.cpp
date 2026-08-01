#include "mini_rtos/mutex.h"

namespace mini_rtos {

bool Mutex::tryLock(std::shared_ptr<Task> owner) {
    if (locked_) {
        return false;
    }

    locked_ = true;
    owner_ = std::move(owner);
    return true;
}

void Mutex::unlock() {
    locked_ = false;
    owner_.reset();
}

void Mutex::addWaitingTask(std::shared_ptr<Task> task) {
    if (task) {
        waiters_.push_back(std::move(task));
    }
}

void Mutex::setWakeCallback(WakeCallback callback) {
    wakeCallback_ = std::move(callback);
}

bool Mutex::isLocked() const noexcept {
    return locked_;
}

std::shared_ptr<Task> Mutex::owner() const noexcept {
    return owner_;
}

} // namespace mini_rtos
