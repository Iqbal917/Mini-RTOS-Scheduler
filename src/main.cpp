#include "mini_rtos/round_robin_scheduler.h"
#include "mini_rtos/priority_scheduler.h"
#include "mini_rtos/rate_monotonic_scheduler.h"
#include "mini_rtos/mutex.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

using namespace mini_rtos;

void runDemo() {
    RoundRobinScheduler scheduler(3);

    std::vector<std::shared_ptr<Task>> tasks;

    // Each entry_ prints once, at the moment it is dispatched, and does NOT
    // call yield(). This lets remainingBurstTime/quantum bookkeeping alone
    // drive the schedule, producing a classic round-robin Gantt chart with
    // real "quantum_expired" and "completed" switches. (See README for why
    // entry_ can only run once per dispatch in this cooperative model — a
    // task that DOES call yield() releases the CPU immediately instead of
    // waiting for its quantum; that path is exercised in the priority demo.)
    tasks.push_back(std::make_shared<Task>(
        1, "A", 100, 64,
        [](Task& task, Task::YieldCallback) {
            std::cout << "Task " << task.name() << " dispatched (burst="
                      << task.remainingBurstTime() << ")\n";
        },
        8));

    tasks.push_back(std::make_shared<Task>(
        2, "B", 90, 64,
        [](Task& task, Task::YieldCallback) {
            std::cout << "Task " << task.name() << " dispatched (burst="
                      << task.remainingBurstTime() << ")\n";
        },
        5));

    tasks.push_back(std::make_shared<Task>(
        3, "C", 80, 64,
        [](Task& task, Task::YieldCallback) {
            std::cout << "Task " << task.name() << " dispatched (burst="
                      << task.remainingBurstTime() << ")\n";
        },
        10));

    for (const auto& task : tasks) {
        scheduler.addTask(task);
    }

    std::cout << "Round Robin demo" << std::endl;
    for (int tick = 0; tick < 25; ++tick) {
        scheduler.tick(1);
        if (scheduler.currentTask()) {
            std::cout << "tick " << tick + 1 << " -> " << scheduler.currentTask()->name() << std::endl;
        } else {
            std::cout << "tick " << tick + 1 << " -> idle" << std::endl;
        }
    }

    std::cout << "\nContext switch log" << std::endl;
    for (const auto& event : scheduler.log().events()) {
        std::cout << "tick=" << event.tick
                  << " from=" << event.fromTask
                  << " to=" << event.toTask
                  << " reason=" << event.reason << std::endl;
    }
}

} // namespace

void runPriorityDemo() {
    using namespace mini_rtos;
    PriorityScheduler scheduler;

    // Priority convention: higher number = higher priority (see common.h).
    auto low = std::make_shared<Task>(
        1, "Low", 10, 64,
        [](Task& task, Task::YieldCallback) {
            std::cout << "Task " << task.name() << " dispatched (burst="
                      << task.remainingBurstTime() << ")\n";
        },
        6);

    auto mid = std::make_shared<Task>(
        2, "Mid", 20, 64,
        [](Task& task, Task::YieldCallback) {
            std::cout << "Task " << task.name() << " dispatched (burst="
                      << task.remainingBurstTime() << ")\n";
        },
        4);

    // High priority task is NOT added until tick 5, to demonstrate that it
    // preempts whichever lower-priority task is already running the instant
    // it becomes ready — it doesn't wait for the current task to finish.
    auto high = std::make_shared<Task>(
        3, "High", 100, 64,
        [](Task& task, Task::YieldCallback) {
            std::cout << "Task " << task.name() << " dispatched (burst="
                      << task.remainingBurstTime() << ")\n";
        },
        3);

    scheduler.addTask(low);
    scheduler.addTask(mid);

    std::cout << "\nPriority Scheduler demo (preemption)" << std::endl;
    for (int tick = 0; tick < 16; ++tick) {
        if (tick == 4) { // arrives before the 5th tick() call
            std::cout << "-- Task High becomes ready --\n";
            scheduler.addTask(high);
        }
        scheduler.tick(1);
        if (scheduler.currentTask()) {
            std::cout << "tick " << tick + 1 << " -> " << scheduler.currentTask()->name() << std::endl;
        } else {
            std::cout << "tick " << tick + 1 << " -> idle" << std::endl;
        }
    }

    std::cout << "\nContext switch log" << std::endl;
    for (const auto& event : scheduler.log().events()) {
        std::cout << event << std::endl;
    }
}

