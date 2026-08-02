#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mini_rtos {

using TaskId = std::size_t;

/// Priority convention used throughout this project: HIGHER numeric value =
/// HIGHER priority (runs first). E.g. Priority 100 preempts Priority 10.
/// This matches the convention used by FreeRTOS; it is the opposite of POSIX
/// nice values, so it's called out explicitly here to avoid ambiguity.
using Priority = std::uint32_t;

using TickCount = std::size_t;

enum class TaskState {
    Ready,
    Running,
    Blocked,
    Terminated
};

} // namespace mini_rtos
