#include "mini_rtos/mutex.h"

#include <algorithm>

namespace mini_rtos {

Mutex::Mutex(bool priorityInheritanceEnabled) : priorityInheritanceEnabled_(priorityInheritanceEnabled) {
}

bool Mutex::lock(std::shared_ptr<Task> requester) {
    if (!requester) {
        return false;
    }

    if (!locked_) {
        locked_ = true;
        owner_ = std::move(requester);
        return true;
    }

    // Already held. Add to the wait list (avoid duplicates).
    if (std::find(waiters_.begin(), waiters_.end(), requester) == waiters_.end()) {
        waiters_.push_back(requester);
    }

    // Priority inheritance: if the requester outranks the current owner,
    // boost the owner immediately so it can't be starved off the CPU by a
    // medium-priority task while the requester waits. This is the classic
    // fix for unbounded priority inversion.
    if (priorityInheritanceEnabled_ && owner_ && requester->priority() > owner_->priority()) {
        owner_->boostPriority(requester->priority());
    }

    return false;
}

void Mutex::unlock() {
    if (!locked_) {
        return;
    }

    if (owner_) {
        owner_->restorePriority();
    }

    if (waiters_.empty()) {
        locked_ = false;
        owner_.reset();
        return;
    }

    // Hand off to the highest-priority waiter.
    auto highest = waiters_.begin();
    for (auto it = waiters_.begin() + 1; it != waiters_.end(); ++it) {
        if ((*it)->priority() > (*highest)->priority()) {
            highest = it;
        }
    }

    std::shared_ptr<Task> next = *highest;
    waiters_.erase(highest);

    owner_ = next;
    // locked_ stays true — ownership transfers directly to the next waiter.

    if (wakeCallback_) {
        wakeCallback_(next);
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

const std::vector<std::shared_ptr<Task>>& Mutex::waiters() const noexcept {
    return waiters_;
}

bool Mutex::priorityInheritanceEnabled() const noexcept {
    return priorityInheritanceEnabled_;
}

} // namespace mini_rtos
