#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scene/ecs.h" 
#include <string>
#include <vector>

class CustomerScript : public ScriptableEntity
{
public:
    std::string WantedIngredient = "";
    bool IsServed = false;

    void OnCreate() override
    {
        // Na razie mamy tylko pomidora
        WantedIngredient = "Tomato";
        spdlog::info("Klient nr {} usiadl i chce: {}", m_Entity.id, WantedIngredient);
    }

    // Ta funkcja b�dzie wywo�ywana p�niej przez Kelnera
    bool IsOrderMatching(const std::vector<std::string>& ingredientsOnPlate)
    {
        for (const auto& item : ingredientsOnPlate)
        {
            if (item == WantedIngredient) return true;
        }
        return false;
    }

    virtual void ReceiveFood(bool isCorrectOrder = true)
    {
        IsServed = true;

        if (isCorrectOrder)
        {
            spdlog::info("Klient nr {} dostal to, czego chcial! Zjada ze smakiem.", m_Entity.id);
            if (GameManagerScript::s_Instance)
            {
                OrderFulfilledEvent e(10.0f); // 10.0f to nagroda
                GetScene()->GetWorld().GetEventBus().Publish(e);
                spdlog::info("Klient nr {} zaplacil 50 monet!", m_Entity.id);
            }
            auto* tag = GetComponent<TagComponent>();
            if (tag) tag->Tag = "ZadowolonyKlient";
        }
        else
        {
            spdlog::info("Klient nr {} dostal puste/zle zamowienie! Wychodzi bez placenia.", m_Entity.id);
            auto* tag = GetComponent<TagComponent>();
            if (tag) tag->Tag = "ZlyKlient";
        }

        // Soft Deletion 
        auto* tf = GetComponent<TransformComponent>();
        if (tf) tf->SetPosition(glm::vec3(0.0f, -1000.0f, 0.0f));
    }
};