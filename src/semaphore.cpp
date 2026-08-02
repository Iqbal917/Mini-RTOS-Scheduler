#include "mini_rtos/semaphore.h"

#include <algorithm>

namespace mini_rtos {

Semaphore::Semaphore(std::size_t initialCount) : count_(initialCount) {
}

bool Semaphore::wait(std::shared_ptr<Task> requester) {
    if (!requester) {
        return false;
    }

    if (count_ > 0) {
        --count_;
        return true;
    }

    if (std::find(waiters_.begin(), waiters_.end(), requester) == waiters_.end()) {
        waiters_.push_back(requester);
    }
    return false;
}

void Semaphore::signal() {
    if (!waiters_.empty()) {
        auto highest = waiters_.begin();
        for (auto it = waiters_.begin() + 1; it != waiters_.end(); ++it) {
            if ((*it)->priority() > (*highest)->priority()) {
                highest = it;
            }
        }
        std::shared_ptr<Task> next = *highest;
        waiters_.erase(highest);
        if (wakeCallback_) {
            wakeCallback_(next);
        }
        return;
    }

    ++count_;
}

void Semaphore::setWakeCallback(WakeCallback callback) {
    wakeCallback_ = std::move(callback);
}

std::size_t Semaphore::count() const noexcept {
    return count_;
}

const std::vector<std::shared_ptr<Task>>& Semaphore::waiters() const noexcept {
    return waiters_;
}

} // namespace mini_rtos
