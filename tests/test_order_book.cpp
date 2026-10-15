#include "trading/OrderBook.hpp"

#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>


namespace {

int failures = 0;

void expect(bool condition, std::string_view description) {
    if (!condition) {
        std::cerr << "FAILED: " << description << '\n';
        ++failures;
    }
}    



void test_empty_book() {
    trading::OrderBook book;

    const auto bid = book.best_bid();
    const auto ask = book.best_ask();

    expect(!bid.has_value(), "empty book has no best bid");
    expect(!ask.has_value(), "empty book has no best ask");
}
void test_add_orders() {
    trading::OrderBook book;
    book.add({1, trading::Side::Buy, trading::OrderType::Limit, 10025, 100, 1});
    expect(book.best_bid() == 10025, "buy order establishes best bid");
    expect(!book.best_ask(), "buy order does not create an ask");

    book.add({2, trading::Side::Buy, trading::OrderType::Limit, 9900, 50, 2});
    expect(book.best_bid() == 10025, "worse bid does not replace best bid");
    book.add({3, trading::Side::Buy, trading::OrderType::Limit, 10050, 50, 3});
    expect(book.best_bid() == 10050, "higher bid becomes best bid");
    book.add({4, trading::Side::Buy, trading::OrderType::Limit, 10050, 25, 4});
    expect(book.best_bid() == 10050, "same-price insertion retains best bid");

    book.add({5, trading::Side::Sell, trading::OrderType::Limit, 10200, 100, 5});
    expect(book.best_ask() == 10200, "sell order establishes best ask");
    book.add({6, trading::Side::Sell, trading::OrderType::Limit, 10300, 50, 6});
    expect(book.best_ask() == 10200, "worse ask does not replace best ask");
    book.add({7, trading::Side::Sell, trading::OrderType::Limit, 10100, 50, 7});
    expect(book.best_ask() == 10100, "lower ask becomes best ask");
    expect(book.best_bid() == 10050, "adding asks preserves best bid");
}

void test_sell_only_book() {
    trading::OrderBook book;
    book.add({1, trading::Side::Sell, trading::OrderType::Limit, 10100, 20, 1});
    expect(book.best_ask() == 10100, "sell-only book has best ask");
    expect(!book.best_bid(), "sell order does not create a bid");
}

void test_rejected_orders() {
    trading::OrderBook book;
    for (const auto side : {trading::Side::Buy, trading::Side::Sell}) {
        try {
            book.add({1, side, trading::OrderType::Market, std::nullopt, 10, 1});
            expect(false, "market orders must be rejected");
        } catch (const std::invalid_argument&) {
            // Expected: market orders cannot rest.
        }
    }

    trading::Order filled{2, trading::Side::Buy, trading::OrderType::Limit, 100, 10, 2};
    filled.fill(10);
    try {
        book.add(filled);
        expect(false, "fully filled orders must be rejected");
    } catch (const std::invalid_argument&) {
        // Expected: there is no remaining quantity to store.
    }
    expect(!book.best_bid() && !book.best_ask(),
           "rejected orders leave the book empty");
}

void test_duplicate_ids() {
    // Exercise both same-side and cross-side duplicates, including an ID
    // stored behind another order in a non-best price level.
    for (const auto original_side : {trading::Side::Buy, trading::Side::Sell}) {
        for (const auto duplicate_side : {trading::Side::Buy, trading::Side::Sell}) {
            for (const trading::Price duplicate_price : {10000, 12000}) {
                trading::OrderBook book;
                book.add({1, original_side, trading::OrderType::Limit, 9000, 10, 1});
                book.add({2, original_side, trading::OrderType::Limit, 10000, 20, 2});
                book.add({42, original_side, trading::OrderType::Limit, 10000, 30, 3});
                book.add({3, original_side, trading::OrderType::Limit, 11000, 40, 4});
                const auto bid_before = book.best_bid();
                const auto ask_before = book.best_ask();

                try {
                    book.add({42, duplicate_side, trading::OrderType::Limit,
                              duplicate_price, 50, 5});
                    expect(false, "duplicate ID must be rejected across the entire book");
                } catch (const std::invalid_argument& error) {
                    expect(std::string_view{error.what()} ==
                               "order ID already exists in the book",
                           "duplicate rejection reports the reason");
                }
                expect(book.best_bid() == bid_before && book.best_ask() == ask_before,
                       "duplicate rejection preserves existing quotes");
            }
        }
    }

    trading::OrderBook book;
    book.add({1, trading::Side::Buy, trading::OrderType::Limit, 10000, 10, 1});
    book.add({2, trading::Side::Buy, trading::OrderType::Limit, 10000, 10, 1});
    book.add({3, trading::Side::Sell, trading::OrderType::Limit, 10000, 10, 1});
    expect(book.best_bid() == 10000 && book.best_ask() == 10000,
           "distinct IDs may share price, quantity, and timestamp");
}
void test_fifo_and_cancellation() {
    for (const auto side : {trading::Side::Buy, trading::Side::Sell}) {
        trading::OrderBook book;
        const auto best_order = [&]() {
            return side == trading::Side::Buy ? book.best_bid_order()
                                             : book.best_ask_order();
        };
        const auto best_price = [&]() {
            return side == trading::Side::Buy ? book.best_bid() : book.best_ask();
        };
        const trading::Price worse = side == trading::Side::Buy ? 9900 : 10100;
        expect(!book.cancel(123), "cancelling from empty book returns false");
        expect(!best_order(), "empty book has no priority order");
        book.add({1, side, trading::OrderType::Limit, worse, 10, 1});
        book.add({2, side, trading::OrderType::Limit, 10000, 20, 2});
        book.add({3, side, trading::OrderType::Limit, 10000, 30, 2});
        book.add({4, side, trading::OrderType::Limit, 10000, 40, 3});
        book.add({5, side, trading::OrderType::Limit, 10000, 50, 4});
        auto first = best_order();
        expect(first && first->id() == 2,
               "better price precedes earlier arrivals at worse prices");
        expect(book.cancel(3), "middle order can be cancelled");
        expect(book.cancel(5), "last order can be cancelled");
        expect(!book.cancel(999), "unknown ID returns false");
        expect(!book.cancel(3), "repeated cancellation returns false");
        first = best_order();
        expect(first && first->id() == 2,
               "cancelling other orders preserves FIFO front");
        expect(best_price() == 10000,
               "nonempty level survives cancellation");
        expect(book.cancel(2), "oldest order can be cancelled");
        first = best_order();
        expect(first && first->id() == 4,
               "next surviving order takes priority");
        expect(book.cancel(4), "final order at best price can be cancelled");
        expect(best_price() == worse,
               "empty best level is removed and next price becomes best");
        expect(book.cancel(1), "final order on side can be cancelled");
        expect(!best_price() && !best_order(),
               "fully cancelled side has no quote or priority order");
    }
}

void test_cancellation_isolation_and_reinsertion() {
    trading::OrderBook book;
    book.add({1, trading::Side::Buy, trading::OrderType::Limit, 10000, 10, 1});
    book.add({2, trading::Side::Sell, trading::OrderType::Limit, 10100, 20, 2});
    book.add({3, trading::Side::Sell, trading::OrderType::Limit, 10200, 30, 3});
    expect(book.cancel(3), "non-best ask level can be removed");
    expect(book.best_bid() == 10000 && book.best_ask() == 10100,
           "removing non-best ask preserves both quotes");
    expect(book.cancel(1), "bid can be removed while asks remain");
    expect(!book.best_bid() && book.best_ask() == 10100,
           "bid cancellation does not affect asks");
    // IDs are unique among resting orders; cancellation releases the ID.
    book.add({1, trading::Side::Sell, trading::OrderType::Limit, 10100, 40, 4});
    auto first = book.best_ask_order();
    expect(first && first->id() == 2, "reinserted ID joins the back of the queue");
    expect(book.cancel(2), "original ask can be cancelled");
    first = book.best_ask_order();
    expect(first && first->id() == 1 && first->remaining_quantity() == 40,
           "reinserted order retains its own data");
    expect(book.cancel(1), "reinserted order can be cancelled");
    expect(!book.best_bid() && !book.best_ask(), "both sides finish empty");
}
} // namespace



int main() {
    test_empty_book();
    test_add_orders();
    test_sell_only_book();
    test_rejected_orders();
    test_duplicate_ids();
    test_fifo_and_cancellation();
    test_cancellation_isolation_and_reinsertion();
    if (failures != 0) {
        std::cerr << failures << " order test(s) failed\n";
        return 1;
    }

    std::cout << "All order-book tests passed\n";
    return 0;

}

