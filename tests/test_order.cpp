#include "trading/Order.hpp"

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

template <typename Function>
void expect_invalid_argument(Function function, std::string_view description) {
    try {
        function();
        std::cerr << "FAILED: " << description
                  << " (no exception was thrown)\n";
        ++failures;
    } catch (const std::invalid_argument&) {
        // This is the expected result.
    } catch (...) {
        std::cerr << "FAILED: " << description
                  << " (wrong exception type)\n";
        ++failures;
    }
}

void test_valid_limit_order_and_fills() {
    trading::Order order{
        1,
        trading::Side::Buy,
        trading::OrderType::Limit,
        std::optional<trading::Price>{10025},
        100,
        1
    };

    expect(order.id() == 1, "limit order stores its ID");
    expect(order.side() == trading::Side::Buy,
           "limit order stores its side");
    expect(order.type() == trading::OrderType::Limit,
           "limit order stores its type");

    const std::optional<trading::Price> price = order.price();
    expect(price.has_value(), "limit order has a price");
    expect(price.has_value() && *price == 10025,
           "limit order stores its price");
    expect(order.original_quantity() == 100,
           "limit order stores its original quantity");
    expect(order.remaining_quantity() == 100,
           "remaining quantity initially equals original quantity");
    expect(order.timestamp() == 1, "limit order stores its timestamp");
    expect(!order.is_filled(), "new order is not filled");

    order.fill(40);
    expect(order.original_quantity() == 100,
           "partial fill does not change original quantity");
    expect(order.remaining_quantity() == 60,
           "partial fill reduces remaining quantity");
    expect(!order.is_filled(), "partially filled order is not complete");

    order.fill(60);
    expect(order.remaining_quantity() == 0,
           "full fill reduces remaining quantity to zero");
    expect(order.is_filled(), "order reports complete after full fill");
}

void test_valid_market_order() {
    const trading::Order order{
        2,
        trading::Side::Sell,
        trading::OrderType::Market,
        std::nullopt,
        50,
        2
    };

    expect(order.type() == trading::OrderType::Market,
           "market order stores its type");
    expect(order.side() == trading::Side::Sell,
           "market order stores its side");
    expect(!order.price().has_value(), "market order has no price");
}

void test_invalid_orders() {
    expect_invalid_argument([] {
        trading::Order order{
            0, trading::Side::Buy, trading::OrderType::Limit,
            trading::Price{10025}, 100, 1
        };
    }, "zero order ID is rejected");

    expect_invalid_argument([] {
        trading::Order order{
            1, trading::Side::Buy, trading::OrderType::Limit,
            trading::Price{10025}, 0, 1
        };
    }, "zero order quantity is rejected");

    expect_invalid_argument([] {
        trading::Order order{
            1, trading::Side::Buy, trading::OrderType::Limit,
            std::nullopt, 100, 1
        };
    }, "limit order without a price is rejected");

    expect_invalid_argument([] {
        trading::Order order{
            1, trading::Side::Buy, trading::OrderType::Limit,
            trading::Price{0}, 100, 1
        };
    }, "zero limit price is rejected");

    expect_invalid_argument([] {
        trading::Order order{
            1, trading::Side::Buy, trading::OrderType::Limit,
            trading::Price{-1}, 100, 1
        };
    }, "negative limit price is rejected");

    expect_invalid_argument([] {
        trading::Order order{
            1, trading::Side::Buy, trading::OrderType::Market,
            trading::Price{10025}, 100, 1
        };
    }, "market order with a price is rejected");
}

void test_invalid_fills() {
    trading::Order order{
        1,
        trading::Side::Buy,
        trading::OrderType::Limit,
        trading::Price{10025},
        100,
        1
    };

    expect_invalid_argument([&order] {
        order.fill(0);
    }, "zero fill quantity is rejected");

    expect_invalid_argument([&order] {
        order.fill(101);
    }, "fill larger than remaining quantity is rejected");

    expect(order.remaining_quantity() == 100,
           "rejected fills do not change remaining quantity");
}

} // namespace

int main() {
    test_valid_limit_order_and_fills();
    test_valid_market_order();
    test_invalid_orders();
    test_invalid_fills();

    if (failures != 0) {
        std::cerr << failures << " order test(s) failed\n";
        return 1;
    }

    std::cout << "All order tests passed\n";
    return 0;
}
