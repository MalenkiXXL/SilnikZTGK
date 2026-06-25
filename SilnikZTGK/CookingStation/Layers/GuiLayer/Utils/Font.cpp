#include "Font.h"
#include <vector>
#include <spdlog/spdlog.h>

#include "CookingStation/Core/VFS/VFS.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

Font::Font(const std::string& fontPath, float fontSize) {

    auto initFallback = [this]() {
        m_Texture = std::make_shared<Texture>(1, 1);
        uint32_t white = 0xffffffff;
        m_Texture->SetData(&white, sizeof(uint32_t));
        for (int i = 0; i < 96; i++) {
            m_Characters[(char)(32 + i)] = { {0,0}, {1,1}, {0,0}, {0,0}, 10.0f };
        }
        };

    std::vector<uint8_t> fontBuffer = VFS::ReadFile(fontPath);

    if (fontBuffer.size() < 256) {
        spdlog::error("[Font] Nie udalo sie zaladowac czcionki (brak pliku lub uszkodzony): {}", fontPath);
        initFallback();
        return;
    }

    
    int fontOffset = stbtt_GetFontOffsetForIndex(fontBuffer.data(), 0);
    if (fontOffset < 0) {
        spdlog::error("[Font] Plik nie ma poprawnego naglowka TTF/TTC: {}", fontPath);
        initFallback();
        return;
    }

    const int atlasWidth = 512;
    const int atlasHeight = 512;
    std::vector<unsigned char> bitmap(atlasWidth * atlasHeight, 0);
    stbtt_bakedchar chardata[96]; 

    int result = stbtt_BakeFontBitmap(fontBuffer.data(), fontOffset, fontSize, bitmap.data(),
        atlasWidth, atlasHeight, 32, 96, chardata);

    if (result <= 0) {
        spdlog::warn("[Font] Ostrzezenie: atlas {}x{} moze byc za maly dla rozmiaru czcionki {}", atlasWidth, atlasHeight, fontSize);
    }

    std::vector<uint32_t> rgbaData(atlasWidth * atlasHeight);
    for (int i = 0; i < atlasWidth * atlasHeight; i++) {
        unsigned char alpha = bitmap[i];
        rgbaData[i] = (alpha << 24) | (0xffffff); 
    }

    m_Texture = std::make_shared<Texture>(atlasWidth, atlasHeight);
    m_Texture->SetData(rgbaData.data(), rgbaData.size() * sizeof(uint32_t));

    for (int i = 0; i < 96; i++) {
        char c = (char)(32 + i);
        stbtt_bakedchar b = chardata[i];
        Character ch;
        ch.UV_Min = { (float)b.x0 / atlasWidth, (float)b.y0 / atlasHeight };
        ch.UV_Max = { (float)b.x1 / atlasWidth, (float)b.y1 / atlasHeight };
        ch.Size = { (float)(b.x1 - b.x0), (float)(b.y1 - b.y0) };
        ch.Offset = { (float)b.xoff, (float)b.yoff };
        ch.Advance = b.xadvance;
        m_Characters[c] = ch;
    }
}