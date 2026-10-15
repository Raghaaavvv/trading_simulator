#pragma once

#include "trading/OrderBook.hpp"
#include "trading/Trade.hpp"

#include <vector>

namespace trading {

struct MatchResult {
    std::vector<Trade> trades;
    Quantity remaining_quantity;
    bool resting; // False for a cancelled market remainder or a full fill
};

class MatchingEngine {
public:
    [[nodiscard]] MatchResult submit(Order order);
    [[nodiscard]] bool cancel(OrderId id);
    [[nodiscard]] const OrderBook& book() const noexcept;

private:
    OrderBook book_;
};

} // namespace trading
