#include "trading/CsvMarketDataFeed.hpp"
#include "trading/EventDispatcher.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace {
int failures = 0;
void expect(bool value, std::string_view message) {
    if (!value) { ++failures; std::cerr << "FAILED: " << message << '\n'; }
}
const char* header = "event,id,side,type,price,quantity,timestamp\n";

void test_ordering_and_notifications() {
    std::istringstream input(std::string(header) +
        "NEW,1,SELL,LIMIT,10100,100,30\n"
        "NEW,2,SELL,LIMIT,10100,50,10\n"
        "NEW,3,BUY,LIMIT,10100,120,20\n"
        "CANCEL,2,,,,,0\n"
        "NEW,4,BUY,MARKET,,10,20\n");
    trading::EventDispatcher dispatcher;
    for (auto event : trading::read_events(input)) dispatcher.enqueue(std::move(event));
    const auto results = dispatcher.run();
    expect(results.size() == 5, "all input events processed");
    if (results.size() != 5) return;
    expect(results[0].id == 1 && results[0].timestamp == 30 && results[1].id == 2,
           "timestamps do not reorder arrivals");
    expect(results[4].sequence == 5, "arrival sequence increases monotonically");
    const auto& trades = dispatcher.monitor().trades();
    expect(trades.size() == 2, "monitor records each execution once");
    if (trades.size() == 2) {
        expect(trades[0].maker_id == 1 && trades[0].quantity == 100 &&
               trades[1].maker_id == 2 && trades[1].quantity == 20,
               "FIFO respects arrival order despite reversed timestamps");
        expect(trades[0].timestamp == 20, "trade retains triggering event timestamp");
    }
    expect(std::get<bool>(results[3].outcome), "cancellation follows earlier fills");
    const auto& last = std::get<trading::MatchResult>(results[4].outcome);
    expect(last.trades.empty() && last.remaining_quantity == 10 && !last.resting,
           "later event observes cancellation immediately");
    expect(dispatcher.run().empty(), "queue is consumed without replaying notifications");
    dispatcher.enqueue(trading::CancelOrderEvent{999, 0});
    const auto next = dispatcher.run();
    expect(next.front().sequence == 6 && !std::get<bool>(next.front().outcome),
           "sequence persists across runs and unknown cancellation is reported");
}

void test_rejections_and_ties() {
    trading::EventDispatcher dispatcher;
    dispatcher.enqueue(trading::NewOrderEvent{{1, trading::Side::Sell, trading::OrderType::Limit, 100, 10, 1}});
    dispatcher.enqueue(trading::NewOrderEvent{{1, trading::Side::Buy, trading::OrderType::Market, std::nullopt, 10, 1}});
    dispatcher.enqueue(trading::NewOrderEvent{{2, trading::Side::Buy, trading::OrderType::Market, std::nullopt, 10, 1}});
    const auto results = dispatcher.run();
    expect(std::holds_alternative<std::string>(results[1].outcome), "invalid order produces rejection");
    expect(dispatcher.monitor().trades().size() == 1 &&
           dispatcher.monitor().trades().front().taker_id == 2,
           "rejection does not prevent subsequent same-timestamp event execution");
    expect(!dispatcher.engine().book().best_ask(), "trade updates engine book");

    trading::EventDispatcher cancelled;
    cancelled.enqueue(trading::NewOrderEvent{{1, trading::Side::Sell, trading::OrderType::Limit, 100, 10, 1}});
    cancelled.enqueue(trading::CancelOrderEvent{1, 1});
    cancelled.enqueue(trading::NewOrderEvent{{2, trading::Side::Buy, trading::OrderType::Market, std::nullopt, 10, 1}});
    (void)cancelled.run();
    expect(cancelled.monitor().trades().empty(), "cancel-before-match prevents execution at tied timestamps");
}

void test_csv_validation() {
    for (const char* row : {
        "NEW,1,BUY,LIMIT,,10,1", "NEW,1,BUY,MARKET,100,10,1",
        "NEW,1,BUY,LIMIT,100,0,1", "NEW,-1,BUY,LIMIT,100,10,1",
        "NEW,1,BUY,LIMIT,100,-10,1", "NEW,1,BUY,LIMIT,100,10,-1",
        "NEW,1,OTHER,LIMIT,100,10,1", "NEW,1,BUY,OTHER,100,10,1",
        "OTHER,1,BUY,LIMIT,100,10,1", "CANCEL,1,BUY,,,,1",
        "CANCEL,0,,,,,1", "NEW,1,BUY,LIMIT,100,10,1,extra",
        "NEW,1,BUY,LIMIT,100,10", "NEW,1x,BUY,LIMIT,100,10,1",
        "NEW,18446744073709551616,BUY,LIMIT,100,10,1"}) {
        std::istringstream input(std::string(header) + row + '\n');
        try { (void)trading::read_events(input); expect(false, "invalid CSV rejected"); }
        catch (const std::invalid_argument& error) {
            expect(std::string_view(error.what()).find("CSV line 2:") == 0,
                   "CSV errors identify line number");
        }
    }
    std::istringstream missing("");
    try { (void)trading::read_events(missing); expect(false, "missing header rejected"); }
    catch (const std::invalid_argument&) {}
    std::istringstream bad_header("wrong\n");
    try { (void)trading::read_events(bad_header); expect(false, "wrong header rejected"); }
    catch (const std::invalid_argument&) {}
    std::istringstream crlf("event,id,side,type,price,quantity,timestamp\r\n\r\nCANCEL,1,,,,,0\r\n");
    expect(trading::read_events(crlf).size() == 1, "CRLF and blank lines supported");
    std::istringstream empty(header);
    expect(trading::read_events(empty).empty(), "header-only input gives no events");
}
} // namespace
int main() {
    test_ordering_and_notifications();
    test_rejections_and_ties();
    test_csv_validation();
    if (failures) return 1;
    std::cout << "All event tests passed\n";
}
