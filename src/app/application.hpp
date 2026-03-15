#pragma once
#include <memory>

#include "application_types.hpp"
#include "runner.hpp"

namespace hammock::app {
    class Application final {
       public:
        explicit Application(ExecutionMode mode);

        void launch() const;

       private:
        std::unique_ptr<Runner> runner_;
    };
}  // namespace hammock::app
