#pragma once
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <vector>

#include "clay/clay.h"
#include "font.hpp"
#include "render_types.hpp"
#include "utils/singleton_base.hpp"

namespace hammock::ui {
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
        void lay(renderer::UserInterfaceDrawBatch& uiDrawBatch);

        /// @brief Returns a current font atlas
        FontAtlas& getFontAtlas() { return atlas; }

        /// @brief Sets pointer position
        void setPointerPosition(math::Vec2 pos) {
            pointerState.position = pos;
        }

        /// @brief Sets the down state of the pointer
        void setPointerDown(bool down){
            pointerState.down = down;
        }

        /// @brief Sets the scroll data for the cursor
        void setScrollDelta(math::Vec2 delta){
            pointerState.scrollDelta = delta;
        }

       private:
        Ui(uint32_t width, uint32_t height);

        static Clay_Dimensions measureText(
            Clay_StringSlice text, Clay_TextElementConfig* config, void* userData);

        static void handleError(Clay_ErrorData error);

        void createDrawBatch(
            renderer::UserInterfaceDrawBatch& uiDrawBatch, Clay_RenderCommandArray& cmds, const FontAtlas& font);

        void* arenaMemory_{nullptr};
        uint32_t width_{1280};
        uint32_t height_{720};
        LayUiCallback layUiCallback_{nullptr};
        FontAtlas atlas;

        struct PointerState {
            math::Vec2 position;
            math::Vec2 scrollDelta;
            bool down;
        } pointerState;
    };

}  // namespace hammock::renderer
