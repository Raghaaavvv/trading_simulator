cmake_minimum_required(VERSION 3.20)

project(
    trading_simulator
    VERSION 0.1.0
    LANGUAGES CXX
)

add_library(trading_core STATIC
    src/Strategy.cpp
    src/EventDispatcher.cpp
    src/CsvMarketDataFeed.cpp
    src/MatchingEngine.cpp
    src/OrderBook.cpp
    src/Order.cpp
    src/Simulator.cpp

)

target_include_directories(trading_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(trading_core PUBLIC cxx_std_17)

add_executable(trading-simulator
    src/main.cpp
)

target_link_libraries(trading-simulator
    PRIVATE
        trading_core
)

if(MSVC)
    target_compile_options(trading_core PRIVATE /W4)
    target_compile_options(trading-simulator PRIVATE /W4)
else()
    target_compile_options(trading_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(trading-simulator PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)

if(BUILD_TESTING)
    add_executable(strategy-tests tests/test_strategy.cpp)
    target_link_libraries(strategy-tests PRIVATE trading_core)
    if(MSVC)
        target_compile_options(strategy-tests PRIVATE /W4)
    else()
        target_compile_options(strategy-tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME strategy-tests COMMAND strategy-tests)
    add_test(NAME strategy-replay COMMAND trading-simulator
        ${CMAKE_CURRENT_SOURCE_DIR}/data/example_events.csv --buy-below 10100 10 100)
    add_executable(event-tests tests/test_events.cpp)
    target_link_libraries(event-tests PRIVATE trading_core)
    if(MSVC)
        target_compile_options(event-tests PRIVATE /W4)
    else()
        target_compile_options(event-tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME event-tests COMMAND event-tests)
    add_test(NAME csv-replay COMMAND trading-simulator ${CMAKE_CURRENT_SOURCE_DIR}/data/example_events.csv)
    add_executable(matching-engine-tests tests/test_matching_engine.cpp)
    target_link_libraries(matching-engine-tests PRIVATE trading_core)
    if(MSVC)
        target_compile_options(matching-engine-tests PRIVATE /W4)
    else()
        target_compile_options(matching-engine-tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME matching-engine-tests COMMAND matching-engine-tests)
    add_executable(trading-tests
        tests/test_main.cpp
    )

    target_link_libraries(trading-tests
        PRIVATE
            trading_core
    )

    if(MSVC)
        target_compile_options(trading-tests PRIVATE /W4)
    else()
        target_compile_options(trading-tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME simulator-startup
        COMMAND trading-tests
    )

    add_executable(order-tests
        tests/test_order.cpp
    )

    target_link_libraries(order-tests
        PRIVATE
            trading_core
    )

    if(MSVC)
        target_compile_options(order-tests PRIVATE /W4)
    else()
        target_compile_options(order-tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME order-tests
        COMMAND order-tests
    )

    add_executable(order-book-tests
        tests/test_order_book.cpp
    )

    target_link_libraries(order-book-tests
        PRIVATE
            trading_core
    )

    if (MSVC)
        target_compile_options(order-book-tests PRIVATE /W4)
    else()
        target_compile_options(order-book-tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME order-book-tests
        COMMAND order-book-tests
    )
    
endif()
