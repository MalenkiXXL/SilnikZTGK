#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Scripts/MachineScript.h"
#include "CookingStation/Scripts/PotScript.h"
#include <glm/glm.hpp>
#include <limits> 

class DragAndDropScript : public ScriptableEntity
{
public:
    static inline bool IsDragging = false;
    static inline IngredientType CurrentIngredient = IngredientType::None;
    static inline Entity DraggedEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    static inline Scene* ActiveScene = nullptr;

    // --- LOGIKA NO¯A NA HOVERZE ---
    static inline Entity KnifeVisualEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    static inline bool ShowKnife = false;
    // Œcie¿ka do Twojego modelu no¿a! Popraw, jeœli jest inna!
    const std::string m_KnifeModelPath = "CookingStation/Assets/models/przybory_kuchenne/noz/knife.gltf";
    // ---------------------------------

    void OnCreate() override {
        ActiveScene = GetScene();
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

            // Zabezpieczenie: jeœli przenosimy jedzenie, nó¿ nie mo¿e siê pokazaæ
            SetKnifeVisibility(false);
        }
        // ========================================================
        // SYTUACJA B: Rêka gracza jest pusta -> Sprawdzamy hover nad desk¹
        // ========================================================
        else if (!IsDragging)
        {
            CheckForCuttingBoardHover(mousePos);

            // Jeœli nó¿ ma byæ widoczny, przyklejamy go do myszki
            if (ShowKnife && KnifeVisualEntity.id != std::numeric_limits<std::size_t>::max()) {
                auto* knifeTf = GetScene()->GetWorld().GetComponent<TransformComponent>(KnifeVisualEntity);
                if (knifeTf) {
                    // Nó¿ nieco wy¿ej i przesuniêty, ¿eby nie zas³ania³ punktu klikniêcia
                    knifeTf->SetPosition(mousePos + glm::vec3(0.2f, 0.6f, -0.2f));
                }
            }
        }
    }

    static void StartDrag(IngredientType type, const std::string& modelPath)
    {
        if (!ActiveScene) return;
        if (IsDragging) CancelDrag();

        // Kiedy zaczynamy Drag, upewniamy siê, ¿e nó¿ znika
        SetKnifeVisibility(false);

        IsDragging = true;
        CurrentIngredient = type;

        auto builder = ActiveScene->GetWorld().BuildEntity();
        builder.With<TagComponent>({ "DraggedIngredient" });

        TransformComponent tc;
        tc.SetPosition(glm::vec3(0.0f, -100.0f, 0.0f)); // Pod mapê
        tc.SetScale(glm::vec3(0.6f, 0.6f, 0.6f));
        builder.With<TransformComponent>(tc);

        MeshComponent mesh;
        mesh.ModelPtr = AssetManager::GetModel(modelPath);
        builder.With<MeshComponent>(mesh);

        DraggedEntity = builder.Build();
    }

    static void CancelDrag()
    {
        IsDragging = false;
        if (DraggedEntity.id != std::numeric_limits<std::size_t>::max() && ActiveScene) {
            auto* transform = ActiveScene->GetWorld().GetComponent<TransformComponent>(DraggedEntity);
            if (transform) transform->SetPosition(glm::vec3(0.0f, -1000.0f, 0.0f));

            DraggedEntity = { std::numeric_limits<std::size_t>::max(), 0 };
        }
    }

private:
    // --- POMOCNICZA FUNKCJA DO NO¯A ---
    static void SetKnifeVisibility(bool visible) {
        ShowKnife = visible;
        // Dodajemy sprawdzenie ActiveScene i zamieniamy GetScene() na ActiveScene
        if (KnifeVisualEntity.id != std::numeric_limits<std::size_t>::max() && ActiveScene) {
            auto* knifeTf = ActiveScene->GetWorld().GetComponent<TransformComponent>(KnifeVisualEntity);
            if (knifeTf) {
                if (visible) {
                    // Kiedyœ tutaj np. wy³¹czymy systemowy kursor
                }
                else {
                    knifeTf->SetPosition(glm::vec3(0.0f, -1000.0f, 0.0f)); // Chowamy pod mapê
                }
            }
        }
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

                // Sprawdzamy, czy obiekt pod kursorem to Deska do Krojenia
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

                        // Sprawdzamy dystans hoveru (trochê wiêkszy ni¿ dystans klikniêcia)
                        if (glm::distance(mousePos2D, boardPos2D) < 4.0f) {

                            // SPRAWDZAMY CZY DESKA JEST DOSTÊPNA DO KROJENIA
                            // (Nie jest pusta I sk³adnik nie jest gotowy)
                            if (!machineInstance->m_Ingredients.empty() && !machineInstance->m_IsReady) {
                                hoveredOverChoppableBoard = true;
                                break;
                            }
                        }
                    }
                }
            }
        }

        // Jeœli nó¿ ma byæ pokazany, a model jeszcze nie istnieje, tworzymy go!
        if (hoveredOverChoppableBoard) {
            if (KnifeVisualEntity.id == std::numeric_limits<std::size_t>::max()) {
                if (!ActiveScene) return;
                auto builder = ActiveScene->GetWorld().BuildEntity();
                builder.With<TagComponent>({ "KnifeCursor" });
                TransformComponent tc;
                tc.SetPosition(glm::vec3(0.0f, -100.0f, 0.0f));
                // Ustaw odpowiedni¹ skalê no¿a! (np. 0.3f, 0.3f, 0.3f)
                tc.SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
                // Tu w przysz³oœci mo¿na dodaæ rotacjê, ¿eby nó¿ by³ ustawiony ³adnie
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

        auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();

        if (scripts && transforms) {
            for (size_t i = 0; i < scripts->dense.size(); ++i) {
                auto& nsc = scripts->dense[i];
                MachineScript* tempMachine = nullptr;

                for (auto& scriptElement : nsc.Scripts) {
                    if (scriptElement.Name == "PotScript" || scriptElement.Name == "CuttingBoardScript") {
                        tempMachine = dynamic_cast<MachineScript*>(scriptElement.Instance);
                        break;
                    }
                }

                if (tempMachine) {
                    Entity machineEntity = scripts->reverse[i];
                    auto* machineTransform = transforms->Get(machineEntity);

                    if (machineTransform) {
                        float dist = glm::distance(dropPos, machineTransform->GetPosition());
                        if (dist < closestDist) {
                            closestDist = dist;
                            closestMachine = machineEntity;
                            targetMachineScript = tempMachine;
                        }
                    }
                }
            }
        }

        if (closestMachine.id != std::numeric_limits<std::size_t>::max() && targetMachineScript) {
            if (targetMachineScript->AddIngredient(CurrentIngredient)) {
                spdlog::info("DragAndDrop: Skladnik pomyslnie upuszczony na maszyne!");
                CancelDrag();
            }
            else {
                spdlog::warn("DragAndDrop: Maszyna odrzucila skladnik (nie ten typ lub zajeta)!");
            }
        }
    }
};