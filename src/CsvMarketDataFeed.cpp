#include "trading/CsvMarketDataFeed.hpp"
#include <charconv>
#include <stdexcept>
#include <string>
#include <string_view>

namespace trading {
namespace {
template<typename T> T number(const std::string& text) {
    T result{};
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
    if (text.empty() || parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()) {
        throw std::invalid_argument("invalid integer: " + text);
    }
    return result;
}

Event parse(const std::string& line) {
    std::vector<std::string> fields;
    std::size_t start = 0;
    for (;;) {
        const auto end = line.find(',', start);
        fields.push_back(line.substr(start, end == std::string::npos ? end : end - start));
        if (end == std::string::npos) break;
        start = end + 1;
    }
    if (fields.size() != 7) throw std::invalid_argument("expected seven columns");
    const auto id = number<OrderId>(fields[1]);
    const auto timestamp = number<Timestamp>(fields[6]);
    if (!id) throw std::invalid_argument("order ID must be nonzero");
    if (fields[0] == "CANCEL") {
        for (std::size_t i = 2; i < 6; ++i) {
            if (!fields[i].empty()) throw std::invalid_argument("CANCEL order fields must be empty");
        }
        return CancelOrderEvent{id, timestamp};
    }
    if (fields[0] != "NEW") throw std::invalid_argument("unknown event type");
    if (fields[2] != "BUY" && fields[2] != "SELL") throw std::invalid_argument("invalid side");
    if (fields[3] != "LIMIT" && fields[3] != "MARKET") throw std::invalid_argument("invalid order type");
    std::optional<Price> price;
    if (!fields[4].empty()) price = number<Price>(fields[4]);
    return NewOrderEvent{Order{id, fields[2] == "BUY" ? Side::Buy : Side::Sell,
        fields[3] == "LIMIT" ? OrderType::Limit : OrderType::Market,
        price, number<Quantity>(fields[5]), timestamp}};
}
} // namespace

std::vector<Event> read_events(std::istream& input) {
    const std::string header = "event,id,side,type,price,quantity,timestamp";
    std::string line;
    if (!std::getline(input, line)) throw std::invalid_argument("missing CSV header");
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line != header) throw std::invalid_argument("unexpected CSV header");
    std::vector<Event> events;
    std::size_t row = 1;
    while (std::getline(input, line)) {
        ++row;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        try {
            events.push_back(parse(line));
        } catch (const std::invalid_argument& error) {
            throw std::invalid_argument("CSV line " + std::to_string(row) + ": " + error.what());
        }
    }
    if (input.bad()) throw std::runtime_error("failed to read CSV stream");
    return events;
}
} // namespace trading
