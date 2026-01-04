#include <print>
#include <cstdlib>
#include <memory>

import hammock.engine.engine;
import hammock.engine.editor;


int main() {
    try {
        auto engine = std::make_unique<hammock::engine::Engine>(hammock::engine::LaunchMode::Editor);
        engine->launch();


    } catch(std::exception &e) {
        std::println("{}", e.what());
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
