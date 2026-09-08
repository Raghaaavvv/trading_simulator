# C++ Trading Simulator

An in-progress, event-driven trading simulator built in modern C++. The goal is
to develop a correct and testable simulation engine before measuring and
optimizing its performance.

This is a learning and portfolio project rather than a live trading system. It
is being developed incrementally to explore C++ software design, market
microstructure, testing, data structures, and performance engineering.

## Current status

Phase 1 is complete, and Phase 2 is in progress.

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
- CTest integration with two passing test suites

The limit order book, matching engine, event system, strategies, and portfolio
analytics have not been implemented yet.

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
    Limit Order Book
           |
           v
    Matching Engine
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

- A unique order ID
- Buy or sell side
- Limit or market order type
- An optional limit price
- Original and remaining quantity
- A monotonic timestamp

Prices are represented as integer ticks rather than floating-point values. For
example, if one tick represents one cent, `$100.25` is stored as `10025`. This
avoids floating-point equality and rounding problems.

The constructor enforces these invariants:

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
│       ├── Order.hpp
│       └── Simulator.hpp
├── src/
│   ├── main.cpp
│   ├── Order.cpp
│   └── Simulator.cpp
├── tests/
│   ├── test_main.cpp
│   └── test_order.cpp
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
limit orders, partial and complete fills, and invalid input handling.

## Development roadmap

- [x] Phase 1: Project structure, CMake, and testing setup
- [ ] Phase 2: Order and trade models
- [ ] Phase 3: Price-time-priority limit order book
- [ ] Phase 4: Matching engine and partial fills
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

## Disclaimer

This software is for education and simulation only. It does not connect to an
exchange or brokerage and should not be used to make financial decisions.
