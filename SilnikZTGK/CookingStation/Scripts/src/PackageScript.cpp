#include "CookingStation/Scripts/Delivery/PackageScript.h"
#include "CookingStation/Events/GameEvents.h" 
#include <spdlog/spdlog.h>
#include "CookingStation/Core/AudioEngine.h"

void PackageScript::HandleClick()
{
    if (m_IsCollected) return;
    m_IsCollected = true;

    GetScene()->GetWorld().GetEventBus().Publish(AddIngredientEvent{ m_Type, m_IngredientAmount });

    GetScene()->GetWorld().GetEventBus().Publish(DeliveryCollectedEvent{});

    spdlog::info("Gracz zebrał paczkę (Wysłano zdarzenie AddIngredientEvent)");
    AudioEngine::Play("CookingStation/Assets/sounds/handleSmallLeather.mp3 ");

    std::vector<Entity> allPackages = s_ActivePackages;
    s_ActivePackages.clear();

    for (Entity e : allPackages)
    {
        GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ e });
    }

}

IngredientType PackageScript::getType() const {
    return m_Type;
}
