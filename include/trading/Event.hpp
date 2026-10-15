#pragma once

#include "trading/Order.hpp"
#include <variant>

namespace trading {
struct NewOrderEvent { Order order; };
struct CancelOrderEvent { OrderId id; Timestamp timestamp; };
using Event = std::variant<NewOrderEvent, CancelOrderEvent>;
} // namespace trading
