module;

#include <vector>
#include <memory>

export module hammock.renderer.task_graph:task;

import :socket;

namespace hammock::renderer {
    class IGpuTask {
    public:
        virtual ~IGpuTask() = default;

    private:
        std::vector<std::unique_ptr<ISocket> > sockets; // Direction is implied by the socket state
    };

    class GraphicsTask : public IGpuTask {
    };

    class ComputeTask : public IGpuTask {
    };

    class TransferTask : public IGpuTask {
    };
}
