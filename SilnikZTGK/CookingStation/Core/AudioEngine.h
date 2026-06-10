#pragma once
#include <string>

// Forward declaration
struct ma_engine;
struct ma_sound;

class AudioEngine
{
public:
    static void Init();
    static void Shutdown();

    static void Play(const std::string& filepath);

    static void PlayMusic(const std::string& filepath, bool loop = true, float volume = 1.0f);
    static void StopMusic();

private:
    static ma_engine* s_Engine;

    static ma_sound* s_BackgroundMusic;
    static bool s_IsMusicPlaying;
};