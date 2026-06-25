#include "CookingStation/Scripts/Delivery/DeliveryManagerScript.h"
#include "CookingStation/Scripts/Managers/GameManagerScript.h"
#include "CookingStation/Scene/PrefabSerializer.h"
#include "CookingStation/Scripts/Delivery/DeliveryLogic.h"
#include <spdlog/spdlog.h>
#include <algorithm>

void DeliveryManagerScript::OnCreate()
{
    m_MinThreshold[IngredientType::Tomato]    = 3;
    m_MinThreshold[IngredientType::Cheese]    = 3;
    m_MinThreshold[IngredientType::Ham]       = 3;
    m_MinThreshold[IngredientType::Mozzarella]= 3;
    m_MinThreshold[IngredientType::Milk]      = 2;
    m_MinThreshold[IngredientType::Flour]     = 2;
    m_MinThreshold[IngredientType::Egg]       = 2;

    spdlog::info("DeliveryManager uruchomiony!");

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

                glm::vec3 spawnPositions[2] = {
                        e.DropPosition + m_PackageOffsets[0],
                        e.DropPosition + m_PackageOffsets[1]
                };

                for (const auto& pos : spawnPositions)
                {
                    PrefabSerializer::Deserialize(GetScene(), m_PackagePrefabPath, pos);
                }
            }
    );

    m_PackageSpawnedSubId = GetScene()->GetWorld().GetEventBus().Subscribe<PackageSpawnedEvent>(
            [this](const PackageSpawnedEvent& e) {
                IngredientType typeForThisPackage = IngredientType::None;

                if (m_SpawnedPackagesCount < m_CurrentOrderTypes.size()) {
                    typeForThisPackage = m_CurrentOrderTypes[m_SpawnedPackagesCount];
                } else if (!m_CurrentOrderTypes.empty()) {
                    typeForThisPackage = m_CurrentOrderTypes[0];
                }

                int packageNumberForLog = m_SpawnedPackagesCount + 1;

                spdlog::info("[DeliveryManager] Konfiguruję paczkę nr {} - Zawartość: {}",
                             packageNumberForLog,
                             IngredientTypeToString(typeForThisPackage));

                m_SpawnedPackagesCount++;

                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> distrib(2, 5);
                int randomAmount = distrib(gen);

                GetScene()->GetWorld().GetEventBus().Publish(ConfigurePackageEvent{ e.TargetEntity,
                                                                                    typeForThisPackage,
                                                                                    randomAmount });
            }
    );

    auto& bus = GetScene()->GetWorld().GetEventBus();

    m_CustomerSeatedSubId = bus.Subscribe<KitchenOrderPlacedEvent>([this](const KitchenOrderPlacedEvent& e) {
        if (e.WantedDish != IngredientType::None) {
            m_ActiveOrdersQueue.push_back({ e.Customer.id, e.WantedDish });
            spdlog::info("Magazyn dopisal zamowienie na pozycje {}: {}", m_ActiveOrdersQueue.size(), IngredientTypeToString(e.WantedDish));
        }
    });

    m_ValidationResponseSubId = bus.Subscribe<ValidateOrderResponseEvent>([this](const ValidateOrderResponseEvent& e) {

        auto it = std::find_if(m_ActiveOrdersQueue.begin(), m_ActiveOrdersQueue.end(),
                               [&e](const OrderRecord& order) { return order.CustomerId == e.Customer.id; });

        if (it != m_ActiveOrdersQueue.end()) {
            m_ActiveOrdersQueue.erase(it);
            spdlog::info("Zamowienie klienta {} zrealizowane i usuniete z kolejki magazynu.", e.Customer.id);
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
    if (GameManagerScript::s_IsTutorialMode) return;

    if (m_IsDeliveryOnTheWay) return;

    auto* gm = GameManagerScript::s_Instance;
    if (!gm) return;

    spdlog::info("[DeliveryAI] Drzewo decyzyjne uruchomione. Aktualna kolejka zamówień: {}", m_ActiveOrdersQueue.size());

    std::map<IngredientType, int> realInventory;
    for (const auto& [type, threshold] : m_MinThreshold) {
        realInventory[type] = gm->GetIngredientCount(type);
    }

    std::vector<IngredientType> typesToDeliver = DeliveryLogic::CalculateWhatToOrder(
            m_ActiveOrdersQueue,
            realInventory,
            m_MinThreshold
    );

    if (!typesToDeliver.empty() && typesToDeliver[0] != IngredientType::None) {
        spdlog::info("[DeliveryAI] DeliveryManager zdecydował zamówić paczki. Opcja 1: {}", IngredientTypeToString(typesToDeliver[0]));
        if(typesToDeliver.size() > 1) {
            spdlog::info("[DeliveryAI] Opcja 2: {}", IngredientTypeToString(typesToDeliver[1]));
        }
        CallForDelivery(typesToDeliver);
    }
}

void DeliveryManagerScript::CallForDelivery(std::vector<IngredientType> types)
{
    m_CurrentOrderTypes = types;
    m_SpawnedPackagesCount = 0;

    Entity car = PrefabSerializer::Deserialize(GetScene(), m_VanPrefabPath, m_CarStartPos)[0];

    if (car.id != std::numeric_limits<std::size_t>::max())
    {
        m_DeliveryCarEntityId = car.id;
        m_IsDeliveryOnTheWay = true;
        spdlog::info("[DeliveryAI] Dostawczak wyruszyl z 2 roznymi paczkami.");
    }
}

void DeliveryManagerScript::OnDestroy()
{
    GetScene()->GetWorld().GetEventBus().Unsubscribe<EntityDestroyedEvent>(m_DeliveryDestroySubId);
    GetScene()->GetWorld().GetEventBus().Unsubscribe<CarArrivedEvent>(m_CarArrivedSubId);
    GetScene()->GetWorld().GetEventBus().Unsubscribe<PackageSpawnedEvent>(m_PackageSpawnedSubId);
}