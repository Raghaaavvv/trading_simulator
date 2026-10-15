#include "trading/EventDispatcher.hpp"
#include <iostream>
#include <memory>
#include <type_traits>

namespace {
int failures = 0;
void expect(bool value, const char* text) {
    if (!value) { ++failures; std::cerr << "FAILED: " << text << '\n'; }
}
struct Script final : trading::Strategy {
    std::vector<trading::MarketView> views;
    std::vector<trading::StrategyFill> fills;
    std::vector<trading::ActionResult> feedback;
    std::vector<trading::Event> actions;
    std::vector<trading::Event> on_market_update(const trading::MarketView& view) override {
        views.push_back(view);
        auto result = std::move(actions);
        actions.clear();
        return result;
    }
    void on_fill(const trading::StrategyFill& fill) override { fills.push_back(fill); }
    void on_action_result(const trading::ActionResult& result) override { feedback.push_back(result); }
};

void test_barrier_and_ownership() {
    static_assert(std::is_same_v<decltype(std::declval<trading::EventDispatcher>().engine()),
                                 const trading::MatchingEngine&>);
    auto strategy = std::make_unique<Script>();
    auto* observer = strategy.get();
    observer->actions = {
        trading::CancelOrderEvent{1, 1}, // Another participant's order
        trading::NewOrderEvent{{1, trading::Side::Buy, trading::OrderType::Limit, 99, 10, 1}},
        trading::NewOrderEvent{{100, trading::Side::Buy, trading::OrderType::Limit, 100, 20, 1}}
    };
    trading::EventDispatcher dispatcher(std::move(strategy));
    dispatcher.enqueue(trading::NewOrderEvent{{1, trading::Side::Sell, trading::OrderType::Limit, 100, 10, 1}});
    dispatcher.enqueue(trading::NewOrderEvent{{2, trading::Side::Sell, trading::OrderType::Market, std::nullopt, 5, 0}});
    auto results = dispatcher.run();
    expect(results.size() == 5, "requested actions run before next external event");
    expect(observer->views.size() == 2, "actions do not recursively trigger strategy updates");
    expect(observer->views[0].best_ask == 100 && !observer->views[0].best_bid,
           "snapshot represents state after external event before requests");
    expect(observer->views[0].best_ask == 100, "earlier snapshot stays unchanged");
    expect(observer->feedback.size() == 3, "each strategy request receives feedback");
    expect(std::holds_alternative<std::string>(observer->feedback[0].outcome),
           "strategy cannot cancel other participants' orders");
    expect(std::holds_alternative<std::string>(observer->feedback[1].outcome),
           "strategy duplicate rejected through engine validation");
    expect(observer->fills.size() == 2, "strategy receives taker and later maker fills");
    if (observer->fills.size() == 2) {
        expect(observer->fills[0].order_id == 100 && observer->fills[0].side == trading::Side::Buy &&
               observer->fills[0].trade.quantity == 10, "own taker execution reported");
        expect(observer->fills[1].order_id == 100 && observer->fills[1].side == trading::Side::Buy &&
               observer->fills[1].trade.quantity == 5, "own resting maker execution reported");
    }
    expect(observer->views[1].sequence == 5, "fills and actions finish before next market callback");
    auto order = dispatcher.engine().book().best_bid_order();
    expect(order && order->remaining_quantity() == 5, "strategy requests update book through engine");
    observer->actions = {trading::CancelOrderEvent{100, 2}};
    dispatcher.enqueue(trading::CancelOrderEvent{999, 2});
    (void)dispatcher.run();
    expect(std::get<bool>(observer->feedback.back().outcome), "strategy can cancel own resting order");
    expect(!dispatcher.engine().book().best_bid(), "own cancellation removes remainder");
}

void test_example() {
    auto strategy = std::make_unique<trading::BuyBelowStrategy>(100, 10000, 10);
    trading::EventDispatcher dispatcher(std::move(strategy));
    dispatcher.enqueue(trading::NewOrderEvent{{1, trading::Side::Sell, trading::OrderType::Limit, 10100, 10, 1}});
    dispatcher.enqueue(trading::NewOrderEvent{{2, trading::Side::Sell, trading::OrderType::Limit, 9900, 20, 2}});
    dispatcher.enqueue(trading::CancelOrderEvent{999, 3});
    const auto results = dispatcher.run();
    expect(results.size() == 4, "example requests exactly one order after threshold crossed");
    const auto& trades = dispatcher.monitor().trades();
    expect(trades.size() == 1 && trades[0].maker_id == 2 && trades[0].taker_id == 100 &&
           trades[0].price == 9900 && trades[0].quantity == 10, "example executes at resting ask");
}

void test_no_unrelated_fills_and_id_reuse() {
    auto strategy = std::make_unique<Script>();
    auto* observer = strategy.get();
    observer->actions = {trading::NewOrderEvent{{100, trading::Side::Buy, trading::OrderType::Limit, 100, 10, 1}}};
    trading::EventDispatcher dispatcher(std::move(strategy));
    dispatcher.enqueue(trading::CancelOrderEvent{999, 1});
    dispatcher.enqueue(trading::NewOrderEvent{{1, trading::Side::Sell, trading::OrderType::Market, std::nullopt, 10, 2}});
    dispatcher.enqueue(trading::NewOrderEvent{{100, trading::Side::Sell, trading::OrderType::Limit, 100, 10, 3}});
    dispatcher.enqueue(trading::NewOrderEvent{{2, trading::Side::Buy, trading::OrderType::Market, std::nullopt, 10, 4}});
    (void)dispatcher.run();
    expect(dispatcher.monitor().trades().size() == 2 && observer->fills.size() == 1,
           "completed strategy ID reused externally does not inherit ownership");
}
} // namespace
int main() {
    test_barrier_and_ownership();
    test_example();
    test_no_unrelated_fills_and_id_reuse();
    if (failures) return 1;
    std::cout << "All strategy tests passed\n";
}
