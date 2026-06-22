#pragma once
#include "CustomerScript.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Scripts/Machines/MachineScript.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Core/Application.h"
#include "CookingStation/Events/GameEvents.h"
#include <glm/glm.hpp>
#include <limits>
#include <cmath>

class HelperCustomerScript : public CustomerScript
{
public:
    float m_YOffset = 0.2f;
    float m_ActionYOffset = 0.4f;
    Entity m_AssignedMachine = { std::numeric_limits<std::size_t>::max(), 0 };
    glm::vec3 m_HighlightColor = glm::vec3(0.2f, 0.8f, 0.2f);
    float m_RotationOffset = -90.0f;
    float m_WaitingRotation = 270.0f;

    bool m_IsFalling = false;
    Entity m_FloorTile = { std::numeric_limits<std::size_t>::max(), 0 };
    float m_FallSpeed = 25.0f;
    float m_CurrentFallY = 14.0f;

    // Zmienne do pulsowania i pierwszego podnoszenia
    bool m_IsWaitingForPickup = false;
    bool m_IsDragged = false;
    float m_DragDelayTimer = 0.0f;
    bool m_IsFirstHelperInstance = false;
    glm::vec3 m_BaseScale = { 1.0f, 1.0f, 1.0f };
    bool m_BaseScaleInitialized = false;
    float m_PulseTimer = 0.0f;
    std::size_t m_ClickSubId = 0;

    std::shared_ptr<Model> m_OriginalModel = nullptr;
    std::shared_ptr<Model> m_ActionModel = nullptr;
    bool m_IsWorking = false;
    std::size_t m_ProcessingSubId = 0;


