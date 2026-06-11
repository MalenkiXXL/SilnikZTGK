#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Core/GridSystem.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Core/VFS/VFS.h"
#include "CookingStation/Scripts/ConveyorBelt/ConveyorScript.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <vector>
#include <string>
#include <limits>

struct DynamicQuest {
    std::string Title = "Brak zadania";
    std::string Description = "Oczekiwanie na zamówienia...";
    std::string DishId = "pomidorowa";
    int PortionsRequired = 0;
    int PortionsDelivered = 0;
    std::string Reward = "Brak";
    bool IsActive = false;
};

class DeliveryBoothScript : public ScriptableEntity
{
public:
    static inline DeliveryBoothScript* s_Instance = nullptr;
    Entity GetEntity() const { return m_Entity; }

private:
    glm::ivec2 m_BoothCell = { 0, 0 };
    glm::ivec2 m_InputCell = { 0, 0 };
    glm::vec3 m_InputWorldPos = { 0.0f, 0.0f, 0.0f };
    DynamicQuest m_ActiveQuest;
    std::vector<DynamicQuest> m_AllQuests;

    std::string MapPythonDishIdToEngineTag(const std::string& pythonId)
    {
        if (pythonId == "pomidorowa")    return "TomatoSoup";
        if (pythonId == "kanapka")       return "Sandwich";
        if (pythonId == "babeczka")      return "Cupcake";
        if (pythonId == "caprese")       return "Caprese";
        if (pythonId == "kopytka")       return "Gnocchi";
        if (pythonId == "kopytka-zlote") return "GoldenGnocchi";
        if (pythonId == "kawa")          return "Coffee";
        if (pythonId == "kawa-mleko")    return "CoffeeMilk";
        return pythonId;
    }

public:
    void OnCreate() override
    {
        s_Instance = this;
        auto* transform = GetComponent<TransformComponent>();
        if (transform)
        {
            glm::vec3 pos = transform->GetPosition();
            m_BoothCell = GridSystem::WorldToCell(pos);

            float rotationY = transform->GetRotation().y;
            glm::ivec2 direction = { -1, 0 };

            if (std::abs(rotationY - 90.0f) < 5.0f || std::abs(rotationY - (-270.0f)) < 5.0f)
                direction = { -2, 0 };
            else if (std::abs(rotationY - 270.0f) < 5.0f || std::abs(rotationY - (-90.0f)) < 5.0f)
                direction = { 2, 0 };
            else if (std::abs(rotationY - 180.0f) < 5.0f)
                direction = { 0, 2 };
            else
                direction = { 0, -2 };

            m_InputWorldPos = pos + glm::vec3(direction.x, 0.0f, direction.y);
            m_InputCell = GridSystem::WorldToCell(m_InputWorldPos);

            ReloadGenerativeQuest();
        }
    }

