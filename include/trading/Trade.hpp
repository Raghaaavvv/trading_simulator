#pragma once

#include "trading/Order.hpp"

namespace trading {

struct Trade {
    OrderId maker_id; // Resting order
    OrderId taker_id; // Incoming order
    Side taker_side;
    Price price;
    Quantity quantity;
    Timestamp timestamp; // Incoming event's timestamp
};

} // namespace trading
