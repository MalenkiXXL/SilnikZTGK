#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include "IFileSystem.h"
#include <spdlog/spdlog.h>

class VFS {
public:
    // Hermetyczna mapa - chroni przed problemami z kompilacj¹ i DLL
    static std::unordered_map<std::string, std::shared_ptr<IFileSystem>>& GetMountPoints() {
        static std::unordered_map<std::string, std::shared_ptr<IFileSystem>> s_MountPoints;
        return s_MountPoints;
    }

    static void Mount(const std::string& virtualPath, std::shared_ptr<IFileSystem> fileSystem) {
        GetMountPoints()[virtualPath] = fileSystem;
        spdlog::info("[VFS] Zamontowano dysk wirtualny: {}", virtualPath);
    }

    static std::vector<uint8_t> ReadFile(const std::string& virtualFilepath) {
        size_t delimiterPos = virtualFilepath.find("://");
        if (delimiterPos == std::string::npos) return {};

        std::string mountPoint = virtualFilepath.substr(0, delimiterPos);
        std::string relativePath = virtualFilepath.substr(delimiterPos + 3);

        // KLUCZOWE: Usuwamy ukoœniki z przodu, by std::filesystem nie oszala³!
        while (!relativePath.empty() && (relativePath.front() == '/' || relativePath.front() == '\\')) {
            relativePath.erase(0, 1);
        }

        auto& mounts = GetMountPoints();
        auto it = mounts.find(mountPoint);
        if (it != mounts.end()) {
            return it->second->ReadFile(relativePath);
        }
        return {};
    }
};