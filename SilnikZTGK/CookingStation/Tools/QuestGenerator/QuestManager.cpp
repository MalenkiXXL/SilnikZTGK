#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "CookingStation/Core/VFS/VFS.h"

using json = nlohmann::json;

struct Quest {
    std::string Title;
    std::string Description;
    int Portions;
    int Frequency;
    std::string Reward;
};

class QuestManager {
public:
    static std::vector<Quest> LoadQuests(const std::string& filepath) {
        std::vector<Quest> quests;
        std::vector<uint8_t> fileData = VFS::ReadFile(filepath);
        if (fileData.empty()) {
            spdlog::error("Nie udao sie otworzyc pliku z questami przez VFS: {}", filepath);
            return quests;
        }
        try {
            json data = json::parse(fileData.begin(), fileData.end());
            for (auto& item : data) {
                Quest q;
                q.Title = item["title"].get<std::string>();
                q.Description = item["description"].get<std::string>();
                q.Portions = item["portions"].get<int>();
                q.Frequency = item["frequency"].get<int>();
                q.Reward = item["reward"].get<std::string>();
                quests.push_back(q);
            }
            spdlog::info("Pomyslnie wczytano {} questow!", quests.size());
        }
        catch (json::parse_error& e) {
            spdlog::error("Blad parsowania questow JSON: {}", e.what());
        }

        return quests;
    }
};