#pragma once
#include <utility>
#include <GLFW/glfw3.h>

struct GraphicsSettingsChangedEvent {};

struct GraphicsSettings {
    int MsaaSamples = 4;
    int WindowWidth = 1920;   
    int WindowHeight = 1080;
    bool Fullscreen = true;

    static constexpr int ResolutionCount = 5;
    static constexpr std::pair<int, int> Resolutions[ResolutionCount] = {
        {1280, 720},
        {1600, 900},
        {1920, 1080},
        {2560, 1440},
        {2560, 1600}
    };

    static int FindBestResolutionIndex(int monitorWidth, int monitorHeight) {
        int bestIndex = 0;
        for (int i = 0; i < ResolutionCount; i++) {
            if (Resolutions[i].first <= monitorWidth && Resolutions[i].second <= monitorHeight) {
                bestIndex = i;
            }
        }
        return bestIndex;
    }

    static GraphicsSettings& Get() {
        static GraphicsSettings s_Instance;
        return s_Instance;
    }

private:
    GraphicsSettings() {
     
    }
};