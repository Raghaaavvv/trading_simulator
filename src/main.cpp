#include "trading/Simulator.hpp"
#include "trading/CsvMarketDataFeed.hpp"
#include "trading/EventDispatcher.hpp"

#include <fstream>
#include <charconv>
#include <memory>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {
template<typename T> T parse_number(const char* argument) {
    const std::string text(argument);
    T value{};
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || text.empty()) {
        throw std::invalid_argument("invalid strategy numeric argument");
    }
    return value;
}
}

int main(int argc, char* argv[]) {
    try {
    if (argc != 1 && argc != 2 && argc != 6) {
        throw std::invalid_argument("usage: trading-simulator [events.csv [--buy-below PRICE QUANTITY ID]]");
    }
    if (argc >= 2) {
        std::ifstream input(argv[1]);
        if (!input) throw std::runtime_error("cannot open input file");
        auto events = trading::read_events(input);
        std::unique_ptr<trading::Strategy> strategy;
        if (argc == 6) {
            if (std::string(argv[2]) != "--buy-below") throw std::invalid_argument("unknown strategy option");
            strategy = std::make_unique<trading::BuyBelowStrategy>(
                parse_number<trading::OrderId>(argv[5]),
                parse_number<trading::Price>(argv[3]),
                parse_number<trading::Quantity>(argv[4]));
        }
        trading::EventDispatcher dispatcher(std::move(strategy));
        for (auto& event : events) dispatcher.enqueue(std::move(event));
        const auto results = dispatcher.run();
        for (const auto& result : results) {
            std::cout << "Event " << result.sequence << " ID " << result.id
                      << (result.strategy_action ? " [strategy]: " : ": ");
            if (const auto* match = std::get_if<trading::MatchResult>(&result.outcome)) {
                std::cout << match->trades.size() << " trades, remaining=" << match->remaining_quantity
                          << ", resting=" << match->resting << '\n';
                for (const auto& trade : match->trades) {
                    std::cout << "  TRADE maker=" << trade.maker_id << " taker=" << trade.taker_id
                              << " price=" << trade.price << " quantity=" << trade.quantity << '\n';
                }
            } else if (const auto* cancelled = std::get_if<bool>(&result.outcome)) {
                std::cout << (*cancelled ? "cancelled" : "order not found") << '\n';
            } else {
                std::cout << "rejected: " << std::get<std::string>(result.outcome) << '\n';
            }
        }
        std::cout << "Monitor recorded " << dispatcher.monitor().trades().size() << " trades\n";
        return 0;
    }
    const trading::Simulator simulator;
    std::cout << simulator.status() << '\n';
    
    return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
