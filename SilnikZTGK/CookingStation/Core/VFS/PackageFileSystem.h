#pragma once
#include "IFileSystem.h"
#include <fstream>
#include <unordered_map>
#include <spdlog/spdlog.h>
#include <algorithm> // Wymagane dla std::replace

struct PakEntry {
    uint64_t Offset;
    uint64_t Size;
};

class PackageFileSystem : public IFileSystem {
private:
    std::string m_PakFilePath;
    std::unordered_map<std::string, PakEntry> m_FileEntries;

    // Pomocnicza funkcja do normalizacji œcie¿ek
    std::string CleanPath(const std::string& filepath) const {
        std::string cleanPath = filepath;

        // 1. Usuwamy ewentualny prefiks (np. assets:// lub shaders://), jeœli VFS go nam przekaza³
        size_t pos = cleanPath.find("://");
        if (pos != std::string::npos) {
            cleanPath = cleanPath.substr(pos + 3);
        }

        // 2. Zamieniamy wszystkie ukoœniki wsteczne (Windows) na zwyk³e, bo w .pak u¿yliœmy generic_string()
        std::replace(cleanPath.begin(), cleanPath.end(), '\\', '/');

        return cleanPath;
    }

public:
    PackageFileSystem(const std::string& pakPath) : m_PakFilePath(pakPath) {
        LoadTableOfContents();
    }

    std::vector<uint8_t> ReadFile(const std::string& filepath) override {
        std::string cleanPath = CleanPath(filepath);

        if (m_FileEntries.find(cleanPath) == m_FileEntries.end()) {
            spdlog::error("[PackageFS] Nie znaleziono pliku w archiwum: {}", cleanPath);
            return {}; // Plik nie istnieje w archiwum
        }

        const auto& entry = m_FileEntries[cleanPath];
        std::ifstream file(m_PakFilePath, std::ios::binary);
        if (!file.is_open()) {
            spdlog::error("[PackageFS] Nie udalo sie otworzyc pliku .pak na dysku: {}", m_PakFilePath);
            return {};
        }

        // Przesuñ wskaŸnik czytania w odpowiednie miejsce pliku .pak
        file.seekg(entry.Offset, std::ios::beg);

        std::vector<uint8_t> buffer(entry.Size);
        if (file.read(reinterpret_cast<char*>(buffer.data()), entry.Size)) {
            return buffer;
        }

        spdlog::error("[PackageFS] Blad odczytu bajtow dla pliku: {}", cleanPath);
        return {};
    }

    bool Exists(const std::string& filepath) override {
        return m_FileEntries.find(CleanPath(filepath)) != m_FileEntries.end();
    }

private:
    void LoadTableOfContents() {
        std::ifstream file(m_PakFilePath, std::ios::binary);
        if (!file.is_open()) {
            spdlog::error("[PackageFS] Nie mozna znalezc lub otworzyc archiwum: {}", m_PakFilePath);
            return;
        }

        // 1. Odczyt iloœci plików z samego pocz¹tku archiwum
        uint32_t numFiles = 0;
        if (!file.read(reinterpret_cast<char*>(&numFiles), sizeof(uint32_t))) {
            spdlog::error("[PackageFS] Plik archiwum jest pusty lub uszkodzony: {}", m_PakFilePath);
            return;
        }

        // 2. Czytamy w pêtli poszczególne wpisy w Spisie Treœci
        for (uint32_t i = 0; i < numFiles; ++i) {
            uint32_t pathLen = 0;
            file.read(reinterpret_cast<char*>(&pathLen), sizeof(uint32_t));

            std::string path(pathLen, '\0');
            file.read(&path[0], pathLen);

            uint64_t offset = 0;
            uint64_t size = 0;
            file.read(reinterpret_cast<char*>(&offset), sizeof(uint64_t));
            file.read(reinterpret_cast<char*>(&size), sizeof(uint64_t));

            // Na wszelki wypadek normalizujemy te¿ klucz zapamiêtywany w mapie
            std::replace(path.begin(), path.end(), '\\', '/');

            // Zapisujemy w RAM gdzie i ile bajtów musimy przeczytaæ 
            m_FileEntries[path] = { offset, size };
        }

        file.close();
        spdlog::info("[PackageFS] Zarejestrowano {} plikow z archiwum {}", numFiles, m_PakFilePath);
    }
};