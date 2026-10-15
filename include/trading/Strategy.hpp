#pragma once

#include "trading/Event.hpp"
#include "trading/MatchingEngine.hpp"
#include <string>
#include <vector>

namespace trading {
// Values only: no reference or pointer to mutable exchange state.
struct MarketView {
    std::optional<Price> best_bid;
    std::optional<Price> best_ask;
    std::uint64_t sequence;
    Timestamp timestamp;
};

struct StrategyFill {
    OrderId order_id;
    Side side;
    Trade trade;
    std::uint64_t sequence;
};

struct ActionResult {
    OrderId id;
    std::variant<MatchResult, bool, std::string> outcome;
};

class Strategy {
public:
    virtual ~Strategy() = default;
    // Called once after each external event. Requests are processed in returned
    // order before the next external event; requests do not recursively call this.
    [[nodiscard]] virtual std::vector<Event> on_market_update(const MarketView& view) = 0;
    virtual void on_fill(const StrategyFill&) {}
    virtual void on_action_result(const ActionResult&) {}
};

// Educational example: make one limit-buy attempt when the best ask is at or
// below the configured threshold. IDs share the exchange's resting-ID namespace.
class BuyBelowStrategy final : public Strategy {
public:
    BuyBelowStrategy(OrderId id, Price threshold, Quantity quantity);
    [[nodiscard]] std::vector<Event> on_market_update(const MarketView& view) override;
private:
    OrderId id_;
    Price threshold_;
    Quantity quantity_;
    bool attempted_ = false;
};
} // namespace trading
