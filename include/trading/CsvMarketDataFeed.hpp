#pragma once
#include "trading/Event.hpp"
#include <istream>
#include <vector>

namespace trading {
// Strict, unquoted CSV in exchange-arrival order. Parse completely before replay.
[[nodiscard]] std::vector<Event> read_events(std::istream& input);
} // namespace trading
