#include "trading/Simulator.hpp"

#include <iostream>

int main() {
    const trading::Simulator simulator;
    std::cout << simulator.status() << '\n';

    return 0;
}
