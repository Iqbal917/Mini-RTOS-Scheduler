#pragma once

#include "mini_rtos/common.h"
#include "mini_rtos/task.h"

#include <ostream>
#include <string>
#include <vector>

namespace mini_rtos {

struct ContextSwitchEvent {
    TickCount tick;
    std::string fromTask;
    std::string toTask;
    std::string reason;
};

class ContextSwitchLog {
public:
    void record(ContextSwitchEvent event);
    const std::vector<ContextSwitchEvent>& events() const noexcept;
    void clear() noexcept;

private:
    std::vector<ContextSwitchEvent> events_;
};

std::ostream& operator<<(std::ostream& os, const ContextSwitchEvent& event);

} // namespace mini_rtos
