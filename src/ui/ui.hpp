#pragma once
#include <cstdint>
#include <functional>

#include "clay/clay.h"
#include "font.hpp"
#include "render_types.hpp"
#include "utils/singleton_base.hpp"

namespace hammock::ui {

    /// @class Ui
    /// @brief Manages Clay UI layout. Call lay() on the logic thread to produce
    ///        a UiSnapshot. Pass the snapshot to the render thread for drawing.
    ///        No shared state between threads — Clay has no global context issues.
    /// TODO Implement scrolling
    /// TODO Implement scaling so that mouse position in real pixels (actuall windows size) can be converted
    /// to relative pixel coordinates if the internal resolution is different from the window size
    class Ui : public helpers::SingletonBase<Ui> {
        friend class helpers::SingletonBase<Ui>;

       public:
        ~Ui() { free(arenaMemory_); }
        /// @brief Update layout dimensions on resize.
        void setDisplaySize(uint32_t width, uint32_t height) {
            width_ = width;
            height_ = height;
        }



        void beginLayout();

        void endLayout(renderer::UserInterfaceDrawBatch& uiDrawBatch);

        /// @brief Returns a current font atlas
        FontAtlas& getFontAtlas() { return atlas; }

        /// @brief Sets pointer position
        void setPointerPosition(math::Vec2 pos) { pointerState.position = pos; }

        /// @brief Sets the down state of the pointer
        void setPointerDown(bool down) { pointerState.down = down; }

        /// @brief Sets the scroll data for the cursor
        void setScrollDelta(math::Vec2 delta) { pointerState.scrollDelta = delta; }

       private:
        Ui(uint32_t width, uint32_t height);

        static Clay_Dimensions measureText(
            Clay_StringSlice text, Clay_TextElementConfig* config, void* userData);

        static void handleError(Clay_ErrorData error);

        void createDrawBatch(renderer::UserInterfaceDrawBatch& uiDrawBatch, Clay_RenderCommandArray& cmds,
            const FontAtlas& font);

        void* arenaMemory_{nullptr};
        uint32_t width_{1280};
        uint32_t height_{720};
        FontAtlas atlas;

        struct PointerState {
            math::Vec2 position;
            math::Vec2 scrollDelta;
            bool down;
        } pointerState;
    };

}  // namespace hammock::ui
