#include "mini_rtos/semaphore.h"

namespace mini_rtos {

Semaphore::Semaphore(std::size_t initialCount) : count_(initialCount) {
}

void Semaphore::signal() {
    ++count_;
}

void Semaphore::wait() {
    if (count_ > 0) {
        --count_;
    }
}

std::size_t Semaphore::count() const noexcept {
    return count_;
}

} // namespace mini_rtos
