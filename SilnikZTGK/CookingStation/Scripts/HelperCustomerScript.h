#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Scripts/MachineScript.h"
#include <glm/glm.hpp>
#include <limits>
#include <cmath>
#include <spdlog/spdlog.h>

class HelperCustomerScript : public ScriptableEntity
{
public:
    // Statyczne zmienne, ¿eby ¿aden inny Helper nie by³ podnoszony w tym samym czasie
    static inline bool IsAnyHelperDragged = false;

    // Ustawienia
    float m_YOffset = 0.5f;       // Wysokoœæ nad ziemi¹ (dostosuj do skali!)
    float m_CellSize = 2.0f;      // Rozmiar jednej "kratki" siatki budowania
    float m_InteractRange = 1.5f; // Z jakiej odleg³oœci mo¿na go klikn¹æ

    // Stany
    bool m_IsCarried = false;
    bool m_IsWorking = false;
    bool m_IsWaitingToHelp = true;

    Entity m_AssignedMachine = { std::numeric_limits<std::size_t>::max(), 0 };
    glm::vec3 m_PositionBeforeDrag;

    void OnUpdate(Timestep ts) override
    {
        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;

        glm::vec3 mousePos = GetMouseWorldPosition();
        glm::vec2 mousePos2D = { mousePos.x, mousePos.z };
        glm::vec2 myPos2D = { transform->GetPosition().x, transform->GetPosition().z };

        // -----------------------------------------------------
        // 1. KLIKNIÊCIE I PODNOSZENIE (Gdy nie jest niesiony)
        // -----------------------------------------------------
        if (!m_IsCarried)
        {
            if (Input::IsMouseButtonJustPressed(0) && !IsAnyHelperDragged)
            {
                // Sprawdzamy, czy gracz klikn¹³ w tego konkretnego Helpera
                if (glm::distance(mousePos2D, myPos2D) <= m_InteractRange)
                {
                    PickUpHelper(transform);
                }
            }
        }
        // -----------------------------------------------------
        // 2. PRZENOSZENIE (Drag & Drop)
        // -----------------------------------------------------
        else
        {
            // Snapowanie do siatki (Grid Snapping) tak jak w Unity
            float snappedX = std::floor(mousePos.x / m_CellSize) * m_CellSize + (m_CellSize / 2.0f);
            float snappedZ = std::floor(mousePos.z / m_CellSize) * m_CellSize + (m_CellSize / 2.0f);
            glm::vec3 snappedPos = { snappedX, m_YOffset + 0.5f, snappedZ }; // +0.5f ¿eby unosi³ siê wy¿ej podczas noszenia

            Entity hoveredMachine = GetHoveredMachine(mousePos);

            // Jeœli najechaliœmy na maszynê, przyklejamy Helpera obok niej
            if (hoveredMachine.id != std::numeric_limits<std::size_t>::max())
            {
                snappedPos = GetClosestTileAroundMachine(hoveredMachine, mousePos);
            }

            // Aktualizujemy pozycjê modelu w czasie rzeczywistym
            transform->SetPosition(snappedPos);

            // UPUSZCZANIE (Lewy Klik)
            if (Input::IsMouseButtonJustPressed(0))
            {
                TryDropHelper(transform, hoveredMachine, snappedPos);
            }
            // ANULOWANIE (Prawy Klik)
            else if (Input::IsMouseButtonJustPressed(1))
            {
                CancelCarry(transform);
            }
        }
    }

private:
    void PickUpHelper(TransformComponent* transform)
    {
        m_IsCarried = true;
        IsAnyHelperDragged = true;
        m_PositionBeforeDrag = transform->GetPosition();

        m_IsWaitingToHelp = false;

        // Odpinamy go od starej maszyny
        if (m_AssignedMachine.id != std::numeric_limits<std::size_t>::max())
        {
            // TODO: Wywo³aj na starej maszynie funkcjê wy³¹czaj¹c¹ automatyzacjê
            m_AssignedMachine = { std::numeric_limits<std::size_t>::max(), 0 };
        }

        m_IsWorking = false;
        spdlog::info("Podniesiono Helpera!");
    }

