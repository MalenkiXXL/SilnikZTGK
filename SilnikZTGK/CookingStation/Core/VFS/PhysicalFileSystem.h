#pragma once
#include "IFileSystem.h"
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>

class PhysicalFileSystem : public IFileSystem {
private:
    std::string m_RootDirectory;

public:
    PhysicalFileSystem(const std::string& rootDir) : m_RootDirectory(rootDir) {}

    std::vector<uint8_t> ReadFile(const std::string& filepath) override {
        std::filesystem::path fullPath = std::filesystem::path(m_RootDirectory) / filepath;

        // U¿ywamy .string(), dla starszych implementacji std::ifstream
        std::ifstream file(fullPath.string(), std::ios::binary | std::ios::ate);

        if (!file.is_open()) {
            // Tego loga potrzebujemy, jeœli cokolwiek jeszcze nie zadzia³a:
            spdlog::error("[PhysicalFS] System operacyjny odrzucil probe otwarcia pliku. Szukano pod adresem: {}", fullPath.string());
            return {};
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(size);
        if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            return buffer;
        }

        spdlog::error("[PhysicalFS] Blad odczytu bitow z pliku (pusty?): {}", fullPath.string());
        return {};
    }

    bool Exists(const std::string& filepath) override {
        std::filesystem::path fullPath = std::filesystem::path(m_RootDirectory) / filepath;
        return std::filesystem::exists(fullPath);
    }
};