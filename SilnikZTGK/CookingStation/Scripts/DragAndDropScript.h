#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Core/GridSystem.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Scripts/Machines/PanScript.h"
#include "CookingStation/Scripts/Machines/MachineScript.h"
#include "CookingStation/Scripts/Managers/GameManagerScript.h"
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
    static inline std::vector<IngredientType> CurrentHistory; // PAMIĘĆ HISTORII W RĘKACH
    static inline std::vector<std::string> CurrentMachineHistory; // NOWE: PAMIĘĆ MASZYN W RĘKACH
    static inline Entity DraggedEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    static inline Scene* ActiveScene = nullptr;

    std::size_t m_DragSubId;
    std::size_t m_HoverSubId;

    Entity m_Hovered3DEntity = { std::numeric_limits<std::size_t>::max(), 0 };

    void OnCreate() override {
        ActiveScene = GetScene();
        m_DragSubId = GetScene()->GetWorld().GetEventBus().Subscribe<StartDragRequestEvent>(
            [this](const StartDragRequestEvent& e) {
                // Nowe składniki z lodówki nie mają głębokiej historii
                this->StartDrag(e.Type);
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
        if (GameManagerScript::s_IsTutorialMode) return;

        glm::vec3 mousePos = GetMouseWorldPosition();

        if (IsDragging && DraggedEntity.id != std::numeric_limits<std::size_t>::max())
        {
            auto* transform = GetScene()->GetWorld().GetComponent<TransformComponent>(DraggedEntity);
            if (!transform) return;

            transform->SetPosition(mousePos + glm::vec3(0.0f, 0.5f, 0.0f));

            if (m_Hovered3DEntity.id != std::numeric_limits<std::size_t>::max()) {
                auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Hovered3DEntity);
                if (nsc) {
                    for (auto& s : nsc->Scripts) {
                        if (s.Name == "PotScript" || s.Name == "CuttingBoardScript" || s.Name == "MixerScript" || s.Name == "OvenScript" || s.Name == "PanScript" || s.Name == "CoffeeMakerScript") {
                            MachineScript* mScript = static_cast<MachineScript*>(s.Instance);
                            if (mScript && mScript->CanAcceptIngredient(CurrentIngredient)) {
                                TriggerInfiniteHighlight(m_Hovered3DEntity);
                            }
                        }
                        else if (s.Name == "PlateScript") {
                            PlateScript* pScript = static_cast<PlateScript*>(s.Instance);
                            if (pScript && pScript->m_CompletedDish == IngredientType::None && pScript->m_Ingredients.size() < 5) {
                                TriggerInfiniteHighlight(m_Hovered3DEntity, pScript);
                            }
                        }
                    }
                }
            }

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

    // Dodano możliwość przekazywania historii
    static void StartDrag(IngredientType type, std::vector<IngredientType> history = {}, std::vector<std::string> machineHistory = {})
    {
        if (!ActiveScene) return;
        if (MachineScript::GlobalIsMachineHeld || MachineScript::PendingPickup.id != std::numeric_limits<std::size_t>::max()) {
            return;
        }
        if (IsDragging) CancelDrag();

        IsDragging = true;
        CurrentIngredient = type;
        CurrentHistory = history;
        CurrentMachineHistory = machineHistory; // Zapisujemy maszyny!

        auto builder = ActiveScene->GetWorld().BuildEntity();
        builder.With<TagComponent>({ "DraggedIngredient" });

        TransformComponent tc;
        tc.SetPosition(glm::vec3(0.0f, -100.0f, 0.0f));

        IngredientMetadata meta = GetIngredientMetadata(type);
        tc.SetScale(meta.scale);
        tc.SetRotation(meta.rotation);

        builder.With<TransformComponent>(tc);

        MeshComponent mesh;
        mesh.ModelPtr = AssetManager::GetModel(GetModelPath(type));
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
        CurrentHistory.clear();
        CurrentMachineHistory.clear(); // Czyścimy przy anulowaniu
        if (DraggedEntity.id != std::numeric_limits<std::size_t>::max()) {
            ActiveScene->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ DraggedEntity });
            DraggedEntity = { std::numeric_limits<std::size_t>::max(), 0 };
        }
    }

private:
    struct NeighborResult {
        Entity TargetEntity = { std::numeric_limits<std::size_t>::max(), 0 };
        MachineScript* MachineInstance = nullptr;
        PlateScript* PlateInstance = nullptr;
    };

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

    static void TriggerInfiniteHighlight(Entity entity, PlateScript* plateScript = nullptr) {
        if (entity.id == std::numeric_limits<std::size_t>::max() || !ActiveScene) return;

        ActiveScene->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                entity, glm::vec3(1.0f, 0.9f, 0.0f), 0.0f, true
            });

        if (plateScript) {
            for (Entity e : plateScript->m_VisualModels) {
                ActiveScene->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{
                        e, glm::vec3(1.0f, 0.9f, 0.0f), 0.0f, true
                    });
            }
        }
    }

    void CheckBeltToMachineTransfer(glm::vec3 mousePos)
    {
        if (MachineScript::GlobalIsMachineHeld) return;

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
                if (!mScript || (mScript->m_IsReady && name != "PanScript" && name != "MixerScript" && name != "CoffeeMakerScript")) return false;

                if (name == "CuttingBoardScript") {
                    return mScript->m_Ingredients.empty() &&
                        (hoveredType == IngredientType::Tomato || hoveredType == IngredientType::Baguette ||
                            hoveredType == IngredientType::Cheese || hoveredType == IngredientType::Ham ||
                            hoveredType == IngredientType::Mozzarella || hoveredType == IngredientType::Apple ||
                            hoveredType == IngredientType::Raspberry || hoveredType == IngredientType::Potato);
                }
                else if (name == "PotScript") {
                    return mScript->CanAcceptIngredient(hoveredType);
                }
                else if (name == "MixerScript") {
                    return mScript->CanAcceptIngredient(hoveredType);
                }
                else if (name == "OvenScript") {
                    return mScript->CanAcceptIngredient(hoveredType);
                }
                else if (name == "PanScript") {
                    return mScript->CanAcceptIngredient(hoveredType);
                }
                else if (name == "CoffeeMakerScript") {
                    return mScript->CanAcceptIngredient(hoveredType);
                }
                return false;
                });

            if (neighbor.TargetEntity.id != std::numeric_limits<std::size_t>::max()) {
                TriggerInfiniteHighlight(neighbor.TargetEntity);
            }

            bool isActionPressed = Input::IsMouseButtonJustPressed(0) || (Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0));
            if (isActionPressed && neighbor.TargetEntity.id != std::numeric_limits<std::size_t>::max() && !Input::IsUICapturingMouse()) {
                if (!Input::IsKeyPressed(340) && neighbor.MachineInstance && neighbor.MachineInstance->AddIngredient(hoveredType)) {
                    spdlog::info("Składnik z taśmy wskoczył prosto na maszynę!");
                    GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ hoveredBeltItem });
                }
            }
        }
    }

    void CheckMachinePullFromBelt(glm::vec3 mousePos)
    {
        if (MachineScript::GlobalIsMachineHeld) return;

        Entity hoveredMachineEntity = { std::numeric_limits<std::size_t>::max(), 0 };
        MachineScript* hoveredMachineScript = nullptr;
        std::string machineName = "";

        if (m_Hovered3DEntity.id != std::numeric_limits<std::size_t>::max()) {
            auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Hovered3DEntity);
            if (nsc) {
                for (auto& s : nsc->Scripts) {
                    if (s.Name == "CuttingBoardScript" || s.Name == "PotScript" || s.Name == "MixerScript" || s.Name == "OvenScript" || s.Name == "PanScript" || s.Name == "CoffeeMakerScript") {
                        hoveredMachineEntity = m_Hovered3DEntity;
                        hoveredMachineScript = dynamic_cast<MachineScript*>(s.Instance);
                        machineName = s.Name;
                        break;
                    }
                }
            }
        }

        if (hoveredMachineScript && (!hoveredMachineScript->m_IsReady || machineName == "PanScript" || machineName == "MixerScript" || machineName == "CoffeeMakerScript")) {
            Entity closestBeltItem = { std::numeric_limits<std::size_t>::max(), 0 };
            IngredientType foundType = IngredientType::None;
            float closestDist = std::numeric_limits<float>::max();

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
                                    canAccept = hoveredMachineScript->CanAcceptIngredient(type);
                                }
                                else if (machineName == "PotScript") {
                                    canAccept = hoveredMachineScript->CanAcceptIngredient(type);
                                }
                                else if (machineName == "MixerScript") {
                                    canAccept = hoveredMachineScript->CanAcceptIngredient(type);
                                }
                                else if (machineName == "OvenScript") {
                                    canAccept = hoveredMachineScript->CanAcceptIngredient(type);
                                }
                                else if (machineName == "PanScript") {
                                    canAccept = hoveredMachineScript->CanAcceptIngredient(type);
                                }
                                else if (machineName == "CoffeeMakerScript") {
                                    canAccept = hoveredMachineScript->CanAcceptIngredient(type);
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

            if (closestBeltItem.id != std::numeric_limits<std::size_t>::max()) {
                TriggerInfiniteHighlight(closestBeltItem);

                bool isActionPressed = Input::IsMouseButtonJustPressed(0) || (Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0));
                if (isActionPressed && !Input::IsUICapturingMouse()) {
                    if (!Input::IsKeyPressed(340)) {
                        // Tutaj przekazujemy puste historie, bo przedmiot z taśmy nie ma historii
                        if (hoveredMachineScript->AddIngredient(foundType, {}, {})) {
                            spdlog::info("Maszyna zassała składnik z taśmy!");
                            GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ closestBeltItem });
                        }
                    }
                }
            }
        }
    }

    void CheckPlatePullFromMachine(glm::vec3 mousePos)
    {
        if (MachineScript::GlobalIsMachineHeld) return;

        Entity currentHoveredPlate = { std::numeric_limits<std::size_t>::max(), 0 };
        PlateScript* hoveredPlateScript = nullptr;

        if (m_Hovered3DEntity.id != std::numeric_limits<std::size_t>::max()) {
            auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Hovered3DEntity);
            if (nsc) {
                for (auto& s : nsc->Scripts) {
                    if (s.Name == "PlateScript") {
                        currentHoveredPlate = m_Hovered3DEntity;
                        hoveredPlateScript = static_cast<PlateScript*>(s.Instance);
                        break;
                    }
                }
            }
        }

        if (hoveredPlateScript && hoveredPlateScript->m_CompletedDish == IngredientType::None && hoveredPlateScript->m_Ingredients.size() < 5) {
            auto* plateTf = GetScene()->GetWorld().GetComponent<TransformComponent>(currentHoveredPlate);
            if (!plateTf) return;

            auto neighbor = FindClosestNeighbor(plateTf->GetPosition(), [](const std::string& name, ScriptableEntity* instance) {
                MachineScript* mScript = dynamic_cast<MachineScript*>(instance);
                return mScript && mScript->m_IsReady;
                });

            if (neighbor.TargetEntity.id != std::numeric_limits<std::size_t>::max()) {
                TriggerInfiniteHighlight(neighbor.TargetEntity);
            }

            bool isActionPressed = Input::IsMouseButtonJustPressed(0) || (Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0));
            if (isActionPressed && neighbor.TargetEntity.id != std::numeric_limits<std::size_t>::max() && !Input::IsUICapturingMouse()) {
                if (!Input::IsKeyPressed(340) && neighbor.MachineInstance) {
                    neighbor.MachineInstance->TryTransferToPlate();
                }
            }
        }
    }

    void CheckMachinePullFromPlate(glm::vec3 mousePos)
    {
        if (MachineScript::GlobalIsMachineHeld) return;

        Entity hoveredMachineEntity = { std::numeric_limits<std::size_t>::max(), 0 };
        MachineScript* hoveredMachineScript = nullptr;
        std::string machineName = "";

        if (m_Hovered3DEntity.id != std::numeric_limits<std::size_t>::max()) {
            auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Hovered3DEntity);
            if (nsc) {
                for (auto& s : nsc->Scripts) {
                    if (s.Name == "PotScript" || s.Name == "CuttingBoardScript" || s.Name == "MixerScript" || s.Name == "OvenScript" || s.Name == "PanScript" || s.Name == "CoffeeMakerScript") {
                        hoveredMachineEntity = m_Hovered3DEntity;
                        hoveredMachineScript = dynamic_cast<MachineScript*>(s.Instance);
                        machineName = s.Name;
                        break;
                    }
                }
            }
        }

        if (hoveredMachineScript && (!hoveredMachineScript->m_IsReady || machineName == "PanScript" || machineName == "MixerScript" || machineName == "CoffeeMakerScript")) {
            auto* machineTf = GetScene()->GetWorld().GetComponent<TransformComponent>(hoveredMachineEntity);
            if (!machineTf) return;

            auto neighbor = FindClosestNeighbor(machineTf->GetPosition(), [&](const std::string& name, ScriptableEntity* instance) {
                if (name != "PlateScript") return false;
                PlateScript* pScript = static_cast<PlateScript*>(instance);
                if (!pScript) return false;

                IngredientType itemToTransfer = IngredientType::None;
                if (pScript->m_CompletedDish != IngredientType::None) {
                    itemToTransfer = pScript->m_CompletedDish;
                } else if (!pScript->m_Ingredients.empty()) {
                    itemToTransfer = pScript->m_Ingredients.back();
                }

                if (itemToTransfer == IngredientType::None) return false; // Talerz jest pusty

                return hoveredMachineScript->CanAcceptIngredient(itemToTransfer);
            });

            if (neighbor.TargetEntity.id != std::numeric_limits<std::size_t>::max() && neighbor.PlateInstance) {
                TriggerInfiniteHighlight(neighbor.TargetEntity, neighbor.PlateInstance);
            }

            bool isActionPressed = Input::IsMouseButtonJustPressed(0) || (Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0));
            if (isActionPressed && neighbor.TargetEntity.id != std::numeric_limits<std::size_t>::max() && !Input::IsUICapturingMouse()) {
                if (!Input::IsKeyPressed(340) && neighbor.PlateInstance) {

                    IngredientType itemToTransfer = (neighbor.PlateInstance->m_CompletedDish != IngredientType::None)
                                                    ? neighbor.PlateInstance->m_CompletedDish
                                                    : neighbor.PlateInstance->m_Ingredients.back();

                    if (hoveredMachineScript->AddIngredient(itemToTransfer, neighbor.PlateInstance->m_DeepHistory, neighbor.PlateInstance->m_MachineHistory)) {
                        spdlog::info("Maszyna zassała składnik ze stojącego obok talerza!");

                        if (neighbor.PlateInstance->m_CompletedDish != IngredientType::None) {
                            neighbor.PlateInstance->m_CompletedDish = IngredientType::None;
                        } else {
                            neighbor.PlateInstance->m_Ingredients.pop_back();
                        }

                        if (neighbor.PlateInstance->m_Ingredients.empty() && neighbor.PlateInstance->m_CompletedDish == IngredientType::None) {
                            neighbor.PlateInstance->m_DeepHistory.clear();
                            neighbor.PlateInstance->m_MachineHistory.clear();
                        }

                        if (!neighbor.PlateInstance->m_VisualModels.empty()) {
                            Entity visualToRemove = neighbor.PlateInstance->m_VisualModels.back();
                            GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ visualToRemove });
                            neighbor.PlateInstance->m_VisualModels.pop_back();
                        }
                    }
                }
            }
        }
    }
    void CheckPlateToPotTransfer(glm::vec3 mousePos)
    {
        if (MachineScript::GlobalIsMachineHeld) return;
        Entity currentHoveredPlate = { std::numeric_limits<std::size_t>::max(), 0 };
        PlateScript* hoveredPlateScript = nullptr;

        if (m_Hovered3DEntity.id != std::numeric_limits<std::size_t>::max()) {
            auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Hovered3DEntity);
            if (nsc) {
                for (auto& s : nsc->Scripts) {
                    if (s.Name == "PlateScript") {
                        currentHoveredPlate = m_Hovered3DEntity;
                        hoveredPlateScript = static_cast<PlateScript*>(s.Instance);
                        break;
                    }
                }
            }
        }

        if (hoveredPlateScript && (hoveredPlateScript->m_CompletedDish != IngredientType::None || !hoveredPlateScript->m_Ingredients.empty())) {
            auto* plateTf = GetScene()->GetWorld().GetComponent<TransformComponent>(currentHoveredPlate);
            if (!plateTf) return;

            IngredientType topIngredient = (hoveredPlateScript->m_CompletedDish != IngredientType::None)
                                           ? hoveredPlateScript->m_CompletedDish
                                           : hoveredPlateScript->m_Ingredients.back();

            auto neighbor = FindClosestNeighbor(plateTf->GetPosition(), [topIngredient](const std::string& name, ScriptableEntity* instance) {
                MachineScript* mScript = dynamic_cast<MachineScript*>(instance);
                if (!mScript || (mScript->m_IsReady && name != "PanScript" && name != "MixerScript" && name != "CoffeeMakerScript")) return false;

                return mScript->CanAcceptIngredient(topIngredient);
            });

            if (neighbor.TargetEntity.id != std::numeric_limits<std::size_t>::max()) {
                TriggerInfiniteHighlight(neighbor.TargetEntity);

                bool isActionPressed = Input::IsMouseButtonJustPressed(0) || (Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0));
                if (isActionPressed && !Input::IsUICapturingMouse()) {
                    if (!Input::IsKeyPressed(340)) {

                        if (neighbor.MachineInstance && neighbor.MachineInstance->AddIngredient(topIngredient, hoveredPlateScript->m_DeepHistory, hoveredPlateScript->m_MachineHistory)) {
                            spdlog::info("Składnik wrzucony z talerza do maszyny z historią!");

                            if (hoveredPlateScript->m_CompletedDish != IngredientType::None) {
                                hoveredPlateScript->m_CompletedDish = IngredientType::None;
                            } else {
                                hoveredPlateScript->m_Ingredients.pop_back();
                            }

                            if (hoveredPlateScript->m_Ingredients.empty() && hoveredPlateScript->m_CompletedDish == IngredientType::None) {
                                hoveredPlateScript->m_DeepHistory.clear();
                                hoveredPlateScript->m_MachineHistory.clear();
                            }

                            if (!hoveredPlateScript->m_VisualModels.empty()) {
                                Entity visualToRemove = hoveredPlateScript->m_VisualModels.back();
                                GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ visualToRemove });
                                hoveredPlateScript->m_VisualModels.pop_back();
                            }
                        }
                    }
                }
            }
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
                            scriptElement.Name == "MixerScript" || scriptElement.Name == "OvenScript" ||
                            scriptElement.Name == "PanScript" || scriptElement.Name == "CoffeeMakerScript")
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

            bool canHighlight = false;

            if (targetPlateScript) {
                canHighlight = (targetPlateScript->m_CompletedDish == IngredientType::None && targetPlateScript->m_Ingredients.size() < 5);
            }
            else if (targetMachineScript) {
                canHighlight = targetMachineScript->CanAcceptIngredient(CurrentIngredient);
            }

            if (canHighlight) {
                TriggerInfiniteHighlight(closestMachine, targetPlateScript);
            }

            if (Input::IsMouseButtonJustPressed(0)) {
                if (targetPlateScript && targetPlateScript->AddIngredient(CurrentIngredient)) {
                    spdlog::info("Złożono składnik na talerzu!");

                    if (!CurrentHistory.empty() && !targetPlateScript->m_DeepHistory.empty()) {
                        targetPlateScript->m_DeepHistory.pop_back();
                        targetPlateScript->m_DeepHistory.insert(targetPlateScript->m_DeepHistory.end(), CurrentHistory.begin(), CurrentHistory.end());
                    }

                    if (!CurrentMachineHistory.empty()) {
                        targetPlateScript->m_MachineHistory.insert(targetPlateScript->m_MachineHistory.end(), CurrentMachineHistory.begin(), CurrentMachineHistory.end());
                    }

                    GetScene()->GetWorld().GetEventBus().Publish(IngredientUsedEvent{ CurrentIngredient, 1 });
                    CancelDrag();
                }
                // TUTAJ ZMIANA: Wysyłamy do maszyny całą nową sygnaturę z historiami!
                else if (targetMachineScript && targetMachineScript->AddIngredient(CurrentIngredient, CurrentHistory, CurrentMachineHistory)) {
                    spdlog::info("Wrzucono składnik do maszyny (wraz z zapisaną historią)!");

                    // Usunąłem stąd ręczne nadpisywanie "m_DeepHistory", bo nowe AddIngredient maszyny robi to samo automatycznie!

                    GetScene()->GetWorld().GetEventBus().Publish(IngredientUsedEvent{ CurrentIngredient, 1 });
                    CancelDrag();
                }
            }
        }
    }
};