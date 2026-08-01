cmake_minimum_required(VERSION 3.16)
project(mini_rtos_scheduler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(mini_rtos STATIC
    src/task.cpp
    src/scheduler.cpp
    src/round_robin_scheduler.cpp
    src/priority_scheduler.cpp
    src/rate_monotonic_scheduler.cpp
    src/mutex.cpp
    src/semaphore.cpp
    src/timer.cpp
    src/context_switch_log.cpp
)

target_include_directories(mini_rtos
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(mini_rtos_demo
    src/main.cpp
)

target_link_libraries(mini_rtos_demo PRIVATE mini_rtos)
