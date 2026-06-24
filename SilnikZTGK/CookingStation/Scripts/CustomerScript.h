#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scene/ecs.h" 
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Scripts/Managers/GameManagerScript.h" 
#include "CookingStation/Scripts/PoofEmitterScript.h"
#include <string>
#include <vector>
#include <spdlog/spdlog.h>
#include <random>

enum class CustomerState { Spawning, WalkingToChair, Seated, LeavingReaction };

class CustomerScript : public ScriptableEntity
{
public:
    static inline Entity s_GrandmaTargetChair = { std::numeric_limits<std::size_t>::max(), 0 };
    static inline glm::vec3 s_GrandmaTargetPos = { 0.0f, 0.0f, 0.0f };
    static inline glm::vec3 s_GrandmaFinalRotation = { 0.0f, 0.0f, 0.0f };

    CustomerState State = CustomerState::Spawning;
    bool ReachedWaypoint = false;

    Entity TargetChair = { std::numeric_limits<std::size_t>::max(), 0 };
    glm::vec3 TargetPos = { 0.0f, 0.0f, 0.0f };
    glm::vec3 FinalRotation = { 0.0f, 0.0f, 0.0f };
    bool IsGrandma = false;

    float m_ReactionTimer = 0.0f;
    bool m_WasCorrect = false;

    std::size_t m_ValidationResponseSubId = 0;
    bool IsPendingDestroy = false;

    IngredientType WantedIngredient = IngredientType::None;
    OrderSecondaryRequirement SecondaryReq;

    bool IsServed = false;
    bool OrderTaken = false;

    Entity m_ReceivedFood = { std::numeric_limits<std::size_t>::max(), 0 };

    std::size_t m_ServedSubId = 0;
    std::size_t m_OrderSubId = 0;
    float OrderPrice = 50.0f;

    // Zmienne do efekt�w na wej�cie
    float m_SpawnTimer = 0.0f;
    bool m_PoofPlayed = false;
    bool m_PoofStarted = false;
    Entity m_PoofEntity = { std::numeric_limits<std::size_t>::max(), 0 };

    // Zmienne do efekt�w na znikni�cie
    bool m_ExitPoofStarted = false;
    Entity m_ExitPoofEntity = { std::numeric_limits<std::size_t>::max(), 0 };

