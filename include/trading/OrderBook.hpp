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
            [[nodiscard]] std::optional<Price> best_bid() const noexcept;
            [[nodiscard]] std::optional<Price> best_ask() const noexcept;

        private:
            Bids bids_;
            Asks asks_; 

    };



}