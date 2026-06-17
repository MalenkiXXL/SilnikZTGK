#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Core/GridSystem.h"
#include "CookingStation/Scripts/ConveyorBelt/ConveyorScript.h"
#include "CookingStation/Scripts/Managers/GameManagerScript.h"
#include "CookingStation/Events/GameEvents.h"
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <vector>
#include <string>

class DeliveryBoothScript : public ScriptableEntity
{
private:


    bool IsQuestItem(const std::string& tag, const std::string& questDishId)
    {
        if (tag.empty() || questDishId.empty()) return false;

        static const std::unordered_map<std::string, std::vector<std::string>> s_DishTagMap = {
            { "pomidorowa", { "UgotowaneDanie", "GotoweDanie", "TomatoSoup" } },
            { "kanapka",    { "Sandwich" } },
            { "babeczka",   { "Cupcake" } },
            { "caprese",    { "Caprese" } },
            { "kopytka",    { "Gnocchi" } },
        };

        auto it = s_DishTagMap.find(questDishId);
        if (it == s_DishTagMap.end()) return false;

        for (const auto& validTag : it->second) {
            if (tag.find(validTag) != std::string::npos)
                return true;
        }
        return false;
    }

    void SafeDestroy(Entity e, World& world)
    {
        auto* tf = world.GetComponent<TransformComponent>(e);
        if (tf) {
            glm::vec3 pos = tf->GetPosition();
            pos.y -= 50.0f; 
            tf->SetPosition(pos);
        }
        world.GetEventBus().Publish(EntityDestroyRequestEvent{ e });
    }

public:
    glm::vec3 m_DirectionOffset = { 0.0f, 0.0f, 0.0f };
    void OnCreate() override
    {
        auto* transform = GetComponent<TransformComponent>();
        if (transform)
        {
            // 1. Zabezpieczenie na radiany, które uciekło przy merge'u
            float rotationY = glm::degrees(transform->GetRotation().y);

            if (std::abs(rotationY - 90.0f) < 5.0f || std::abs(rotationY - (-270.0f)) < 5.0f)
                m_DirectionOffset = { 2.0f, 0.0f, 0.0f };
            else if (std::abs(rotationY - 270.0f) < 5.0f || std::abs(rotationY - (-90.0f)) < 5.0f)
                m_DirectionOffset = { -2.0f, 0.0f, 0.0f };
            else if (std::abs(rotationY - 180.0f) < 5.0f)
                m_DirectionOffset = { 0.0f, 0.0f, -2.0f };
            else
                m_DirectionOffset = { 0.0f, 0.0f, 2.0f };
        }
    }

    void OnUpdate(Timestep ts) override
    {
        if (!GameManagerScript::s_Instance) return;
        auto* myTransform = GetComponent<TransformComponent>();
        if (!myTransform) return;

        glm::vec3 myCurrentPos = myTransform->GetPosition();
        glm::vec3 m_InputWorldPos = {
            myCurrentPos.x + m_DirectionOffset.x,
            0.0f,
            myCurrentPos.z + m_DirectionOffset.z
        };
        auto* scene = GetScene();
        if (!scene) return;

        auto& world = scene->GetWorld();
        auto* transformStorage = world.GetComponentVector<TransformComponent>();
        if (!transformStorage) return;

        bool isQuestActive = (GameManagerScript::s_Instance->GetQuestState() == QuestEventState::QuestActive);
        QuestData* currentQuest = GameManagerScript::s_Instance->GetCurrentQuest();
        std::string questDishId = (isQuestActive && currentQuest) ? currentQuest->DishID : "";

        std::vector<Entity> entitiesToDestroy;
        bool clearLineTriggered = false;

        for (size_t i = 0; i < transformStorage->dense.size(); ++i)
        {
            Entity entity = transformStorage->reverse[i];
            if (entity.id == m_Entity.id) continue;

            auto* tagComp = world.GetComponent<TagComponent>(entity);
            if (!tagComp) continue;

            std::string tag = tagComp->Tag;

            if (tag.find("tasma") != std::string::npos || tag.find("Conveyor") != std::string::npos ||
                tag.find("budka") != std::string::npos || tag.find("Podloga") != std::string::npos ||
                tag.find("event") != std::string::npos || tag.find("narożnik") != std::string::npos) {
                continue;
            }

            auto& transform = transformStorage->dense[i];
            glm::vec3 globalPos = transform.GetPosition();

            auto* rel = world.GetComponent<RelationshipComponent>(entity);
            if (rel && rel->Parent != std::numeric_limits<std::size_t>::max()) {
                globalPos = glm::vec3(transform.WorldMatrix[3][0], transform.WorldMatrix[3][1], transform.WorldMatrix[3][2]);
            }

            if (glm::distance(glm::vec2(globalPos.x, globalPos.z), glm::vec2(m_InputWorldPos.x, m_InputWorldPos.z)) < 1.2f)
            {
                if (globalPos.y < -10.0f) continue; 

                bool deliveredValidQuestItem = false;

                if (IsQuestItem(tag, questDishId)) {
                    deliveredValidQuestItem = true;
                }

                if (tag.find("Plate") != std::string::npos || tag.find("Talerz") != std::string::npos || tag.find("bowl") != std::string::npos) {
                    if (rel && rel->FirstChild != std::numeric_limits<std::size_t>::max()) {
                        std::size_t childId = rel->FirstChild;
                        while (childId != std::numeric_limits<std::size_t>::max()) {
                            Entity child = { childId, 0 };
                            auto* childTagComp = world.GetComponent<TagComponent>(child);

                            if (childTagComp && IsQuestItem(childTagComp->Tag, questDishId)) {
                                deliveredValidQuestItem = true;
                                break;
                            }

                            auto* childRel = world.GetComponent<RelationshipComponent>(child);
                            childId = childRel ? childRel->NextSibling : std::numeric_limits<std::size_t>::max();
                        }
                    }
                }

                if (isQuestActive && deliveredValidQuestItem) {
                    GameManagerScript::s_Instance->DeliverQuestPortion();
                    spdlog::info("[DeliveryBooth] POMYSLNIE wciagnieto porcje questowa!");
                }

                clearLineTriggered = true;
                entitiesToDestroy.push_back(entity);
            }
        }

        if (clearLineTriggered)
        {
            for (Entity e : entitiesToDestroy) {
                SafeDestroy(e, world);
            }

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
};