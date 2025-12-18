module;

#include <memory>

export module hammock.engine.editor;

import :window;
import hammock.renderer.renderer;
import hammock.renderer.graphics_context;
import hammock.engine.ui;
import hammock.core.frame_manager;


namespace hammock::engine {
    /// @enum EngineMode
    /// @brief Describes how the engine is launched
    export enum class EngineMode {
        Editor,
        Runtime,
    };

    /// @class Editor
    /// @brief Represents engines editor
    export class Editor final {
    public:
        Editor(const EngineMode mode) : mode(mode) {
            // Create graphics context
            context = std::make_unique<renderer::GraphicsContext>(renderer::GraphicsContextDesc{
                .mode = renderer::GraphicsContextMode::Windowed,
            });

            // Create a window
            window = std::make_unique<Window>("Hammock engine", context->getInstance(), 1920u, 1080u);

            // Attach the window surface to the graphics context
            context->attachSurface(window->getSurface());

            // Set rendering strategy
            auto strategy = std::make_unique<renderer::DeferredRenderingStrategy>(*context);
            renderer = std::make_unique<renderer::Renderer>(*context, std::move(strategy));

            // Init frame manager
            core::FrameManager::initialize(*window, context->getDevice());

            // Only create ui if in Editor mode
            if (mode == EngineMode::Editor) {
                //ui = std::make_unique<Ui>();
            }
        }

        ~Editor() {
            // Destroy frame manager once not needed
            core::FrameManager::dispose();
        }

        void launch() const {
            loop();
        }

    private:
        void loop() const {
            while (!window->shouldClose()) {
                    window->pollEvents();
                // Draw ui only when in editor mode
                if (mode == EngineMode::Editor) {
                    //ui->newFrame();
                }




                // Render the ui last
                if (mode == EngineMode::Editor) {
                    //ui->renderFrame();
                }
            }
        }

        EngineMode mode;
        std::unique_ptr<renderer::GraphicsContext> context;
        std::unique_ptr<renderer::Renderer> renderer;
        std::unique_ptr<engine::Window> window;
        std::unique_ptr<Ui> ui{nullptr};
    };
}
