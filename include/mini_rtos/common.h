#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mini_rtos {

using TaskId = std::size_t;
using Priority = std::uint32_t;
using TickCount = std::size_t;

enum class TaskState {
    Ready,
    Running,
    Blocked,
    Terminated
};

} // namespace mini_rtos
