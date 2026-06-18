#include "CookingStation/Scripts/Managers/GameManagerScript.h"
#include "CookingStation/Scene/PrefabSerializer.h"
#include "CookingStation/Scripts/Managers/CloudManagerScript.h"
#include "CookingStation/Scripts/Managers/HighlightManagerScript.h"
#include <spdlog/spdlog.h>

void GameManagerScript::OnCreate()
{
    s_Instance = this;
    spdlog::info("GameManager uruchomiony!");

    auto& bus = GetScene()->GetWorld().GetEventBus();

    m_IngredientUsedSubId = bus.Subscribe<IngredientUsedEvent>(
        [this](const IngredientUsedEvent& e) {
            this->UseIngredient(e.Type, e.Amount);
        }
    );

    m_AvailableQuests = QuestManager::LoadQuests("assets://wygenerowane_quests.json");
    if (m_AvailableQuests.empty()) {
        spdlog::warn("GameManager: Brak questow, timer nie zostanie uruchomiony.");
    }

    m_QuestTimer = QUEST_INTERVAL;
    m_CurrentQuestState = QuestEventState::WaitingForTimer;

    m_AddIngredientSubId = bus.Subscribe<AddIngredientEvent>(
        [this](const AddIngredientEvent& e) {
            this->AddIngredients(e.Type, e.Amount);
        }
    );

    m_OrderFulfilledSubId = bus.Subscribe<OrderFulfilledEvent>(
        [this](const OrderFulfilledEvent& e) {
            this->OnOrderFulfilled(e);
        }
    );

    // Rejestrowanie Historii Dań
    m_DishCreatedSubId = bus.Subscribe<DishCreatedEvent>(
        [this](const DishCreatedEvent& e) {
            m_DishMemory[e.FoodEntity.id] = e.History;
        }
    );

    // Weryfikacja zamówień przez system
    m_ValidateOrderSubId = bus.Subscribe<ValidateOrderRequestEvent>(
        [this](const ValidateOrderRequestEvent& e) {
            bool isCorrect = false;

            if (m_DishMemory.find(e.ServedFood.id) != m_DishMemory.end()) {
                const auto& history = m_DishMemory[e.ServedFood.id];

                IngredientType wantedType = e.WantedIngredient;

                for (auto ingredient : history.BaseIngredients) {
                    if (ingredient == wantedType ||
                        (wantedType == IngredientType::Tomato && ingredient == IngredientType::ChoppedTomato)) {
                        isCorrect = true;
                        break;
                    }
                }
            }

            GetScene()->GetWorld().GetEventBus().Publish(ValidateOrderResponseEvent{
                    e.Customer,
                    isCorrect
                });
        }
    );

    auto& world = GetScene()->GetWorld();

    Entity cloudManagerEntity = world.CreateEntity();
    world.AddComponent<TagComponent>(cloudManagerEntity, TagComponent{ "CloudManager" });

    NativeScriptComponent nsc;
    nsc.AddScript<CloudManagerScript>("CloudManagerScript");
    world.AddComponent<NativeScriptComponent>(cloudManagerEntity, nsc);

    spdlog::info("GameManager: Utworzono encje Cloud Managera!");

    Entity highlightManagerEntity = world.CreateEntity();
    world.AddComponent<TagComponent>(highlightManagerEntity, TagComponent{ "HighlightManager" });
    NativeScriptComponent nscHighlight;
    nscHighlight.AddScript<HighlightManagerScript>("HighlightManagerScript");
    world.AddComponent<NativeScriptComponent>(highlightManagerEntity, nscHighlight);

    AddIngredients(IngredientType::Tomato, 5);
    AddIngredients(IngredientType::Cheese, 5);
    AddIngredients(IngredientType::Ham, 5);
    AddIngredients(IngredientType::Mozzarella, 5);
    AddIngredients(IngredientType::Milk, 5);
    AddIngredients(IngredientType::Flour, 5);
    AddIngredients(IngredientType::Egg, 5);

    // --- Rejestracja i ukrywanie elementów Questowych ---

    auto findEntityByName = [&](const std::string& targetName) -> Entity {
        auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
        if (tags) {
            for (size_t i = 0; i < tags->dense.size(); ++i) {
                if (tags->dense[i].Tag == targetName) {
                    return tags->reverse[i];
                }
            }
        }
        return { std::numeric_limits<std::size_t>::max(), 0 };
        };

    // 1. Grupa Wyspy Eventowej (Przylatuje z BOKU po osi X)
    std::vector<std::string> eventIslandNames = { "event_78", "event-detal", "balony1", "balony2", "balony3", "balony4" };
    for (const auto& name : eventIslandNames) {
        Entity e = findEntityByName(name);
        auto* transform = GetScene()->GetWorld().GetComponent<TransformComponent>(e);

        if (transform) {
            if (name == "budka" && transform->GetPosition().x > -20.0f) continue;

            // UWAGA: Dla wyspy eventowej w pair.second zapisujemy teraz oryginalną pozycję X!
            float origX = transform->GetPosition().x;
            m_EventIslandGroup.push_back({ e, origX });

            glm::vec3 pos = transform->GetPosition();
            pos.x = origX - 60.0f; // Schowaj 60 jednostek w lewo poza mapę
            transform->SetPosition(pos);
        }
    }

    // 2. Grupa Głównej Wyspy (Przylatuje z GÓRY)
    std::vector<std::string> mainIslandNames = {
            "tasma_questy_1", "tasma_questy_2", "tasma_questy_3", "event-detal2", "tasma_switch_quest"
    };
    for (const auto& name : mainIslandNames) {
        Entity e = findEntityByName(name);
        auto* transform = GetScene()->GetWorld().GetComponent<TransformComponent>(e);

        if (transform) {
            float origY = transform->GetPosition().y;
            m_MainIslandQuestGroup.push_back({ e, origY });

            glm::vec3 pos = transform->GetPosition();
            pos.y = origY + 30.0f; // Schowaj 30 jednostek w GÓRĘ (w niebo)
            transform->SetPosition(pos);
        }
    }

    auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
    if (scripts) {
        for (size_t i = 0; i < scripts->dense.size(); ++i) {
            for (auto& s : scripts->dense[i].Scripts) {
                if (s.Name == "DeliveryBoothScript" && s.Instance) {
                    auto* booth = static_cast<DeliveryBoothScript*>(s.Instance);
                    booth->m_DirectionOffset = { 0.0f, 0.0f, 2.0f }; // nasłuchuj na +Z czyli [-9,0,-7]
                    break;
                }
            }
        }
    }

    // 3. Element wymieniany (Znika dopiero jak quest dojedzie)
    std::vector<std::string> replacedNames = { "tasma_17" };
    for (const auto& name : replacedNames) {
        Entity e = findEntityByName(name);
        auto* transform = GetScene()->GetWorld().GetComponent<TransformComponent>(e);
        if (transform) {
            float origY = transform->GetPosition().y;
            m_ReplacedByQuestGroup.push_back({ e, origY });
        }
    }
}

