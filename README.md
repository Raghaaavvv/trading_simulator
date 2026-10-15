# C++ Trading Simulator

An in-progress, event-driven trading simulator built in modern C++. The goal is
to develop a correct and testable simulation engine before measuring and
optimizing its performance.

This is a learning and portfolio project rather than a live trading system. It
is being developed incrementally to explore C++ software design, market
microstructure, testing, data structures, and performance engineering.

## Current status

Phases 1–6 are complete at the initial functional level. The command-line
application can now replay synthetic order-event CSV files through the dispatcher
and matching engine, optionally with a strategy. Historical-data adapters come later.

Currently implemented:

- A CMake-based C++17 project
- A reusable `trading_core` static library
- Separate application and test executables
- Compiler warnings using `-Wall -Wextra -Wpedantic` or `/W4`
- An `Order` model supporting limit and market orders
- Integer tick-based prices and quantities
- Constructor validation that prevents invalid order states
- Partial and complete order fills
- Automated tests for valid orders, getters, fills, and rejected inputs
- An ordered limit order book using `std::map` price levels and `std::deque` FIFO queues
- Best bid/ask prices and snapshots of priority orders
- Cancellation by ID, empty-level cleanup, and duplicate resting-ID rejection
- A separate matching engine supporting price-time priority and multi-level execution
- Trade records with maker/taker IDs, taker side, execution price, quantity, and timestamp
- FIFO event dispatch with immediate trade notifications to a market monitor
- Strict CSV event parsing and a command-line replay demonstration
- Read-only strategy snapshots, requested actions, ownership tracking, and fill feedback
- An example threshold-based buy strategy
- Eight passing CTest entries (six test executables and two CLI replay smoke tests)

Real historical datasets, more advanced strategies, and portfolio analytics are planned features.

## Matching behavior and design

`MatchingEngine` owns an `OrderBook`. Incoming orders are submitted through
`submit(Order)`, which returns executed trades, remaining quantity, and whether
that remainder is resting. Read-only book access is available through `book()`;
`cancel(OrderId)` removes a resting order.

- Buy limits execute against asks at or below their limit.
- Sell limits execute against bids at or above their limit.
- Better prices take priority; orders at equal prices follow arrival order at submission.
- Each trade executes at the resting order's price.
- Unfilled limit quantities rest in the book; unfilled market quantities are cancelled.
- Fully filled orders and empty price levels are removed automatically.
- IDs must be unique among resting orders and may be reused after removal.

Best bid and ask are derived from current resting prices, independently of any
external last-traded price. The book orders arrivals directly; it does not sort
equal-price orders by their supplied timestamps.

This is a correctness-first implementation with no performance claims yet.
Duplicate detection and cancellation scan resting orders in O(N). Each engine
submission also stages changes on a book copy before committing, protecting the
original state if processing throws but adding O(N) copying overhead. An order-ID
index and a more efficient commit approach are future design and profiling work.

## Planned architecture

```text
Historical / Simulated Market Data
                |
                v
         Market Data Feed
                |
                v
         Event Dispatcher
            /       \
           v         v
Trading Strategy   Market Monitor
           |
           v
     Order Manager
           |
           v
    Matching Engine
    (owns Limit Order Book)
           |
           v
    Executed Trades
        /       \
       v         v
Position Manager  P&L Analytics
```

The first implementation will remain deterministic and single-threaded.
Concurrency and performance optimizations will be introduced only after the
core behavior is correct and covered by tests.

## Order model

The current `Order` type records:

- An order ID (uniqueness among resting orders is enforced by the book)
- Buy or sell side
- Limit or market order type
- An optional limit price
- Original and remaining quantity
- A caller-supplied timestamp (monotonicity is not validated)

Prices are represented as integer ticks rather than floating-point values. For
example, if one tick represents one cent, `$100.25` is stored as `10025`. This
avoids floating-point equality and rounding problems.

The constructor and fill operation enforce these invariants:

- Order IDs must be nonzero.
- Quantities must be nonzero.
- Limit orders must have a positive price.
- Market orders must not have a price.
- Fill quantities must be positive and cannot exceed the remaining quantity.

## Project structure

```text
trading-simulator/
├── CMakeLists.txt
├── include/
│   └── trading/
│       ├── CsvMarketDataFeed.hpp
│       ├── Event.hpp
│       ├── EventDispatcher.hpp
│       ├── MatchingEngine.hpp
│       ├── Order.hpp
│       ├── OrderBook.hpp
│       ├── Trade.hpp
│       ├── Strategy.hpp
│       └── Simulator.hpp
├── src/
│   ├── main.cpp
│   ├── CsvMarketDataFeed.cpp
│   ├── EventDispatcher.cpp
│   ├── MatchingEngine.cpp
│   ├── Order.cpp
│   ├── OrderBook.cpp
│   ├── Strategy.cpp
│   └── Simulator.cpp
├── tests/
│   ├── test_main.cpp
│   ├── test_events.cpp
│   ├── test_strategy.cpp
│   ├── test_matching_engine.cpp
│   ├── test_order.cpp
│   └── test_order_book.cpp
├── benchmarks/
├── data/
└── README.md
```

Public declarations live under `include/trading`, implementations live in
`src`, and automated tests live in `tests`. Production code is compiled into
`trading_core`, which is linked by both the command-line application and test
executables.

## Requirements

- CMake 3.20 or newer
- A C++17-compatible compiler, such as AppleClang, Clang, GCC, or MSVC

## Configure and build

From the project root:

```bash
cmake -S . -B build
cmake --build build
```

## Run

```bash
./build/trading-simulator
```

Current output:

```text
Trading simulator is running.
```

## Test