    void OnCreate() override
    {
        CustomerScript::OnCreate();

        auto* meshComp = GetComponent<MeshComponent>();
        if (meshComp) {
            m_OriginalModel = meshComp->ModelPtr;
        }

        auto* tagComp = GetComponent<TagComponent>();
        if (tagComp) {
            if (tagComp->Tag.find("Marchewka") != std::string::npos) {
                m_HighlightColor = glm::vec3(0.3f, 0.4f, 0.71f);
                m_ActionModel = AssetManager::GetModel("assets://models/animacje/klienci/marchewka-kroi/marchewka-kroi.gltf");
            }
            else if (tagComp->Tag.find("Pomidor") != std::string::npos) {
                m_HighlightColor = glm::vec3(0.94f, 0.31f, 0.47f);
                m_ActionModel = AssetManager::GetModel("assets://models/animacje/klienci/pomidor-kroi/pomidor-kroi.gltf");
            }
            else if (tagComp->Tag.find("Rzodkiewka") != std::string::npos) {
                m_HighlightColor = glm::vec3(0.66f, 0.52f, 0.95f);
                m_RotationOffset = 90.0f;
                m_WaitingRotation = 90.0f;
                m_ActionModel = AssetManager::GetModel("assets://models/animacje/klienci/rzodkiewka-kroi/rzodkiewka-kroi.gltf");
            }
        }

        // --- NAS�UCHIWANIE NA KLIKNI�CIE ---
        m_ClickSubId = GetScene()->GetWorld().GetEventBus().Subscribe<EntityClickedEvent>(
            [this](const EntityClickedEvent& e) {
                // Pozwalamy z�apa� pracownika tylko je�li siedzi w poczekalni (nie spada i nie zosta� ju� u�yty)
                if (e.TargetEntity.id == m_Entity.id && m_IsWaitingForPickup && !m_IsFalling) {
                    m_IsWaitingForPickup = false;
                    m_IsDragged = true;
                    m_DragDelayTimer = 0.0f;

                    if (m_IsWorking) {
                        m_IsWorking = false;
                        SwapModel(false);
                    }

                    // Resetujemy skal� do naturalnej i wy��czamy fioletowy shader
                    auto* myTransform = GetComponent<TransformComponent>();
                    if (myTransform) myTransform->SetScale(m_BaseScale);

                    auto* mesh = GetComponent<MeshComponent>();
                    if (mesh) mesh->ShaderName = "Default";

                    // Wy��czamy niesko�czone �wiat�o w menad�erze
                    if (m_IsFirstHelperInstance) {
                        GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                            m_Entity, glm::vec3(0.0f), 0.01f, false
                            });
                    }

                    // Kasujemy kafelek z pod�ogi
                    if (m_FloorTile.id != std::numeric_limits<std::size_t>::max()) {
                        GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_FloorTile });
                        m_FloorTile = { std::numeric_limits<std::size_t>::max(), 0 };
                    }
                }
            }
        );


        m_ProcessingSubId = GetScene()->GetWorld().GetEventBus().Subscribe<MachineProcessingEvent>(
                [this](const MachineProcessingEvent& e) {
                    if (m_AssignedMachine.id != std::numeric_limits<std::size_t>::max() && e.Machine.id == m_AssignedMachine.id) {

                        bool isCuttingBoard = false;
                        auto* machineTag = GetScene()->GetWorld().GetComponent<TagComponent>(m_AssignedMachine);
                        if (machineTag && (machineTag->Tag.find("CuttingBoard") != std::string::npos ||
                                           machineTag->Tag.find("Deska") != std::string::npos ||
                                           machineTag->Tag.find("deska") != std::string::npos)) {
                            isCuttingBoard = true;
                        }

                        if (e.IsProcessing && isCuttingBoard && !m_IsWorking) {
                            m_IsWorking = true;
                            SwapModel(true);
                            PlayAnimation("Cut");
                        }
                        else if (!e.IsProcessing && m_IsWorking) {
                            m_IsWorking = false;
                            SwapModel(false);
                            PlayAnimation("SitIdle");
                        }
                    }
                }
        );
    }

    void OnDestroy() override
    {
        GetScene()->GetWorld().GetEventBus().Unsubscribe<EntityClickedEvent>(m_ClickSubId);
        GetScene()->GetWorld().GetEventBus().Unsubscribe<MachineProcessingEvent>(m_ProcessingSubId);

        if (m_AssignedMachine.id != std::numeric_limits<std::size_t>::max()) {
            SetMachineAutomated(m_AssignedMachine, false);
        }
        if (m_FloorTile.id != std::numeric_limits<std::size_t>::max()) {
            GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_FloorTile });
        }
        CustomerScript::OnDestroy();
    }

    void ReceiveFood(bool isCorrectOrder = true) override
    {
        IsServed = true;

        if (m_ReceivedFood.id != std::numeric_limits<std::size_t>::max()) {
            GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_ReceivedFood });
            m_ReceivedFood = { std::numeric_limits<std::size_t>::max(), 0 };
        }

        if (isCorrectOrder)
        {
            auto* tag = GetComponent<TagComponent>();
            if (tag) tag->Tag = "NajedzonyPomocnik";

            TeleportToWaitingArea();
        }
        else
        {
            GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_Entity });
        }
    }

    void OnUpdate(Timestep ts) override
    {
        if (!IsServed) return;

        // --- ZMODYFIKOWANY SYSTEM SPADANIA ---
        if (m_IsFalling) {
            auto* myTransform = GetComponent<TransformComponent>();
            auto* tileTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(m_FloorTile);

            if (myTransform && tileTransform) {
                m_CurrentFallY -= m_FallSpeed * ts.GetSeconds();

                if (m_CurrentFallY <= 0.0f) {
                    m_CurrentFallY = 0.0f;
                    m_IsFalling = false;
                }

                glm::vec3 helperPos = myTransform->GetPosition();
                helperPos.y = m_CurrentFallY + 0.25f;
                myTransform->SetPosition(helperPos);

                glm::vec3 tilePos = tileTransform->GetPosition();
                tilePos.y = m_CurrentFallY - 0.05f;
                tileTransform->SetPosition(tilePos);
            }
            return;
        }

        // --- OCZEKIWANIE (PULSOWANIE) ---
        if (m_IsWaitingForPickup) {
            if (m_IsFirstHelperInstance) {
                m_PulseTimer += ts.GetSeconds();
                float wave = std::sin(m_PulseTimer * 4.0f);

                auto* myTransform = GetComponent<TransformComponent>();
                if (myTransform) {
                    myTransform->SetScale(m_BaseScale + glm::vec3(wave * 0.15f));
                }

                auto* mesh = GetComponent<MeshComponent>();
                if (mesh) {
                    float currentOpacity = (wave + 1.0f) * 0.5f * 0.6f;
                    mesh->ShaderName = "HighlightShader";
                    // Fioletowy kolor paczek
                    mesh->HighlightColor = glm::vec4(0.513f, 0.109f, 0.364f, currentOpacity);
                }
            }
            return;
        }

        // --- PRZECIĄGANIE DO MASZYNY ---
        if (m_IsDragged) {
            glm::vec3 mousePos = GetMouseWorldPosition();

            // Matematyczne przyciąganie do siatki
            float snapX = std::round((mousePos.x - 1.0f) / 2.0f) * 2.0f + 1.0f;
            float snapZ = std::round((mousePos.z - 1.0f) / 2.0f) * 2.0f + 1.0f;

            auto* myTransform = GetComponent<TransformComponent>();
            if (myTransform) {
                myTransform->SetPosition({ snapX, m_YOffset, snapZ });
            }

            m_DragDelayTimer += ts.GetSeconds();
            if (m_DragDelayTimer > 0.15f && Input::IsMouseButtonJustPressed(0)) {
                m_IsDragged = false;
            }
            return;
        }
        // -------------------------------------

        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;

        Entity closestMachine = FindAdjacentMachine(transform->GetPosition());

        if (closestMachine.id != m_AssignedMachine.id) {

            if (m_AssignedMachine.id != std::numeric_limits<std::size_t>::max()) {
                SetMachineAutomated(m_AssignedMachine, false);

                m_IsWorking = false;
                SwapModel(false);
                PlayAnimation("SitIdle");
            }

            m_AssignedMachine = closestMachine;

            if (m_AssignedMachine.id != std::numeric_limits<std::size_t>::max()) {
                SetMachineAutomated(m_AssignedMachine, true);
                RotateTowardsMachine(transform, m_AssignedMachine);

                glm::vec3 snapPos = transform->GetPosition();
                snapPos.y = m_YOffset;
                transform->SetPosition(snapPos);

                glm::vec3 highlightColor = m_HighlightColor;

                GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                        m_Entity, highlightColor, 0.8f, false
                });

                GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                        m_AssignedMachine, highlightColor, 0.8f, false
                });

                auto* rels = GetScene()->GetWorld().GetComponentVector<RelationshipComponent>();
                if (rels) {
                    for (size_t i = 0; i < rels->dense.size(); ++i) {
                        if (rels->dense[i].Parent == m_AssignedMachine.id) {
                            Entity childEnt = rels->reverse[i];
                            auto* childTag = GetScene()->GetWorld().GetComponent<TagComponent>(childEnt);

                            bool isItem = false;
                            if (childTag) {
                                if (childTag->Tag.find("Item") != std::string::npos ||
                                    childTag->Tag.find("Ingredient") != std::string::npos) {
                                    isItem = true;
                                }
                            }

                            if (!isItem) {
                                GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                                        childEnt, highlightColor, 0.8f, false
                                });
                            }
                        }
                    }
                }
            }
        }
        else if (m_AssignedMachine.id != std::numeric_limits<std::size_t>::max()) {
            RotateTowardsMachine(transform, m_AssignedMachine);

            float currentTargetY = m_IsWorking ? m_ActionYOffset : m_YOffset;

            glm::vec3 snapPos = transform->GetPosition();
            if (std::abs(snapPos.y - currentTargetY) > 0.001f) {
                snapPos.y = currentTargetY;
                transform->SetPosition(snapPos);
            }
        }
    }

