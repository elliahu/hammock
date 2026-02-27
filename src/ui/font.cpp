#include "font.hpp"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"
#include <fstream>
#include <stdexcept>

hammock::ui::FontAtlas hammock::ui::loadFont(const std::filesystem::path& ttfPath) {
    // Read TTF into memory
    if (!std::filesystem::exists(ttfPath)) {
        throw std::runtime_error("Font file not found: " + ttfPath.string());
    }

    std::ifstream file(ttfPath, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Failed to open font file: " + ttfPath.string());
    }

    const auto fileSize = static_cast<std::size_t>(file.tellg());
    file.seekg(0);

    std::vector<uint8_t> ttfBuffer(fileSize);
    file.read(reinterpret_cast<char*>(ttfBuffer.data()), fileSize);

    // Allocate atlas bitmap
    FontAtlas atlas{};
    atlas.bitmap.resize(ATLAS_WIDTH * ATLAS_HEIGHT, 0);

    // Pack glyphs into bitmap
    stbtt_pack_context packCtx{};
    stbtt_PackBegin(&packCtx,
        atlas.bitmap.data(),
        ATLAS_WIDTH,
        ATLAS_HEIGHT,
        0,  // stride (0 = tightly packed)
        1,  // padding between glyphs in pixels
        nullptr);

    stbtt_packedchar packedChars[CHAR_COUNT]{};
    stbtt_PackFontRange(&packCtx,
        ttfBuffer.data(),
        0,  // font index
        FONT_SIZE_PX,
        FIRST_CHAR,
        CHAR_COUNT,
        packedChars);

    stbtt_PackEnd(&packCtx);

    // Convert stb glyph data to our GlyphInfo
    for (int i = 0; i < CHAR_COUNT; i++) {
        const stbtt_packedchar& pc = packedChars[i];
        GlyphInfo& g = atlas.glyphs[i];

        g.u0 = static_cast<float>(pc.x0) / ATLAS_WIDTH;
        g.v0 = static_cast<float>(pc.y0) / ATLAS_HEIGHT;
        g.u1 = static_cast<float>(pc.x1) / ATLAS_WIDTH;
        g.v1 = static_cast<float>(pc.y1) / ATLAS_HEIGHT;

        g.width = static_cast<float>(pc.x1 - pc.x0);
        g.height = static_cast<float>(pc.y1 - pc.y0);
        g.bearingX = pc.xoff;
        g.bearingY = pc.yoff;
        g.advanceX = pc.xadvance;
    }

    // Extract line metrics
    stbtt_fontinfo fontInfo{};
    stbtt_InitFont(&fontInfo, ttfBuffer.data(), stbtt_GetFontOffsetForIndex(ttfBuffer.data(), 0));

    const float scale = stbtt_ScaleForPixelHeight(&fontInfo, FONT_SIZE_PX);
    int ascent{}, descent{}, lineGap{};
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

    atlas.ascent = static_cast<float>(ascent) * scale;
    atlas.descent = static_cast<float>(descent) * scale;  // negative
    atlas.lineHeight = static_cast<float>(ascent - descent + lineGap) * scale;

    return atlas;
}
