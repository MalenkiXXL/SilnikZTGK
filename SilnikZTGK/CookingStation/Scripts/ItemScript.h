#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scripts/ConveyorScript.h"
#include <cmath>

class ItemScript : public ScriptableEntity
{
public:
    void OnCreate() override {}

    void OnUpdate(Timestep ts) override
    {
        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;

        auto* scriptStorage = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        if (!scriptStorage) return;

        ConveyorScript* closestConveyor = nullptr;

        // ZWIÊKSZONY ZASIÊG RADARU DO 2.0 (¿eby pokryæ modele o skali 1.5+)
        float minDistance = 2.0f;

        // 1. Zawsze szukamy NAJBLI¯SZEJ taœmy, ¿eby nigdy nie by³o "martwego punktu"
        for (size_t i = 0; i < scriptStorage->dense.size(); i++)
        {
            auto& scriptComp = scriptStorage->dense[i];
            if (scriptComp.Instance)
            {
                ConveyorScript* conveyor = dynamic_cast<ConveyorScript*>(scriptComp.Instance);
                if (conveyor)
                {
                    auto* conveyorTransform = conveyor->GetComponent<TransformComponent>();
                    if (conveyorTransform)
                    {
                        float distX = transform->Position.x - conveyorTransform->Position.x;
                        float distZ = transform->Position.z - conveyorTransform->Position.z;
                        float currentDist = std::sqrt(distX * distX + distZ * distZ);

                        if (currentDist < minDistance)
                        {
                            minDistance = currentDist;
                            closestConveyor = conveyor;
                        }
                    }
                }
            }
        }

        // --- ROBOTYCZNY RUCH FACTORIO (Twarde zakrêty) ---
        if (closestConveyor)
        {
            auto* conveyorTransform = closestConveyor->GetComponent<TransformComponent>();
            float speed = closestConveyor->Speed * ts.GetSeconds();

            float diffX = conveyorTransform->Position.x - transform->Position.x;
            float diffZ = conveyorTransform->Position.z - transform->Position.z;

            // Jeœli nowa taœma pcha w osi X (Prawo/Lewo)
            if (std::abs(closestConveyor->PushDirection.x) > 0.1f)
            {
                // Zanim skrêcimy, musimy dojechaæ dok³adnie do centrum na osi Z
                if (std::abs(diffZ) > 0.01f) {
                    // Dobijamy do œrodka kratki, kontynuuj¹c stary ruch
                    if (std::abs(diffZ) <= speed) transform->Position.z = conveyorTransform->Position.z;
                    else transform->Position.z += std::copysign(speed, diffZ);
                }
                else {
                    // KLIK - Jesteœmy w centrum! Oœ Z zablokowana, ruszamy twardo w X
                    transform->Position.x += closestConveyor->PushDirection.x * speed;
                    transform->Position.z = conveyorTransform->Position.z;
                }
            }
            // Jeœli nowa taœma pcha w osi Z (Góra/Dó³)
            else if (std::abs(closestConveyor->PushDirection.z) > 0.1f)
            {
                // Zanim skrêcimy, musimy dojechaæ dok³adnie do centrum na osi X
                if (std::abs(diffX) > 0.01f) {
                    // Dobijamy do œrodka kratki, kontynuuj¹c stary ruch
                    if (std::abs(diffX) <= speed) transform->Position.x = conveyorTransform->Position.x;
                    else transform->Position.x += std::copysign(speed, diffX);
                }
                else {
                    // KLIK - Jesteœmy w centrum! Oœ X zablokowana, ruszamy twardo w Z
                    transform->Position.z += closestConveyor->PushDirection.z * speed;
                    transform->Position.x = conveyorTransform->Position.x;
                }
            }
        }
    }
};