    void TryDropHelper(TransformComponent* transform, Entity hoveredMachine, glm::vec3 dropPos)
    {
        m_IsCarried = false;
        IsAnyHelperDragged = false;

        // Opuszczamy go z powrotem na ziemiê
        dropPos.y = m_YOffset;
        transform->SetPosition(dropPos);

        if (hoveredMachine.id != std::numeric_limits<std::size_t>::max())
        {
            m_AssignedMachine = hoveredMachine;
            m_IsWorking = true;
            m_IsWaitingToHelp = false;

            // TODO: Wywo³aj na hoveredMachine funkcjê w³¹czaj¹c¹ automatyzacjê!

            // Obracamy Helpera w stronê maszyny (matematyka z atan2)
            auto* machineTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(hoveredMachine);
            if (machineTransform)
            {
                glm::vec3 dir = machineTransform->GetPosition() - dropPos;
                float angle = glm::degrees(std::atan2(dir.x, dir.z));
                transform->SetRotation({ 0.0f, angle + 180.0f, 0.0f }); // Dodaj korektê z Blendera jeœli trzeba
            }

            spdlog::info("Helper przypisany do nowej maszyny!");
        }
        else
        {
            m_IsWaitingToHelp = true;
            spdlog::info("Helper postawiony na ziemi, czeka na zadanie.");
        }
    }

    void CancelCarry(TransformComponent* transform)
    {
        m_IsCarried = false;
        IsAnyHelperDragged = false;
        transform->SetPosition(m_PositionBeforeDrag);
        spdlog::info("Anulowano przenoszenie Helpera.");
    }

    Entity GetHoveredMachine(glm::vec3 mousePos)
    {
        Entity closestMachine = { std::numeric_limits<std::size_t>::max(), 0 };
        float closestDist = 2.0f; // Promieñ przyci¹gania do maszyny

        auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();

        if (scripts && transforms) {
            glm::vec2 mousePos2D = { mousePos.x, mousePos.z };

            for (size_t i = 0; i < scripts->dense.size(); ++i) {
                auto& nsc = scripts->dense[i];
                bool isMachine = false;

                // Sprawdzamy czy to jakakolwiek maszyna (Deska, Garnek, Patelnia itp.)
                for (auto& scriptElement : nsc.Scripts) {
                    if (scriptElement.Name == "PotScript" || scriptElement.Name == "CuttingBoardScript") {
                        isMachine = true;
                        break;
                    }
                }

                if (isMachine) {
                    Entity machineEntity = scripts->reverse[i];
                    auto* machineTransform = transforms->Get(machineEntity);

                    if (machineTransform) {
                        glm::vec2 machinePos2D = { machineTransform->GetPosition().x, machineTransform->GetPosition().z };
                        float dist = glm::distance(mousePos2D, machinePos2D);

                        if (dist < closestDist) {
                            closestDist = dist;
                            closestMachine = machineEntity;
                        }
                    }
                }
            }
        }
        return closestMachine;
    }

    // Uproszczona wersja GetClosestEmptyTileAround (szuka miejsca wokó³ maszyny)
    glm::vec3 GetClosestTileAroundMachine(Entity machine, glm::vec3 mousePos)
    {
        auto* machineTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(machine);
        if (!machineTransform) return mousePos;

        glm::vec3 mPos = machineTransform->GetPosition();

        // Offset zale¿y od tego, z której strony maszyny jest myszka
        float offsetX = (mousePos.x > mPos.x) ? m_CellSize : -m_CellSize;
        float offsetZ = (mousePos.z > mPos.z) ? m_CellSize : -m_CellSize;

        // Wybieramy oœ, w której myszka jest dalej od œrodka maszyny
        if (std::abs(mousePos.x - mPos.x) > std::abs(mousePos.z - mPos.z)) {
            return glm::vec3(mPos.x + offsetX, m_YOffset + 0.5f, mPos.z);
        }
        else {
            return glm::vec3(mPos.x, m_YOffset + 0.5f, mPos.z + offsetZ);
        }
    }
};