#include "hammock/rendergraph/Graph.h"

Hammock::Rendergraph::Graph::Graph(const CreateInfo &createInfo): rm{createInfo.resourceManager} {
    // Create a logical resource that represents a swap chain image
    /*LogicalResource logicalSwapImage{
        .type = LogicalResource::Type::SwapChainImage,
        .name = "SWAPCHAIN_IMAGE",
        .resolver = [this, &](ResourceManager &rm, uint32_t frameIndex) {
            
        }
    };*/
}
