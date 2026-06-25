#include "AudioEngine.h"
#include "CookingStation/Core/VFS/VFS.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "CookingStation/miniaudio.h"

ma_engine* AudioEngine::s_Engine = nullptr;
ma_sound* AudioEngine::s_BackgroundMusic = nullptr;
bool AudioEngine::s_IsMusicPlaying = false;
bool AudioEngine::s_MusicEnabled = true;
bool AudioEngine::s_SoundsEnabled = true;
float AudioEngine::s_MusicBaseVolume = 1.0f;

struct VfsAudioFile {
    std::vector<uint8_t> data;
    size_t cursor = 0;
};

static ma_result vfs_open(ma_vfs* pVFS, const char* pFilePath, ma_uint32 openMode, ma_vfs_file* pFile) {
    if (openMode & MA_OPEN_MODE_WRITE) return MA_ERROR;

    std::string path = pFilePath;
    std::replace(path.begin(), path.end(), '\\', '/');

    const std::string prefix = "CookingStation/Assets/";
    if (path.find(prefix) == 0) {
        path = "assets://" + path.substr(prefix.length());
    }

    std::vector<uint8_t> data = VFS::ReadFile(path);
    if (data.empty()) {
        return MA_DOES_NOT_EXIST;
    }

    VfsAudioFile* handle = new VfsAudioFile();
    handle->data = std::move(data);
    handle->cursor = 0;

    *pFile = (ma_vfs_file)handle;
    return MA_SUCCESS;
}

static ma_result vfs_close(ma_vfs* pVFS, ma_vfs_file file) {
    VfsAudioFile* handle = (VfsAudioFile*)file;
    delete handle;
    return MA_SUCCESS;
}

static ma_result vfs_read(ma_vfs* pVFS, ma_vfs_file file, void* pDst, size_t sizeInBytes, size_t* pBytesRead) {
    VfsAudioFile* handle = (VfsAudioFile*)file;
    size_t bytesToRead = sizeInBytes;

    if (handle->cursor + bytesToRead > handle->data.size()) {
        bytesToRead = handle->data.size() - handle->cursor;
    }

    if (bytesToRead > 0) {
        std::memcpy(pDst, handle->data.data() + handle->cursor, bytesToRead);
        handle->cursor += bytesToRead;
    }

    if (pBytesRead) *pBytesRead = bytesToRead;
    return MA_SUCCESS;
}

static ma_result vfs_seek(ma_vfs* pVFS, ma_vfs_file file, ma_int64 offset, ma_seek_origin origin) {
    VfsAudioFile* handle = (VfsAudioFile*)file;
    ma_int64 newCursor = handle->cursor;

    if (origin == ma_seek_origin_start) {
        newCursor = offset;
    }
    else if (origin == ma_seek_origin_current) {
        newCursor += offset;
    }
    else if (origin == ma_seek_origin_end) {
        newCursor = handle->data.size() + offset;
    }

    if (newCursor < 0) newCursor = 0;
    if (newCursor > handle->data.size()) newCursor = handle->data.size();
    handle->cursor = newCursor;

    return MA_SUCCESS;
}

static ma_result vfs_tell(ma_vfs* pVFS, ma_vfs_file file, ma_int64* pCursor) {
    VfsAudioFile* handle = (VfsAudioFile*)file;
    if (pCursor) *pCursor = handle->cursor;
    return MA_SUCCESS;
}

static ma_result vfs_info(ma_vfs* pVFS, ma_vfs_file file, ma_file_info* pInfo) {
    VfsAudioFile* handle = (VfsAudioFile*)file;
    if (pInfo) {
        pInfo->sizeInBytes = handle->data.size();
    }
    return MA_SUCCESS;
}


struct MyCustomVFS {
    ma_vfs_callbacks cb;
};

static MyCustomVFS g_CustomVFS;
static ma_resource_manager g_ResourceManager;

void AudioEngine::Init()
{
    s_Engine = new ma_engine();

    static ma_vfs_callbacks vfsCallbacks = {
        vfs_open,
        NULL,
        vfs_close,
        vfs_read,
        NULL,
        vfs_seek,
        vfs_tell,
        vfs_info
    };
    g_CustomVFS.cb = vfsCallbacks;

    ma_resource_manager_config rmConfig = ma_resource_manager_config_init();
    rmConfig.pVFS = (ma_vfs*)&g_CustomVFS;

    ma_result rmResult = ma_resource_manager_init(&rmConfig, &g_ResourceManager);
    if (rmResult != MA_SUCCESS) {
        std::cerr << "[AudioEngine] Blad inicjalizacji Menedzera Zasobow (miniaudio)!" << std::endl;
        return;
    }

    ma_engine_config engineConfig = ma_engine_config_init();
    engineConfig.pResourceManager = &g_ResourceManager;

    ma_result result = ma_engine_init(&engineConfig, s_Engine);
    if (result != MA_SUCCESS)
    {
        std::cerr << "[AudioEngine] Blad inicjalizacji miniaudio! Kod: " << result << std::endl;
        delete s_Engine;
        s_Engine = nullptr;
        return;
    }
    std::cout << "[AudioEngine] Zainicjowano pomyslnie z uzyciem VFS." << std::endl;
}

