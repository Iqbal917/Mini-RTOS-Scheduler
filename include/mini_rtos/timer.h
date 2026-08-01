#pragma once

#include "mini_rtos/common.h"

#include <chrono>
#include <functional>

namespace mini_rtos {

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

private:
    std::chrono::milliseconds period_;
    bool running_;
    TickCallback callback_;
};

} // namespace mini_rtos
