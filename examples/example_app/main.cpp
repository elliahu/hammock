#include <memory>

#include "app/runner.hpp"

using namespace hammock;

int main() {
    auto runner = std::make_unique<app::Runner>();
    runner->run();

    return EXIT_SUCCESS;
}
