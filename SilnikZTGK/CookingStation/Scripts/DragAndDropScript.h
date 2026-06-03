#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Core/GridSystem.h" // NOWE: Importujemy system siatki!
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Scripts/Machines/MachineScript.h"
#include "CookingStation/Scripts/Machines/PotScript.h"
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Scripts/PlateScript.h"
#include <glm/glm.hpp>
#include <limits>
#include <string>

class DragAndDropScript : public ScriptableEntity
{
public:
    static inline bool IsDragging = false;
    static inline IngredientType CurrentIngredient = IngredientType::None;
    static inline Entity DraggedEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    static inline Scene* ActiveScene = nullptr;

    std::size_t m_DragSubId;
    std::size_t m_HoverSubId;

    static inline Entity HighlightedPotFromPlate = { std::numeric_limits<std::size_t>::max(), 0 };
    static inline Entity HighlightedMachineFromBelt = { std::numeric_limits<std::size_t>::max(), 0 };

    Entity m_Hovered3DEntity = { std::numeric_limits<std::size_t>::max(), 0 };

    void OnCreate() override {
        ActiveScene = GetScene();
        m_DragSubId = GetScene()->GetWorld().GetEventBus().Subscribe<StartDragRequestEvent>(
            [this](const StartDragRequestEvent& e) {
                this->StartDrag(e.Type, e.ModelPath);
            }
        );

        m_HoverSubId = GetScene()->GetWorld().GetEventBus().Subscribe<EntityHoveredEvent>(
            [this](const EntityHoveredEvent& e) {
                m_Hovered3DEntity = e.TargetEntity;
            }
        );
    }

    void OnDestroy() override
    {
        GetScene()->GetWorld().GetEventBus().Unsubscribe<StartDragRequestEvent>(m_DragSubId);
        GetScene()->GetWorld().GetEventBus().Unsubscribe<EntityHoveredEvent>(m_HoverSubId);
    }

    void OnUpdate(Timestep ts) override
    {
        glm::vec3 mousePos = GetMouseWorldPosition();

        if (IsDragging && DraggedEntity.id != std::numeric_limits<std::size_t>::max())
        {
            auto* transform = GetScene()->GetWorld().GetComponent<TransformComponent>(DraggedEntity);
            if (!transform) return;

            transform->SetPosition(mousePos + glm::vec3(0.0f, 0.5f, 0.0f));

            if (Input::IsMouseButtonJustPressed(0))
            {
                TryDropIngredient(transform->GetPosition());
            }
            else if (Input::IsMouseButtonJustPressed(1))
            {
                CancelDrag();
            }
        }
        else if (!IsDragging)
        {
            CheckPlateToPotTransfer(mousePos);
            CheckBeltToMachineTransfer(mousePos);
        }
    }

    static void StartDrag(IngredientType type, const std::string& modelPath)
    {
        if (!ActiveScene) return;
        if (MachineScript::GlobalIsMachineHeld || MachineScript::PendingPickup.id != std::numeric_limits<std::size_t>::max()) {
            return;
        }
        if (IsDragging) CancelDrag();

        IsDragging = true;
        CurrentIngredient = type;

        auto builder = ActiveScene->GetWorld().BuildEntity();
        builder.With<TagComponent>({ "DraggedIngredient" });

        TransformComponent tc;
        tc.SetPosition(glm::vec3(0.0f, -100.0f, 0.0f));

        IngredientMetadata meta = GetIngredientMetadata(type);
        tc.SetScale(meta.scale);
        tc.SetRotation(meta.rotation);

        builder.With<TransformComponent>(tc);

        MeshComponent mesh;
        mesh.ModelPtr = AssetManager::GetModel(modelPath);
        builder.With<MeshComponent>(mesh);

        DraggedEntity = builder.Build();
    }

    static void PickupSpawnedMachine(Entity spawnedMachine)
    {
        if (spawnedMachine.id == std::numeric_limits<std::size_t>::max()) return;

        if (MachineScript::PendingPickup.id != std::numeric_limits<std::size_t>::max() ||
            MachineScript::GlobalIsMachineHeld || IsDragging)
        {
            if (ActiveScene) ActiveScene->DestroyEntity(spawnedMachine);
            return;
        }

        MachineScript::PendingPickup = spawnedMachine;
        spdlog::info("DragAndDrop: Maszyna ustawiona jako PendingPickup, zostanie podniesiona w nastepnym OnUpdate.");
    }

