#pragma once

#include "frame_graph.hpp"

namespace hammock::renderer {

    class PassRecorder {
       public:
        void record(PassDescription& pass, core::ResourceRef<core::CommandBuffer> commandBuffer) const;

       private:
        void recordGraphicsPass(
            PassDescription& pass, core::ResourceRef<core::CommandBuffer> commandBuffer) const;
        void recordComputePass(
            PassDescription& pass, core::ResourceRef<core::CommandBuffer> commandBuffer) const;
    };

}  // namespace hammock::renderer
