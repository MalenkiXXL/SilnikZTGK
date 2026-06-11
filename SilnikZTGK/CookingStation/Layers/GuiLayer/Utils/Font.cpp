#include "Font.h"
#include <vector>
#include <spdlog/spdlog.h>
#include "CookingStation/Core/VFS/VFS.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

Font::Font(const std::string& fontPath, float fontSize) {
    std::vector<uint8_t> fontBuffer = VFS::ReadFile(fontPath);

    if (fontBuffer.empty()) {
        spdlog::error("[Font] Nie udalo sie zaladowac czcionki z VFS: {}", fontPath);
        return;
    }

    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, fontBuffer.data(), 0)) {
        spdlog::error("[Font] Nie udalo sie zainicjowac czcionki stb_truetype!");
        return;
    }

    const int atlasWidth = 1024;
    const int atlasHeight = 1024;
    std::vector<unsigned char> bitmap(atlasWidth * atlasHeight, 0);

    float scale = stbtt_ScaleForPixelHeight(&info, fontSize);
    int padding = 4; 
    unsigned char onedge_value = 128; 
    float pixel_dist_scale = 32.0f; 

    int currentX = 0;
    int currentY = 0;
    int maxRowHeight = 0;

    for (int i = 0; i < 96; i++) {
        char c = (char)(32 + i);
        int w = 0, h = 0, xoff = 0, yoff = 0;

        unsigned char* sdf = stbtt_GetCodepointSDF(&info, scale, c, padding, onedge_value, pixel_dist_scale, &w, &h, &xoff, &yoff);

        if (!sdf || w == 0 || h == 0) {
            if (sdf) stbtt_FreeSDF(sdf, nullptr);

            int advanceWidth, leftSideBearing;
            stbtt_GetCodepointHMetrics(&info, c, &advanceWidth, &leftSideBearing);

            Character ch;
            ch.UV_Min = { 0.0f, 0.0f };
            ch.UV_Max = { 0.0f, 0.0f };
            ch.Size = { 0.0f, 0.0f };
            ch.Offset = { 0.0f, 0.0f };
            ch.Advance = advanceWidth * scale;

            m_Characters[c] = ch;
            continue;
        }

        if (currentX + w >= atlasWidth) {
            currentX = 0;
            currentY += maxRowHeight;
            maxRowHeight = 0;
        }

        if (currentY + h > atlasHeight) {
            spdlog::error("[Font] Atlas przepelniony! Zwieksz rozmiar atlasu lub zmniejsz fontSize.");
            stbtt_FreeSDF(sdf, nullptr);
            break;
        }

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                bitmap[(currentY + y) * atlasWidth + (currentX + x)] = sdf[y * w + x];
            }
        }
        stbtt_FreeSDF(sdf, nullptr);

        int advanceWidth, leftSideBearing;
        stbtt_GetCodepointHMetrics(&info, c, &advanceWidth, &leftSideBearing);

        Character ch;
        ch.UV_Min = { (float)currentX / atlasWidth, (float)currentY / atlasHeight };
        ch.UV_Max = { (float)(currentX + w) / atlasWidth, (float)(currentY + h) / atlasHeight };
        ch.Size = { (float)w, (float)h };
        ch.Offset = { (float)xoff, (float)yoff };
        ch.Advance = advanceWidth * scale;

        m_Characters[c] = ch;

        currentX += w;
        if (h > maxRowHeight) maxRowHeight = h;
    }

    std::vector<uint32_t> rgbaData(atlasWidth * atlasHeight);
    for (int i = 0; i < atlasWidth * atlasHeight; i++) {
        unsigned char alpha = bitmap[i];
        rgbaData[i] = (alpha << 24) | (0xffffff); 
    }

    m_Texture = std::make_shared<Texture>(atlasWidth, atlasHeight);
    m_Texture->SetData(rgbaData.data(), rgbaData.size() * sizeof(uint32_t));
}