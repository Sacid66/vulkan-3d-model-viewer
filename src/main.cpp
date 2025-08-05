#include "core/Application.hpp"
#include <iostream>
#include <stdexcept>

int main() {
    VulkanViewer::Application app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}