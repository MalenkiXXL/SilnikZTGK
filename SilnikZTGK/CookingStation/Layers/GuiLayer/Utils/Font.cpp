#include "Font.h"
#include <vector>
#include <spdlog/spdlog.h>
#include "CookingStation/Core/VFS/VFS.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

Font::Font(const std::string& fontPath, float fontSize) {
    // 1. Wczytujemy plik czcionki do pamiêci RAM
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

    // U¿ywamy nieco wiêkszego atlasu dla wy¿szej jakoœci SDF
    const int atlasWidth = 1024;
    const int atlasHeight = 1024;
    std::vector<unsigned char> bitmap(atlasWidth * atlasHeight, 0);

    // Parametry generatora SDF
    float scale = stbtt_ScaleForPixelHeight(&info, fontSize);
    int padding = 4; // Margines wokó³ znaku dla p³ynnego przejœcia SDF
    unsigned char onedge_value = 128; // Po³owa kana³u alfa to matematyczna granica litery (0.5)
    float pixel_dist_scale = 32.0f; // Skala szybkoœci opadania promienia (im wy¿sza, tym ostrzejszy gradient)

    int currentX = 0;
    int currentY = 0;
    int maxRowHeight = 0;

    // Generowanie mapy SDF dla znaków ASCII (32-126)
    for (int i = 0; i < 96; i++) {
        char c = (char)(32 + i);
        int w = 0, h = 0, xoff = 0, yoff = 0;

        // Magia dzieje siê tutaj - zamiast standardowej bitmapy, wyci¹gamy Dystans!
        unsigned char* sdf = stbtt_GetCodepointSDF(&info, scale, c, padding, onedge_value, pixel_dist_scale, &w, &h, &xoff, &yoff);

        // POPRAWKA 1: jeœli glif jest pusty (np. spacja), pomijamy zapis do atlasu
        if (!sdf || w == 0 || h == 0) {
            if (sdf) stbtt_FreeSDF(sdf, nullptr);

            // Mimo to zapisujemy metryki znaku (np. szerokoœæ spacji)
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

        // POPRAWKA 2: sprawdzenie poziome (zawijanie wiersza)
        if (currentX + w >= atlasWidth) {
            currentX = 0;
            currentY += maxRowHeight;
            maxRowHeight = 0;
        }

        // POPRAWKA 3: sprawdzenie pionowe — atlas nie mo¿e byæ przepe³niony
        if (currentY + h > atlasHeight) {
            spdlog::error("[Font] Atlas przepelniony! Zwieksz rozmiar atlasu lub zmniejsz fontSize.");
            stbtt_FreeSDF(sdf, nullptr);
            break;
        }

        // Zapis SDF do bitmapy atlasu
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

        // POPRAWKA 4: aktualizujemy pozycjê TYLKO gdy glif faktycznie istnieje
        currentX += w;
        if (h > maxRowHeight) maxRowHeight = h;
    }

    // Przerzucamy czarno-bia³y SDF do kana³u ALFA dla naszej tekstury!
    std::vector<uint32_t> rgbaData(atlasWidth * atlasHeight);
    for (int i = 0; i < atlasWidth * atlasHeight; i++) {
        unsigned char alpha = bitmap[i];
        rgbaData[i] = (alpha << 24) | (0xffffff); // Bia³y kolor (RGB), Dystans SDF (Alpha)
    }

    m_Texture = std::make_shared<Texture>(atlasWidth, atlasHeight);
    m_Texture->SetData(rgbaData.data(), rgbaData.size() * sizeof(uint32_t));
}