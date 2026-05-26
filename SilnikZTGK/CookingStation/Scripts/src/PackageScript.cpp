#include "CookingStation/Scripts/Delivery/PackageScript.h"
#include "CookingStation/Events/GameEvents.h" 
#include <spdlog/spdlog.h>

void PackageScript::HandleClick()
{
    
    GetScene()->GetWorld().GetEventBus().Publish(AddIngredientEvent{ m_Type, m_IngredientAmount });

    spdlog::info("Gracz zebrał paczkę (Wysłano zdarzenie AddIngredientEvent)");

    std::vector<Entity> allPackages = s_ActivePackages;
    s_ActivePackages.clear();

    for (Entity e : allPackages)
    {
        if (e.id != m_Entity.id)
        {
            GetScene()->GetWorld().DestroyEntity(e);
        }
    }

    GetScene()->GetWorld().DestroyEntity(m_Entity);
}