void GameManagerScript::OnDestroy()
{
    auto& bus = GetScene()->GetWorld().GetEventBus();

    bus.Unsubscribe<IngredientUsedEvent>(m_IngredientUsedSubId);
    bus.Unsubscribe<AddIngredientEvent>(m_AddIngredientSubId);
    bus.Unsubscribe<OrderFulfilledEvent>(m_OrderFulfilledSubId);
    bus.Unsubscribe<DishCreatedEvent>(m_DishCreatedSubId);
    bus.Unsubscribe<ValidateOrderRequestEvent>(m_ValidateOrderSubId);

    s_Instance = nullptr;
}

void GameManagerScript::AddIngredients(IngredientType type, int amount)
{
    m_Inventory[type] += amount;

    InventoryChangedEvent e;
    e.Type = type;
    e.NewAmount = m_Inventory[type];
    GetScene()->GetWorld().GetEventBus().Publish(e);

    spdlog::info("GameManager: Wysłano InventoryChangedEvent dla {} ilość: {}", (int)type, e.NewAmount);
}

void GameManagerScript::UseIngredient(IngredientType type, int amount)
{
    if (m_Inventory[type] >= amount)
    {
        m_Inventory[type] -= amount;
        GetScene()->GetWorld().GetEventBus().Publish(InventoryChangedEvent{ type, m_Inventory[type] });
    }
}

int GameManagerScript::GetIngredientCount(IngredientType type)
{
    if (m_Inventory.count(type) > 0)
    {
        return m_Inventory[type];
    }
    return 0;
}

int GameManagerScript::GetMoney() {
    return money;
}

bool GameManagerScript::AddMoney(int amount) {
    money += amount;
    GetScene()->GetWorld().GetEventBus().Publish(MoneyChangedEvent{ money });
    return true;
}

bool GameManagerScript::SpendMoney(int amount) {
    if (money >= amount) {
        money -= amount;
        GetScene()->GetWorld().GetEventBus().Publish(MoneyChangedEvent{ money });
        return true;
    }
    return false;
}

void GameManagerScript::OnOrderFulfilled(const OrderFulfilledEvent& e)
{
    AddMoney(static_cast<int>(e.RewardAmount));
    spdlog::info("Order fulfilled! Reward added: {}", e.RewardAmount);
}

