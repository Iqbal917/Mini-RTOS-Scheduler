#include "mini_rtos/round_robin_scheduler.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

using namespace mini_rtos;

void runDemo() {
    RoundRobinScheduler scheduler(3);

    std::vector<std::shared_ptr<Task>> tasks;

    tasks.push_back(std::make_shared<Task>(
        1,
        "A",
        100,
        64,
        [](Task& task, Task::YieldCallback yield) {
            for (int i = 0; i < 3; ++i) {
                std::cout << "Task " << task.name() << " step " << i + 1 << "\n";
                if (i == 1) {
                    yield();
                }
            }
        },
        8));

    tasks.push_back(std::make_shared<Task>(
        2,
        "B",
        90,
        64,
        [](Task& task, Task::YieldCallback yield) {
            for (int i = 0; i < 2; ++i) {
                std::cout << "Task " << task.name() << " step " << i + 1 << "\n";
                if (i == 0) {
                    yield();
                }
            }
        },
        5));

    tasks.push_back(std::make_shared<Task>(
        3,
        "C",
        80,
        64,
        [](Task& task, Task::YieldCallback yield) {
            for (int i = 0; i < 4; ++i) {
                std::cout << "Task " << task.name() << " step " << i + 1 << "\n";
                if (i == 1) {
                    yield();
                }
            }
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

int main() {
    runDemo();
    return 0;
}