    static void CancelDrag()
    {
        IsDragging = false;
        if (DraggedEntity.id != std::numeric_limits<std::size_t>::max()) {
            ActiveScene->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ DraggedEntity });
            DraggedEntity = { std::numeric_limits<std::size_t>::max(), 0 };
        }
    }

private:

    // ==========================================================
    // SEKCJA: TRANSFER Z TAŚMY DO MASZYNY (Kwadrat 3x3)
    // ==========================================================

    static void SetMachineHighlight(Entity machineEntity, bool state) {
        if (machineEntity.id == std::numeric_limits<std::size_t>::max() || !ActiveScene) return;
        const std::string targetShader = state ? "HighlightShader" : "ModelShader";
        auto* mesh = ActiveScene->GetWorld().GetComponent<MeshComponent>(machineEntity);
        if (mesh) mesh->ShaderName = targetShader;
    }

    static void ClearMachineHighlight() {
        if (HighlightedMachineFromBelt.id != std::numeric_limits<std::size_t>::max()) {
            SetMachineHighlight(HighlightedMachineFromBelt, false);
            HighlightedMachineFromBelt = { std::numeric_limits<std::size_t>::max(), 0 };
        }
    }

    void CheckBeltToMachineTransfer(glm::vec3 mousePos)
    {
        if (MachineScript::GlobalIsMachineHeld) {
            ClearMachineHighlight();
            return;
        }

        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();
        auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        if (!transforms || !scripts) return;

        Entity hoveredBeltItem = { std::numeric_limits<std::size_t>::max(), 0 };
        IngredientType hoveredType = IngredientType::None;

        if (m_Hovered3DEntity.id != std::numeric_limits<std::size_t>::max()) {
            auto* tagComp = GetScene()->GetWorld().GetComponent<TagComponent>(m_Hovered3DEntity);
            if (tagComp && tagComp->Tag.find("BeltItem_") != std::string::npos) {
                hoveredBeltItem = m_Hovered3DEntity;
                int typeId = std::stoi(tagComp->Tag.substr(9));
                hoveredType = static_cast<IngredientType>(typeId);
            }
        }

        if (hoveredBeltItem.id != std::numeric_limits<std::size_t>::max()) {
            Entity closestMachine = { std::numeric_limits<std::size_t>::max(), 0 };
            MachineScript* targetMachineScript = nullptr;
            float closestDist = 999.0f; // Resetujemy do dużej wartości, by szukać wewnątrz kratek

            auto* itemTf = transforms->Get(hoveredBeltItem);
            // ZMIANA: Zczytujemy pozycję na siatce elementu jadącego po taśmie
            glm::ivec2 itemCell = GridSystem::WorldToCell(itemTf->GetPosition());

            for (size_t i = 0; i < scripts->dense.size(); ++i) {
                auto& nsc = scripts->dense[i];
                for (auto& s : nsc.Scripts) {
                    MachineScript* mScript = dynamic_cast<MachineScript*>(s.Instance);
                    if (mScript) {
                        bool canAccept = false;

                        if (s.Name == "CuttingBoardScript") {
                            if (mScript->m_Ingredients.empty() && !mScript->m_IsReady) {
                                canAccept = (hoveredType == IngredientType::Tomato || hoveredType == IngredientType::Baguette ||
                                    hoveredType == IngredientType::Cheese || hoveredType == IngredientType::Ham ||
                                    hoveredType == IngredientType::Mozzarella);
                            }
                        }
                        else if (s.Name == "PotScript") {
                            if (mScript->m_Ingredients.size() < 2 && !mScript->m_IsReady) {
                                canAccept = (hoveredType == IngredientType::ChoppedTomato);
                            }
                        }

                        if (canAccept) {
                            Entity machineEntity = scripts->reverse[i];
                            auto* machineTf = transforms->Get(machineEntity);
                            if (machineTf && itemTf) {
                                // ZMIANA: Zamieniamy maszynę na współrzędne kratek (Grid)
                                glm::ivec2 machineCell = GridSystem::WorldToCell(machineTf->GetPosition());

                                // Ograniczamy do 8 kratek dookoła (odległość w osi X i Y to max 1)
                                if (std::abs(itemCell.x - machineCell.x) <= 1 && std::abs(itemCell.y - machineCell.y) <= 1)
                                {
                                    // Bierzemy najbliższą fizycznie wewnątrz tego obszaru
                                    float dist = glm::distance(itemTf->GetPosition(), machineTf->GetPosition());
                                    if (dist < closestDist) {
                                        closestDist = dist;
                                        closestMachine = machineEntity;
                                        targetMachineScript = mScript;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (closestMachine.id != HighlightedMachineFromBelt.id) {
                ClearMachineHighlight();
                if (closestMachine.id != std::numeric_limits<std::size_t>::max()) {
                    SetMachineHighlight(closestMachine, true);
                }
                HighlightedMachineFromBelt = closestMachine;
            }

            bool isActionPressed = Input::IsMouseButtonJustPressed(0) || (Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0));
            if (isActionPressed && closestMachine.id != std::numeric_limits<std::size_t>::max() && !MachineScript::GlobalIsHoveringUI) {
                if (!Input::IsKeyPressed(340)) {
                    if (targetMachineScript && targetMachineScript->AddIngredient(hoveredType)) {
                        spdlog::info("Składnik z taśmy wskoczył prosto na maszynę!");
                        GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ hoveredBeltItem });
                        ClearMachineHighlight();
                    }
                }
            }
        }
        else {
            ClearMachineHighlight();
        }
    }

    // ==========================================================
    // SEKCJA: TRANSFER Z TALERZA DO GARNKA (Kwadrat 3x3)
    // ==========================================================

    static void SetPotHighlight(Entity potEntity, bool state) {
        SetMachineHighlight(potEntity, state);
    }

    static void ClearPotHighlight() {
        if (HighlightedPotFromPlate.id != std::numeric_limits<std::size_t>::max()) {
            SetPotHighlight(HighlightedPotFromPlate, false);
            HighlightedPotFromPlate = { std::numeric_limits<std::size_t>::max(), 0 };
        }
    }

    void CheckPlateToPotTransfer(glm::vec3 mousePos)
    {
        if (MachineScript::GlobalIsMachineHeld) {
            ClearPotHighlight();
            return;
        }

        auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();
        if (!scripts || !transforms) return;

        glm::vec2 mousePos2D = { mousePos.x, mousePos.z };

        PlateScript* hoveredPlateScript = nullptr;
        Entity currentHoveredPlate = { std::numeric_limits<std::size_t>::max(), 0 };

        for (size_t i = 0; i < scripts->dense.size(); ++i) {
            auto& nsc = scripts->dense[i];
            for (auto& s : nsc.Scripts) {
                if (s.Name == "PlateScript") {
                    Entity plateEntity = scripts->reverse[i];
                    auto* tf = transforms->Get(plateEntity);
                    if (tf && glm::distance(mousePos2D, glm::vec2(tf->GetPosition().x, tf->GetPosition().z)) < 2.0f) {
                        PlateScript* pScript = static_cast<PlateScript*>(s.Instance);
                        if (pScript && pScript->m_CompletedDish == IngredientType::None && !pScript->m_Ingredients.empty()) {
                            hoveredPlateScript = pScript;
                            currentHoveredPlate = plateEntity;
                        }
                    }
                    break;
                }
            }
            if (hoveredPlateScript) break;
        }

        if (hoveredPlateScript) {
            Entity closestPot = { std::numeric_limits<std::size_t>::max(), 0 };
            MachineScript* targetPotScript = nullptr;
            float closestDist = 999.0f; // Reset do dużej wartości

            auto* plateTf = transforms->Get(currentHoveredPlate);
            // ZMIANA: Zczytujemy pozycję na siatce Talerza
            glm::ivec2 plateCell = GridSystem::WorldToCell(plateTf->GetPosition());

            for (size_t i = 0; i < scripts->dense.size(); ++i) {
                auto& nsc = scripts->dense[i];
                for (auto& s : nsc.Scripts) {
                    if (s.Name == "PotScript") {
                        Entity potEntity = scripts->reverse[i];
                        auto* potTf = transforms->Get(potEntity);
                        MachineScript* mScript = static_cast<MachineScript*>(s.Instance);

                        if (potTf && mScript && !mScript->m_IsReady && mScript->m_Ingredients.size() < 2) {

                            // ZMIANA: Zczytujemy pozycję na siatce Garnka
                            glm::ivec2 potCell = GridSystem::WorldToCell(potTf->GetPosition());

                            // Ograniczamy do 8 kratek dookoła
                            if (std::abs(plateCell.x - potCell.x) <= 1 && std::abs(plateCell.y - potCell.y) <= 1)
                            {
                                float dist = glm::distance(plateTf->GetPosition(), potTf->GetPosition());
                                if (dist < closestDist) {
                                    closestDist = dist;
                                    closestPot = potEntity;
                                    targetPotScript = mScript;
                                }
                            }
                        }
                        break;
                    }
                }
            }

            if (closestPot.id != HighlightedPotFromPlate.id) {
                ClearPotHighlight();
                if (closestPot.id != std::numeric_limits<std::size_t>::max()) {
                    SetPotHighlight(closestPot, true);
                }
                HighlightedPotFromPlate = closestPot;
            }

            bool isActionPressed = Input::IsMouseButtonJustPressed(0) || (Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0));
            if (isActionPressed && closestPot.id != std::numeric_limits<std::size_t>::max() && !MachineScript::GlobalIsHoveringUI) {
                if (!Input::IsKeyPressed(340))
                {
                    IngredientType topIngredient = hoveredPlateScript->m_Ingredients.back();
                    if (targetPotScript && targetPotScript->AddIngredient(topIngredient)) {
                        spdlog::info("Składnik wrzucony z talerza z powrotem do garnka!");
                        hoveredPlateScript->m_Ingredients.pop_back();
                        Entity visualToRemove = hoveredPlateScript->m_VisualModels.back();
                        GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ visualToRemove });
                        hoveredPlateScript->m_VisualModels.pop_back();
                        ClearPotHighlight();
                    }
                    else {
                        spdlog::warn("Garnek nie potrafi ugotować składnika, który chcesz w nim umieścić!");
                    }
                }
            }
        }
        else {
            ClearPotHighlight();
        }
    }

    void TryDropIngredient(glm::vec3 dropPos)
    {
        Entity closestMachine = { std::numeric_limits<std::size_t>::max(), 0 };
        float closestDist = 2.0f;

        MachineScript* targetMachineScript = nullptr;
        PlateScript* targetPlateScript = nullptr;

        auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();

        if (scripts && transforms) {
            for (size_t i = 0; i < scripts->dense.size(); ++i) {
                auto& nsc = scripts->dense[i];

                for (auto& scriptElement : nsc.Scripts) {
                    Entity entity = scripts->reverse[i];
                    auto* transform = transforms->Get(entity);
                    if (!transform) continue;

                    float dist = glm::distance(dropPos, transform->GetPosition());
                    if (dist < closestDist) {
                        if (scriptElement.Name == "PotScript" || scriptElement.Name == "CuttingBoardScript" ||
                            scriptElement.Name == "MixerScript" || scriptElement.Name == "OvenScript")
                        {
                            closestDist = dist;
                            closestMachine = entity;
                            targetMachineScript = dynamic_cast<MachineScript*>(scriptElement.Instance);
                            targetPlateScript = nullptr;
                        }
                        else if (scriptElement.Name == "PlateScript")
                        {
                            closestDist = dist;
                            closestMachine = entity;
                            targetPlateScript = dynamic_cast<PlateScript*>(scriptElement.Instance);
                            targetMachineScript = nullptr;
                        }
                    }
                }
            }
        }

        if (closestMachine.id != std::numeric_limits<std::size_t>::max()) {
            if (targetPlateScript && targetPlateScript->AddIngredient(CurrentIngredient)) {
                spdlog::info("Złożono składnik na talerzu!");
                GetScene()->GetWorld().GetEventBus().Publish(IngredientUsedEvent{ CurrentIngredient, 1 });
                CancelDrag();
            }
            else if (targetMachineScript && targetMachineScript->AddIngredient(CurrentIngredient)) {
                spdlog::info("Wrzucono składnik do maszyny!");
                GetScene()->GetWorld().GetEventBus().Publish(IngredientUsedEvent{ CurrentIngredient, 1 });
                CancelDrag();
            }
        }
    }
};