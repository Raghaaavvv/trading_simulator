#include "trading/OrderBook.hpp"
#include <optional>

namespace trading {





    // void OrderBook::add(Order order) {
    //     std::optional<Price> orderPrice = order.price();

        
    //     bids_.insert(order);
    // }

    // bool OrderBook::cancel(OrderId id) {
    //     if (bids_.contains(id)) {
    //         bids_.erase(id);
    //         return true;
    //     }
    //     if (asks_.contains(id)) {
    //         asks_.erase(id);
    //         return true;
    //     }
    //     return false;

    // }

    std::optional<Price> OrderBook::best_bid() const noexcept {
        if (bids_.empty()) {
            return std::nullopt;
        }
        return bids_.begin()->first;
    }
    std::optional<Price> OrderBook::best_ask() const noexcept {
        if (asks_.empty()) {
            return std::nullopt;
        }
        return asks_.begin()->first;
    }
} //namespace trading