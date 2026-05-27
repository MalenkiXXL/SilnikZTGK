#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Scripts/Machines/MachineScript.h"
#include "CookingStation/Scripts/Machines/PotScript.h"
#include "CookingStation/Events/GameEvents.h"
#include <glm/glm.hpp>
#include <limits> 
#include "CookingStation/Scripts/PlateScript.h"

class DragAndDropScript : public ScriptableEntity
{
public:
    static inline bool IsDragging = false;
    static inline IngredientType CurrentIngredient = IngredientType::None;
    static inline Entity DraggedEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    static inline Scene* ActiveScene = nullptr;

    // --- LOGIKA NO�A NA HOVERZE ---
    static inline Entity KnifeVisualEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    static inline bool ShowKnife = false;
    // �cie�ka do Twojego modelu no�a! Popraw, je�li jest inna!
    const std::string m_KnifeModelPath = "assets://models/przybory_kuchenne/noz/knife.gltf";
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

            // Zabezpieczenie: je�li przenosimy jedzenie, n� nie mo�e si� pokaza�
            SetKnifeVisibility(false);
        }
        // ========================================================
        // SYTUACJA B: R�ka gracza jest pusta -> Sprawdzamy hover nad desk�
        // ========================================================
        else if (!IsDragging)
        {
            CheckForCuttingBoardHover(mousePos);

            // Je�li n� ma by� widoczny, przyklejamy go do myszki
            if (ShowKnife && KnifeVisualEntity.id != std::numeric_limits<std::size_t>::max()) {
                auto* knifeTf = GetScene()->GetWorld().GetComponent<TransformComponent>(KnifeVisualEntity);
                if (knifeTf) {
                    // N� nieco wy�ej i przesuni�ty, �eby nie zas�ania� punktu klikni�cia
                    knifeTf->SetPosition(mousePos + glm::vec3(0.2f, 0.6f, -0.2f));
                }
            }
        }
    }

    static void StartDrag(IngredientType type, const std::string& modelPath)
    {
        if (!ActiveScene) return;
        if (IsDragging) CancelDrag();

        SetKnifeVisibility(false);

        IsDragging = true;
        CurrentIngredient = type;

        auto builder = ActiveScene->GetWorld().BuildEntity();
        builder.With<TagComponent>({ "DraggedIngredient" });

        TransformComponent tc;
        tc.SetPosition(glm::vec3(0.0f, -100.0f, 0.0f));

        // ZMIANA: Pobieramy dane z naszego nowego centralnego rejestru!
        IngredientMetadata meta = GetIngredientMetadata(type);
        tc.SetScale(meta.scale);
        tc.SetRotation(meta.rotation);

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
    // --- POMOCNICZA FUNKCJA DO NO�A ---
    static void SetKnifeVisibility(bool visible) {
        ShowKnife = visible;
        // Dodajemy sprawdzenie ActiveScene i zamieniamy GetScene() na ActiveScene
        if (KnifeVisualEntity.id != std::numeric_limits<std::size_t>::max() && ActiveScene) {
            auto* knifeTf = ActiveScene->GetWorld().GetComponent<TransformComponent>(KnifeVisualEntity);
            if (knifeTf) {
                if (visible) {
                    // Kiedy� tutaj np. wy��czymy systemowy kursor
                }
                else {
                    knifeTf->SetPosition(glm::vec3(0.0f, -1000.0f, 0.0f)); // Chowamy pod map�
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

                        // Sprawdzamy dystans hoveru (troch� wi�kszy ni� dystans klikni�cia)
                        if (glm::distance(mousePos2D, boardPos2D) < 4.0f) {

                            // SPRAWDZAMY CZY DESKA JEST DOST�PNA DO KROJENIA
                            // (Nie jest pusta I sk�adnik nie jest gotowy)
                            if (!machineInstance->m_Ingredients.empty() && !machineInstance->m_IsReady) {
                                hoveredOverChoppableBoard = true;
                                break;
                            }
                        }
                    }
                }
            }
        }

        // Je�li n� ma by� pokazany, a model jeszcze nie istnieje, tworzymy go!
        if (hoveredOverChoppableBoard) {
            if (KnifeVisualEntity.id == std::numeric_limits<std::size_t>::max()) {
                if (!ActiveScene) return;
                auto builder = ActiveScene->GetWorld().BuildEntity();
                builder.With<TagComponent>({ "KnifeCursor" });
                TransformComponent tc;
                tc.SetPosition(glm::vec3(0.0f, -100.0f, 0.0f));
                // Ustaw odpowiedni� skal� no�a! (np. 0.3f, 0.3f, 0.3f)
                tc.SetScale(glm::vec3(1.0f, 1.0f, 1.0f));
                // Tu w przysz�o�ci mo�na doda� rotacj�, �eby n� by� ustawiony �adnie
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