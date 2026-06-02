#include "CookingStation/Scripts/Delivery/DeliveryManagerScript.h"
#include "CookingStation/Scripts/Managers/GameManagerScript.h"
#include "CookingStation/Scripts/Delivery/PackageScript.h"
#include "CookingStation/Scene/PrefabSerializer.h"
#include <spdlog/spdlog.h>
#include <algorithm>

void DeliveryManagerScript::OnCreate()
{
    // Konfiguracja progów minimalnych
    m_MinThreshold[IngredientType::Tomato]    = 3;
    m_MinThreshold[IngredientType::Cheese]    = 3;
    m_MinThreshold[IngredientType::Ham]       = 3;
    m_MinThreshold[IngredientType::Mozzarella]= 3;
    m_MinThreshold[IngredientType::Milk]      = 2;
    m_MinThreshold[IngredientType::Flour]     = 2;
    m_MinThreshold[IngredientType::Egg]       = 2;

    spdlog::info("DeliveryManager uruchomiony!");

    // Nasłuchujemy zniszczenia samochodu dostawczego
    m_DeliveryDestroySubId = GetScene()->GetWorld().GetEventBus().Subscribe<EntityDestroyedEvent>(
            [this](const EntityDestroyedEvent& e) {
                if (e.TargetEntity.id == m_DeliveryCarEntityId)
                {
                    m_IsDeliveryOnTheWay = false;
                    m_DeliveryCarEntityId = std::numeric_limits<std::size_t>::max();
                    spdlog::info("[DeliveryAI] Dostawczak wrocil, mozna zamawiać ponownie.");
                }
            }
    );

    m_CarArrivedSubId = GetScene()->GetWorld().GetEventBus().Subscribe<CarArrivedEvent>(
            [this](const CarArrivedEvent& e) {

                // Definiujemy pozycje paczek obok auta
                glm::vec3 spawnPositions[2] = {
                        e.DropPosition + m_PackageOffsets[0],
                        e.DropPosition + m_PackageOffsets[1]
                };

                // Tworzymy paczki
                for (const auto& pos : spawnPositions)
                {
                    PrefabSerializer::Deserialize(GetScene(), m_PackagePrefabPath, pos);
                }
            }
    );

    m_PackageSpawnedSubId = GetScene()->GetWorld().GetEventBus().Subscribe<PackageSpawnedEvent>(
            [this](const PackageSpawnedEvent& e) {
                // Odpowiadamy konkretnej paczce
                GetScene()->GetWorld().GetEventBus().Publish(ConfigurePackageEvent{ e.TargetEntity, m_CurrentOrderType, 5 });
            }
    );
}

void DeliveryManagerScript::OnUpdate(Timestep ts)
{
    m_DecisionTimer -= (float)ts.GetSeconds();
    if (m_DecisionTimer > 0.0f) return;
    m_DecisionTimer = 20.0f;

    RunDeliveryDecisionTree();
}

void DeliveryManagerScript::RunDeliveryDecisionTree()
{
    spdlog::info("[DeliveryAI] Sprawdzam spizarnie...");

    if (m_IsDeliveryOnTheWay)
    {
        spdlog::info("[DeliveryAI] Dostawa w trakcie, pomijam.");
        return;
    }

    if (!GameManagerScript::s_Instance) return;

    std::vector<IngredientType> shortages;
    for (auto& [type, threshold] : m_MinThreshold)
    {
        int current = GameManagerScript::s_Instance->GetIngredientCount(type);
        if (current < threshold)
        {
            shortages.push_back(type);
            spdlog::info("[DeliveryAI] Brakuje typu {}: mam {}, minimum {}", (int)type, current, threshold);
        }
    }

    std::sort(shortages.begin(), shortages.end(), [](IngredientType a, IngredientType b) {
        return static_cast<uint32_t>(a) < static_cast<uint32_t>(b);
    });

    if (shortages.empty())
    {
        spdlog::info("[DeliveryAI] Spizarnia pelna.");
        return;
    }

    IngredientType toDeliver = shortages.front();
    spdlog::info("[DeliveryAI] Zamawiam dostawe skladnika {}.", (int)toDeliver);
    CallForDelivery(toDeliver);
}

void DeliveryManagerScript::CallForDelivery(IngredientType type)
{
    m_CurrentOrderType = type;

    Entity car = PrefabSerializer::Deserialize(GetScene(), m_VanPrefabPath, m_CarStartPos);

    if (car.id != std::numeric_limits<std::size_t>::max())
    {
        m_DeliveryCarEntityId = car.id;
        m_IsDeliveryOnTheWay = true;
        spdlog::info("[DeliveryAI] Dostawczak wyruszyl ze skladnikiem {}.", (int)type);
    }
}
void DeliveryManagerScript::OnDestroy()
{
    GetScene()->GetWorld().GetEventBus().Unsubscribe<EntityDestroyedEvent>(m_DeliveryDestroySubId);
    GetScene()->GetWorld().GetEventBus().Unsubscribe<CarArrivedEvent>(m_CarArrivedSubId);
    GetScene()->GetWorld().GetEventBus().Unsubscribe<PackageSpawnedEvent>(m_PackageSpawnedSubId);
}