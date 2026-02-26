#pragma once
#include <string>
#include "stage.hpp"

namespace hammock::renderer{
    /// @interface StageLoaderIface
    /// Interface for loading stage object from files
    class StageLoaderIface {
       public:
        virtual ~StageLoaderIface() = default;
        virtual void load(const std::string& path, Stage& stage) = 0;
    };
}