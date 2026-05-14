#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include <string>
#include <vector>

class CustomerScript : public ScriptableEntity
{
public:
    std::string WantedIngredient = "";
    bool IsServed = false;

    void OnCreate() override
    {
        // Na razie "na sztywno" chcemy pomidora, tak jak prosi³aœ
        WantedIngredient = "Tomato";

        spdlog::info("Klient nr {} usiadl i chce: {}", m_Entity.id, WantedIngredient);

        // TODO: PóŸniej dodamy tutaj Renderer2D rysuj¹cy ikonkê chmurki nad jego g³ow¹
    }

    // Ta funkcja bêdzie wywo³ywana póŸniej przez Kelnera
    bool IsOrderMatching(const std::vector<std::string>& ingredientsOnPlate)
    {
        for (const auto& item : ingredientsOnPlate)
        {
            if (item == WantedIngredient) return true;
        }
        return false;
    }

    void ReceiveFood()
    {
        IsServed = true;
        spdlog::info("Klient nr {} dostal to, czego chcial! Zjada ze smakiem.", m_Entity.id);

        // --- OBEJŒCIE B£ÊDU SILNIKA (Soft Deletion) ---
        // 1. Zrzucamy klienta 1000 metrów pod mapê
        auto* tf = GetComponent<TransformComponent>();
        if (tf) tf->SetPosition(glm::vec3(0.0f, -1000.0f, 0.0f));

        // 2. Zmieniamy tag. Dziêki temu Manager uzna, ¿e krzes³o jest znowu puste!
        auto* tag = GetComponent<TagComponent>();
        if (tag) tag->Tag = "ZadowolonyKlient";
    }
};