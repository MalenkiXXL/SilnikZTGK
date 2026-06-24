#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Core/GridSystem.h"
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Scripts/ConveyorBelt/ConveyorScript.h"
#include <spdlog/spdlog.h>
#include <unordered_set>

class BinScript : public ScriptableEntity
{
private:
    enum class BinState { IDLE, OPENING, CLOSING };
    BinState m_State = BinState::IDLE;

    float m_AnimationTimer = 0.0f;
    bool m_IsInitialized = false;
    glm::vec3 m_BinPos = { 0.0f, 0.0f, 0.0f };
    Entity m_PendingItem;

public:
    void OnCreate() override {}

    void OnUpdate(Timestep ts) override
    {
        auto* scene = GetScene();
        if (!scene) return;

        if (!m_IsInitialized)
        {
            auto* myTransform = GetComponent<TransformComponent>();
            if (myTransform) {
                m_BinPos = myTransform->GetPosition();
            }

            auto* animator = GetComponent<TransformAnimatorComponent>();
            if (animator) {
                auto& world = scene->GetWorld();
                auto* myRel = world.GetComponent<RelationshipComponent>(m_Entity);

                if (myRel && myRel->FirstChild != NULL_ENTITY)
                {
                    std::string firstTrackName = "";
                    for (const auto& [clipName, animPtr] : animator->Animations) {
                        if (animPtr && !animPtr->GetTracks().empty()) {
                            firstTrackName = animPtr->GetTracks().begin()->first;
                            break;
                        }
                    }

                    if (!firstTrackName.empty()) {
                        animator->TargetEntities[firstTrackName] = myRel->FirstChild;
                        spdlog::info("[Bin] Podpieto klape pod wezel animacji: '{}'", firstTrackName);
                    }
                }
            }

            auto* conveyor = scene->GetConveyorAt(m_BinPos.x, m_BinPos.z);
            if (conveyor) {
                conveyor->IsJammed = true;
            }

            m_IsInitialized = true;
            return;
        }

        switch (m_State)
        {
            case BinState::IDLE:
            {
                auto* transforms = scene->GetWorld().GetComponentVector<TransformComponent>();
                auto* tags       = scene->GetWorld().GetComponentVector<TagComponent>();
                auto* scripts    = scene->GetWorld().GetComponentVector<NativeScriptComponent>();

                if (!transforms || !tags || !scripts) return;

                for (size_t i = 0; i < transforms->dense.size(); ++i)
                {
                    Entity entity = transforms->reverse[i];
                    if (entity.id == m_Entity.id) continue;

                    glm::vec3 platePos = transforms->dense[i].GetPosition();

                    float dist = glm::distance(glm::vec2(platePos.x, platePos.z), glm::vec2(m_BinPos.x, m_BinPos.z));

                    if (dist < 3.0f)
                    {
                        auto* tagComp = tags->Get(entity);
                        auto* nsc     = scripts->Get(entity);
                        if (!tagComp || !nsc) continue;

                        bool isItem = false;
                        for (auto& script : nsc->Scripts) {
                            if (script.Name == "ItemScript") { isItem = true; break; }
                        }

                        if (isItem && tagComp->Tag != "PlateCarried" && tagComp->Tag != "PendingDestroy")
                        {
                            tagComp->Tag = "PendingDestroy";
                            m_PendingItem = entity;

                            spdlog::info("[Bin] Wykryto zblizajacy sie talerz (odleglosc: {:.2f}). Otwieram!", dist);

                            auto* animator = GetComponent<TransformAnimatorComponent>();
                            if (animator) {
                                animator->IsPlaying     = true;
                                animator->PlaybackSpeed = 1.25f;
                                animator->PlayAnimation("Open", true, false);

                                float duration = 1.0f;
                                if (animator->CurrentAnimation) {
                                    float fullTime = animator->CurrentAnimation->GetDuration();
                                    float tps = animator->CurrentAnimation->GetTicksPerSecond();
                                    duration = (tps > 0.0f) ? (fullTime / tps) : 1.0f;
                                }
                                m_AnimationTimer = duration;
                            }

                            m_State = BinState::OPENING;
                            break;
                        }
                    }
                }
                break;
            }

            case BinState::OPENING:
            {
                m_AnimationTimer -= (float)ts;

                if (m_AnimationTimer <= 0.0f)
                {
                    auto* rels = scene->GetWorld().GetComponentVector<RelationshipComponent>();
                    if (rels) {
                        auto* relComp = rels->Get(m_PendingItem);
                        if (relComp && relComp->FirstChild != NULL_ENTITY) {
                            scene->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{{ relComp->FirstChild, 0 }});
                        }
                    }
                    scene->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_PendingItem });
                    AudioEngine::PlayLoopingSound("assets://sounds/trash.mp3", 0.5f, false);

                    spdlog::info("[Bin] Talerz jest w srodku! Zjedzono, zamykam klape.");

                    auto* animator = GetComponent<TransformAnimatorComponent>();
                    if (animator) {
                        animator->IsPlaying = true;
                        animator->PlayAnimation("Close", true, false);

                        float duration = 1.0f;
                        if (animator->CurrentAnimation) {
                            float fullTime = animator->CurrentAnimation->GetDuration();
                            float tps = animator->CurrentAnimation->GetTicksPerSecond();
                            duration = (tps > 0.0f) ? (fullTime / tps) : 1.0f;
                        }
                        m_AnimationTimer = duration;
                    }
                    m_State = BinState::CLOSING;
                }
                break;
            }

            case BinState::CLOSING:
            {
                auto* conveyor = scene->GetConveyorAt(m_BinPos.x, m_BinPos.z);
                if (conveyor) conveyor->IsOccupied = true;

                m_AnimationTimer -= (float)ts;

                if (m_AnimationTimer <= 0.0f)
                {
                    auto* animator = GetComponent<TransformAnimatorComponent>();
                    if (animator) animator->IsPlaying = false;

                    m_State = BinState::IDLE;

                    if (conveyor) conveyor->IsOccupied = false;

                    spdlog::info("[Bin] Kosz gotowy na kolejne.");
                }
                break;
            }
        }
    }
};