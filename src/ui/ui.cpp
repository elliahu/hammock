#include "ui.hpp"
#include "render_types.hpp"
#define CLAY_IMPLEMENTATION
#include "clay/clay.h"


void hammock::ui::Ui::lay(hammock::renderer::UserInterfaceDrawBatch& uiDrawBatch) {
    Clay_SetLayoutDimensions({static_cast<float>(width_), static_cast<float>(height_)});
    Clay_SetPointerState({pointerState.position.X, pointerState.position.Y}, pointerState.down);
    //Clay_UpdateScrollContainers(true, {pointerState.scrollDelta.X, pointerState.scrollDelta.Y}, 0.00694);

    Clay_BeginLayout();
    if (layUiCallback_) {
        layUiCallback_();
    }
    Clay_RenderCommandArray cmds = Clay_EndLayout();
    createDrawBatch(uiDrawBatch, cmds, atlas);
}
hammock::ui::Ui::Ui(uint32_t width, uint32_t height) : width_(width), height_(height) {
    // Allocate Clay's arena
    const uint64_t memorySize = Clay_MinMemorySize();
    arenaMemory_ = malloc(memorySize);

    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(memorySize, arenaMemory_);

    // Initialize Clay
    Clay_Initialize(arena,
        Clay_Dimensions{static_cast<float>(width_), static_cast<float>(height_)},
        Clay_ErrorHandler{handleError});

    // Bind text measurement — required before any layout with text
    Clay_SetMeasureTextFunction(measureText, &atlas);

    // Create texture atlas
    atlas = loadFont("../../../data/fonts/GoogleSansCode-Medium.ttf");

    Clay_SetDebugModeEnabled(true);
}
Clay_Dimensions hammock::ui::Ui::measureText(
    Clay_StringSlice text, Clay_TextElementConfig* config, void* userData) {
    const FontAtlas* font = static_cast<const FontAtlas*>(userData);
    const float scale = config->fontSize / FONT_SIZE_PX;  // ratio to baked size

    float totalWidth = 0.f;
    for (int i = 0; i < text.length; i++) {
        const unsigned char ch = static_cast<unsigned char>(text.chars[i]);
        if (ch < FIRST_CHAR || ch >= FIRST_CHAR + CHAR_COUNT) continue;
        totalWidth += font->glyphs[ch - FIRST_CHAR].advanceX * scale;
    }

    return {totalWidth, font->lineHeight * scale};
}
void hammock::ui::Ui::handleError(Clay_ErrorData error) {
    printf("[Clay] Error: %s\n", error.errorText.chars);
}
void hammock::ui::Ui::createDrawBatch(
    renderer::UserInterfaceDrawBatch& uiDrawBatch, Clay_RenderCommandArray& cmds, const FontAtlas& font) {
    auto emitQuad = [&](float left,
                        float top,
                        float right,
                        float bottom,
                        float u0,
                        float v0,
                        float u1,
                        float v1,
                        Vec4 color,
                        float useTexture) {
        // Triangle 1
        uiDrawBatch.vertices.push_back({{left, top}, {u0, v0}, color, useTexture});
        uiDrawBatch.vertices.push_back({{right, top}, {u1, v0}, color, useTexture});
        uiDrawBatch.vertices.push_back({{right, bottom}, {u1, v1}, color, useTexture});
        // Triangle 2
        uiDrawBatch.vertices.push_back({{left, top}, {u0, v0}, color, useTexture});
        uiDrawBatch.vertices.push_back({{right, bottom}, {u1, v1}, color, useTexture});
        uiDrawBatch.vertices.push_back({{left, bottom}, {u0, v1}, color, useTexture});
    };

    for (int32_t i = 0; i < cmds.length; i++) {
        const Clay_RenderCommand& cmd = cmds.internalArray[i];
        const Clay_BoundingBox& bb = cmd.boundingBox;

        switch (cmd.commandType) {
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
                const Clay_RectangleRenderData& r = cmd.renderData.rectangle;
                Vec4 color = {
                    r.backgroundColor.r / 255.f,
                    r.backgroundColor.g / 255.f,
                    r.backgroundColor.b / 255.f,
                    r.backgroundColor.a / 255.f,
                };
                emitQuad(bb.x,
                    bb.y,
                    bb.x + bb.width,
                    bb.y + bb.height,
                    0.f,
                    0.f,
                    0.f,
                    0.f,  // UVs unused for solid rects
                    color,
                    0.f);
                break;
            }

            case CLAY_RENDER_COMMAND_TYPE_TEXT: {
                const Clay_TextRenderData& t = cmd.renderData.text;
                const float scale = t.fontSize / FONT_SIZE_PX;
                Vec4 color = {
                    t.textColor.r / 255.f,
                    t.textColor.g / 255.f,
                    t.textColor.b / 255.f,
                    t.textColor.a / 255.f,
                };

                // Cursor starts at the baseline of the bounding box
                float cursorX = bb.x;
                float cursorY = bb.y + font.ascent * scale;  // move down to baseline

                for (int32_t c = 0; c < t.stringContents.length; c++) {
                    const unsigned char ch = static_cast<unsigned char>(t.stringContents.chars[c]);
                    if (ch < FIRST_CHAR || ch >= FIRST_CHAR + CHAR_COUNT) continue;

                    const GlyphInfo& g = font.glyphs[ch - FIRST_CHAR];

                    const float left = cursorX + g.bearingX * scale;
                    const float top = cursorY + g.bearingY * scale;
                    const float right = left + g.width * scale;
                    const float bottom = top + g.height * scale;

                    emitQuad(left, top, right, bottom, g.u0, g.v0, g.u1, g.v1, color, 1.f);

                    cursorX += g.advanceX * scale;
                }
                break;
            }

            case CLAY_RENDER_COMMAND_TYPE_BORDER: {
                // Emit 4 thin quads forming the border edges
                const Clay_BorderRenderData& b = cmd.renderData.border;

                auto emitBorderQuad = [&](float l, float t, float r, float bot, Clay_Color c) {
                    Vec4 color = {c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f};
                    emitQuad(l, t, r, bot, 0.f, 0.f, 0.f, 0.f, color, 0.f);
                };

                const float x = bb.x, y = bb.y, w = bb.width, h = bb.height;

                if (b.width.top > 0) emitBorderQuad(x, y, x + w, y + b.width.top, b.color);
                if (b.width.bottom > 0) emitBorderQuad(x, y + h - b.width.bottom, x + w, y + h, b.color);
                if (b.width.left > 0) emitBorderQuad(x, y, x + b.width.left, y + h, b.color);
                if (b.width.right > 0) emitBorderQuad(x + w - b.width.right, y, x + w, y + h, b.color);
                break;
            }

            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START:
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END:
                // Handled externally — caller must flush the current batch,
                // issue vkCmdSetScissor, then continue filling a new batch.
                // These are left as no-ops here intentionally.
                break;

            case CLAY_RENDER_COMMAND_TYPE_IMAGE:
                // TODO: emitQuad with image texture, useTexture = 1
                // Will need a separate descriptor set per image
                break;

            case CLAY_RENDER_COMMAND_TYPE_CUSTOM:
            case CLAY_RENDER_COMMAND_TYPE_NONE:
                break;

            default:
                throw std::runtime_error("Unknown Clay render command type");
        }
    }
}
