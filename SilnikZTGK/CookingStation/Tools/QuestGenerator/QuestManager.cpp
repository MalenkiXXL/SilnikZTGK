#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

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
        std::ifstream file(filepath);

        if (!file.is_open()) {
            spdlog::error("Nie udao sie otworzyc pliku z questami: {}", filepath);
            return quests;
        }

        try {
            json data = json::parse(file);
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