void runRateMonotonicDemo() {
    using namespace mini_rtos;
    using namespace std::chrono_literals;
    RateMonotonicScheduler scheduler;

    // Classic RMS textbook example: two periodic tasks.
    // Task Fast: period=4 ticks, needs 1 tick of CPU work per instance.
    // Task Slow: period=8 ticks, needs 2 ticks of CPU work per instance.
    // RMS assigns Fast the higher priority since it has the shorter period —
    // it should preempt Slow every time both are ready simultaneously.
    auto fast = std::make_shared<Task>(
        1, "Fast", 0, 64,
        [](Task& task, Task::YieldCallback) {
            std::cout << "Task " << task.name() << " running (burst="
                      << task.remainingBurstTime() << ")\n";
        },
        1, std::chrono::milliseconds(4));

    auto slow = std::make_shared<Task>(
        2, "Slow", 0, 64,
        [](Task& task, Task::YieldCallback) {
            std::cout << "Task " << task.name() << " running (burst="
                      << task.remainingBurstTime() << ")\n";
        },
        2, std::chrono::milliseconds(8));

    scheduler.addTask(fast);
    scheduler.addTask(slow);

    std::cout << "\nRate Monotonic Scheduler demo" << std::endl;
    std::cout << "(Fast: period=4 burst=1, Slow: period=8 burst=2 -- Fast should always preempt Slow)" << std::endl;
    for (int tick = 0; tick < 16; ++tick) {
        scheduler.tick(1);
        if (scheduler.currentTask()) {
            std::cout << "tick " << tick + 1 << " -> " << scheduler.currentTask()->name() << std::endl;
        } else {
            std::cout << "tick " << tick + 1 << " -> idle" << std::endl;
        }
    }

    std::cout << "\nContext switch log" << std::endl;
    for (const auto& event : scheduler.log().events()) {
        std::cout << event << std::endl;
    }
}

void runMutexDemo(bool priorityInheritance) {
    using namespace mini_rtos;
    PriorityScheduler scheduler(priorityInheritance);
    auto mutex = std::make_shared<Mutex>(priorityInheritance);
    mutex->setWakeCallback([&scheduler](std::shared_ptr<Task> woken) {
        scheduler.enqueueReady(std::move(woken));
    });

    std::shared_ptr<Task> low, medium, high;

    // Low priority task: acquires the mutex on its first dispatch and holds
    // it for a 6-tick "critical section". Priority inheritance (if enabled)
    // needs Low to be preemptible by Medium so there's something to invert.
    low = std::make_shared<Task>(
        1, "Low", 10, 64,
        [mutex, &low](Task&, Task::YieldCallback) {
            if (mutex->owner() == low) {
                std::cout << "  Low: continuing critical section (burst=" << low->remainingBurstTime() << ")\n";
                return;
            }
            mutex->lock(low);
            std::cout << "  Low: acquired mutex, entering critical section (burst=" << low->remainingBurstTime() << ")\n";
        },
        6);

    // Medium priority task: pure CPU-bound work, no mutex involvement at all.
    // It has nothing to do with the mutex but its priority sits BETWEEN Low
    // and High -- that's what makes the inversion possible.
    medium = std::make_shared<Task>(
        2, "Medium", 50, 64,
        [](Task& task, Task::YieldCallback) {
            std::cout << "  Medium: running CPU-bound work (burst=" << task.remainingBurstTime() << ")\n";
        },
        10);

    // High priority task: wants the same mutex Low holds.
    high = std::make_shared<Task>(
        3, "High", 100, 64,
        [mutex, &high, &scheduler](Task&, Task::YieldCallback) {
            if (mutex->owner() == high) {
                std::cout << "  High: continuing critical section (burst=" << high->remainingBurstTime() << ")\n";
                return;
            }
            if (mutex->lock(high)) {
                std::cout << "  High: acquired mutex immediately (burst=" << high->remainingBurstTime() << ")\n";
            } else {
                std::cout << "  High: BLOCKED waiting for mutex (held by "
                          << (mutex->owner() ? mutex->owner()->name() : "?") << ")\n";
                scheduler.enqueueBlocked(high);
            }
        },
        2);

    scheduler.addTask(low);

    std::cout << "\n=== Mutex demo: priority inheritance "
              << (priorityInheritance ? "ENABLED" : "DISABLED") << " ===" << std::endl;

    for (int tick = 0; tick < 20; ++tick) {
        if (tick == 1) {
            std::cout << "-- Medium becomes ready --\n";
            scheduler.addTask(medium);
        }
        if (tick == 3) {
            std::cout << "-- High becomes ready --\n";
            scheduler.addTask(high);
        }

        scheduler.tick(1);

        // The scheduling core has no built-in "task completed" hook for
        // external resource cleanup, so the demo harness detects Low's
        // completion here and releases the mutex on its behalf.
        if (mutex->owner() == low && low->isTerminated()) {
            std::cout << "-- Low releases mutex --\n";
            mutex->unlock();
        }

        std::cout << "tick " << tick + 1 << " -> "
                  << (scheduler.currentTask() ? scheduler.currentTask()->name() : "idle") << std::endl;
    }

    std::cout << "\nContext switch log" << std::endl;
    for (const auto& event : scheduler.log().events()) {
        std::cout << event << std::endl;
    }
}

int main() {
    runDemo();
    runPriorityDemo();
    runRateMonotonicDemo();
    runMutexDemo(false);
    runMutexDemo(true);
    return 0;
}