void AudioEngine::Shutdown()
{
    if (s_BackgroundMusic)
    {
        ma_sound_stop(s_BackgroundMusic);
        ma_sound_uninit(s_BackgroundMusic);
        delete s_BackgroundMusic;
        s_BackgroundMusic = nullptr;
    }
    s_IsMusicPlaying = false;

    if (s_Engine)
    {
        ma_engine_uninit(s_Engine);
        ma_resource_manager_uninit(&g_ResourceManager);
        delete s_Engine;
        s_Engine = nullptr;
        std::cout << "[AudioEngine] Zamknieto." << std::endl;
    }
}

void AudioEngine::Play(const std::string& filepath)
{
    if (!s_Engine || !s_SoundsEnabled) return;

    std::string vfsPath = filepath;
    std::replace(vfsPath.begin(), vfsPath.end(), '\\', '/');

    const std::string prefix = "CookingStation/Assets/";
    if (vfsPath.find(prefix) == 0) {
        vfsPath = "assets://" + vfsPath.substr(prefix.length());
    }

    ma_result result = ma_engine_play_sound(s_Engine, vfsPath.c_str(), NULL);
    if (result != MA_SUCCESS)
    {
        std::cerr << "[AudioEngine] Blad (Play): Nie mozna odtworzyc " << vfsPath
            << " | Kod bledu miniaudio: " << result << std::endl;
    }
}

void AudioEngine::PlayMusic(const std::string& filepath, bool loop, float volume)
{
    if (!s_Engine) return;

    if (s_IsMusicPlaying)
    {
        StopMusic();
    }

    if (!s_BackgroundMusic) {
        s_BackgroundMusic = new ma_sound();
    }

    std::string vfsPath = filepath;
    std::replace(vfsPath.begin(), vfsPath.end(), '\\', '/');

    const std::string prefix = "CookingStation/Assets/";
    if (vfsPath.find(prefix) == 0) {
        vfsPath = "assets://" + vfsPath.substr(prefix.length());
    }

    ma_result result = ma_sound_init_from_file(s_Engine, vfsPath.c_str(), MA_SOUND_FLAG_STREAM, NULL, NULL, s_BackgroundMusic);
    if (result != MA_SUCCESS)
    {
        std::cerr << "[AudioEngine] Blad ladowania muzyki tla: " << vfsPath << " Kod: " << result << std::endl;

        delete s_BackgroundMusic;
        s_BackgroundMusic = nullptr;
        return;
    }

    s_MusicBaseVolume = volume;
    float finalVolume = s_MusicEnabled ? volume : 0.0f;
    ma_sound_set_volume(s_BackgroundMusic, finalVolume);

    ma_sound_set_looping(s_BackgroundMusic, loop ? MA_TRUE : MA_FALSE);
    ma_result startResult = ma_sound_start(s_BackgroundMusic);

    if (startResult == MA_SUCCESS)
    {
        s_IsMusicPlaying = true;
        std::cout << "[AudioEngine] Muzyka tla wystartowala: " << vfsPath << std::endl;
    }
}

void AudioEngine::StopMusic()
{
    if (!s_Engine || !s_IsMusicPlaying || !s_BackgroundMusic) return;

    ma_sound_stop(s_BackgroundMusic);
    ma_sound_uninit(s_BackgroundMusic);

    delete s_BackgroundMusic;
    s_BackgroundMusic = nullptr;

    s_IsMusicPlaying = false;
    std::cout << "[AudioEngine] Muzyka tla zatrzymana." << std::endl;
}

void AudioEngine::SetMusicEnabled(bool enabled)
{
    s_MusicEnabled = enabled;

    if (s_BackgroundMusic && s_IsMusicPlaying) {
        ma_sound_set_volume(s_BackgroundMusic, enabled ? s_MusicBaseVolume : 0.0f);
    }
}

bool AudioEngine::IsMusicEnabled()
{
    return s_MusicEnabled;
}

void AudioEngine::SetSoundsEnabled(bool enabled)
{
    s_SoundsEnabled = enabled;
}

bool AudioEngine::AreSoundsEnabled()
{
    return s_SoundsEnabled;
}

ma_sound* AudioEngine::PlayLoopingSound(const std::string& filepath, float volume, bool loop)
{
    if (!s_Engine || !s_SoundsEnabled) return nullptr;

    std::string vfsPath = filepath;
    std::replace(vfsPath.begin(), vfsPath.end(), '\\', '/');

    const std::string prefix = "CookingStation/Assets/";
    if (vfsPath.find(prefix) == 0) {
        vfsPath = "assets://" + vfsPath.substr(prefix.length());
    }

    ma_sound* sound = new ma_sound();

    ma_result result = ma_sound_init_from_file(s_Engine, vfsPath.c_str(), 0, NULL, NULL, sound);
    if (result != MA_SUCCESS)
    {
        std::cerr << "[AudioEngine] Blad ladowania zapetlonego dzwieku: " << vfsPath << std::endl;
        delete sound;
        return nullptr;
    }

    ma_sound_set_volume(sound, volume);
    ma_sound_set_looping(sound, loop ? MA_TRUE : MA_FALSE);
    ma_sound_start(sound);

    return sound;
}

void AudioEngine::ReplaySound(ma_sound* sound)
{
    if (!sound) return;

    ma_sound_seek_to_pcm_frame(sound, 0);
    ma_sound_start(sound);
}

void AudioEngine::StopLoopingSound(ma_sound* sound)
{
    if (!sound) return;

    ma_sound_stop(sound);
    ma_sound_uninit(sound);
    delete sound;
}