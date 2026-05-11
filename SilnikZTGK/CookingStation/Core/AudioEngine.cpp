#include "AudioEngine.h"
#include <iostream>

// Zwyk³y include. miniaudio_impl.cpp przejmuje definicjê!
#include "CookingStation/miniaudio.h"

// FIZYCZNA DEFINICJA ZMIENNEJ STATYCZNEJ (bez tego dostajesz b³¹d "niezdefiniowany")
ma_engine* AudioEngine::s_Engine = nullptr;

void AudioEngine::Init()
{
    s_Engine = new ma_engine();

    ma_result result = ma_engine_init(NULL, s_Engine);
    if (result != MA_SUCCESS)
    {
        std::cerr << "[AudioEngine] Blad inicjalizacji miniaudio! Kod: " << result << std::endl;
        delete s_Engine;
        s_Engine = nullptr;
        return;
    }
    std::cout << "[AudioEngine] Zainicjowano pomyslnie." << std::endl;
}

void AudioEngine::Shutdown()
{
    if (s_Engine)
    {
        ma_engine_uninit(s_Engine);
        delete s_Engine;
        s_Engine = nullptr;
        std::cout << "[AudioEngine] Zamknieto." << std::endl;
    }
}

// ZMIENIONO NAZWÊ na Play
void AudioEngine::Play(const std::string& filepath)
{
    if (!s_Engine) return;

    ma_engine_play_sound(s_Engine, filepath.c_str(), NULL);
}