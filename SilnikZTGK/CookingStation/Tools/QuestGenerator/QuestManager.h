#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "CookingStation/Core/VFS/VFS.h"

using json = nlohmann::json;

struct QuestData {
    std::string Title;
    std::string Description;
    std::string DishID;
    int Portions;
    int Frequency;
    int RewardCoins;
    std::string RewardFlag;
};

class QuestManager {
public:
    // Magiczne s³ówko 'inline' rozwi¹zuje b³¹d LNK2001
    static inline std::vector<QuestData> LoadQuests(const std::string& filepath) {
        std::vector<QuestData> quests;
        std::vector<uint8_t> fileData = VFS::ReadFile(filepath);
        if (fileData.empty()) {
            spdlog::error("Nie udao sie otworzyc pliku z questami przez VFS: {}", filepath);
            return quests;
        }
        try {
            json data = json::parse(fileData.begin(), fileData.end());
            for (auto& item : data) {
                QuestData q;
                q.Title = item.value("title", "");
                q.Description = item.value("description", "");
                q.DishID = item.value("dish_id", "");
                q.Portions = item.value("portions", 0);
                q.Frequency = item.value("frequency", 0);
                q.RewardCoins = item.value("reward_coins", 0);
                q.RewardFlag = item.value("reward_flag", "");
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