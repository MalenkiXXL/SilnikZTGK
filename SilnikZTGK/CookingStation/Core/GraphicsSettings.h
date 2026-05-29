#pragma once
#include <utility>

struct GraphicsSettings {
    int MsaaSamples = 4;   // 1 = wylaczone, 2, 4, 8
    int WindowWidth = 1280;
    int WindowHeight = 720;

    // Preset rozdzielczosci
    static constexpr int ResolutionCount = 5;
    static constexpr std::pair<int, int> Resolutions[ResolutionCount] = {
        {1280, 720},
        {1600, 900},
        {1920, 1080},
        {2560, 1440},
        {3840, 2160}
    };

    static GraphicsSettings& Get() {
        static GraphicsSettings s_Instance;
        return s_Instance;
    }

private:
    GraphicsSettings() = default;
};