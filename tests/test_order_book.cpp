#include "trading/OrderBook.hpp"

#include <iostream>
#include <optional>
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
} // namespace



int main() {
    test_empty_book();
    if (failures != 0) {
        std::cerr << failures << " order test(s) failed\n";
        return 1;
    }

    std::cout << "All order-book tests passed\n";
    return 0;

}

