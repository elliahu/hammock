module;

#include <vector>
#include <memory>

export module hammock.renderer.task_graph:task;

import :socket;
import :push_constants;

namespace hammock::renderer {
    /// Interface representing general GPU task that has input and outputs (sockets)
    class IGpuTask {
    public:
        virtual ~IGpuTask() = default;

    private:
        std::vector<std::unique_ptr<ISocket> > sockets; // Direction is implied by the socket state
        std::vector<std::unique_ptr<PushConstantsBlock> > pushConstantsBlock;
    };

    class GraphicsTask : public IGpuTask {
    };

    class ComputeTask : public IGpuTask {
    };
}
