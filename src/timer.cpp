#include "mini_rtos/timer.h"

namespace mini_rtos {

Timer::Timer(std::chrono::milliseconds period) : period_(period), running_(false) {
}

void Timer::start() {
    running_ = true;
}

void Timer::stop() {
    running_ = false;
}

bool Timer::running() const noexcept {
    return running_;
}

std::chrono::milliseconds Timer::period() const noexcept {
    return period_;
}

void Timer::setPeriod(std::chrono::milliseconds period) noexcept {
    period_ = period;
}

void Timer::onTick(TickCallback callback) {
    callback_ = std::move(callback);
}

} // namespace mini_rtos
