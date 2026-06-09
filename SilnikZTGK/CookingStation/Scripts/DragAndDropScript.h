#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Core/GridSystem.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Scripts/Machines/MachineScript.h"
#include "CookingStation/Scripts/Machines/PotScript.h"
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Scripts/PlateScript.h"
#include <glm/glm.hpp>
#include <limits>
#include <string>
#include <functional> 
#include <algorithm>

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
    static inline Entity HighlightedMachineForPlate = { std::numeric_limits<std::size_t>::max(), 0 };
    static inline Entity HighlightedBeltItemForMachine = { std::numeric_limits<std::size_t>::max(), 0 };
    static inline Entity HighlightedPlateForMachine = { std::numeric_limits<std::size_t>::max(), 0 };

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
            CheckPlatePullFromMachine(mousePos);
            CheckMachinePullFromBelt(mousePos);
            CheckMachinePullFromPlate(mousePos);
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

    // SEKCJA HELPERÓW
    struct NeighborResult {
        Entity TargetEntity = { std::numeric_limits<std::size_t>::max(), 0 };
        MachineScript* MachineInstance = nullptr;
        PlateScript* PlateInstance = nullptr;
    };

    PlateScript* FindHoveredPlate(const glm::vec3& mousePos, Entity& outEntity) {
        auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();
        if (!scripts || !transforms) return nullptr;

        glm::vec2 mouse2D = { mousePos.x, mousePos.z };
        for (size_t i = 0; i < scripts->dense.size(); ++i) {
            for (auto& s : scripts->dense[i].Scripts) {
                if (s.Name == "PlateScript") {
                    Entity entity = scripts->reverse[i];
                    auto* tf = transforms->Get(entity);
                    if (tf && glm::distance(mouse2D, glm::vec2(tf->GetPosition().x, tf->GetPosition().z)) < 2.0f) {
                        outEntity = entity;
                        return static_cast<PlateScript*>(s.Instance);
                    }
                    break;
                }
            }
        }
        return nullptr;
    }

    NeighborResult FindClosestNeighbor(const glm::vec3& sourcePos, std::function<bool(const std::string&, ScriptableEntity*)> filter) {
        NeighborResult result;
        auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();
        if (!scripts || !transforms) return result;

        float closestDist = 999.0f;
        glm::ivec2 sourceCell = GridSystem::WorldToCell(sourcePos);

        for (size_t i = 0; i < scripts->dense.size(); ++i) {
            for (auto& s : scripts->dense[i].Scripts) {
                if (filter(s.Name, s.Instance)) {
                    Entity currentEntity = scripts->reverse[i];
                    auto* currentTf = transforms->Get(currentEntity);
                    if (currentTf) {
                        glm::ivec2 currentCell = GridSystem::WorldToCell(currentTf->GetPosition());
                        if (std::abs(sourceCell.x - currentCell.x) <= 1 && std::abs(sourceCell.y - currentCell.y) <= 1) {
                            float dist = glm::distance(sourcePos, currentTf->GetPosition());
                            if (dist < closestDist) {
                                closestDist = dist;
                                result.TargetEntity = currentEntity;
                                result.MachineInstance = dynamic_cast<MachineScript*>(s.Instance);
                                result.PlateInstance = dynamic_cast<PlateScript*>(s.Instance);
                            }
                        }
                    }
                    break;
                }
            }
        }
        return result;
    }

    // SEKCJA HIGHLIGHTÓW
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

    static void SetPotHighlight(Entity potEntity, bool state) {
        SetMachineHighlight(potEntity, state);
    }

    static void ClearPotHighlight() {
        if (HighlightedPotFromPlate.id != std::numeric_limits<std::size_t>::max()) {
            SetPotHighlight(HighlightedPotFromPlate, false);
            HighlightedPotFromPlate = { std::numeric_limits<std::size_t>::max(), 0 };
        }
    }

    static void ClearMachineForPlateHighlight() {
        if (HighlightedMachineForPlate.id != std::numeric_limits<std::size_t>::max()) {
            SetMachineHighlight(HighlightedMachineForPlate, false);
            HighlightedMachineForPlate = { std::numeric_limits<std::size_t>::max(), 0 };
        }
    }

    static void SetBeltItemHighlight(Entity entity, bool state) {
        if (entity.id == std::numeric_limits<std::size_t>::max() || !ActiveScene) return;
        auto* mesh = ActiveScene->GetWorld().GetComponent<MeshComponent>(entity);
        if (mesh) mesh->ShaderName = state ? "HighlightShader" : "ModelShader";
    }

    static void ClearBeltItemHighlight() {
        if (HighlightedBeltItemForMachine.id != std::numeric_limits<std::size_t>::max()) {
            SetBeltItemHighlight(HighlightedBeltItemForMachine, false);
            HighlightedBeltItemForMachine = { std::numeric_limits<std::size_t>::max(), 0 };
        }
    }

    static void ClearPlateForMachineHighlight() {
        if (HighlightedPlateForMachine.id != std::numeric_limits<std::size_t>::max() && ActiveScene) {
            auto* nsc = ActiveScene->GetWorld().GetComponent<NativeScriptComponent>(HighlightedPlateForMachine);
            if (nsc) {
                for (auto& s : nsc->Scripts) {
                    if (s.Name == "PlateScript" && s.Instance) {
                        static_cast<PlateScript*>(s.Instance)->SetHighlight(false);
                        break;
                    }
                }
            }
            HighlightedPlateForMachine = { std::numeric_limits<std::size_t>::max(), 0 };
        }
    }

    // SEKCJA INTERAKCJI 
    void CheckBeltToMachineTransfer(glm::vec3 mousePos)
    {
        if (MachineScript::GlobalIsMachineHeld) { ClearMachineHighlight(); return; }

        Entity hoveredBeltItem = { std::numeric_limits<std::size_t>::max(), 0 };
        IngredientType hoveredType = IngredientType::None;

        if (m_Hovered3DEntity.id != std::numeric_limits<std::size_t>::max()) {
            auto* tagComp = GetScene()->GetWorld().GetComponent<TagComponent>(m_Hovered3DEntity);
            if (tagComp && tagComp->Tag.find("BeltItem_") != std::string::npos) {
                hoveredBeltItem = m_Hovered3DEntity;
                hoveredType = static_cast<IngredientType>(std::stoi(tagComp->Tag.substr(9)));
            }
        }

        if (hoveredBeltItem.id != std::numeric_limits<std::size_t>::max()) {
            auto* itemTf = GetScene()->GetWorld().GetComponent<TransformComponent>(hoveredBeltItem);
            if (!itemTf) return;

            auto neighbor = FindClosestNeighbor(itemTf->GetPosition(), [hoveredType](const std::string& name, ScriptableEntity* instance) {
                MachineScript* mScript = dynamic_cast<MachineScript*>(instance);
                if (!mScript || mScript->m_IsReady) return false;

                if (name == "CuttingBoardScript") {
                    return mScript->m_Ingredients.empty() &&
                        (hoveredType == IngredientType::Tomato || hoveredType == IngredientType::Baguette ||
                            hoveredType == IngredientType::Cheese || hoveredType == IngredientType::Ham || hoveredType == IngredientType::Mozzarella);
                }
                else if (name == "PotScript") {
                    return mScript->m_Ingredients.size() < 2 && (hoveredType == IngredientType::ChoppedTomato);
                }
                else if (name == "MixerScript") {
                    bool hasType = std::find(mScript->m_Ingredients.begin(), mScript->m_Ingredients.end(), hoveredType) != mScript->m_Ingredients.end();
                    return mScript->m_Ingredients.size() < 2 && !hasType && (hoveredType == IngredientType::Flour || hoveredType == IngredientType::Milk);
                }
                else if (name == "OvenScript") {
                    return mScript->m_Ingredients.empty() && (hoveredType == IngredientType::RawDough);
                }
                return false;
                });

            if (neighbor.TargetEntity.id != HighlightedMachineFromBelt.id) {
                ClearMachineHighlight();
                if (neighbor.TargetEntity.id != std::numeric_limits<std::size_t>::max()) SetMachineHighlight(neighbor.TargetEntity, true);
                HighlightedMachineFromBelt = neighbor.TargetEntity;
            }

            bool isActionPressed = Input::IsMouseButtonJustPressed(0) || (Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0));
            if (isActionPressed && neighbor.TargetEntity.id != std::numeric_limits<std::size_t>::max() && !Input::IsUICapturingMouse()) {
                if (!Input::IsKeyPressed(340) && neighbor.MachineInstance && neighbor.MachineInstance->AddIngredient(hoveredType)) {
                    spdlog::info("Składnik z taśmy wskoczył prosto na maszynę!");
                    GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ hoveredBeltItem });
                    ClearMachineHighlight();
                }
            }
        }
        else {
            ClearMachineHighlight();
        }
    }

    void CheckMachinePullFromBelt(glm::vec3 mousePos)
    {
        if (MachineScript::GlobalIsMachineHeld) {
            ClearBeltItemHighlight();
            return;
        }

        Entity hoveredMachineEntity = { std::numeric_limits<std::size_t>::max(), 0 };
        MachineScript* hoveredMachineScript = nullptr;
        std::string machineName = "";

        if (m_Hovered3DEntity.id != std::numeric_limits<std::size_t>::max()) {
            auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Hovered3DEntity);
            if (nsc) {
                for (auto& s : nsc->Scripts) {
                    if (s.Name == "CuttingBoardScript" || s.Name == "PotScript" || s.Name == "MixerScript" || s.Name == "OvenScript") {
                        hoveredMachineEntity = m_Hovered3DEntity;
                        hoveredMachineScript = dynamic_cast<MachineScript*>(s.Instance);
                        machineName = s.Name;
                        break;
                    }
                }
            }
        }

        if (hoveredMachineScript && !hoveredMachineScript->m_IsReady) {
            Entity closestBeltItem = { std::numeric_limits<std::size_t>::max(), 0 };
            float closestDist = 999.0f;
            IngredientType foundType = IngredientType::None;

            auto* machineTf = GetScene()->GetWorld().GetComponent<TransformComponent>(hoveredMachineEntity);
            auto* tagSet = GetScene()->GetWorld().GetComponentVector<TagComponent>();
            auto* tfSet = GetScene()->GetWorld().GetComponentVector<TransformComponent>();

            if (machineTf && tagSet && tfSet) {
                glm::ivec2 machineCell = GridSystem::WorldToCell(machineTf->GetPosition());

                for (size_t i = 0; i < tagSet->dense.size(); ++i) {
                    if (tagSet->dense[i].Tag.find("BeltItem_") != std::string::npos) {
                        Entity beltEntity = tagSet->reverse[i];
                        auto* itemTf = tfSet->Get(beltEntity);
                        if (itemTf) {
                            glm::ivec2 itemCell = GridSystem::WorldToCell(itemTf->GetPosition());
                            if (std::abs(machineCell.x - itemCell.x) <= 1 && std::abs(machineCell.y - itemCell.y) <= 1) {
                                int typeId = std::stoi(tagSet->dense[i].Tag.substr(9));
                                IngredientType type = static_cast<IngredientType>(typeId);

                                bool canAccept = false;
                                if (machineName == "CuttingBoardScript" && hoveredMachineScript->m_Ingredients.empty()) {
                                    canAccept = (type == IngredientType::Tomato || type == IngredientType::Baguette || type == IngredientType::Cheese || type == IngredientType::Ham || type == IngredientType::Mozzarella);
                                }
                                else if (machineName == "PotScript" && hoveredMachineScript->m_Ingredients.size() < 2) {
                                    canAccept = (type == IngredientType::ChoppedTomato);
                                }
                                else if (machineName == "MixerScript" && hoveredMachineScript->m_Ingredients.size() < 2) {
                                    bool hasType = std::find(hoveredMachineScript->m_Ingredients.begin(), hoveredMachineScript->m_Ingredients.end(), type) != hoveredMachineScript->m_Ingredients.end();
                                    canAccept = (!hasType && (type == IngredientType::Flour || type == IngredientType::Milk));
                                }
                                else if (machineName == "OvenScript" && hoveredMachineScript->m_Ingredients.empty()) {
                                    canAccept = (type == IngredientType::RawDough);
                                }

                                if (canAccept) {
                                    float dist = glm::distance(machineTf->GetPosition(), itemTf->GetPosition());
                                    if (dist < closestDist) {
                                        closestDist = dist;
                                        closestBeltItem = beltEntity;
                                        foundType = type;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (closestBeltItem.id != HighlightedBeltItemForMachine.id) {
                ClearBeltItemHighlight();
                if (closestBeltItem.id != std::numeric_limits<std::size_t>::max()) {
                    SetBeltItemHighlight(closestBeltItem, true);
                }
                HighlightedBeltItemForMachine = closestBeltItem;
            }

            bool isActionPressed = Input::IsMouseButtonJustPressed(0) || (Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0));
            if (isActionPressed && closestBeltItem.id != std::numeric_limits<std::size_t>::max() && !Input::IsUICapturingMouse()) {
                if (!Input::IsKeyPressed(340)) {
                    if (hoveredMachineScript->AddIngredient(foundType)) {
                        spdlog::info("Maszyna zassała składnik z taśmy obok!");
                        GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ closestBeltItem });
                        ClearBeltItemHighlight();
                    }
                }
            }
        }
        else {
            ClearBeltItemHighlight();
        }
    }

    void CheckPlatePullFromMachine(glm::vec3 mousePos)
    {
        if (MachineScript::GlobalIsMachineHeld) { ClearMachineForPlateHighlight(); return; }

        Entity currentHoveredPlate = { std::numeric_limits<std::size_t>::max(), 0 };
        PlateScript* hoveredPlateScript = FindHoveredPlate(mousePos, currentHoveredPlate);

        if (hoveredPlateScript && hoveredPlateScript->m_CompletedDish == IngredientType::None && hoveredPlateScript->m_Ingredients.size() < 5) {
            auto* plateTf = GetScene()->GetWorld().GetComponent<TransformComponent>(currentHoveredPlate);
            if (!plateTf) return;

            auto neighbor = FindClosestNeighbor(plateTf->GetPosition(), [](const std::string& name, ScriptableEntity* instance) {
                MachineScript* mScript = dynamic_cast<MachineScript*>(instance);
                return mScript && mScript->m_IsReady;
                });

            if (neighbor.TargetEntity.id != HighlightedMachineForPlate.id) {
                ClearMachineForPlateHighlight();
                if (neighbor.TargetEntity.id != std::numeric_limits<std::size_t>::max()) SetMachineHighlight(neighbor.TargetEntity, true);
                HighlightedMachineForPlate = neighbor.TargetEntity;
            }

            bool isActionPressed = Input::IsMouseButtonJustPressed(0) || (Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0));
            if (isActionPressed && neighbor.TargetEntity.id != std::numeric_limits<std::size_t>::max() && !Input::IsUICapturingMouse()) {
                if (!Input::IsKeyPressed(340) && neighbor.MachineInstance) {
                    neighbor.MachineInstance->TryTransferToPlate();
                    ClearMachineForPlateHighlight();
                }
            }
        }
        else {
            ClearMachineForPlateHighlight();
        }
    }

    void CheckMachinePullFromPlate(glm::vec3 mousePos)
    {
        if (MachineScript::GlobalIsMachineHeld) { ClearPlateForMachineHighlight(); return; }

        Entity hoveredMachineEntity = { std::numeric_limits<std::size_t>::max(), 0 };
        MachineScript* hoveredMachineScript = nullptr;
        std::string machineName = "";

        if (m_Hovered3DEntity.id != std::numeric_limits<std::size_t>::max()) {
            auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Hovered3DEntity);
            if (nsc) {
                for (auto& s : nsc->Scripts) {
                    if (s.Name == "PotScript" || s.Name == "CuttingBoardScript" || s.Name == "MixerScript" || s.Name == "OvenScript") {
                        hoveredMachineEntity = m_Hovered3DEntity;
                        hoveredMachineScript = dynamic_cast<MachineScript*>(s.Instance);
                        machineName = s.Name;
                        break;
                    }
                }
            }
        }

        if (hoveredMachineScript && !hoveredMachineScript->m_IsReady) {
            auto* machineTf = GetScene()->GetWorld().GetComponent<TransformComponent>(hoveredMachineEntity);
            if (!machineTf) return;

            auto neighbor = FindClosestNeighbor(machineTf->GetPosition(), [&](const std::string& name, ScriptableEntity* instance) {
                if (name != "PlateScript") return false;
                PlateScript* pScript = static_cast<PlateScript*>(instance);
                if (!pScript || pScript->m_CompletedDish != IngredientType::None || pScript->m_Ingredients.empty()) return false;

                IngredientType topIngredient = pScript->m_Ingredients.back();
                if (machineName == "PotScript" && hoveredMachineScript->m_Ingredients.size() < 2) {
                    return topIngredient == IngredientType::ChoppedTomato;
                }
                else if (machineName == "CuttingBoardScript" && hoveredMachineScript->m_Ingredients.empty()) {
                    return topIngredient == IngredientType::Tomato || topIngredient == IngredientType::Baguette ||
                        topIngredient == IngredientType::Cheese || topIngredient == IngredientType::Ham || topIngredient == IngredientType::Mozzarella;
                }
                else if (machineName == "MixerScript" && hoveredMachineScript->m_Ingredients.size() < 2) {
                    bool hasType = std::find(hoveredMachineScript->m_Ingredients.begin(), hoveredMachineScript->m_Ingredients.end(), topIngredient) != hoveredMachineScript->m_Ingredients.end();
                    return (!hasType && (topIngredient == IngredientType::Flour || topIngredient == IngredientType::Milk));
                }
                else if (machineName == "OvenScript" && hoveredMachineScript->m_Ingredients.empty()) {
                    return topIngredient == IngredientType::RawDough;
                }
                return false;
                });

            if (neighbor.TargetEntity.id != HighlightedPlateForMachine.id) {
                ClearPlateForMachineHighlight();
                if (neighbor.TargetEntity.id != std::numeric_limits<std::size_t>::max() && neighbor.PlateInstance) {
                    neighbor.PlateInstance->SetHighlight(true);
                }
                HighlightedPlateForMachine = neighbor.TargetEntity;
            }

            bool isActionPressed = Input::IsMouseButtonJustPressed(0) || (Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0));
            if (isActionPressed && neighbor.TargetEntity.id != std::numeric_limits<std::size_t>::max() && !Input::IsUICapturingMouse()) {
                if (!Input::IsKeyPressed(340) && neighbor.PlateInstance) {
                    IngredientType topIngredient = neighbor.PlateInstance->m_Ingredients.back();
                    if (hoveredMachineScript->AddIngredient(topIngredient)) {
                        spdlog::info("Maszyna zassała składnik ze stojącego obok talerza!");
                        neighbor.PlateInstance->m_Ingredients.pop_back();
                        Entity visualToRemove = neighbor.PlateInstance->m_VisualModels.back();
                        GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ visualToRemove });
                        neighbor.PlateInstance->m_VisualModels.pop_back();
                        ClearPlateForMachineHighlight();
                    }
                }
            }
        }
        else {
            ClearPlateForMachineHighlight();
        }
    }

    void CheckPlateToPotTransfer(glm::vec3 mousePos)
    {
        if (MachineScript::GlobalIsMachineHeld) { ClearPotHighlight(); return; }

        Entity currentHoveredPlate = { std::numeric_limits<std::size_t>::max(), 0 };
        PlateScript* hoveredPlateScript = FindHoveredPlate(mousePos, currentHoveredPlate);

        if (hoveredPlateScript && hoveredPlateScript->m_CompletedDish == IngredientType::None && !hoveredPlateScript->m_Ingredients.empty()) {
            auto* plateTf = GetScene()->GetWorld().GetComponent<TransformComponent>(currentHoveredPlate);
            if (!plateTf) return;

            IngredientType topIngredient = hoveredPlateScript->m_Ingredients.back();

            auto neighbor = FindClosestNeighbor(plateTf->GetPosition(), [topIngredient](const std::string& name, ScriptableEntity* instance) {
                MachineScript* mScript = dynamic_cast<MachineScript*>(instance);
                if (!mScript || mScript->m_IsReady) return false;

                if (name == "PotScript") {
                    return mScript->m_Ingredients.size() < 2 && topIngredient == IngredientType::ChoppedTomato;
                }
                else if (name == "CuttingBoardScript") {
                    return mScript->m_Ingredients.empty() && (topIngredient == IngredientType::Tomato || topIngredient == IngredientType::Baguette || topIngredient == IngredientType::Cheese || topIngredient == IngredientType::Ham || topIngredient == IngredientType::Mozzarella);
                }
                else if (name == "MixerScript") {
                    bool hasType = std::find(mScript->m_Ingredients.begin(), mScript->m_Ingredients.end(), topIngredient) != mScript->m_Ingredients.end();
                    return mScript->m_Ingredients.size() < 2 && !hasType && (topIngredient == IngredientType::Flour || topIngredient == IngredientType::Milk);
                }
                else if (name == "OvenScript") {
                    return mScript->m_Ingredients.empty() && topIngredient == IngredientType::RawDough;
                }
                return false;
                });

            if (neighbor.TargetEntity.id != HighlightedPotFromPlate.id) {
                ClearPotHighlight();
                if (neighbor.TargetEntity.id != std::numeric_limits<std::size_t>::max()) SetPotHighlight(neighbor.TargetEntity, true);
                HighlightedPotFromPlate = neighbor.TargetEntity;
            }

            bool isActionPressed = Input::IsMouseButtonJustPressed(0) || (Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0));
            if (isActionPressed && neighbor.TargetEntity.id != std::numeric_limits<std::size_t>::max() && !Input::IsUICapturingMouse()) {
                if (!Input::IsKeyPressed(340)) {
                    IngredientType topIngredient = hoveredPlateScript->m_Ingredients.back();
                    if (neighbor.MachineInstance && neighbor.MachineInstance->AddIngredient(topIngredient)) {
                        spdlog::info("Składnik wrzucony z talerza z powrotem do maszyny!");
                        hoveredPlateScript->m_Ingredients.pop_back();
                        Entity visualToRemove = hoveredPlateScript->m_VisualModels.back();
                        GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ visualToRemove });
                        hoveredPlateScript->m_VisualModels.pop_back();
                        ClearPotHighlight();
                    }
                    else {
                        spdlog::warn("Maszyna nie potrafi przetworzyć składnika, który chcesz w niej umieścić!");
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