#include <iostream>
#include <cstdlib>
#include <memory>

import hammock_engine;

int main() {
    try {
        auto engine = std::make_unique<hammock::engine::Application>(hammock::engine::RunnerMode::Editor);
        engine->launch();
    } catch(std::exception &e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
