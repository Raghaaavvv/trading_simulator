cmake_minimum_required(VERSION 3.20)

project(trading_simulator VERSION 0.1.0 LANGUAGES CXX)

# Export the exact compiler commands for editor tools such as IntelliSense.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(trading_core STATIC
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

target_link_libraries(trading-simulator PRIVATE trading_core)

if(MSVC)
    target_compile_options(trading_core PRIVATE /W4)
    target_compile_options(trading-simulator PRIVATE /W4)
else()
    target_compile_options(trading_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(trading-simulator PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)

if(BUILD_TESTING)
    add_executable(trading-tests
        tests/test_main.cpp
    )

    add_executable(order-tests
        tests/test_order.cpp
    )

    target_link_libraries(trading-tests PRIVATE trading_core)
    target_link_libraries(order-tests PRIVATE trading_core)

    if(MSVC)
        target_compile_options(trading-tests PRIVATE /W4)
        target_compile_options(order-tests PRIVATE /W4)
    else()
        target_compile_options(trading-tests PRIVATE -Wall -Wextra -Wpedantic)
        target_compile_options(order-tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME simulator-startup COMMAND trading-tests)
    add_test(NAME order-tests COMMAND order-tests)
endif()
