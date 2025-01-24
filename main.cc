#include <exception>
#include <iostream>
#include <cstdlib>
#include <stdexcept>

#include "src/garnish_window.hpp"
#include "src/garnish_app.hpp"

int main() {
    std::cout << "Hello World!\n";
    garnish::GarnishApp app{};
    try {
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
