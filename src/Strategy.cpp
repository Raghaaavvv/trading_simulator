#include "trading/Strategy.hpp"
#include <stdexcept>

namespace trading {
BuyBelowStrategy::BuyBelowStrategy(OrderId id, Price threshold, Quantity quantity)
    : id_(id), threshold_(threshold), quantity_(quantity) {
    if (!id || threshold <= 0 || !quantity) {
        throw std::invalid_argument("strategy requires positive ID, threshold, and quantity");
    }
}
std::vector<Event> BuyBelowStrategy::on_market_update(const MarketView& view) {
    if (attempted_ || !view.best_ask || *view.best_ask > threshold_) return {};
    attempted_ = true;
    return {NewOrderEvent{Order{id_, Side::Buy, OrderType::Limit,
                               threshold_, quantity_, view.timestamp}}};
}
} // namespace trading
