#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Scripts/Machines/MachineScript.h"
#include "CookingStation/Scripts/Machines/PotScript.h"
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Scripts/PlateScript.h"
#include <glm/glm.hpp>
#include <limits>

class DragAndDropScript : public ScriptableEntity
{
public:
    static inline bool IsDragging = false;
    static inline IngredientType CurrentIngredient = IngredientType::None;
    static inline Entity DraggedEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    static inline Scene* ActiveScene = nullptr;
    std::size_t m_DragSubId;

    // --- LOGIKA NOZA NA HOVERZE ---
    static inline Entity KnifeVisualEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    static inline bool ShowKnife = false;
    const std::string m_KnifeModelPath = "assets://models/przybory_kuchenne/noz/knife.gltf";

    void OnCreate() override {
        ActiveScene = GetScene();
        m_DragSubId = GetScene()->GetWorld().GetEventBus().Subscribe<StartDragRequestEvent>(
            [this](const StartDragRequestEvent& e) {
                this->StartDrag(e.Type, e.ModelPath);
            }
        );
    }

    void OnDestroy() override
    {
        GetScene()->GetWorld().GetEventBus().Unsubscribe<StartDragRequestEvent>(m_DragSubId);
    }

    void OnUpdate(Timestep ts) override
    {
        glm::vec3 mousePos = GetMouseWorldPosition();

        // ========================================================
        // SYTUACJA A: Gracz przenosi jedzenie (Drag & Drop)
        // ========================================================
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

            SetKnifeVisibility(false);
        }
        // ========================================================
        // SYTUACJA B: Reka gracza jest pusta
        // ========================================================
        else if (!IsDragging)
        {
            CheckForCuttingBoardHover(mousePos);

            if (ShowKnife && KnifeVisualEntity.id != std::numeric_limits<std::size_t>::max()) {
                auto* knifeTf = GetScene()->GetWorld().GetComponent<TransformComponent>(KnifeVisualEntity);
                if (knifeTf) {
                    knifeTf->SetPosition(mousePos + glm::vec3(0.2f, 0.6f, -0.2f));
                }
            }

            // ========================================================
            // RAYCAST: Publikujemy EntityClickedEvent przy kliknieciu LPM.
            // To jedyne miejsce gdzie generujemy ten event - MachineScript go odbiera.
            // ========================================================
            if (Input::IsMouseButtonJustPressed(0))
            {
                PublishClickEvent(mousePos);
            }
        }
    }

    // Drag skladnika (pomidor, itp.)
    static void StartDrag(IngredientType type, const std::string& modelPath)
    {
        if (!ActiveScene) return;
        if (MachineScript::GlobalIsMachineHeld || MachineScript::PendingPickup.id != std::numeric_limits<std::size_t>::max()) {
            return;
        }
        if (IsDragging) CancelDrag();

        SetKnifeVisibility(false);

        IsDragging = true;
        CurrentIngredient = type;

        auto builder = ActiveScene->GetWorld().BuildEntity();
        builder.With<TagComponent>({ "DraggedIngredient" });

        TransformComponent tc;
        tc.SetPosition(glm::vec3(0.0f, -100.0f, 0.0f));
        tc.SetScale(glm::vec3(0.6f, 0.6f, 0.6f));
        builder.With<TransformComponent>(tc);

        MeshComponent mesh;
        mesh.ModelPtr = AssetManager::GetModel(modelPath);
        builder.With<MeshComponent>(mesh);

        DraggedEntity = builder.Build();
    }

    // Drag maszyny - encja jest juz spawned przez GameGuiLayer, tu tylko ja "podnosimy"
    static void PickupSpawnedMachine(Entity spawnedMachine)
    {
        if (spawnedMachine.id == std::numeric_limits<std::size_t>::max()) return;

        if (MachineScript::PendingPickup.id != std::numeric_limits<std::size_t>::max() ||
            MachineScript::GlobalIsMachineHeld || IsDragging)
        {
            if (ActiveScene) ActiveScene->DestroyEntity(spawnedMachine);
            return;
        }

        SetKnifeVisibility(false);

        // MachineScript::OnUpdate sprawdza PendingPickup co frame i ustawi m_IsHeld = true
        MachineScript::PendingPickup = spawnedMachine;
        spdlog::info("DragAndDrop: Maszyna ustawiona jako PendingPickup, zostanie podniesiona w nastepnym OnUpdate.");
    }

    static void CancelDrag()
    {
        IsDragging = false;
        if (DraggedEntity.id != std::numeric_limits<std::size_t>::max()) {
            ActiveScene->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{DraggedEntity});
            DraggedEntity = { std::numeric_limits<std::size_t>::max(), 0 };
        }
    }