void GameManagerScript::OnUpdate(Timestep ts)
{
    // === 1. AKTUALIZACJA TIMERA BRAKU PIENIĘDZY ===
    // Musi być przed returnem od questów, żeby działało w BuildMode nawet bez aktywnego questa!
    if (m_MoneyWarningTimer > 0.0f) {
        m_MoneyWarningTimer -= ts;
    }

    // === 2. LOGIKA QUESTÓW ===
    if (m_AvailableQuests.empty()) return;

    switch (m_CurrentQuestState)
    {
    case QuestEventState::WaitingForTimer:
    {
        m_QuestTimer -= ts;
        if (m_QuestTimer <= 0.0f)
        {
            spdlog::info("Quest Timer minal! Rozpoczynamy przylot wyspy eventowej.");
            m_AnimationProgress = 0.0f;
            m_CurrentQuestState = QuestEventState::IslandArriving;
        }
        break;
    }
    case QuestEventState::IslandArriving:
    {
        m_AnimationProgress += ts * 0.5f;
        if (m_AnimationProgress > 1.0f) m_AnimationProgress = 1.0f;

        float easeOut = 1.0f - (1.0f - m_AnimationProgress) * (1.0f - m_AnimationProgress);

        // Wyspa przylatuje z lewej strony (od -60 do 0 po osi X)
        float xOffset = -60.0f * (1.0f - easeOut);

        for (auto& pair : m_EventIslandGroup) {
            auto* transform = GetScene()->GetWorld().GetComponent<TransformComponent>(pair.first);
            if (transform) {
                glm::vec3 pos = transform->GetPosition();
                pos.x = pair.second + xOffset; // Animujemy oś X
                transform->SetPosition(pos);
            }
        }

        if (m_AnimationProgress >= 1.0f) {
            m_CurrentQuestState = QuestEventState::WaitingForAccept;
        }
        break;
    }
    case QuestEventState::WaitingForAccept:
    {
        // Oczekujemy na UI
        break;
    }
    case QuestEventState::QuestActive:
    {
        if (m_AnimationProgress < 1.0f) {
            m_AnimationProgress += ts * 0.5f;
            bool finishedNow = false; // Flaga sprawdzająca czy animacja W TYM MOMENCIE dobiła do końca
            if (m_AnimationProgress >= 1.0f) {
                m_AnimationProgress = 1.0f;
                finishedNow = true;
            }

            float easeOut = 1.0f - (1.0f - m_AnimationProgress) * (1.0f - m_AnimationProgress);

            // 1. Dobudówka zlatuje z góry
            float yOffset = 30.0f * (1.0f - easeOut);
            for (auto& pair : m_MainIslandQuestGroup) {
                auto* transform = GetScene()->GetWorld().GetComponent<TransformComponent>(pair.first);
                if (transform) {
                    glm::vec3 pos = transform->GetPosition();
                    pos.y = pair.second + yOffset;
                    transform->SetPosition(pos);
                }
            }

            // 2. Opóźnione znikanie starej taśmy
            float hideProgress = std::max(0.0f, (m_AnimationProgress - 0.8f) / 0.2f);
            float hideOffset = -30.0f * hideProgress;

            for (auto& pair : m_ReplacedByQuestGroup) {
                auto* transform = GetScene()->GetWorld().GetComponent<TransformComponent>(pair.first);
                if (transform) {
                    glm::vec3 pos = transform->GetPosition();
                    pos.y = pair.second + hideOffset;
                    transform->SetPosition(pos);
                }
            }

            // 3. Jeśli to była ostatnia klatka animacji, PRZEBUDUJ MAPĘ TAŚM!
            if (finishedNow) {
                for (auto& pair : m_ReplacedByQuestGroup) {
                    Entity e = pair.first;
                    if (e.id != std::numeric_limits<std::size_t>::max()) {
                        GetScene()->DestroyEntity(e);
                        // Czyścimy ID, żeby nie próbować jej usuwać ponownie
                        pair.first.id = std::numeric_limits<std::size_t>::max();
                    }
                }
            }
        }
        break;
    }
    case QuestEventState::IslandLeaving:
    {
        m_AnimationProgress += ts * 0.5f;
        if (m_AnimationProgress > 1.0f) m_AnimationProgress = 1.0f;

        float easeIn = m_AnimationProgress * m_AnimationProgress;

        // 1. Wyspa eventowa odlatuje w lewo (od 0 do -60 po osi X)
        float xOffset = -60.0f * easeIn;
        for (auto& pair : m_EventIslandGroup) {
            auto* transform = GetScene()->GetWorld().GetComponent<TransformComponent>(pair.first);
            if (transform) {
                glm::vec3 pos = transform->GetPosition();
                pos.x = pair.second + xOffset;
                transform->SetPosition(pos);
            }
        }

        // 2. Budka questowa odlatuje z powrotem do góry (od 0 do +30)
        float yOffsetMain = 30.0f * easeIn;
        for (auto& pair : m_MainIslandQuestGroup) {
            auto* transform = GetScene()->GetWorld().GetComponent<TransformComponent>(pair.first);
            if (transform) {
                glm::vec3 pos = transform->GetPosition();
                pos.y = pair.second + yOffsetMain;
                transform->SetPosition(pos);
            }
        }

        // 3. Stara, zwykła taśma natychmiastowo wraca z dołu na górę (od -30 do 0)
        // Robimy to w pierwszych 20% animacji odlotu, żeby od razu zakryła dziurę.
        float showProgress = std::min(1.0f, m_AnimationProgress / 0.2f);
        float showOffset = -500.0f * (1.0f - showProgress);

        for (auto& pair : m_ReplacedByQuestGroup) {
            auto* transform = GetScene()->GetWorld().GetComponent<TransformComponent>(pair.first);
            if (transform) {
                glm::vec3 pos = transform->GetPosition();
                pos.y = pair.second + showOffset;
                transform->SetPosition(pos);
            }
        }

        if (m_AnimationProgress >= 1.0f) {
            m_QuestTimer = QUEST_INTERVAL; // Reset timera
            m_CurrentQuestState = QuestEventState::WaitingForTimer;

            // RESPRAWN STAREJ TAŚMY!
            for (auto& pair : m_ReplacedByQuestGroup) {
                // Skoro ją usunęliśmy, to jest martwa, musimy zbudować nową
                auto builder = GetScene()->GetWorld().BuildEntity();

                TransformComponent tc;
                tc.SetPosition(glm::vec3(-9.0f, pair.second, -1.0f)); // Oryginalna pozycja Y zapisana wcześniej
                tc.SetRotation(glm::vec3(0.0f, glm::radians(180.0f), 0.0f)); // Twój kąt z JSON
                builder.With<TransformComponent>(tc);

                MeshComponent mesh;
                mesh.ModelPtr = AssetManager::GetModel("assets://models/przybory_kuchenne/tasma/base/conveyor_base.gltf");
                builder.With<MeshComponent>(mesh);

                BoxColliderComponent collider;
                collider.Offset = glm::vec3(0.0f, 0.5f, 0.0f);
                collider.Size = glm::vec3(1.0f, 0.5f, 1.0f);
                builder.With<BoxColliderComponent>(collider);

                NativeScriptComponent nsc;
                nsc.AddScript<ConveyorScript>("ConveyorScript");
                builder.With<NativeScriptComponent>(nsc);

                builder.With<TagComponent>({ "tasma_17" });

                Entity newBelt = builder.Build();

                // Dodanie animowanego pasa transmisyjnego jako child
                auto beltBuilder = GetScene()->GetWorld().BuildEntity();
                TransformComponent btc;
                btc.SetPosition(glm::vec3(0.0f, 0.01f, 0.0f));
                beltBuilder.With<TransformComponent>(btc);
                MeshComponent bmesh;
                bmesh.ModelPtr = AssetManager::GetModel("assets://models/przybory_kuchenne/tasma/belt/conveyor_belt.gltf");
                beltBuilder.With<MeshComponent>(bmesh);
                NativeScriptComponent bnsc;
                bnsc.AddScript<BeltVisualScript>("BeltVisualScript");
                beltBuilder.With<NativeScriptComponent>(bnsc);
                Entity visualBelt = beltBuilder.Build();

                GetScene()->SetParent(visualBelt, newBelt);

                // Zapisujemy nowe ID, na wypadek kolejnego questa
                pair.first = newBelt;
            }

            GetScene()->RebuildConveyorCache();
            spdlog::info("Koniec cyklu Questa. Zwykle tasmy przywrocone (respawn).");
        }
        break;
    }
    }
}


