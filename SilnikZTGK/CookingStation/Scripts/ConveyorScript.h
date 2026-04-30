#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include <glm/glm.hpp>
#include <CookingStation/Core/Input.h>


class ConveyorScript : public ScriptableEntity
{
public:
    glm::vec3 PushDirection = { 0.0f, 0.0f, 0.0f };
    float Speed = 2.0f;
    bool IsSwich = true;

    void OnCreate() override
    {
        SetPushDirection();
    }

    void SetPushDirection()
    {
        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;

        float rotY = transform->Rotation.y;

        // KLUCZOWE: normalizacja do zakresu [0, 360)
        while (rotY < 0.0f) rotY += 360.0f;
        while (rotY >= 360.0f) rotY -= 360.0f;

        if (std::abs(rotY - 90.0f) < 1.0f) PushDirection = { 1.0f, 0.0f,  0.0f };
        else if (std::abs(rotY - 270.0f) < 1.0f) PushDirection = { -1.0f, 0.0f,  0.0f };
        else if (std::abs(rotY - 180.0f) < 1.0f) PushDirection = { 0.0f, 0.0f, -1.0f };
        else                                      PushDirection = { 0.0f, 0.0f,  1.0f };
    }

    void OnUpdate(Timestep ts) override
    {

    }


    void OnClick() override
    {
        auto* transform = GetComponent<TransformComponent>();
        auto* scriptStorage = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        if (!transform || !scriptStorage) return;

        // --- 1. SKANER S¥SIADÓW ---
        int neighborCount = 0;

        // Szukamy wszystkich taœm na mapie, ¿eby sprawdziæ, ile z nich to nasi bezpoœredni s¹siedzi
        for (size_t i = 0; i < scriptStorage->dense.size(); i++)
        {
            auto& scriptComp = scriptStorage->dense[i];
            if (scriptComp.Instance && scriptComp.Instance != this)
            {
                ConveyorScript* otherConveyor = dynamic_cast<ConveyorScript*>(scriptComp.Instance);
                if (otherConveyor)
                {
                    auto* otherTransform = otherConveyor->GetComponent<TransformComponent>();
                    if (otherTransform)
                    {
                        // Liczymy ró¿nicê w pozycjach
                        float diffX = std::abs(otherTransform->Position.x - transform->Position.x);
                        float diffZ = std::abs(otherTransform->Position.z - transform->Position.z);

                        // Sprawdzamy, czy inna taœma jest dok³adnie 1 kratkê (czyli 2.0f) od nas
                        // Musi byæ w równej linii na osi X albo na osi Z
                        bool isNeighborX = (diffZ < 0.5f && std::abs(diffX - 2.0f) < 0.5f);
                        bool isNeighborZ = (diffX < 0.5f && std::abs(diffZ - 2.0f) < 0.5f);

                        if (isNeighborX || isNeighborZ)
                        {
                            neighborCount++;
                        }
                    }
                }
            }
        }

        // Jeœli mamy 2 lub mniej s¹siadów, jesteœmy zwyk³¹ drog¹ albo zakrêtem. Ignorujemy klikniêcie!
        if (neighborCount < 3)
        {
            spdlog::info("Taœma ma tylko {} sasiadow. To nie jest zwrotnica!", neighborCount);
            return;
        }


        float validAngles[4]; // Tu zapiszemy wszystkie bezpieczne kierunki
        int validCount = 0;

        // Sprawdzamy sztywno 4 strony œwiata: Pó³noc, Wschód, Po³udnie, Zachód
        float testAngles[4] = { 0.0f, 90.0f, 180.0f, 270.0f };

        // 1. Zbieramy WSZYSTKIE mo¿liwe kierunki ucieczki z tej taœmy
        for (int i = 0; i < 4; i++)
        {
            float testRot = testAngles[i];
            glm::vec3 testDirection = { 0.0f, 0.0f, 0.0f };
            int rotY = (int)testRot;

            if (rotY == 90)       testDirection = { 1.0f, 0.0f, 0.0f };
            else if (rotY == 270) testDirection = { -1.0f, 0.0f, 0.0f };
            else if (rotY == 180) testDirection = { 0.0f, 0.0f, -1.0f };
            else                  testDirection = { 0.0f, 0.0f, 1.0f };

            glm::vec3 expectedNeighborPos = transform->Position + (testDirection * 2.0f);

            for (size_t j = 0; j < scriptStorage->dense.size(); j++)
            {
                auto& scriptComp = scriptStorage->dense[j];
                if (scriptComp.Instance && scriptComp.Instance != this)
                {
                    ConveyorScript* otherConveyor = dynamic_cast<ConveyorScript*>(scriptComp.Instance);
                    if (otherConveyor)
                    {
                        auto* otherTransform = otherConveyor->GetComponent<TransformComponent>();
                        if (otherTransform)
                        {
                            float distX = std::abs(otherTransform->Position.x - expectedNeighborPos.x);
                            float distZ = std::abs(otherTransform->Position.z - expectedNeighborPos.z);

                            if (distX < 0.5f && distZ < 0.5f)
                            {
                                // Anty-Kolizja: Czy taœma obok pcha prosto w nas?
                                bool isHeadOnCollision = (std::abs(otherConveyor->PushDirection.x + testDirection.x) < 0.1f &&
                                    std::abs(otherConveyor->PushDirection.z + testDirection.z) < 0.1f);

                                if (!isHeadOnCollision)
                                {
                                    // ZnaleŸliœmy bezpieczne wyjœcie! Zapisujemy je do listy.
                                    validAngles[validCount] = testRot;
                                    validCount++;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        // 2. Prze³¹czamy siê na nastêpne wolne wyjœcie
        if (validCount > 0)
        {
            float currentRot = transform->Rotation.y;
            // Normalizujemy obecny k¹t do czystego formatu 0-360
            while (currentRot < 0.0f) currentRot += 360.0f;
            while (currentRot >= 360.0f) currentRot -= 360.0f;

            // Szukamy, w którym miejscu na liœcie obecnie jesteœmy
            int currentIndex = -1;
            for (int i = 0; i < validCount; i++)
            {
                if (std::abs(validAngles[i] - currentRot) < 1.0f)
                {
                    currentIndex = i;
                    break;
                }
            }

            // Wybieramy nastêpny k¹t z listy (dziêki operacji modulo '%' fajnie siê to zapêtla)
            int nextIndex = (currentIndex + 1) % validCount;
            transform->Rotation.y = validAngles[nextIndex];
            SetPushDirection();// Przelicz PushDirection!

            // --- LOGI DEBUGOWE ---
            spdlog::info("Zwrotnica ID:{} przeskanowala teren i znalazla {} bezpiecznych wyjsc:", validCount);
            for (int k = 0; k < validCount; k++) {
                spdlog::info(" -> Mozna bezpiecznie jechac w strone: {} stopni", validAngles[k]);
            }

        }
    }
};