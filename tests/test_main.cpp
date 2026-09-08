#include "trading/Simulator.hpp"

#include <iostream>
#include <string_view>

int main() {
    const trading::Simulator simulator;
    const std::string_view expected = "Trading simulator is running.";

    if (simulator.status() != expected) {
        std::cerr << "Test failed: unexpected simulator status\n";
        return 1;
    }

    std::cout << "All tests passed\n";
    return 0;
}