void GameManagerScript::AcceptQuest()
{
    if (m_CurrentQuestState == QuestEventState::WaitingForAccept) {
        spdlog::info("Quest Zaakceptowany! Rozbudowuje glowna wyspe.");
        m_AnimationProgress = 0.0f;
        m_CurrentQuestProgress = 0;
        m_CurrentQuestState = QuestEventState::QuestActive;
    }
}

void GameManagerScript::SkipQuest()
{
    if (m_CurrentQuestState == QuestEventState::WaitingForAccept) {
        if (m_SkipsLeft > 0) {
            m_SkipsLeft--;
            m_CurrentQuestIndex++;
            if (m_CurrentQuestIndex >= m_AvailableQuests.size()) m_CurrentQuestIndex = 0; // Zapętlenie listy
            spdlog::info("Quest pominiety! Pozostalo pominięc: {}", m_SkipsLeft);
            // Tutaj w następnym etapie odświeżymy wizualnie HUD z nowym zadaniem
        }
        else {
            spdlog::warn("Brak pominiec!");
            // Ewentualnie wymuszamy QuestActive
        }
    }
}

void GameManagerScript::CompleteQuest()
{
    if (m_CurrentQuestState == QuestEventState::QuestActive) {
        spdlog::info("Quest Zrealizowany! Rozpoczynam odlot wszystkiego.");
        m_AnimationProgress = 0.0f;
        m_SkipsLeft = 3;

        QuestData currentQuest = m_AvailableQuests[m_CurrentQuestIndex];
        AddMoney(currentQuest.RewardCoins);

        m_CurrentQuestIndex++;
        if (m_CurrentQuestIndex >= m_AvailableQuests.size()) m_CurrentQuestIndex = 0;

        m_CurrentQuestState = QuestEventState::IslandLeaving;
        SpawnCollectibleFlag(currentQuest.RewardFlag);
        m_CurrentQuestProgress = 0;
    }
}

