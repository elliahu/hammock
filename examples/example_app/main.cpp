#include <memory>

#include "app/application.hpp"
#include "app/application_types.hpp"

using namespace hammock;

int main() {
    auto app = std::make_unique<app::Application>(app::ExecutionMode::Engine);

    app->launch();

    return EXIT_SUCCESS;
}
