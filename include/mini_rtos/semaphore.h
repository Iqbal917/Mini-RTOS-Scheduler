#pragma once

#include <cstddef>

namespace mini_rtos {

class Semaphore {
public:
    explicit Semaphore(std::size_t initialCount = 0);

    void signal();
    void wait();
    std::size_t count() const noexcept;

private:
    std::size_t count_;
};

} // namespace mini_rtos
