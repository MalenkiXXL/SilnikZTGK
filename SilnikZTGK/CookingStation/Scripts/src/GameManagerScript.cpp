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

    m_DishCreatedSubId = bus.Subscribe<DishCreatedEvent>(
        [this](const DishCreatedEvent& e) {
            m_DishMemory[e.FoodEntity.id] = e.History;
        }
    );

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
    AddIngredients(IngredientType::Apple, 5);
    AddIngredients(IngredientType::Raspberry, 5);
    AddIngredients(IngredientType::Strawberry, 5);
    AddIngredients(IngredientType::CoffeeBeans, 5);


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

    std::vector<std::string> eventIslandNames = { "event_78", "event-detal", "balony1", "balony2", "balony3", "balony4" };
    for (const auto& name : eventIslandNames) {
        Entity e = findEntityByName(name);
        auto* transform = GetScene()->GetWorld().GetComponent<TransformComponent>(e);

        if (transform) {
            if (name == "budka" && transform->GetPosition().x > -20.0f) continue;

            float origX = transform->GetPosition().x;
            m_EventIslandGroup.push_back({ e, origX });

            glm::vec3 pos = transform->GetPosition();
            pos.x = origX - 60.0f; 
            transform->SetPosition(pos);
        }
    }

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
            pos.y = origY + 30.0f; 
            transform->SetPosition(pos);
        }
    }

    auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
    if (scripts) {
        for (size_t i = 0; i < scripts->dense.size(); ++i) {
            for (auto& s : scripts->dense[i].Scripts) {
                if (s.Name == "DeliveryBoothScript" && s.Instance) {
                    auto* booth = static_cast<DeliveryBoothScript*>(s.Instance);
                    booth->m_DirectionOffset = { 0.0f, 0.0f, 2.0f };
                    break;
                }
            }
        }
    }

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
    m_TotalMoneyEarned += static_cast<int>(e.RewardAmount); // Dodajemy do całkowitego zarobku

    spdlog::info("Order fulfilled! Reward added: {}", e.RewardAmount);

    m_CustomersServed++;

    if (m_CustomersServed >= MAX_CUSTOMERS)
    {
        int stars = 0;

        if (m_TotalMoneyEarned >= 501) {
            stars = 3;
        }
        else if (m_TotalMoneyEarned >= 201) {
            stars = 2;
        }
        else if (m_TotalMoneyEarned >= 51) {
            stars = 1;
        }

        spdlog::info("Poziom ukończony! Zarobiono łącznie: {}, Gwiazdki: {}", m_TotalMoneyEarned, stars);
        GetScene()->GetWorld().GetEventBus().Publish(LevelCompletedEvent{ m_TotalMoneyEarned, stars });
    }
}

