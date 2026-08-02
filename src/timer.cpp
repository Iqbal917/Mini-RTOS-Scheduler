#include "mini_rtos/timer.h"

namespace mini_rtos {

Timer::Timer(std::chrono::milliseconds period)
    : period_(period), running_(false), elapsed_(0), totalTicks_(0) {
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

void Timer::tick() {
    if (!running_) {
        return;
    }

    ++elapsed_;
    ++totalTicks_;

    if (static_cast<std::chrono::milliseconds::rep>(elapsed_) >= period_.count()) {
        elapsed_ = 0;
        if (callback_) {
            callback_(totalTicks_);
        }
    }
}

TickCount Timer::elapsedTicks() const noexcept {
    return totalTicks_;
}

} // namespace mini_rtos
