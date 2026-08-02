#pragma once

#include "mini_rtos/common.h"

#include <chrono>
#include <functional>

namespace mini_rtos {

/// A software-driven timer for this simulation. There is no real OS timer
/// or thread here — the demo harness calls tick() once per simulated clock
/// tick, and Timer fires its callback whenever enough simulated ticks have
/// elapsed to complete one 'period'. This mirrors how a real RTOS timer
/// interrupt would drive scheduler ticks, without needing actual threads.
class Timer {
public:
    using TickCallback = std::function<void(TickCount)>;

    explicit Timer(std::chrono::milliseconds period);

    void start();
    void stop();
    bool running() const noexcept;
    std::chrono::milliseconds period() const noexcept;
    void setPeriod(std::chrono::milliseconds period) noexcept;

    void onTick(TickCallback callback);

    /// Advances the timer by one simulated millisecond. If running and a
    /// full period has elapsed, invokes the registered callback with the
    /// current elapsed tick count and resets the internal counter.
    void tick();

    TickCount elapsedTicks() const noexcept;

private:
    std::chrono::milliseconds period_;
    bool running_;
    TickCallback callback_;
    TickCount elapsed_;
    TickCount totalTicks_;
};

} // namespace mini_rtos
