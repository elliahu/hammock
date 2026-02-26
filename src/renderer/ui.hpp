#pragma once
#include <cstdint>
#include <functional>
#include <vector>

#include "clay/clay.h"
#include "utils/singleton_base.hpp"

namespace hammock::renderer {

    // A deep-copied, thread-safe snapshot of Clay render commands.
    // Generated on the logic thread, consumed on the render thread.
    struct UiSnapshot {
        std::vector<Clay_RenderCommand> commands;
        uint32_t width{0};
        uint32_t height{0};
    };

    using LayUiCallback = std::function<void()>;

    /// @class Ui
    /// @brief Manages Clay UI layout. Call lay() on the logic thread to produce
    ///        a UiSnapshot. Pass the snapshot to the render thread for drawing.
    ///        No shared state between threads — Clay has no global context issues.
    class Ui : public helpers::SingletonBase<Ui> {
        friend class helpers::SingletonBase<Ui>;

       public:
        ~Ui() { free(arenaMemory_); }

        /// @brief Set the callback that declares UI elements each frame.
        void setUiCallback(LayUiCallback cb) { layUiCallback_ = std::move(cb); }

        /// @brief Update layout dimensions on resize.
        void setDisplaySize(uint32_t width, uint32_t height) {
            width_ = width;
            height_ = height;
        }

        /// @brief Run layout on the logic thread.
        ///        Returns a UiSnapshot that is safe to move to the render thread.
        ///        Clay_RenderCommand is plain-old-data — no pointers into Clay
        ///        internal state that could be invalidated next frame.
        UiSnapshot lay() {
            Clay_SetLayoutDimensions({static_cast<float>(width_), static_cast<float>(height_)});
            Clay_SetPointerState({-1.0f, -1.0f}, false);

            Clay_BeginLayout();
            if (layUiCallback_) {
                layUiCallback_();
            }
            Clay_RenderCommandArray cmds = Clay_EndLayout();

            UiSnapshot snap;
            snap.width = width_;
            snap.height = height_;
            // Copy commands into owned vector — safe across threads
            snap.commands.assign(cmds.internalArray, cmds.internalArray + cmds.length);
            return snap;
        }

       private:
        Ui(uint32_t width, uint32_t height) : width_(width), height_(height) {
            // Allocate Clay's arena
            const uint64_t memorySize = Clay_MinMemorySize();
            arenaMemory_ = malloc(memorySize);

            Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(memorySize, arenaMemory_);

            // Initialize Clay
            Clay_Initialize(arena,
                Clay_Dimensions{static_cast<float>(width_), static_cast<float>(height_)},
                Clay_ErrorHandler{handleError});

            // Bind text measurement — required before any layout with text
            Clay_SetMeasureTextFunction(measureText, nullptr);
        }

        static Clay_Dimensions measureText(
            Clay_StringSlice text, Clay_TextElementConfig* config, void* /*userData*/) {
            // Stub measurement — replace with real font metrics.
            // Clay calls this frequently; keep it fast.
            const float charWidth = config->fontSize * 0.5f;
            const float charHeight = static_cast<float>(config->fontSize);
            return {charWidth * static_cast<float>(text.length), charHeight};
        }

        static void handleError(Clay_ErrorData error) { printf("[Clay] Error: %s\n", error.errorText.chars); }

        void* arenaMemory_{nullptr};
        uint32_t width_{1280};
        uint32_t height_{720};
        LayUiCallback layUiCallback_{nullptr};
    };

}  // namespace hammock::renderer