#pragma once 

#include <cstdint>
#include "trading/Order.hpp"
#include <deque>
#include <map>
#include <optional>
#include <functional>

namespace trading {
    using PriceLevel = std::deque<Order>;
    using Bids = std::map<Price, PriceLevel, std::greater<Price>>;
    using Asks = std::map<Price, PriceLevel>;



    class OrderBook {
        public:
            void add(Order order); 
            [[nodiscard]] bool cancel(OrderId id);
            [[nodiscard]] bool contains(OrderId id) const noexcept;
            // Fill the oldest order at the best price on the specified side.
            void fill_best(Side side, Quantity quantity);
            [[nodiscard]] std::optional<Price> best_bid() const noexcept;
            [[nodiscard]] std::optional<Price> best_ask() const noexcept;
            // Snapshots of the oldest orders at the best price levels.
            [[nodiscard]] std::optional<Order> best_bid_order() const;
            [[nodiscard]] std::optional<Order> best_ask_order() const;

        private:
            Bids bids_;
            Asks asks_; 

    };



}
