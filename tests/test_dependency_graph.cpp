#include <gtest/gtest.h>

#include "hammock_dependency_graph.hpp"
#include "hammock_app.hpp"
#include "hammock_renderer.hpp"
#include "hammock_core.hpp"

using namespace hammock;
using namespace hammock::renderer;
using namespace hammock::core;
using namespace hammock::app;

TEST(DependencyGraphTests, GraphCompilationDoesNotThrow) {
    EXPECT_NO_THROW([&]() {
        
    }());
}