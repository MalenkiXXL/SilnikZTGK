#include "Font.h"
#include <vector>
#include <spdlog/spdlog.h>

// ZMIANA VFS: Dodajemy system wirtualny
#include "CookingStation/Core/VFS/VFS.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

Font::Font(const std::string& fontPath, float fontSize) {
    // 1. VFS: Wczytujemy plik .ttf do pamiêci RAM jako ci¹g bajtów
    std::vector<uint8_t> fontBuffer = VFS::ReadFile(fontPath);

    if (fontBuffer.empty()) {
        spdlog::error("[Font] Nie udalo sie zaladowac czcionki z VFS: {}", fontPath);
        return;
    }

    // 2. przygotowujemy parametry atlasu
    const int atlasWidth = 512;
    const int atlasHeight = 512;
    std::vector<unsigned char> bitmap(atlasWidth * atlasHeight);
    stbtt_bakedchar chardata[96]; // ASCII 32-126

    // 3. bakujemy bitmapê czcionki bezposrednio z RAM-u
    stbtt_BakeFontBitmap(fontBuffer.data(), 0, fontSize, bitmap.data(),
        atlasWidth, atlasHeight, 32, 96, chardata);

    // 4. konwertujemy 8-bitow¹ bitmapê (alpha) na 32-bitowe RGBA
    std::vector<uint32_t> rgbaData(atlasWidth * atlasHeight);
    for (int i = 0; i < atlasWidth * atlasHeight; i++) {
        unsigned char alpha = bitmap[i];
        rgbaData[i] = (alpha << 24) | (0xffffff); // bia³y kolor + kana³ alpha
    }

    // 5. tworzymy teksturê i przesy³amy dane
    m_Texture = std::make_shared<Texture>(atlasWidth, atlasHeight);
    m_Texture->SetData(rgbaData.data(), rgbaData.size() * sizeof(uint32_t));

    // 6. mapujemy dane znaków
    for (int i = 0; i < 96; i++) {
        char c = (char)(32 + i);
        stbtt_bakedchar b = chardata[i];
        Character ch;
        // skalujemy wspó³rzêdne pikselowe na zakres 0.0 - 1.0 (UV)
        ch.UV_Min = { (float)b.x0 / atlasWidth, (float)b.y0 / atlasHeight };
        ch.UV_Max = { (float)b.x1 / atlasWidth, (float)b.y1 / atlasHeight };
        ch.Size = { (float)(b.x1 - b.x0), (float)(b.y1 - b.y0) };
        ch.Offset = { (float)b.xoff, (float)b.yoff };
        ch.Advance = b.xadvance;
        m_Characters[c] = ch;
    }
}