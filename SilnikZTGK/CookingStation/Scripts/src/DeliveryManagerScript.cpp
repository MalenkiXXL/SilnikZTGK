#include "CookingStation/Scripts/Delivery/DeliveryManagerScript.h"
#include "CookingStation/Scripts/Managers/GameManagerScript.h"
#include "CookingStation/Scripts/Delivery/PackageScript.h"
#include "CookingStation/Scene/PrefabSerializer.h"
#include "CookingStation/Scripts/Delivery/DeliveryLogic.h"
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

    auto& bus = GetScene()->GetWorld().GetEventBus();

    m_CustomerSeatedSubId = bus.Subscribe<KitchenOrderPlacedEvent>([this](const KitchenOrderPlacedEvent& e) {
        if (e.WantedDish != IngredientType::None) {
            m_ActiveOrdersQueue.push_back({ e.Customer.id, e.WantedDish });
            spdlog::info("Magazyn dopisał zamówienie na pozycję {}: {}", m_ActiveOrdersQueue.size(), IngredientTypeToString(e.WantedDish));
        }
    });

    // 2. Usuwamy zrealizowane zamówienie z kolejki
    m_ValidationResponseSubId = bus.Subscribe<ValidateOrderResponseEvent>([this](const ValidateOrderResponseEvent& e) {

        auto it = std::find_if(m_ActiveOrdersQueue.begin(), m_ActiveOrdersQueue.end(),
                               [&e](const OrderRecord& order) { return order.CustomerId == e.Customer.id; });

        if (it != m_ActiveOrdersQueue.end()) {
            m_ActiveOrdersQueue.erase(it);
            spdlog::info("Zamówienie klienta {} zrealizowane i usunięte z kolejki magazynu.", e.Customer.id);
        }
    });
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

    if (m_IsDeliveryOnTheWay) return;

    auto* gm = GameManagerScript::s_Instance;
    if (!gm) return;

    spdlog::info("[DeliveryAI] Drzewo decyzyjne uruchomione. Aktualna kolejka zamówień: {}", m_ActiveOrdersQueue.size());

    std::map<IngredientType, int> realInventory;
    for (const auto& [type, threshold] : m_MinThreshold) {
        realInventory[type] = gm->GetIngredientCount(type);
    }

    IngredientType typeToDeliver = DeliveryLogic::CalculateWhatToOrder(
            m_ActiveOrdersQueue,
            realInventory,
            m_MinThreshold
    );

    if (typeToDeliver != IngredientType::None) {
        spdlog::info("[DeliveryAI] Mózg zdecydował zamówić: {}", IngredientTypeToString(typeToDeliver));
        CallForDelivery(typeToDeliver);
    }
}

void DeliveryManagerScript::CallForDelivery(IngredientType type)
{
    m_CurrentOrderType = type;

    Entity car = PrefabSerializer::Deserialize(GetScene(), m_VanPrefabPath, m_CarStartPos);

    if (car.id != std::numeric_limits<std::size_t>::max())
    {
        m_DeliveryCarEntityId = car.id;
        m_IsDeliveryOnTheWay = true;
        spdlog::info("[DeliveryAI] Dostawczak wyruszyl z: {}.", IngredientTypeToString(type));
    }
}
void DeliveryManagerScript::OnDestroy()
{
    GetScene()->GetWorld().GetEventBus().Unsubscribe<EntityDestroyedEvent>(m_DeliveryDestroySubId);
    GetScene()->GetWorld().GetEventBus().Unsubscribe<CarArrivedEvent>(m_CarArrivedSubId);
    GetScene()->GetWorld().GetEventBus().Unsubscribe<PackageSpawnedEvent>(m_PackageSpawnedSubId);
}