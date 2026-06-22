#pragma once
#include <string>

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

    static void SetMusicEnabled(bool enabled);
    static bool IsMusicEnabled();

    static void SetSoundsEnabled(bool enabled);
    static bool AreSoundsEnabled();

    static ma_sound* PlayLoopingSound(const std::string& filepath, float volume, bool loop = true);
    static void StopLoopingSound(ma_sound *sound);

    static void ReplaySound(ma_sound *sound);

private:
    static ma_engine* s_Engine;

    static ma_sound* s_BackgroundMusic;
    static bool s_IsMusicPlaying;

    static bool s_MusicEnabled;
    static bool s_SoundsEnabled; 

    static float s_MusicBaseVolume;
};