void GameManagerScript::OnUpdate(Timestep ts)
{
    if (m_MoneyWarningTimer > 0.0f) {
        m_MoneyWarningTimer -= ts;
    }

    static float s_PythonCooldown = 0.0f;
    if (s_PythonCooldown > 0.0f) s_PythonCooldown -= ts;

    if (Input::IsKeyPressed(80) && s_PythonCooldown <= 0.0f) {
        spdlog::warn("KLAWISZ P: Wymuszono generacje nowych questow AI! (Gra moze na chwile zaciac...)");
        system("python CookingStation\\Tools\\QuestGenerator\\main.py");
        m_AvailableQuests = QuestManager::LoadQuests("assets://wygenerowane_quests.json");
        m_CurrentQuestIndex = 0;
        s_PythonCooldown = 10.0f;
        spdlog::info("Gotowe! Nowe questy wczytane do gry.");
    }

    /*
    //finaly 

    if (m_NewQuestsReady) {
        m_AvailableQuests = QuestManager::LoadQuests("assets://wygenerowane_quests.json");
        m_NewQuestsReady = false;
        m_IsGeneratingQuests = false;
        spdlog::info("Nowe questy pobrane w tle i zaladowane do gry bez scinki!");
    }

    if (Input::IsKeyPressed(80) && s_PythonCooldown <= 0.0f && !m_IsGeneratingQuests) {
        spdlog::warn("KLAWISZ P: Odpalam generator questow w tle...");
        m_IsGeneratingQuests = true;
        s_PythonCooldown = 10.0f;

        std::thread([](GameManagerScript* manager) {
            system("python CookingStation\\Tools\\QuestGenerator\\main.py");
            manager->m_NewQuestsReady = true;
        }, this).detach();
    }
    */

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
        break;
    }
    case QuestEventState::QuestActive:
    {
        if (m_AnimationProgress < 1.0f) {
            m_AnimationProgress += ts * 0.5f;
            bool finishedNow = false; 
            if (m_AnimationProgress >= 1.0f) {
                m_AnimationProgress = 1.0f;
                finishedNow = true;
            }

            float easeOut = 1.0f - (1.0f - m_AnimationProgress) * (1.0f - m_AnimationProgress);

            float yOffset = 30.0f * (1.0f - easeOut);
            for (auto& pair : m_MainIslandQuestGroup) {
                auto* transform = GetScene()->GetWorld().GetComponent<TransformComponent>(pair.first);
                if (transform) {
                    glm::vec3 pos = transform->GetPosition();
                    pos.y = pair.second + yOffset;
                    transform->SetPosition(pos);
                }
            }

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

            if (finishedNow) {
                for (auto& pair : m_ReplacedByQuestGroup) {
                    Entity e = pair.first;
                    if (e.id != std::numeric_limits<std::size_t>::max()) {
                        GetScene()->DestroyEntity(e);
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

        float xOffset = -60.0f * easeIn;
        for (auto& pair : m_EventIslandGroup) {
            auto* transform = GetScene()->GetWorld().GetComponent<TransformComponent>(pair.first);
            if (transform) {
                glm::vec3 pos = transform->GetPosition();
                pos.x = pair.second + xOffset;
                transform->SetPosition(pos);
            }
        }

        float yOffsetMain = 30.0f * easeIn;
        for (auto& pair : m_MainIslandQuestGroup) {
            auto* transform = GetScene()->GetWorld().GetComponent<TransformComponent>(pair.first);
            if (transform) {
                glm::vec3 pos = transform->GetPosition();
                pos.y = pair.second + yOffsetMain;
                transform->SetPosition(pos);
            }
        }

        float showProgress = std::min(1.0f, m_AnimationProgress / 0.2f);
        float showOffset = -30.0f * (1.0f - showProgress);

        for (auto& pair : m_ReplacedByQuestGroup) {
            auto* transform = GetScene()->GetWorld().GetComponent<TransformComponent>(pair.first);
            if (transform) {
                glm::vec3 pos = transform->GetPosition();
                pos.y = pair.second + showOffset;
                transform->SetPosition(pos);
            }
        }

        if (m_AnimationProgress >= 1.0f) {
            m_QuestTimer = QUEST_INTERVAL; 
            m_CurrentQuestState = QuestEventState::WaitingForTimer;
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
            if (m_CurrentQuestIndex >= m_AvailableQuests.size()) m_CurrentQuestIndex = 0;

            /*
            // finaly
            // if (!m_AvailableQuests.empty()) m_AvailableQuests.erase(m_AvailableQuests.begin());
            //
            // if (m_AvailableQuests.empty() && !m_IsGeneratingQuests) {
            //     spdlog::warn("Pula questow pusta po pominieciu! Odpalam generator w tle...");
            //     m_IsGeneratingQuests = true;
            //     std::thread([](GameManagerScript* manager) {
            //         system("python CookingStation\\Tools\\QuestGenerator\\main.py");
            //         manager->m_NewQuestsReady = true;
            //     }, this).detach();
            // }
            */

            spdlog::info("Quest pominiety! Pozostalo pominięc: {}", m_SkipsLeft);
        }
        else {
            spdlog::warn("Brak pominiec!");
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

        /*
        //finaly
        // if (!m_AvailableQuests.empty()) m_AvailableQuests.erase(m_AvailableQuests.begin());
        //
        // if (m_AvailableQuests.empty() && !m_IsGeneratingQuests) {
        //     spdlog::warn("Pula questow pusta! Odpalam generator w tle...");
        //     m_IsGeneratingQuests = true;
        //     std::thread([](GameManagerScript* manager) {
        //         system("python CookingStation\\Tools\\QuestGenerator\\main.py");
        //         manager->m_NewQuestsReady = true;
        //     }, this).detach();
        // }
        */

        m_CurrentQuestState = QuestEventState::IslandLeaving;
        SpawnCollectibleFlag(currentQuest.RewardFlag);
        m_CurrentQuestProgress = 0;

        for (auto& pair : m_ReplacedByQuestGroup) {
            auto builder = GetScene()->GetWorld().BuildEntity();

            TransformComponent tc;
            tc.SetPosition(glm::vec3(-9.0f, pair.second - 30.0f, -1.0f));
            tc.SetRotation(glm::vec3(0.0f, 180.0f, 0.0f));
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

            pair.first = newBelt;
        }

        for (auto& pair : m_MainIslandQuestGroup) {
            auto* tagComp = GetScene()->GetWorld().GetComponent<TagComponent>(pair.first);
            if (tagComp && tagComp->Tag == "tasma_switch_quest") {
                auto* tf = GetScene()->GetWorld().GetComponent<TransformComponent>(pair.first);
                if (tf) {
                    glm::vec3 rot = tf->GetRotation();
                    rot.y = 180.0f;
                    tf->SetRotation(rot);
                }
                auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(pair.first);
                if (nsc) {
                    for (auto& s : nsc->Scripts) {
                        if (s.Instance) {
                            static_cast<ConveyorScript*>(s.Instance)->SetPushDirection();
                        }
                    }
                }
            }
        }
        GetScene()->RebuildConveyorCache();
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

    glm::vec3 spawnPos = { 12.0f, 1.0f, -9.0f }; 

    if (wydawkaEntity.id != std::numeric_limits<std::size_t>::max()) {
        auto* tf = GetScene()->GetWorld().GetComponent<TransformComponent>(wydawkaEntity);
        if (tf) {
            int flagsPerRow = 3; 
            int row = (m_CollectedFlagsCount / flagsPerRow) % 2;
            int col = m_CollectedFlagsCount % flagsPerRow;

            
            float startX = tf->GetPosition().x - 1.2f;
            float spacingX = 1.2f; 
            float startZ = tf->GetPosition().z + 0.8f;
            float spacingZ = -0.8f; 

            spawnPos.x = startX + (col * spacingX);
            spawnPos.y = 3.76f;
            spawnPos.z = startZ + (row * spacingZ);
        }
    }


    auto& world = GetScene()->GetWorld();
    auto builder = world.BuildEntity();

    builder.With<TagComponent>({ "CollectibleFlag" });

    TransformComponent tc;
    tc.SetPosition(spawnPos);
    tc.SetScale(glm::vec3(0.80f)); 
    tc.SetRotation(glm::vec3(0.0f, glm::radians(20.0f), 0.0f));
    builder.With<TransformComponent>(tc);

    MeshComponent mesh;
    mesh.ModelPtr = AssetManager::GetModel("assets://models/wystroj/flaga.gltf");
    builder.With<MeshComponent>(mesh);

    NativeScriptComponent nsc;
    nsc.AddScript<FlagScript>("FlagScript");
    builder.With<NativeScriptComponent>(nsc);

    Entity newFlagEntity = builder.Build();

    auto* tagComp = world.GetComponent<TagComponent>(newFlagEntity);
    if (tagComp) tagComp->Tag = "CollectibleFlag_" + countryCode;

    spdlog::info("[GameManager] Spawning flag for country: {}", countryCode);
    m_CollectedFlagsCount++;
}