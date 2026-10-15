#pragma once

#include "trading/Event.hpp"
#include "trading/MatchingEngine.hpp"
#include "trading/Strategy.hpp"
#include <deque>
#include <memory>
#include <unordered_set>
#include <string>
#include <vector>

namespace trading {

class MarketMonitor {
public:
    void on_trade(const Trade& trade);
    [[nodiscard]] const std::vector<Trade>& trades() const noexcept;
private:
    std::vector<Trade> trades_;
};

struct DispatchResult {
    std::uint64_t sequence;
    OrderId id;
    Timestamp timestamp; // Metadata: never used to reorder events.
    // Match result, cancellation found/not found, or rejected-event message.
    std::variant<MatchResult, bool, std::string> outcome;
    bool strategy_action = false;
};

class EventDispatcher {
public:
    explicit EventDispatcher(std::unique_ptr<Strategy> strategy = nullptr);
    void enqueue(Event event);
    // Consume the queue in arrival order. Invalid orders become rejection
    // results; processing continues with subsequent input events.
    [[nodiscard]] std::vector<DispatchResult> run();
    [[nodiscard]] const MatchingEngine& engine() const noexcept;
    [[nodiscard]] const MarketMonitor& monitor() const noexcept;
private:
    [[nodiscard]] DispatchResult dispatch(const Event& event, bool strategy_action);
    std::deque<Event> queue_;
    MatchingEngine engine_;
    MarketMonitor monitor_;
    std::uint64_t sequence_ = 0;
    std::unique_ptr<Strategy> strategy_;
    std::unordered_set<OrderId> owned_orders_;
};
} // namespace trading
