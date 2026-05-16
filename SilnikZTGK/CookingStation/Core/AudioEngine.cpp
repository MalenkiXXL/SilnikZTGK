#include "AudioEngine.h"
#include "CookingStation/Core/VFS/VFS.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// Zwyk³y include. miniaudio_impl.cpp przejmuje definicjê!
#include "CookingStation/miniaudio.h"

// FIZYCZNA DEFINICJA ZMIENNEJ STATYCZNEJ
ma_engine* AudioEngine::s_Engine = nullptr;

// ====================================================================
// W£ASNY SYSTEM WEJŒCIA/WYJŒCIA DLA MINIAUDIO (INTEGRACJA Z VFS)
// ====================================================================

// Struktura, która udaje plik dla miniaudio (trzyma ci¹g bajtów w RAM)
struct VfsAudioFile {
    std::vector<uint8_t> data;
    size_t cursor = 0;
};

// Funkcja otwieraj¹ca plik - zasilana przez Twój VFS
static ma_result vfs_open(ma_vfs* pVFS, const char* pFilePath, ma_uint32 openMode, ma_vfs_file* pFile) {
    if (openMode & MA_OPEN_MODE_WRITE) return MA_ERROR; // Obs³ugujemy tylko odczyt

    std::string path = pFilePath;
    std::replace(path.begin(), path.end(), '\\', '/');

    // Sanitizer œcie¿ek (gdyby jakaœ encja wci¹¿ mia³a star¹ fizyczn¹ œcie¿kê)
    const std::string prefix = "CookingStation/Assets/";
    if (path.find(prefix) == 0) {
        path = "assets://" + path.substr(prefix.length());
    }

    std::vector<uint8_t> data = VFS::ReadFile(path);
    if (data.empty()) {
        return MA_DOES_NOT_EXIST;
    }

    // Tworzymy uchwyt do "pliku" w pamiêci RAM
    VfsAudioFile* handle = new VfsAudioFile();
    handle->data = std::move(data);
    handle->cursor = 0;

    *pFile = (ma_vfs_file)handle;
    return MA_SUCCESS;
}

// Funkcja zamykaj¹ca plik i zwalniaj¹ca pamiêæ RAM
static ma_result vfs_close(ma_vfs* pVFS, ma_vfs_file file) {
    VfsAudioFile* handle = (VfsAudioFile*)file;
    delete handle;
    return MA_SUCCESS;
}

// Funkcja czytaj¹ca paczki bajtów
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

// Przewijanie dŸwiêku
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

// ====================================================================
// INICJALIZACJA SILNIKA
// ====================================================================

// W£ASNA STRUKTURA VFS DLA MINIAUDIO (Rozwi¹zuje b³¹d ma_vfs aka void)
struct MyCustomVFS {
    ma_vfs_callbacks cb; // Musi byæ na pierwszym miejscu!
};

// Trzymamy instancje globalnie w tym pliku, z dala od nag³ówka
static MyCustomVFS g_CustomVFS;
static ma_resource_manager g_ResourceManager;

void AudioEngine::Init()
{
    s_Engine = new ma_engine();

    // 1. Zapiêcie naszych funkcji pod strukturê miniaudio
    static ma_vfs_callbacks vfsCallbacks = {
        vfs_open,
        NULL, // onOpenW (Unicode, pomijamy)
        vfs_close,
        vfs_read,
        NULL, // onWrite (pomijamy, tylko do odczytu)
        vfs_seek,
        vfs_tell,
        vfs_info
    };
    g_CustomVFS.cb = vfsCallbacks;

    // 2. Inicjalizacja profesjonalnego Menad¿era Zasobów z naszym VFS
    ma_resource_manager_config rmConfig = ma_resource_manager_config_init();
    rmConfig.pVFS = (ma_vfs*)&g_CustomVFS; // Rzutujemy nasz¹ strukturê na uchwyt miniaudio

    ma_result rmResult = ma_resource_manager_init(&rmConfig, &g_ResourceManager);
    if (rmResult != MA_SUCCESS) {
        std::cerr << "[AudioEngine] Blad inicjalizacji Menedzera Zasobow (miniaudio)!" << std::endl;
        return;
    }

    // 3. Inicjalizacja samego silnika z podpiêtym menad¿erem
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
    if (s_Engine)
    {
        ma_engine_uninit(s_Engine);
        ma_resource_manager_uninit(&g_ResourceManager); // Wy³¹czamy te¿ menad¿era zasobów
        delete s_Engine;
        s_Engine = nullptr;
        std::cout << "[AudioEngine] Zamknieto." << std::endl;
    }
}

void AudioEngine::Play(const std::string& filepath)
{
    if (!s_Engine) return;

    // --- SANITIZER ŒCIE¯EK ---
    std::string vfsPath = filepath;
    std::replace(vfsPath.begin(), vfsPath.end(), '\\', '/');

    const std::string prefix = "CookingStation/Assets/";
    if (vfsPath.find(prefix) == 0) {
        vfsPath = "assets://" + vfsPath.substr(prefix.length());
    }

    // Wyrzucamy dŸwiêk! Miniaudio nie dotyka ju¿ dysku. 
    // Poprosi nasz "g_CustomVFS" o za³adowanie pliku `assets://...` do strumienia z RAM-u.
    ma_engine_play_sound(s_Engine, vfsPath.c_str(), NULL);
}