```bash
ctest --test-dir build --output-on-failure
```

The current tests cover startup, order construction, getters, valid market and
limit orders, partial and complete fills, and invalid input handling. Order-book
tests cover price ordering, FIFO preservation, cancellation, and duplicate IDs.
Matching tests cover both sides, multi-level sweeps, maker-price execution,
limit boundaries, resting remainders, market remainder cancellation, and
rejection before execution.

CTest runs six test executables, each containing multiple checks:
`simulator-startup`, `order-tests`, `order-book-tests`, `matching-engine-tests`,
`event-tests`, and `strategy-tests`. `csv-replay` and `strategy-replay` exercise
the CLI with and without a strategy.
Event tests cover arrival ordering, timestamp ties, notifications, rejection
handling, cancellation timing, and CSV validation.

## Event dispatch and CSV replay

```bash
./build/trading-simulator data/example_events.csv
```

The feed produces `NewOrderEvent` and `CancelOrderEvent` values stored in a
`std::variant`. `EventDispatcher` queues them in a `std::deque` and processes
them strictly in input order. It assigns a monotonically increasing sequence
across runs. Timestamps are caller metadata and never determine queue priority;
equal or decreasing timestamps are accepted. `MarketMonitor` records each trade
immediately after the triggering submission, before dispatching the next event.

The exact CSV header and example rows are:

```csv
event,id,side,type,price,quantity,timestamp
NEW,1,SELL,LIMIT,10100,100,30
NEW,2,SELL,LIMIT,10100,50,10
NEW,3,BUY,LIMIT,10100,120,20
CANCEL,2,,,,,40
NEW,4,BUY,MARKET,,10,50
```

Prices are integer ticks. IDs and quantities must be positive; timestamps are
nonnegative integers. Market prices must be blank. For cancellations, the side,
type, price, and quantity columns must be blank. Tokens are uppercase, and quoting
or surrounding whitespace is not supported. Blank lines and CRLF are accepted.
The sample produces two trades (100 then 20 units), cancels the remaining ask,
and cancels the final market order's unfilled quantity.

The entire CSV is parsed before execution. Malformed rows abort replay with a
line-numbered error. Validly formed events rejected by the engine, such as a
duplicate resting ID, produce rejection records and replay continues. Unknown
cancellation IDs produce a not-found result. The CLI exits successfully after a
completed replay even when individual events were rejected; input/file errors
exit with status 1. Unexpected resource failures abort processing; recovery or
retry after such failures is not implemented. Replay and monitoring currently
retain all events/results/trades in memory.

This format describes synthetic exchange events, not OHLC price bars or an
adapter for any particular historical vendor dataset.

## Strategy boundary

Strategies implement `Strategy::on_market_update(const MarketView&)` and return
a vector of requested `Event` actions. `MarketView` is a value snapshot containing
only best bid, best ask, sequence, and timestamp. It provides no engine, book,
or mutable container access. Mutating a retained copy cannot change exchange state.

The dispatcher owns one optional strategy through `std::unique_ptr<Strategy>`.
After each external event, it delivers fill notifications, takes a snapshot,
and invokes the strategy. Returned actions execute in vector order before the
next external event. Actions use normal engine validation and receive
`on_action_result` feedback, including rejections and cancellation outcomes.
Actions do not recursively trigger market-update callbacks. This is a synchronous,
zero-simulated-latency policy; there are no extra threads or network messages.

The dispatcher tracks strategy-owned resting IDs and sends `on_fill` notifications
for its maker and taker executions. A strategy cannot cancel another participant's
order. Ownership is removed on full execution or cancellation; reusing a completed
ID externally does not inherit strategy ownership. IDs share the exchange's
namespace, so configure the strategy with an ID not used by resting feed orders.
Strategy callback exceptions abort the run; callbacks should not throw, and
automatic recovery after callback/resource failures is not implemented.

Run the one-shot `BuyBelowStrategy` example (prices in ticks):

```bash
./build/trading-simulator data/example_events.csv --buy-below 10100 10 100
```

This requests one buy limit order with ID 100, quantity 10, and limit 10100
when the best ask is at most 10100. It makes one attempt, even if rejected;
it is an interface demonstration rather than a profitability claim. Strategy
results are marked `[strategy]` in CLI output. Positions, cash, and P&L are
not tracked yet. Tests exercise read-only access, action ordering, feedback,
ownership restrictions, fill routing, and the example strategy.

## Development roadmap

- [x] Phase 1: Project structure, CMake, and testing setup
- [x] Phase 2: Order and trade models
- [x] Phase 3: Price-time-priority limit order book
- [x] Phase 4: Matching engine and partial fills
- [x] Phase 5: Market data and event dispatch (synthetic CSV replay)
- [x] Phase 6: Trading strategy interface and example strategy
- [ ] Phase 7: Positions and P&L accounting
- [ ] Phase 8: Historical market-data replay and backtesting
- [ ] Phase 9: Benchmarking and profiling
- [ ] Phase 10: Evidence-based performance optimization
- [ ] Phase 11: Concurrency experiments

## Concepts practiced so far

- Header and source-file separation
- Classes, constructors, namespaces, and object lifetime
- Encapsulation through private fields and public methods
- Member initializer lists and class invariants
- `enum class`, type aliases, and `std::optional`
- Exception-based input validation
- CMake targets, include paths, compilation, and linking
- Unit-test structure, lambdas, templates, and CTest
- Ordered maps, FIFO deques, iterators, and price-time priority
- Matching rules, quantity conservation, and cancellation semantics
- Exception safety and correctness/performance tradeoffs

## Disclaimer

This software is for education and simulation only. It does not connect to an
exchange or brokerage and should not be used to make financial decisions.
