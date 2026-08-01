#include "mini_rtos/context_switch_log.h"

namespace mini_rtos {

void ContextSwitchLog::record(ContextSwitchEvent event) {
    events_.push_back(std::move(event));
}

const std::vector<ContextSwitchEvent>& ContextSwitchLog::events() const noexcept {
    return events_;
}

void ContextSwitchLog::clear() noexcept {
    events_.clear();
}

std::ostream& operator<<(std::ostream& os, const ContextSwitchEvent& event) {
    os << "tick=" << event.tick << " from=" << event.fromTask << " to=" << event.toTask << " reason=" << event.reason;
    return os;
}

} // namespace mini_rtos
