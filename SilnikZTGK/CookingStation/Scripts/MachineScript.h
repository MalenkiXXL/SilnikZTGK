#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/GridSystem.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Layers/CameraLayer/Camera.h"
#include <vector>
#include <glm/glm.hpp>

enum class IngredientType { Carrot, Potato, None };

class MachineScript : public ScriptableEntity
{
protected:
    float m_CookTime = 10.0f;
    float m_CurrentTime = 0.0f;
    bool m_IsReady = false;
    bool m_IsAutomated = false;
    float m_AutoDetectRadius = 3.0f;

    bool m_IsHeld = false;

    std::vector<IngredientType> m_Ingredients;

public:
    virtual void OnUpdate(Timestep ts) override
    {
        // TRYB PRZENOSZENIA MASZYNY
        if (m_IsHeld)
        {
            auto* transform = GetComponent<TransformComponent>();
            if (!transform) return;

            glm::vec3 mouseWorldPos = GetMouseWorldPosition();
            transform->SetPosition(GridSystem::SnapToGrid(mouseWorldPos));

            // Zakoñczenie przenoszenia (puszczenie LPM)
            if (Input::IsMouseButtonJustPressed(0))
            {
                if (!IsCellOccupied(transform->GetPosition()))
                {
                    m_IsHeld = false;
                    spdlog::info("Maszyna odlozona na siatke!");
                }
                else
                {
                    spdlog::warn("Nie mozesz tu postawic maszyny, pole zajete!");
                }
            }
            return; // Przerwij zwyk³y Update, gdy maszyna "lata" za kursorem
        }
    }

    virtual void OnClick() override
    {
        // 340 to kod dla lewego SHIFTa w GLFW
        if (!m_IsHeld && Input::IsKeyPressed(340))
        {
            m_IsHeld = true;
            spdlog::info("Podniesiono maszyne!");
        }
        else if (m_IsReady && !m_IsAutomated && !m_IsHeld)
        {
            TryTransferToPlate();
        }
    }

protected:
    bool IsCellOccupied(const glm::vec3& targetPos)
    {
        glm::ivec2 targetCell = GridSystem::WorldToCell(targetPos);
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();
        if (!transforms) return false;

        for (size_t i = 0; i < transforms->dense.size(); ++i)
        {
            Entity otherEntity = transforms->reverse[i];
            if (otherEntity.id == m_Entity.id) continue;

            glm::ivec2 otherCell = GridSystem::WorldToCell(transforms->dense[i].GetPosition());
            if (targetCell == otherCell)
            {
                return true;
            }
        }
        return false;
    }

    void TryTransferToPlate()
    {
        auto* myTransform = GetComponent<TransformComponent>();
        if (!myTransform) return;

        Entity closestPlate = { std::numeric_limits<std::size_t>::max(), 0 };
        float closestDist = m_AutoDetectRadius;

        auto* tagSet = GetScene()->GetWorld().GetComponentVector<TagComponent>();
        if (tagSet)
        {
            for (size_t i = 0; i < tagSet->dense.size(); ++i)
            {
                if (tagSet->dense[i].Tag == "Plate" || tagSet->dense[i].Tag == "Talerz")
                {
                    Entity plateEntity = tagSet->reverse[i];
                    auto* plateTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(plateEntity);

                    if (plateTransform)
                    {
                        float dist = glm::distance(myTransform->GetPosition(), plateTransform->GetPosition());
                        if (dist < closestDist)
                        {
                            closestDist = dist;
                            closestPlate = plateEntity;
                        }
                    }
                }
            }
        }

        if (closestPlate.id != std::numeric_limits<std::size_t>::max())
        {
            // TODO: Przekazanie do skryptu talerza
            ResetMachineState();
        }
    }

    virtual void ResetMachineState()
    {
        m_IsReady = false;
        m_CurrentTime = 0.0f;
        m_Ingredients.clear();
        UpdateVisuals();
    }

    virtual void UpdateVisuals() = 0;

private:
    glm::vec3 GetMouseWorldPosition()
    {
        auto* camera = GetScene()->GetCamera();
        if (!camera) return glm::vec3(0.0f);

        auto mousePos = Input::GetMousePosition();
        auto windowSize = Input::GetWindowSize();

        // Offsety UI pobrane z Twojego EditorLayer
        float localMouseX = mousePos.first - 200.0f;
        float localMouseY = mousePos.second - 30.0f;
        float viewWidth = (float)windowSize.first - 500.0f;
        float viewHeight = (float)windowSize.second - 230.0f;

        float orthoSize = 10.0f * (camera->Zoom / 45.0f);
        float aspectRatio = camera->AspectRatio;

        glm::mat4 proj3D = glm::ortho(-aspectRatio * orthoSize, aspectRatio * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
        glm::mat4 view3D = camera->GetViewMatrix();

        Ray ray = Physics::CastRayFromMouse(localMouseX, localMouseY, viewWidth, viewHeight, proj3D, view3D);

        if (std::abs(ray.Direction.y) > 1e-6f) {
            float t = -ray.Origin.y / ray.Direction.y;
            if (t > 0.0f) return ray.Origin + t * ray.Direction;
        }
        return glm::vec3(0.0f);
    }
};