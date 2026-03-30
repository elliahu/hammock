#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include "stb/stb_truetype.h"

namespace hammock::ui {

    // This file contains types and function related to font manipulation

    inline constexpr int ATLAS_WIDTH = 512;       // texture horizontal size — power of 2
    inline constexpr int ATLAS_HEIGHT = 512;      // texture vertical size — power of 2
    inline constexpr float FONT_SIZE_PX = 32.0f;  // font size in pixels in the atlas
    inline constexpr int FIRST_CHAR = 32;         // ASCII space
    inline constexpr int CHAR_COUNT = 96;         // space → tilde (covers all printable ASCII)

    /// @struct GlyphInfo
    /// @brief Information about a glyph in the font atlas
    struct GlyphInfo {
        float u0, v0, u1, v1;  // UV coordinates in atlas (0..1)
        float advanceX;        // cursor advance after this glyph
        float bearingX;        // horizontal offset from cursor to quad left edge
        float bearingY;        // vertical offset from baseline to quad top (usually negative)
        float width, height;   // glyph size in pixels
    };

    /// @struct FontAtlas
    /// @brief Represents a font atlas containing glyphs for all printable ASCII characters
    /// The font atlas is CPU side resource and needs to be uploaded to GPU memory separately
    struct FontAtlas {
        std::vector<uint8_t> bitmap;   // ATLAS_WIDTH * ATLAS_HEIGHT greyscale
        GlyphInfo glyphs[CHAR_COUNT];  // indexed by (char - FIRST_CHAR)
        float lineHeight;              // recommended line spacing in pixels
        float ascent;                  // baseline to top of tallest glyph
        float descent;                 // baseline to bottom (negative)
    };

    /// @brief Loads a font atlas from a TTF file
    /// @param ttfPath Path to the TTF file
    /// @return A FontAtlas struct containing the glyph data
    FontAtlas loadFont(const std::filesystem::path& ttfPath);
}  // namespace hammock::ui