void GameManagerScript::DeliverQuestPortion()
{
    if (m_CurrentQuestState == QuestEventState::QuestActive) {
        m_CurrentQuestProgress++;
        QuestData* q = GetCurrentQuest();
        spdlog::info("Dostarczono porcje AI! Stan: {} / {}", m_CurrentQuestProgress, q ? q->Portions : 0);

        if (q && m_CurrentQuestProgress >= q->Portions) {
            CompleteQuest();
        }
    }
}

void GameManagerScript::SpawnCollectibleFlag(const std::string& countryCode)
{
    // Znajdź wydawkę po tagu
    auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
    Entity wydawkaEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    if (tags) {
        for (size_t i = 0; i < tags->dense.size(); ++i) {
            if (tags->dense[i].Tag == "Wydawka") {
                wydawkaEntity = tags->reverse[i];
                break;
            }
        }
    }

    glm::vec3 spawnPos = { 12.0f, 1.5f, -9.0f }; // Twardy fallback obok wydawki

    if (wydawkaEntity.id != std::numeric_limits<std::size_t>::max()) {
        auto* tf = GetScene()->GetWorld().GetComponent<TransformComponent>(wydawkaEntity);
        // Przesunięcie o +3.0 w osi X (w prawo) i +1.5 w osi Y (żeby nie wbijała się w ziemię)
        if (tf) spawnPos = tf->GetPosition() + glm::vec3(3.0f, 1.5f, 0.0f);
    }

    // Usuń poprzednią flagę jeśli istnieje
    if (m_ActiveFlagEntity.id != std::numeric_limits<std::size_t>::max()) {
        GetScene()->GetWorld().GetEventBus().Publish(
            EntityDestroyRequestEvent{ m_ActiveFlagEntity });
        m_ActiveFlagEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    }

    auto& world = GetScene()->GetWorld();
    auto builder = world.BuildEntity();

    builder.With<TagComponent>({ "CollectibleFlag" });

    TransformComponent tc;
    tc.SetPosition(spawnPos);
    tc.SetScale(glm::vec3(1.0f));
    builder.With<TransformComponent>(tc);

    MeshComponent mesh;
    mesh.ModelPtr = AssetManager::GetModel("assets://models/wystroj/flaga.gltf");
    builder.With<MeshComponent>(mesh);

    NativeScriptComponent nsc;
    nsc.AddScript<FlagScript>("FlagScript");
    builder.With<NativeScriptComponent>(nsc);

    m_ActiveFlagEntity = builder.Build();

    // Ustaw teksturę flagi - potrzebujemy dostępu do FlagScript po spawnie
    // FlagScript::OnCreate zostanie wywołany w następnej klatce przez Scene::OnUpdateRuntime
    // Zamiast tego, przechowaj kod kraju żeby FlagScript mógł go użyć w OnCreate
    // Najprostsze rozwiązanie: tag zawiera kod kraju
    auto* tagComp = world.GetComponent<TagComponent>(m_ActiveFlagEntity);
    if (tagComp) tagComp->Tag = "CollectibleFlag_" + countryCode;

    spdlog::info("[GameManager] Spawning flag for country: {}", countryCode);
}