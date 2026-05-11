#pragma once
#include <string>

// Forward declaration
struct ma_engine;

class AudioEngine
{
public:
    static void Init();
    static void Shutdown();

    // ZMIENIONO NAZWÊ, ¿eby unikn¹æ konfliktu z makrem Windowsa:
    static void Play(const std::string& filepath);

private:
    // Upewnij siê, ¿e ta linijka tu jest!
    static ma_engine* s_Engine;
};