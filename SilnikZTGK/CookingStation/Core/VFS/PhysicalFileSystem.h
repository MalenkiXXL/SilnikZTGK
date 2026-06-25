#pragma once
#include "IFileSystem.h"
#include <fstream>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

class PhysicalFileSystem : public IFileSystem {
private:
    std::string m_RootDirectory;

public:
    PhysicalFileSystem(const std::string& rootDir) : m_RootDirectory(rootDir) {}

    std::vector<uint8_t> ReadFile(const std::string& filepath) override {
        
        std::string fullPath = m_RootDirectory;
        if (!fullPath.empty() && fullPath.back() != '/' && fullPath.back() != '\\') {
            fullPath += '/';
        }
        fullPath += filepath;

        std::ifstream file(fullPath, std::ios::binary | std::ios::ate);

        if (!file.is_open()) {
            spdlog::error("[PhysicalFS] Nie udalo sie otworzyc pliku: {}", fullPath);
            return {};
        }

        std::streamsize size = file.tellg();
        if (size <= 0) {
            spdlog::error("[PhysicalFS] Plik jest pusty lub rozmiar uszkodzony: {}", fullPath);
            return {};
        }

        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer;
        buffer.resize(static_cast<size_t>(size));

        if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            return buffer;
        }

        spdlog::error("[PhysicalFS] Blad odczytu z pliku (pusty?): {}", fullPath);
        return {};
    }

    bool Exists(const std::string& filepath) override {
        std::string fullPath = m_RootDirectory;
        if (!fullPath.empty() && fullPath.back() != '/' && fullPath.back() != '\\') {
            fullPath += '/';
        }
        fullPath += filepath;

        std::ifstream file(fullPath);
        return file.good();
    }
};