    void OnUpdate(Timestep ts) override
    {
        auto* scene = GetScene();
        if (!scene) return;

        auto& world = scene->GetWorld();
        auto* transformStorage = world.GetComponentVector<TransformComponent>();
        auto* tagStorage = world.GetComponentVector<TagComponent>();
        if (!transformStorage || !tagStorage) return;

        std::vector<Entity> entitiesToDestroy;
        bool correctDishDelivered = false;
        bool clearLineTriggered = false;

        std::string requiredEngineTag = MapPythonDishIdToEngineTag(m_ActiveQuest.DishId);

        for (size_t i = 0; i < transformStorage->dense.size(); ++i)
        {
            Entity entity = transformStorage->reverse[i];
            if (entity.id == m_Entity.id) continue;

            auto& transform = transformStorage->dense[i];
            glm::vec3 globalPos = glm::vec3(transform.WorldMatrix[3][0], transform.WorldMatrix[3][1], transform.WorldMatrix[3][2]);

            if (GridSystem::WorldToCell(globalPos) == m_InputCell)
            {
                if (entity.id >= tagStorage->sparse.size()) continue;
                std::size_t index = tagStorage->sparse[entity.id];
                if (index == std::numeric_limits<std::size_t>::max()) continue;
                if (tagStorage->reverse[index].generation != entity.generation) continue;

                auto* tagComp = world.GetComponent<TagComponent>(entity);
                if (!tagComp) continue;

                std::string tag = tagComp->Tag;

                if (tag == "Plate" || tag == "Talerz" || tag == "UgotowaneDanie" || tag == "TomatoSoup" || tag == requiredEngineTag)
                {
                    // POPRAWKA LOGIKI: Akceptujemy potrawê jeœli ma tag dedykowany LUB uniwersalny silnikowy "UgotowaneDanie"
                    if (m_ActiveQuest.IsActive && (tag == requiredEngineTag || tag == "UgotowaneDanie"))
                    {
                        correctDishDelivered = true;
                    }
                    clearLineTriggered = true;
                    entitiesToDestroy.push_back(entity);
                }
            }
        }

        for (Entity entityToKill : entitiesToDestroy)
        {
            if (world.GetComponent<TagComponent>(entityToKill))
            {
                world.DestroyEntity(entityToKill);
            }
        }

        if (clearLineTriggered)
        {
            if (correctDishDelivered && m_ActiveQuest.IsActive)
            {
                m_ActiveQuest.PortionsDelivered++;
                spdlog::info("[PRODUKCJA] Zupa zaakceptowana! Aktualny stan porcji: {}/{}", m_ActiveQuest.PortionsDelivered, m_ActiveQuest.PortionsRequired);

                if (m_ActiveQuest.PortionsDelivered >= m_ActiveQuest.PortionsRequired)
                {
                    spdlog::info("[PRODUKCJA] Quest ukoñczony pomyœlnie!");
                    m_ActiveQuest.IsActive = false;
                }
            }

            // PANCERNY RESET TAŒM I ZWROTNIC:
            // Skanujemy kafel wejœciowy oraz ca³y obszar krzy¿owy wokó³ niego (+/- 2.0 jednostki)
            // Zapobiega to utkniêciu logiki mechanizmu zwrotnic i rozjazdów.
            for (float offsetX = -2.0f; offsetX <= 2.0f; offsetX += 2.0f) {
                for (float offsetZ = -2.0f; offsetZ <= 2.0f; offsetZ += 2.0f) {
                    ConveyorScript* conv = scene->GetConveyorAt(m_InputWorldPos.x + offsetX, m_InputWorldPos.z + offsetZ);
                    if (conv) {
                        conv->IsOccupied = false;
                        conv->IsJammed = false;
                    }
                }
            }
        }
    }


    void ReloadGenerativeQuest()
    {
        m_AllQuests.clear();
        m_ActiveQuest = DynamicQuest();

        std::vector<uint8_t> fileData = VFS::ReadFile("assets://wygenerowane_quests.json");
        if (fileData.empty()) return;

        try
        {
            nlohmann::json data = nlohmann::json::parse(
                fileData.begin(), fileData.end());

            // Obs³ugujemy zarówno tablicê jak i pojedynczy obiekt
            if (data.is_object()) data = nlohmann::json::array({ data });
            if (!data.is_array() || data.empty()) return;

            for (auto& item : data)
            {
                DynamicQuest q;
                q.Title = item.value("title", "Zamowienie AI");
                q.Description = item.value("description", "Brak opisu.");
                q.DishId = item.value("dish_id", "pomidorowa");
                q.PortionsRequired = item.value("portions", 5);
                q.PortionsDelivered = 0;
                q.Reward = item.value("reward_coins", 0) > 0
                    ? std::to_string(item.value("reward_coins", 0)) + " monet"
                    : item.value("reward", "Monety");
                q.IsActive = true;
                m_AllQuests.push_back(q);
            }

            if (!m_AllQuests.empty())
                m_ActiveQuest = m_AllQuests[0];
        }
        catch (...) {}
    }

    // Losuje jeden z wczytanych questów (nie prze³adowuje pliku)
    void ShuffleToRandomQuest()
    {
        if (m_AllQuests.size() <= 1) return;

        // Zbieramy indeksy inne ni¿ aktywny (po tytule)
        std::vector<int> candidates;
        for (int i = 0; i < (int)m_AllQuests.size(); ++i)
        {
            if (m_AllQuests[i].Title != m_ActiveQuest.Title)
                candidates.push_back(i);
        }
        if (candidates.empty()) return;

        int pick = candidates[rand() % candidates.size()];
        m_ActiveQuest = m_AllQuests[pick];
        m_ActiveQuest.PortionsDelivered = 0;
        m_ActiveQuest.IsActive = true;

        spdlog::info("[Quest] Przelaczono na: {}", m_ActiveQuest.Title);
    }

    bool HasActiveQuest() const { return m_ActiveQuest.IsActive; }
    const DynamicQuest* GetActiveQuest() const { return &m_ActiveQuest; }
};