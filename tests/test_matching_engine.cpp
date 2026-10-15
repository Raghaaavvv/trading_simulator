#include "trading/MatchingEngine.hpp"

#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
int failures = 0;
void expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        ++failures;
    }
}

void test_price_time_priority() {
    for (auto maker_side : {trading::Side::Buy, trading::Side::Sell}) {
        trading::MatchingEngine engine;
        const auto taker_side = maker_side == trading::Side::Buy
            ? trading::Side::Sell : trading::Side::Buy;
        const trading::Price worse = maker_side == trading::Side::Buy ? 9900 : 10200;
        const trading::Price better = maker_side == trading::Side::Buy ? 10000 : 10100;
        (void)engine.submit({1, maker_side, trading::OrderType::Limit, worse, 50, 1});
        (void)engine.submit({2, maker_side, trading::OrderType::Limit, better, 30, 2});
        (void)engine.submit({3, maker_side, trading::OrderType::Limit, better, 40, 2});
        const auto result = engine.submit(
            {4, taker_side, trading::OrderType::Limit, worse, 90, 4});
        expect(result.trades.size() == 3, "sweep produces three executions");
        if (result.trades.size() == 3) {
            expect(result.trades[0].maker_id == 2 && result.trades[1].maker_id == 3 &&
                   result.trades[2].maker_id == 1, "price then FIFO priority on both sides");
            expect(result.trades[0].quantity == 30 && result.trades[1].quantity == 40 &&
                   result.trades[2].quantity == 20, "sweep conserves executed quantity");
            expect(result.trades[0].price == better && result.trades[2].price == worse,
                   "executions use maker price, including price improvement");
            expect(result.trades[0].taker_id == 4 &&
                   result.trades[0].taker_side == taker_side &&
                   result.trades[0].timestamp == 4, "trade records incoming event data");
        }
        expect(result.remaining_quantity == 0 && !result.resting, "taker fully fills");
        const auto remaining = maker_side == trading::Side::Buy
            ? engine.book().best_bid_order() : engine.book().best_ask_order();
        expect(remaining && remaining->id() == 1 && remaining->remaining_quantity() == 30,
               "partially filled maker retains remainder");
        expect(!engine.book().contains(2) && !engine.book().contains(3),
               "fully filled makers leave book");
    }
}

void test_limit_boundaries_and_remainder() {
    for (auto side : {trading::Side::Buy, trading::Side::Sell}) {
        trading::MatchingEngine engine;
        const auto opposite = side == trading::Side::Buy ? trading::Side::Sell : trading::Side::Buy;
        const trading::Price outside = side == trading::Side::Buy ? 10100 : 9900;
        (void)engine.submit({1, opposite, trading::OrderType::Limit, 10000, 30, 1});
        (void)engine.submit({2, opposite, trading::OrderType::Limit, outside, 40, 2});
        const auto result = engine.submit({3, side, trading::OrderType::Limit, 10000, 50, 3});
        expect(result.trades.size() == 1 && result.trades.front().quantity == 30,
               "limit crosses at equality and stops beyond its limit");
        expect(result.resting && result.remaining_quantity == 20,
               "unfilled limit quantity rests");
        const auto remainder = side == trading::Side::Buy
            ? engine.book().best_bid_order() : engine.book().best_ask_order();
        expect(remainder && remainder->id() == 3 && remainder->original_quantity() == 50 &&
               remainder->remaining_quantity() == 20, "resting remainder preserves order data");
        expect(engine.book().contains(2), "out-of-limit maker stays untouched");
        const auto no_match = engine.submit(
            {4, side, trading::OrderType::Limit, 10000, 10, 4});
        expect(no_match.trades.empty() && no_match.resting,
               "noncrossing limit order rests without execution");
    }
}

void test_market_orders() {
    for (auto side : {trading::Side::Buy, trading::Side::Sell}) {
        trading::MatchingEngine engine;
        const auto opposite = side == trading::Side::Buy ? trading::Side::Sell : trading::Side::Buy;
        const auto empty = engine.submit({1, side, trading::OrderType::Market, std::nullopt, 10, 1});
        expect(empty.trades.empty() && !empty.resting && empty.remaining_quantity == 10,
               "market order on empty book is cancelled");
        (void)engine.submit({2, opposite, trading::OrderType::Limit, 10000, 30, 2});
        const auto partial = engine.submit({3, side, trading::OrderType::Market, std::nullopt, 50, 3});
        expect(partial.trades.size() == 1 && partial.remaining_quantity == 20 && !partial.resting,
               "market order consumes liquidity and cancels excess");
        expect(!engine.book().best_bid() && !engine.book().best_ask(),
               "market remainder never enters book");
        (void)engine.submit({4, opposite, trading::OrderType::Limit, 10000, 30, 4});
        const auto full = engine.submit({5, side, trading::OrderType::Market, std::nullopt, 10, 5});
        expect(full.remaining_quantity == 0 && !full.resting && full.trades.size() == 1,
               "market order can fully fill");
    }
}

void test_rejection_and_cancellation() {
    trading::MatchingEngine engine;
    (void)engine.submit({1, trading::Side::Buy, trading::OrderType::Limit, 10000, 20, 1});
    (void)engine.submit({2, trading::Side::Sell, trading::OrderType::Limit, 10100, 30, 2});
    try {
        (void)engine.submit({1, trading::Side::Buy, trading::OrderType::Market, std::nullopt, 30, 3});
        expect(false, "duplicate incoming ID must throw before matching");
    } catch (const std::invalid_argument&) {}
    const auto ask = engine.book().best_ask_order();
    expect(ask && ask->remaining_quantity() == 30 && engine.book().contains(1),
           "duplicate rejection leaves both sides unchanged");
    trading::Order filled{3, trading::Side::Buy, trading::OrderType::Limit, 10100, 10, 3};
    filled.fill(10);
    try {
        (void)engine.submit(filled);
        expect(false, "fully filled incoming order must throw");
    } catch (const std::invalid_argument&) {}
    expect(engine.cancel(1) && !engine.cancel(1), "engine delegates cancellation");
    (void)engine.submit({1, trading::Side::Buy, trading::OrderType::Limit, 10100, 30, 4});
    expect(!engine.book().best_bid() && !engine.book().best_ask(),
           "cancelled ID can be reused and exact fill empties book");
}

void test_fill_guards() {
    trading::OrderBook book;
    try {
        book.fill_best(trading::Side::Buy, 1);
        expect(false, "empty side cannot be filled");
    } catch (const std::invalid_argument&) {}
    book.add({1, trading::Side::Buy, trading::OrderType::Limit, 100, 10, 1});
    for (trading::Quantity quantity : {0, 11}) {
        try {
            book.fill_best(trading::Side::Buy, quantity);
            expect(false, "zero fill and overfill must throw");
        } catch (const std::invalid_argument&) {}
    }
    const auto order = book.best_bid_order();
    expect(order && order->remaining_quantity() == 10,
           "invalid book fill leaves quantity unchanged");
}
} // namespace

int main() {
    test_price_time_priority();
    test_limit_boundaries_and_remainder();
    test_market_orders();
    test_rejection_and_cancellation();
    test_fill_guards();
    if (failures) {
        std::cerr << failures << " matching-engine checks failed\n";
        return 1;
    }
    std::cout << "All matching-engine tests passed\n";
    return 0;
}
