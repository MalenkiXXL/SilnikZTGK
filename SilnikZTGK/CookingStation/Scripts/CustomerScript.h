#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scene/ecs.h" // Potrzebne, ¿eby widzia³ AnimatorComponent
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

        // --- ODPALAMY ANIMACJÊ ZARAZ PO USIEDZENIU ---
        auto* animComp = GetComponent<AnimatorComponent>();
        if (animComp && animComp->AnimatorInstance)
        {
            // Podajemy nazwê animacji, któr¹ za³adowaliœmy w CustomerManagerScript
            animComp->AnimatorInstance->PlayAnimation("SitIdle");
            animComp->IsPlaying = true;
        }
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

        // Soft Deletion 
        auto* tf = GetComponent<TransformComponent>();
        if (tf) tf->SetPosition(glm::vec3(0.0f, -1000.0f, 0.0f));

        // Zmiana tagu na zadowolenie -> mo¿na uzyæ do efeków wizualnych 
        auto* tag = GetComponent<TagComponent>();
        if (tag) tag->Tag = "ZadowolonyKlient";
    }
};