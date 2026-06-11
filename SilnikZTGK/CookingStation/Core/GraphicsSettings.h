#pragma once
#include <utility>

struct GraphicsSettings {
    int MsaaSamples = 4;  
    int WindowWidth = 1280;
    int WindowHeight = 720;

    static constexpr int ResolutionCount = 4;
    static constexpr std::pair<int, int> Resolutions[ResolutionCount] = {
        {1280, 720},
        {1600, 900},
        {1920, 1080},
        {2560, 1440}
    };

    static GraphicsSettings& Get() {
        static GraphicsSettings s_Instance;
        return s_Instance;
    }

private:
    GraphicsSettings() = default;
};