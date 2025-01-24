#include <exception>
#include <iostream>
#include <stdexcept>

#include "src/garnish_app.hpp"

int main() {
    garnish::GarnishApp app{};
    try {
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
