# C++ Trading Simulator

An in-progress, event-driven trading simulator built in modern C++. The goal is
to develop a correct and testable simulation engine before measuring and
optimizing its performance.

This is a learning and portfolio project rather than a live trading system. It
is being developed incrementally to explore C++ software design, market
microstructure, testing, data structures, and performance engineering.

## Current status

Phases 1–4 are complete at the initial functional level. The next phase is
market data and event dispatch. The engine currently runs through its C++ API
and automated tests; the command-line application still prints a startup message.

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
- CTest integration with four passing test suites

The event system, market-data replay, strategies, and portfolio analytics are
planned features.

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
│       ├── MatchingEngine.hpp
│       ├── Order.hpp
│       ├── OrderBook.hpp
│       ├── Trade.hpp
│       └── Simulator.hpp
├── src/
│   ├── main.cpp
│   ├── MatchingEngine.cpp
│   ├── Order.cpp
│   ├── OrderBook.cpp
│   └── Simulator.cpp
├── tests/
│   ├── test_main.cpp
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

CTest runs four test executables, each containing multiple checks:
`simulator-startup`, `order-tests`, `order-book-tests`, and `matching-engine-tests`.

## Development roadmap

- [x] Phase 1: Project structure, CMake, and testing setup
- [x] Phase 2: Order and trade models
- [x] Phase 3: Price-time-priority limit order book
- [x] Phase 4: Matching engine and partial fills
- [ ] Phase 5: Market data and event dispatch
- [ ] Phase 6: Trading strategy interface
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