protected:
    void PlayAnimation(const std::string& name)
    {
        auto* animComp = GetComponent<AnimatorComponent>();
        if (animComp && animComp->AnimatorInstance)
        {
            animComp->AnimatorInstance->PlayAnimation(name);
            animComp->IsPlaying = true;
        }
    }

    void SwapModel(bool useActionModel)
    {
        auto* meshComp = GetComponent<MeshComponent>();
        if (!meshComp) return;

        if (useActionModel && m_ActionModel) {
            meshComp->ModelPtr = m_ActionModel;
        }
        else if (!useActionModel && m_OriginalModel) {
            meshComp->ModelPtr = m_OriginalModel;
        }
    }

private:
    void TeleportToWaitingArea()
    {
        float targetZ = 9.0f;
        auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();

        if (tags && transforms) {
            while (true) {
                bool spotTaken = false;
                for (size_t i = 0; i < tags->dense.size(); ++i) {
                    if (tags->dense[i].Tag.find("NajedzonyPomocnik") != std::string::npos) {
                        Entity otherHelper = tags->reverse[i];

                        if (otherHelper.id != m_Entity.id) {
                            auto* otherTf = transforms->Get(otherHelper);
                            if (otherTf) {
                                glm::vec3 otherPos = otherTf->GetPosition();
                                if (std::abs(otherPos.x - 13.0f) < 1.0f && std::abs(otherPos.z - targetZ) < 1.0f) {
                                    spotTaken = true;
                                    break;
                                }
                            }
                        }
                    }
                }

                if (!spotTaken) {
                    break;
                }

                targetZ -= 4.0f;
            }
        }

        glm::vec3 targetPos = { 13.0f, -1.5f, targetZ };

        auto builder = GetScene()->GetWorld().BuildEntity();
        builder.With<TagComponent>({ "HelperFloorTile" });

        m_CurrentFallY = 15.0f;

        TransformComponent tileTf;
        tileTf.SetPosition(targetPos + glm::vec3(0.0f, m_CurrentFallY - 0.05f, 0.0f));
        tileTf.SetScale({ 0.07f, 0.3f, 0.07f });
        builder.With<TransformComponent>(tileTf);

        MeshComponent tileMesh;
        tileMesh.ModelPtr = AssetManager::GetModel("assets://models/wystroj/podloga.gltf");
        builder.With<MeshComponent>(tileMesh);

        m_FloorTile = builder.Build();

        m_IsFalling = true;
        m_IsWaitingForPickup = true;
        m_PulseTimer = 0.0f;

        auto* myTransform = GetComponent<TransformComponent>();
        if (myTransform) {
            if (!m_BaseScaleInitialized) {
                m_BaseScale = myTransform->GetScale();
                m_BaseScaleInitialized = true;
            }
            myTransform->SetPosition(targetPos + glm::vec3(0.0f, m_CurrentFallY, 0.0f));
            myTransform->SetRotation({ 0.0f, m_WaitingRotation, 0.0f });
        }

        static bool s_IsFirstEverHelper = true;
        glm::vec3 purpleColor = glm::vec3(0.513f, 0.109f, 0.364f); // Fioletowy kolor z paczek

        if (s_IsFirstEverHelper) {
            m_IsFirstHelperInstance = true;
            s_IsFirstEverHelper = false;

            GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                m_Entity, purpleColor, 0.0f, true
                });
        }
        else {
            GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                m_Entity, purpleColor, 1.5f, false
                });
        }
    }

    Entity FindAdjacentMachine(glm::vec3 myPos)
    {
        Entity foundMachine = { std::numeric_limits<std::size_t>::max(), 0 };
        float closestDist = 999.0f;

        auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();
        if (!scripts || !transforms) return foundMachine;

        glm::vec2 myPos2D = { myPos.x, myPos.z };

        for (size_t i = 0; i < scripts->dense.size(); ++i) {
            auto& nsc = scripts->dense[i];
            bool isMachine = false;

            for (auto& s : nsc.Scripts) {
                if (s.Name == "PotScript" || s.Name == "CuttingBoardScript" || s.Name == "MixerScript" || s.Name == "OvenScript" || s.Name == "PanScript") {
                    isMachine = true;
                    break;
                }
            }

            if (isMachine) {
                Entity machEnt = scripts->reverse[i];
                auto* machTf = transforms->Get(machEnt);
                if (machTf) {
                    glm::vec2 machPos2D = { machTf->GetPosition().x, machTf->GetPosition().z };
                    float dist = glm::distance(myPos2D, machPos2D);

                    if (dist <= 2.2f && dist < closestDist) {
                        closestDist = dist;
                        foundMachine = machEnt;
                    }
                }
            }
        }
        return foundMachine;
    }

    void SetMachineAutomated(Entity machineEnt, bool state)
    {
        auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(machineEnt);
        if (nsc) {
            for (auto& scriptElement : nsc->Scripts) {
                if (auto* machine = dynamic_cast<MachineScript*>(scriptElement.Instance)) {
                    machine->m_IsAutomated = state;
                    break;
                }
            }
        }
    }

    void RotateTowardsMachine(TransformComponent* myTf, Entity machineEnt)
    {
        auto* machTf = GetScene()->GetWorld().GetComponent<TransformComponent>(machineEnt);
        if (machTf) {
            glm::vec3 dir = machTf->GetPosition() - myTf->GetPosition();
            if (glm::length(dir) > 0.01f) {
                float angle = glm::degrees(std::atan2(dir.x, dir.z));
                myTf->SetRotation({ 0.0f, angle + m_RotationOffset, 0.0f });
            }
        }
    }
};