private:
    // Raycast po maszynach - publikuje EntityClickedEvent dla tej najblizszej kursora
    void PublishClickEvent(glm::vec3 mousePos)
    {
        auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();
        if (!scripts || !transforms) return;

        glm::vec2 mousePos2D = { mousePos.x, mousePos.z };
        Entity closestEntity = { std::numeric_limits<std::size_t>::max(), 0 };
        float closestDist = 2.5f;

        for (size_t i = 0; i < scripts->dense.size(); ++i)
        {
            auto& nsc = scripts->dense[i];
            bool hasMachine = false;
            for (auto& s : nsc.Scripts)
            {
                if (dynamic_cast<MachineScript*>(s.Instance))
                {
                    hasMachine = true;
                    break;
                }
            }
            if (!hasMachine) continue;

            Entity machineEntity = scripts->reverse[i];
            auto* tf = transforms->Get(machineEntity);
            if (!tf) continue;

            glm::vec2 machinePos2D = { tf->GetPosition().x, tf->GetPosition().z };
            float dist = glm::distance(mousePos2D, machinePos2D);
            if (dist < closestDist)
            {
                closestDist = dist;
                closestEntity = machineEntity;
            }
        }

        if (closestEntity.id != std::numeric_limits<std::size_t>::max())
        {
            GetScene()->GetWorld().GetEventBus().Publish(EntityClickedEvent{ closestEntity, 0 });
        }
    }

    static void SetKnifeVisibility(bool visible) {
        ShowKnife = visible;
        if (KnifeVisualEntity.id != std::numeric_limits<std::size_t>::max() && ShowKnife == !visible) {
            ActiveScene
                ->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ KnifeVisualEntity });
        }
        KnifeVisualEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    }

    void CheckForCuttingBoardHover(glm::vec3 mousePos) {
        auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();

        bool hoveredOverChoppableBoard = false;

        if (scripts && transforms) {
            glm::vec2 mousePos2D = { mousePos.x, mousePos.z };

            for (size_t i = 0; i < scripts->dense.size(); ++i) {
                NativeScriptComponent& nsc = scripts->dense[i];
                MachineScript* machineInstance = nullptr;

                for (auto& scriptElement : nsc.Scripts) {
                    if (scriptElement.Name == "CuttingBoardScript") {
                        machineInstance = dynamic_cast<MachineScript*>(scriptElement.Instance);
                        break;
                    }
                }

                if (machineInstance) {
                    Entity machineEntity = scripts->reverse[i];
                    auto* boardTransform = transforms->Get(machineEntity);

                    if (boardTransform) {
                        glm::vec2 boardPos2D = { boardTransform->GetPosition().x, boardTransform->GetPosition().z };

                        if (glm::distance(mousePos2D, boardPos2D) < 4.0f) {
                            if (!machineInstance->m_Ingredients.empty() && !machineInstance->m_IsReady) {
                                hoveredOverChoppableBoard = true;
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (hoveredOverChoppableBoard) {
            if (KnifeVisualEntity.id == std::numeric_limits<std::size_t>::max()) {
                if (!ActiveScene) return;
                auto builder = ActiveScene->GetWorld().BuildEntity();
                builder.With<TagComponent>({ "KnifeCursor" });
                TransformComponent tc;
                tc.SetPosition(glm::vec3(0.0f, -100.0f, 0.0f));
                tc.SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
                builder.With<TransformComponent>(tc);
                MeshComponent mesh;
                mesh.ModelPtr = AssetManager::GetModel(m_KnifeModelPath);
                builder.With<MeshComponent>(mesh);
                KnifeVisualEntity = builder.Build();
            }
            SetKnifeVisibility(true);
        }
        else {
            SetKnifeVisibility(false);
        }
    }

    void TryDropIngredient(glm::vec3 dropPos)
    {
        Entity closestMachine = { std::numeric_limits<std::size_t>::max(), 0 };
        float closestDist = 2.0f;

        MachineScript* targetMachineScript = nullptr;
        PlateScript* targetPlateScript = nullptr; // NOWE

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
                        // Sprawdzamy maszyny
                        if (scriptElement.Name == "PotScript" || scriptElement.Name == "CuttingBoardScript" ||
                            scriptElement.Name == "MixerScript" || scriptElement.Name == "OvenScript")
                        {
                            closestDist = dist;
                            closestMachine = entity;
                            targetMachineScript = dynamic_cast<MachineScript*>(scriptElement.Instance);
                            targetPlateScript = nullptr;
                        }
                        // Sprawdzamy Talerze!
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
            // Jeśli trafiliśmy w talerz:
            if (targetPlateScript && targetPlateScript->AddIngredient(CurrentIngredient)) {
                spdlog::info("Złożono składnik na talerzu!");
                GetScene()->GetWorld().GetEventBus().Publish(IngredientUsedEvent{ CurrentIngredient, 1 });
                CancelDrag();
            }
            // Jeśli trafiliśmy w maszynę:
            else if (targetMachineScript && targetMachineScript->AddIngredient(CurrentIngredient)) {
                spdlog::info("Wrzucono składnik do maszyny!");
                GetScene()->GetWorld().GetEventBus().Publish(IngredientUsedEvent{ CurrentIngredient, 1 });
                CancelDrag();
            }
        }
    }
};