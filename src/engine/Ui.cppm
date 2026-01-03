module;

#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui_imp_vulkansurfer.h"
#include "imgui/imgui.h"


export module hammock.engine.ui;

import hammock.core.command_buffer;
import hammock.core.descriptor;
import hammock.renderer.graphics_context;

namespace hammock::engine {
    export class Ui {
    public:
        Ui(Surfer::Window * window, renderer::GraphicsContext * ctx);

        ~Ui();

        void newFrame();

        static void renderFrame(core::CommandBuffer& commandBuffer);

    private:
        static void draw();
    };
}
