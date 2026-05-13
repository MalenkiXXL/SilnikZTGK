#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scene/ecs.h"
#include "CookingStation/Core/Input.h"
#include <spdlog/spdlog.h>

class MushroomAI : public ScriptableEntity
{
public:
    void OnUpdate(Timestep ts) override
    {
        // 1. Pobieramy animatora
        auto* animComp = GetComponent<AnimatorComponent>();
        if (!animComp || !animComp->AnimatorInstance) return;

        // 2. Detekcja klikniêcia
        if (Input::IsMouseButtonPressed(0)) // 0 = Lewy przycisk myszy
        {
            // Pobieramy ca³¹ pozycjê myszy zamiast GetMouseX/Y
            auto [mx, my] = Input::GetMousePosition();

            if (IsMouseOver(mx, my))
            {
                // WYWO£ANIE ANIMACJI
                animComp->AnimatorInstance->PlayAnimation("Walk");
                animComp->IsPlaying = true;

                spdlog::info("MushroomAI: Kliknieto grzybka! Start animacji 'Walk'. Mysz: {}, {}", mx, my);
            }
        }

        // 3. Przyk³ad zatrzymania (Spacja)
        if (Input::IsKeyPressed(32))
        {
            animComp->IsPlaying = false;
            spdlog::info("MushroomAI: Pauza animacji.");
        }
    }

private:
    bool IsMouseOver(float mouseX, float mouseY)
    {
        // Na ten moment zwracamy true, abyœ móg³ przetestowaæ czy animacja w ogóle odpala.
        // W docelowym silniku tutaj kole¿anki mog¹ dopisaæ sprawdzanie dystansu do TransformComponent
        return true;
    }
};