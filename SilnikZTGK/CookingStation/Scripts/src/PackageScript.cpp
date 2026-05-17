#include "CookingStation/Scripts/PackageScript.h"
#include "CookingStation/Scripts/GameManagerScript.h"
#include <spdlog/spdlog.h>

void PackageScript::OnClick()
{
    // 1. Sprawdzamy, czy Manager gry istnieje
    if (GameManagerScript::s_Instance != nullptr)
    {
        // 2. Dodajemy wybraną ilość składników
        GameManagerScript::s_Instance->AddIngredients(m_IngredientAmount);

        spdlog::info("Gracz zebrał paczkę!");

        std::vector<Entity> allPackages = s_ActivePackages;
        s_ActivePackages.clear();

        // 3. Niszczymy WSZYSTKIE INNE paczki
        for (Entity e : allPackages)
        {
            if (e.id != m_Entity.id)
            {
                GetScene()->GetWorld().DestroyEntity(e);
            }
        }

        // 4. Usuwamy paczkę ze sceny, żeby zniknęła
        GetScene()->GetWorld().DestroyEntity(m_Entity);
    }
    else
    {
        spdlog::warn("Kliknięto paczkę, ale GameManager nie istnieje!");
    }
}