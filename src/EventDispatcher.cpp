#include "trading/EventDispatcher.hpp"
#include <stdexcept>
#include <utility>

namespace trading {
EventDispatcher::EventDispatcher(std::unique_ptr<Strategy> strategy)
    : strategy_(std::move(strategy)) {}
void MarketMonitor::on_trade(const Trade& trade) { trades_.push_back(trade); }
const std::vector<Trade>& MarketMonitor::trades() const noexcept { return trades_; }
void EventDispatcher::enqueue(Event event) { queue_.push_back(std::move(event)); }

DispatchResult EventDispatcher::dispatch(const Event& event, bool strategy_action) {
        const auto* incoming = std::get_if<NewOrderEvent>(&event);
        const OrderId id = incoming ? incoming->order.id() : std::get<CancelOrderEvent>(event).id;
        const Timestamp timestamp = incoming ? incoming->order.timestamp()
                                            : std::get<CancelOrderEvent>(event).timestamp;
        DispatchResult result{++sequence_, id, timestamp, false, strategy_action};
        std::vector<Trade> trades;
        try {
            if (incoming) {
                auto match = engine_.submit(incoming->order);
                if (strategy_action && match.resting) owned_orders_.insert(id);
                for (const auto& trade : match.trades) {
                    monitor_.on_trade(trade);
                }
                trades = match.trades;
                result.outcome = std::move(match);
            } else {
                if (strategy_action && owned_orders_.count(id) == 0) {
                    throw std::invalid_argument("strategy cannot cancel an order it does not own");
                }
                result.outcome = engine_.cancel(id);
                owned_orders_.erase(id);
            }
        } catch (const std::invalid_argument& error) {
            result.outcome = std::string(error.what());
        }
        // Callbacks are outside rejection handling: a callback bug must not
        // misreport an already executed order as an exchange rejection.
        for (const auto& trade : trades) {
            if (strategy_ && owned_orders_.count(trade.maker_id)) {
                const Side side = trade.taker_side == Side::Buy ? Side::Sell : Side::Buy;
                strategy_->on_fill({trade.maker_id, side, trade, result.sequence});
            }
            if (strategy_ && strategy_action) {
                strategy_->on_fill({trade.taker_id, trade.taker_side, trade, result.sequence});
            }
            if (!engine_.book().contains(trade.maker_id)) owned_orders_.erase(trade.maker_id);
        }
        if (strategy_ && strategy_action) strategy_->on_action_result({id, result.outcome});
        return result;
}

std::vector<DispatchResult> EventDispatcher::run() {
    std::vector<DispatchResult> results;
    while (!queue_.empty()) {
        auto result = dispatch(queue_.front(), false);
        queue_.pop_front();
        const MarketView view{engine_.book().best_bid(), engine_.book().best_ask(),
                              result.sequence, result.timestamp};
        results.push_back(std::move(result));
        if (strategy_) {
            const auto actions = strategy_->on_market_update(view);
            for (const auto& action : actions) results.push_back(dispatch(action, true));
        }
    }
    return results;
}
const MatchingEngine& EventDispatcher::engine() const noexcept { return engine_; }
const MarketMonitor& EventDispatcher::monitor() const noexcept { return monitor_; }
} // namespace trading
