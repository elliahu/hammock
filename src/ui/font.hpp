#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include "stb/stb_truetype.h"

namespace hammock::ui {

    //
    // The cursor sits on the baseline. For each character in a string:
    //
    //   const GlyphInfo& g = atlas.glyphs[ch - FIRST_CHAR];
    //
    //   float left   = cursorX + g.bearingX;
    //   float top    = cursorY + g.bearingY;   // bearingY negative → above baseline
    //   float right  = left + g.width;
    //   float bottom = top  + g.height;
    //
    //   // emit quad (left, top) → (right, bottom) with UVs (u0,v0) → (u1,v1)
    //   cursorX += g.advanceX;
    //
    // New line:
    //   cursorY += atlas.lineHeight;
    //
    // Vertically center text in a box of height H:
    //   cursorY = boxTop + (H - atlas.lineHeight) * 0.5f + atlas.ascent;

    inline constexpr int ATLAS_WIDTH = 512;  // texture dimensions — power of 2
    inline constexpr int ATLAS_HEIGHT = 512;
    inline constexpr float FONT_SIZE_PX = 32.0f;
    inline constexpr int FIRST_CHAR = 32;  // ASCII space
    inline constexpr int CHAR_COUNT = 96;  // space → tilde (covers all printable ASCII)

    struct GlyphInfo {
        float u0, v0, u1, v1;  // UV coordinates in atlas (0..1)
        float advanceX;        // cursor advance after this glyph
        float bearingX;        // horizontal offset from cursor to quad left edge
        float bearingY;        // vertical offset from baseline to quad top (usually negative)
        float width, height;   // glyph size in pixels
    };

    struct FontAtlas {
        std::vector<uint8_t> bitmap;   // ATLAS_WIDTH * ATLAS_HEIGHT greyscale
        GlyphInfo glyphs[CHAR_COUNT];  // indexed by (char - FIRST_CHAR)
        float lineHeight;              // recommended line spacing in pixels
        float ascent;                  // baseline to top of tallest glyph
        float descent;                 // baseline to bottom (negative)
    };

    FontAtlas loadFont(const std::filesystem::path& ttfPath);
}  // namespace hammock::renderer