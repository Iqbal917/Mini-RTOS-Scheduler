# Mini RTOS Scheduler

This project is an educational C++17 simulation of the scheduling core of a real-time operating system.

## Design goals

- Separate task management from scheduling policy.
- Model a simple preemptive scheduler without relying on external libraries.
- Keep the implementation portable and buildable with CMake on Linux.

## Planned structure

- include/mini_rtos/common.h
- include/mini_rtos/task.h
- include/mini_rtos/scheduler.h
- include/mini_rtos/round_robin_scheduler.h
- include/mini_rtos/priority_scheduler.h
- include/mini_rtos/rate_monotonic_scheduler.h
- include/mini_rtos/mutex.h
- include/mini_rtos/semaphore.h
- include/mini_rtos/timer.h
- include/mini_rtos/context_switch_log.h
- src/task.cpp
- src/scheduler.cpp
- src/round_robin_scheduler.cpp
- src/priority_scheduler.cpp
- src/rate_monotonic_scheduler.cpp
- src/mutex.cpp
- src/semaphore.cpp
- src/timer.cpp
- src/context_switch_log.cpp
- src/main.cpp

## Scheduling policies

- Round-robin: fixed time quantum, preemptive.
- Priority: static priority with optional priority inheritance support.
- Rate monotonic: periodic tasks receive higher priority when their period is shorter.

## Context switching

The simulation uses cooperative yielding and explicit scheduler decisions rather than real hardware context switching. In a real embedded system, this would involve saving CPU registers, the stack pointer, and the program counter, then restoring them from a saved context frame.

## Priority inversion demo

The priority inversion demo will show a low-priority task holding a mutex while a high-priority task waits. The design includes a priority inheritance mechanism so the low-priority task temporarily inherits the higher priority while holding the lock.
