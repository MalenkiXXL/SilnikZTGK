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

    // Zmienna trzymająca w pamięci ostatnio podświetlony garnek dla talerza
    static inline Entity HighlightedPotFromPlate = { std::numeric_limits<std::size_t>::max(), 0 };

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
            // Obydwa detektory uderzają tylko, jeśli niczego nie niesiemy w ręce
            CheckPlateToPotTransfer(mousePos);
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
    // SEKCJA: TRANSFER Z TALERZA DO GARNKA
    // ==========================================================

    static void SetPotHighlight(Entity potEntity, bool state) {
        if (potEntity.id == std::numeric_limits<std::size_t>::max() || !ActiveScene) return;
        const std::string targetShader = state ? "HighlightShader" : "ModelShader";
        auto* mesh = ActiveScene->GetWorld().GetComponent<MeshComponent>(potEntity);
        if (mesh) mesh->ShaderName = targetShader;
    }

    static void ClearPotHighlight() {
        if (HighlightedPotFromPlate.id != std::numeric_limits<std::size_t>::max()) {
            SetPotHighlight(HighlightedPotFromPlate, false);
            HighlightedPotFromPlate = { std::numeric_limits<std::size_t>::max(), 0 };
        }
    }

    void CheckPlateToPotTransfer(glm::vec3 mousePos)
    {
        // Jeśli aktualnie przestawiamy maszynę Shiftem, dezaktywujemy tę logikę
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

        // 1. Sprawdzamy czy nasza myszka jest nad jakimkolwiek talerzem
        for (size_t i = 0; i < scripts->dense.size(); ++i) {
            auto& nsc = scripts->dense[i];
            for (auto& s : nsc.Scripts) {
                if (s.Name == "PlateScript") {
                    Entity plateEntity = scripts->reverse[i];
                    auto* tf = transforms->Get(plateEntity);
                    // Dystans sprawdzania kolizji kursora z talerzem
                    if (tf && glm::distance(mousePos2D, glm::vec2(tf->GetPosition().x, tf->GetPosition().z)) < 2.0f) {
                        PlateScript* pScript = static_cast<PlateScript*>(s.Instance);
                        // Talerz musi posiadać coś na sobie, co nie jest jeszcze w pełni złożoną kanapką/daniem
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

        // 2. Jeśli jesteśmy nad prawidłowym talerzem, szukamy w pobliżu garnka
        if (hoveredPlateScript) {
            Entity closestPot = { std::numeric_limits<std::size_t>::max(), 0 };
            MachineScript* targetPotScript = nullptr;
            float closestDist = 8.0f; // Limit dystansu = 8 kratek wokół talerza!
            auto* plateTf = transforms->Get(currentHoveredPlate);

            for (size_t i = 0; i < scripts->dense.size(); ++i) {
                auto& nsc = scripts->dense[i];
                for (auto& s : nsc.Scripts) {
                    if (s.Name == "PotScript") { // Namierzamy tylko garnki
                        Entity potEntity = scripts->reverse[i];
                        auto* potTf = transforms->Get(potEntity);
                        MachineScript* mScript = static_cast<MachineScript*>(s.Instance);

                        // Garnek musi nie być skończony i mieć miejsce
                        if (potTf && mScript && !mScript->m_IsReady && mScript->m_Ingredients.size() < 2) {
                            float dist = glm::distance(plateTf->GetPosition(), potTf->GetPosition());
                            if (dist < closestDist) {
                                closestDist = dist;
                                closestPot = potEntity;
                                targetPotScript = mScript;
                            }
                        }
                        break;
                    }
                }
            }

            // 3. Podświetlamy najbliższy garnek (jeśli zmienił się cel podświetlenia)
            if (closestPot.id != HighlightedPotFromPlate.id) {
                ClearPotHighlight();
                if (closestPot.id != std::numeric_limits<std::size_t>::max()) {
                    SetPotHighlight(closestPot, true);
                }
                HighlightedPotFromPlate = closestPot;
            }

            // 4. Mechanika przełożenia po kliknięciu
            if (Input::IsMouseButtonJustPressed(0) && closestPot.id != std::numeric_limits<std::size_t>::max() && !MachineScript::GlobalIsHoveringUI) {
                if (!Input::IsKeyPressed(340)) // Ochrona przez wciśniętym shiftem
                {
                    // Ściągamy najwyższy element ułożony na talerzu
                    IngredientType topIngredient = hoveredPlateScript->m_Ingredients.back();

                    // Wrzucamy go do wybranego garnka (AddIngredient zwraca false, jeśli garnkowi typ nie pasuje, np. bagietka)
                    if (targetPotScript && targetPotScript->AddIngredient(topIngredient)) {
                        spdlog::info("Składnik wrzucony z talerza z powrotem do garnka!");

                        // Zdejmujemy składnik z tablicy talerza
                        hoveredPlateScript->m_Ingredients.pop_back();

                        // Niszczymy ułożony na talerzu model
                        Entity visualToRemove = hoveredPlateScript->m_VisualModels.back();
                        GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ visualToRemove });
                        hoveredPlateScript->m_VisualModels.pop_back();

                        // Oczyszczamy podświetlenie
                        ClearPotHighlight();
                    }
                    else {
                        spdlog::warn("Garnek nie potrafi ugotować składnika, który chcesz w nim umieścić!");
                    }
                }
            }
        }
        else {
            // Skoro kursor nie jest nad żadnym talerzem, po prostu wygaszamy garnki
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