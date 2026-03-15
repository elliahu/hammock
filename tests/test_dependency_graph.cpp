#include <gtest/gtest.h>

TEST(DependencyGraphTests, GraphCompilationDoesNotThrow) {
    EXPECT_NO_THROW([&]() {

    }());
}
