#include <print>
#include <cstdlib>
#include <memory>

import hammock.engine.application;
import hammock.engine.runner;


int main() {
    try {
        auto engine = std::make_unique<hammock::engine::Application>(hammock::engine::RunnerMode::Editor);
        engine->launch();
    } catch(std::exception &e) {
        std::println("{}", e.what());
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
