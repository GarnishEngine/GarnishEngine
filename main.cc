#include <exception>
#include <iostream>
#include <stdexcept>

#include "src/garnish_app.hpp"

#include "Test/test_app.cc"

int main() {
    garnish::TestApp app{};
    try {
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
