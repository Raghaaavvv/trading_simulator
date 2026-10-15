#include "trading/MatchingEngine.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace trading {

MatchResult MatchingEngine::submit(Order order) {
    if (order.side() != Side::Buy && order.side() != Side::Sell) {
        throw std::invalid_argument("invalid order side");
    }
    if (order.type() != OrderType::Limit && order.type() != OrderType::Market) {
        throw std::invalid_argument("invalid order type");
    }
    if (order.is_filled()) {
        throw std::invalid_argument("cannot submit a fully filled order");
    }
    if (book_.contains(order.id())) {
        throw std::invalid_argument("order ID already exists in the book");
    }

    // Stage the submission so allocation failures cannot leave partial trades
    // committed to the book. This correctness-first copy is a profiling target.
    OrderBook next = book_;
    MatchResult result{{}, order.remaining_quantity(), false};
    const Side opposite = order.side() == Side::Buy ? Side::Sell : Side::Buy;
    while (!order.is_filled()) {
        const auto maker = opposite == Side::Sell ? next.best_ask_order()
                                                 : next.best_bid_order();
        if (!maker) {
            break;
        }
        const Price price = *maker->price();
        if (order.type() == OrderType::Limit) {
            const bool crosses = order.side() == Side::Buy
                ? price <= *order.price() : price >= *order.price();
            if (!crosses) {
                break;
            }
        }
        const Quantity quantity = std::min(order.remaining_quantity(),
                                           maker->remaining_quantity());
        result.trades.push_back({maker->id(), order.id(), order.side(), price,
                                 quantity, order.timestamp()});
        next.fill_best(opposite, quantity);
        order.fill(quantity);
    }
    result.remaining_quantity = order.remaining_quantity();
    result.resting = !order.is_filled() && order.type() == OrderType::Limit;
    if (result.resting) {
        next.add(std::move(order));
    }
    book_ = std::move(next);
    return result;
}

bool MatchingEngine::cancel(OrderId id) {
    return book_.cancel(id);
}

const OrderBook& MatchingEngine::book() const noexcept {
    return book_;
}

} // namespace trading
