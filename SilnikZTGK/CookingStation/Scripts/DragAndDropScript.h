#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Scripts/MachineScript.h"
#include "CookingStation/Scripts/PotScript.h"
#include "CookingStation/Scripts/Managers/GameManagerScript.h"
#include <glm/glm.hpp>
#include <limits> 

class DragAndDropScript : public ScriptableEntity
{
public:
    static inline bool IsDragging = false;
    static inline IngredientType CurrentIngredient = IngredientType::None;
    static inline Entity DraggedEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    static inline Scene* ActiveScene = nullptr;

    void OnCreate() override {
        ActiveScene = GetScene();
    }

    void OnUpdate(Timestep ts) override
    {
        if (!IsDragging || DraggedEntity.id == std::numeric_limits<std::size_t>::max()) return;

        auto* transform = GetScene()->GetWorld().GetComponent<TransformComponent>(DraggedEntity);
        if (!transform) return;

        glm::vec3 mousePos = GetMouseWorldPosition();

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

    static void StartDrag(IngredientType type, const std::string& modelPath)
    {
        if (!ActiveScene) return;
        if (IsDragging) CancelDrag();

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
    void TryDropIngredient(glm::vec3 dropPos)
    {
        Entity closestPot = { std::numeric_limits<std::size_t>::max(), 0 };
        float closestDist = 2.0f;
        PotScript* targetPotScript = nullptr;

        auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();

        if (scripts && transforms) {
            for (size_t i = 0; i < scripts->dense.size(); ++i) {
                auto& nsc = scripts->dense[i];
                PotScript* tempPot = nullptr;

                for (auto& scriptElement : nsc.Scripts) {
                    if (scriptElement.Name == "PotScript") {
                        tempPot = (PotScript*)scriptElement.Instance;
                        break;
                    }
                }

                if (tempPot) {
                    Entity potEntity = scripts->reverse[i];
                    auto* potTransform = transforms->Get(potEntity);

                    if (potTransform) {
                        float dist = glm::distance(dropPos, potTransform->GetPosition());
                        if (dist < closestDist) {
                            closestDist = dist;
                            closestPot = potEntity;
                            targetPotScript = tempPot;
                        }
                    }
                }
            }
        }

        if (closestPot.id != std::numeric_limits<std::size_t>::max() && targetPotScript) {
            if (targetPotScript->AddIngredient(CurrentIngredient)) {
                spdlog::info("Skladnik pomyslnie wrzucony do garnka!");
                if (GameManagerScript::s_Instance != nullptr) {
                    GameManagerScript::s_Instance->UseIngredient(CurrentIngredient, 1);
                }
                CancelDrag();
            }
            else {
                spdlog::warn("Garnek jest pelny lub zupa ugotowana!");
            }
        }
    }
};