    void OnCreate() override
    {
        auto* tagComp = GetComponent<TagComponent>();
        IsGrandma = (tagComp && tagComp->Tag == "GrandmaCustomer");

        std::vector<IngredientType> menu = { IngredientType::Tomato };
        std::random_device rd;
        std::mt19937 gen(rd());

        if (IsGrandma) {
            WantedIngredient = IngredientType::Sandwich;
            SecondaryReq.RequirementType = OrderSecondaryRequirement::Type::None;
            State = CustomerState::Spawning;

            TargetChair = s_GrandmaTargetChair;
            TargetPos = s_GrandmaTargetPos;
            FinalRotation = s_GrandmaFinalRotation;
            m_SpawnTimer = 0.0f;
            m_PoofPlayed = false;
            m_PoofStarted = false;
            m_ExitPoofStarted = false;
        }
        else {
            struct Combo {
                IngredientType Primary;
                OrderSecondaryRequirement Secondary;
            };

            std::vector<Combo> combos = {
                { IngredientType::Tomato, { OrderSecondaryRequirement::Type::Machine, IngredientType::None, "Garnek", "assets://UI/pot.png" } },
                { IngredientType::Tomato, { OrderSecondaryRequirement::Type::Machine, IngredientType::None, "Deska", "assets://UI/cuttingBoardMachine.png" } },
                { IngredientType::Tomato, { OrderSecondaryRequirement::Type::Machine, IngredientType::None, "Patelnia", "assets://UI/pan.png" } },
                { IngredientType::Tomato, { OrderSecondaryRequirement::Type::Ingredient, IngredientType::Cheese, "", "" } },
                { IngredientType::Tomato, { OrderSecondaryRequirement::Type::Ingredient, IngredientType::Ham, "", "" } },

                { IngredientType::Cheese, { OrderSecondaryRequirement::Type::Machine, IngredientType::None, "Deska", "assets://UI/cuttingBoardMachine.png" } },
                { IngredientType::Cheese, { OrderSecondaryRequirement::Type::Ingredient, IngredientType::Tomato, "", "" } },

                { IngredientType::Ham, { OrderSecondaryRequirement::Type::Machine, IngredientType::None, "Deska", "assets://UI/cuttingBoardMachine.png" } },
                { IngredientType::Ham, { OrderSecondaryRequirement::Type::Machine, IngredientType::None, "Patelnia", "assets://UI/pan.png" } },

                { IngredientType::Flour, { OrderSecondaryRequirement::Type::Ingredient, IngredientType::Milk, "", "" } },
                { IngredientType::Flour, { OrderSecondaryRequirement::Type::Machine, IngredientType::None, "Mikser", "assets://UI/blender.png" } },
                { IngredientType::Milk, { OrderSecondaryRequirement::Type::Machine, IngredientType::None, "Mikser", "assets://UI/blender.png" } },
                { IngredientType::Flour, { OrderSecondaryRequirement::Type::Machine, IngredientType::None, "Piekarnik", "assets://UI/oven.png" } }
            };

            std::uniform_int_distribution<> distCombo(0, (int)combos.size() - 1);
            Combo selected = combos[distCombo(gen)];

            WantedIngredient = selected.Primary;
            SecondaryReq = selected.Secondary;

            State = CustomerState::Spawning;
            m_SpawnTimer = 0.0f;
            m_PoofPlayed = false;
            m_PoofStarted = false;
            m_ExitPoofStarted = false;
        }

        OrderTaken = false;

        std::vector<float> prices = { 25.0f, 50.0f, 75.0f, 100.0f };
        std::uniform_int_distribution<> priceDist(0, (int)prices.size() - 1);
        OrderPrice = prices[priceDist(gen)];

        auto& bus = GetScene()->GetWorld().GetEventBus();

        m_ServedSubId = bus.Subscribe<CustomerServedEvent>([this](const CustomerServedEvent& e) {
            if (e.Customer.id == m_Entity.id) {
                m_ReceivedFood = e.ServedFood;
                GetScene()->GetWorld().GetEventBus().Publish(ValidateOrderRequestEvent{
                    m_Entity, e.ServedFood, WantedIngredient, SecondaryReq
                    });
            }
            });

        m_ValidationResponseSubId = bus.Subscribe<ValidateOrderResponseEvent>([this](const ValidateOrderResponseEvent& e) {
            if (e.Customer.id == m_Entity.id) { this->ReceiveFood(e.IsCorrect); }
            });

        m_OrderSubId = bus.Subscribe<OrderTakenEvent>([this](const OrderTakenEvent& e) {
            if (e.Customer.id == m_Entity.id) {
                this->OrderTaken = true;
                GetScene()->GetWorld().GetEventBus().Publish(KitchenOrderPlacedEvent{ m_Entity, WantedIngredient });
            }
            });
    }

    void OnUpdate(Timestep ts) override
    {
        if (State == CustomerState::Spawning)
        {
            float dt = (float)ts.GetSeconds();
            if (dt > 0.5f) dt = 0.016f;

            if (!m_PoofPlayed)
            {
                m_PoofPlayed = true;

                auto* tf = GetComponent<TransformComponent>();
                if (tf)
                {
                    TransformComponent poofTf;
                    glm::vec3 targetPos = tf->GetPosition() + glm::vec3(0.0f, 1.0f, 0.0f);
                    poofTf.SetPosition(targetPos);
                    poofTf.SetScale(glm::vec3(1.0f));

                    poofTf.WorldMatrix[3][0] = targetPos.x;
                    poofTf.WorldMatrix[3][1] = targetPos.y;
                    poofTf.WorldMatrix[3][2] = targetPos.z;

                    NativeScriptComponent poofNsc;
                    poofNsc.AddScript<PoofEmitterScript>("PoofEmitterScript");

                    m_PoofEntity = GetScene()->GetWorld().BuildEntity()
                        .With<TagComponent>({ "PoofEmitter" })
                        .With<TransformComponent>(poofTf)
                        .With<NativeScriptComponent>(poofNsc)
                        .Build();
                }
            }

            if (!m_PoofStarted && m_PoofEntity.id != std::numeric_limits<std::size_t>::max())
            {
                auto* addedNsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_PoofEntity);
                if (addedNsc && !addedNsc->Scripts.empty() && addedNsc->Scripts[0].Instance)
                {
                    static_cast<PoofEmitterScript*>(addedNsc->Scripts[0].Instance)->Play();
                    m_PoofStarted = true;
                    spdlog::info("PoofEmitter uruchomiony poprawnie (Spawn)");
                }
            }

            m_SpawnTimer += dt;

            if (m_SpawnTimer >= 2.0f)
            {
                if (m_PoofEntity.id != std::numeric_limits<std::size_t>::max())
                {
                    GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_PoofEntity });
                    m_PoofEntity = { std::numeric_limits<std::size_t>::max(), 0 };
                }

                if (IsGrandma)
                {
                    State = CustomerState::WalkingToChair;
                    auto* animator = GetComponent<AnimatorComponent>();
                    if (animator && animator->AnimatorInstance) {
                        animator->AnimatorInstance->PlayAnimation("Walk");
                    }
                }
                else
                {
                    State = CustomerState::Seated;
                    GetScene()->GetWorld().GetEventBus().Publish(CustomerSeatedEvent{ m_Entity });
                    GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{ m_Entity, { 1.0f, 0.8f, 0.2f }, 0.0f, true });
                }
            }
            return;
        }

        // 1. Sprawdzanie czy klient jest w trakcie odchodzenia (po zjedzeniu)
        if (State == CustomerState::LeavingReaction) {
            m_ReactionTimer -= ts.GetSeconds();

            if (m_ReactionTimer <= 0.0f && !IsPendingDestroy) {
                // Gdy minie 2s na reakcj� (bu�k�), odpalamy puffa i znikamy klienta
                if (!m_ExitPoofStarted) {
                    m_ExitPoofStarted = true;
                    m_ReactionTimer = 2.0f; // Czas potrzebny, aby poof opad� przed usuni�ciem klienta z pami�ci

                    auto* tf = GetComponent<TransformComponent>();
                    if (tf)
                    {
                        TransformComponent poofTf;
                        glm::vec3 targetPos = tf->GetPosition() + glm::vec3(0.0f, 1.0f, 0.0f);
                        poofTf.SetPosition(targetPos);
                        poofTf.SetScale(glm::vec3(1.0f));

                        poofTf.WorldMatrix[3][0] = targetPos.x;
                        poofTf.WorldMatrix[3][1] = targetPos.y;
                        poofTf.WorldMatrix[3][2] = targetPos.z;

                        NativeScriptComponent poofNsc;
                        poofNsc.AddScript<PoofEmitterScript>("PoofEmitterScript");

                        m_ExitPoofEntity = GetScene()->GetWorld().BuildEntity()
                            .With<TagComponent>({ "PoofEmitter" })
                            .With<TransformComponent>(poofTf)
                            .With<NativeScriptComponent>(poofNsc)
                            .Build();

                        auto* addedNsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_ExitPoofEntity);
                        if (addedNsc && !addedNsc->Scripts.empty() && addedNsc->Scripts[0].Instance)
                        {
                            static_cast<PoofEmitterScript*>(addedNsc->Scripts[0].Instance)->Play();
                        }

                        // Z�udzenie znikni�cia: Ukrywamy klienta g��boko pod map�
                        glm::vec3 hidePos = tf->GetPosition();
                        hidePos.y -= 100.0f;
                        tf->SetPosition(hidePos);
                    }
                }
                else {
                    // Puff zako�czy� dzia�anie - ca�kowicie niszczymy encje
                    if (m_ExitPoofEntity.id != std::numeric_limits<std::size_t>::max()) {
                        GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_ExitPoofEntity });
                        m_ExitPoofEntity = { std::numeric_limits<std::size_t>::max(), 0 };
                    }
                    IsPendingDestroy = true;
                    GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_Entity });
                }
            }
            return;
        }

        if (State == CustomerState::WalkingToChair)
        {
            auto* tf = GetComponent<TransformComponent>();
            if (!tf) return;

            glm::vec3 pos = tf->GetPosition();
            glm::vec3 currentTarget = ReachedWaypoint ? TargetPos : glm::vec3(-9.0f, TargetPos.y, -37.0f);

            glm::vec3 dir = currentTarget - pos;
            dir.y = 0.0f;

            float dist = glm::length(dir);
            float speed = 1.4f;
            float step = speed * (float)ts.GetSeconds();

            if (dist <= step || dist < 0.8f)
            {
                if (!ReachedWaypoint) {
                    ReachedWaypoint = true;
                    pos.x = currentTarget.x;
                    pos.z = currentTarget.z;
                    tf->SetPosition(pos);
                }
                else {
                    tf->SetPosition(TargetPos);
                    tf->SetRotation(FinalRotation);
                    State = CustomerState::Seated;

                    s_GrandmaTargetChair = { std::numeric_limits<std::size_t>::max(), 0 };

                    auto* animator = GetComponent<AnimatorComponent>();
                    if (animator && animator->AnimatorInstance) {
                        animator->AnimatorInstance->PlayAnimation("SitIdle");
                    }

                    auto& bus = GetScene()->GetWorld().GetEventBus();
                    GetScene()->GetWorld().GetEventBus().Publish(CustomerSeatedEvent{ m_Entity });
                    GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{ m_Entity, { 1.0f, 0.8f, 0.2f }, 0.0f, true });
                }
            }
            else
            {
                dir = glm::normalize(dir);
                pos += dir * step;
                tf->SetPosition(pos);

                float angle = glm::degrees(std::atan2(dir.x, dir.z));
                tf->SetRotation({ 0.0f, angle, 0.0f });
            }
        }
    }


    bool IsOrderMatching(const std::vector<IngredientType>& ingredientsOnPlate)
    {
        if (ingredientsOnPlate.empty()) return false;
        for (const auto& item : ingredientsOnPlate)
        {
            if (item == WantedIngredient) return true;
        }
        return false;
    }

    virtual void ReceiveFood(bool isCorrectOrder = true)
    {
        if (State == CustomerState::LeavingReaction || IsPendingDestroy) return;
        IsServed = true;

        if (m_ReceivedFood.id != std::numeric_limits<std::size_t>::max()) {
            GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_ReceivedFood });
            m_ReceivedFood = { std::numeric_limits<std::size_t>::max(), 0 };
        }

        auto* tagComp = GetComponent<TagComponent>();
        glm::vec3 highlightColor = { 1.0f, 1.0f, 1.0f };

        if (isCorrectOrder) {
            if (tagComp) tagComp->Tag = "ZadowolonyKlient";
            highlightColor = { 0.1f, 1.0f, 0.2f };
            if (GameManagerScript::s_Instance) GetScene()->GetWorld().GetEventBus().Publish(OrderFulfilledEvent(OrderPrice));
        }
        else {
            if (tagComp) tagComp->Tag = "ZlyKlient";
            highlightColor = { 1.0f, 0.1f, 0.1f };
            if (GameManagerScript::s_Instance) GetScene()->GetWorld().GetEventBus().Publish(OrderFulfilledEvent(0.0f));
        }

        GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{ m_Entity, highlightColor, 2.0f, false });
        m_WasCorrect = isCorrectOrder;

        // Uruchamiamy odliczanie do znikni�cia/puffa
        State = CustomerState::LeavingReaction;
        m_ReactionTimer = 2.0f;
    }

    void OnDestroy() override
    {
        auto* scene = GetScene();
        if (scene) {
            auto& bus = scene->GetWorld().GetEventBus();
            if (m_ServedSubId != 0) bus.Unsubscribe<CustomerServedEvent>(m_ServedSubId);
            if (m_OrderSubId != 0) bus.Unsubscribe<OrderTakenEvent>(m_OrderSubId);
            if (m_ValidationResponseSubId != 0) bus.Unsubscribe<ValidateOrderResponseEvent>(m_ValidationResponseSubId);
        }
        if (IsGrandma && State == CustomerState::WalkingToChair) {
            s_GrandmaTargetChair = { std::numeric_limits<std::size_t>::max(), 0 };
        }

        if (m_PoofEntity.id != std::numeric_limits<std::size_t>::max())
        {
            GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_PoofEntity });
        }
        if (m_ExitPoofEntity.id != std::numeric_limits<std::size_t>::max())
        {
            GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_ExitPoofEntity });
        }
    }
};