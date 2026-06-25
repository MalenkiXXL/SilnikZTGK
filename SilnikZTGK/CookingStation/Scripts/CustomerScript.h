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
    bool m_GrandmaSuccessCutscene = false;
    bool m_MapUnlocked = false;

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
    float AwardedTip = 0.0f;

    // Zmienne do efekt�w na wej�cie
    float m_SpawnTimer = 0.0f;
    bool m_PoofPlayed = false;
    bool m_PoofStarted = false;
    Entity m_PoofEntity = { std::numeric_limits<std::size_t>::max(), 0 };

    // Zmienne do efekt�w na znikni�cie
    bool m_ExitPoofStarted = false;
    Entity m_ExitPoofEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    Entity m_CutsceneSmokeEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    bool m_CameraCaptured = false;
    glm::vec3 m_OriginalCameraPos = { 0.0f, 0.0f, 0.0f };
    float m_OriginalCameraZoom = 45.0f;
    float m_SeatedTimer = 0.0f;

    void OnCreate() override
    {
        auto* tagComp = GetComponent<TagComponent>();
        IsGrandma = (tagComp && tagComp->Tag == "GrandmaCustomer");

        std::vector<IngredientType> menu = { IngredientType::Tomato };
        std::random_device rd;
        std::mt19937 gen(rd());

        if (IsGrandma) {
            WantedIngredient = IngredientType::Tomato;
            SecondaryReq = { OrderSecondaryRequirement::Type::Machine, IngredientType::None, "Garnek", "assets://UI/pot.png" };
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
                { IngredientType::Tomato,      { OrderSecondaryRequirement::Type::Machine,     IngredientType::None,        "Garnek",   "assets://UI/pot.png"     } },
                { IngredientType::Tomato,      { OrderSecondaryRequirement::Type::Machine,     IngredientType::None,        "Patelnia", "assets://UI/pan.png"     } },
                { IngredientType::Tomato,      { OrderSecondaryRequirement::Type::Ingredient,  IngredientType::Ham,         "",         ""                        } },
                { IngredientType::Cheese,      { OrderSecondaryRequirement::Type::Ingredient,  IngredientType::Tomato,      "",         ""                        } },
                { IngredientType::Ham,         { OrderSecondaryRequirement::Type::Machine,     IngredientType::None,        "Patelnia", "assets://UI/pan.png"     } },
                { IngredientType::Flour,       { OrderSecondaryRequirement::Type::Ingredient,  IngredientType::Milk,        "",         ""                        } },
                { IngredientType::Milk,        { OrderSecondaryRequirement::Type::Machine,     IngredientType::None,        "Mikser",   "assets://UI/blender.png" } },
                { IngredientType::Flour,       { OrderSecondaryRequirement::Type::Machine,     IngredientType::None,        "Piekarnik","assets://UI/oven.png"    } },
                { IngredientType::Tomato,      { OrderSecondaryRequirement::Type::Ingredient,  IngredientType::Mozzarella,  "",         ""                        } },
                { IngredientType::Egg,         { OrderSecondaryRequirement::Type::Machine,     IngredientType::None,        "Patelnia", "assets://UI/pan.png"     } },
            };

            if (GameManagerScript::s_GrandmaServed) {
                combos.insert(combos.end(), {
                    { IngredientType::Apple,       { OrderSecondaryRequirement::Type::Ingredient, IngredientType::Flour,      "",         ""                        } },
                    { IngredientType::Strawberry,  { OrderSecondaryRequirement::Type::Machine,    IngredientType::None,       "Mikser",   "assets://UI/blender.png" } },
                    { IngredientType::CoffeeBeans, { OrderSecondaryRequirement::Type::Machine,    IngredientType::None,       "Mikser",   "assets://UI/blender.png" } },
                    { IngredientType::Raspberry,   { OrderSecondaryRequirement::Type::Machine,    IngredientType::None,       "Mikser",   "assets://UI/blender.png" } },
                    { IngredientType::Raspberry,   { OrderSecondaryRequirement::Type::Machine,    IngredientType::None,       "Piekarnik","assets://UI/oven.png"    } },
                    { IngredientType::CoffeeBeans, { OrderSecondaryRequirement::Type::Machine,    IngredientType::None,       "Ekspres",  "assets://UI/coffeeMachine.png"} },
                    { IngredientType::Raspberry,   { OrderSecondaryRequirement::Type::Ingredient, IngredientType::SleepyDust, "",         ""                        } },
                    { IngredientType::Potato,      { OrderSecondaryRequirement::Type::Machine,    IngredientType::None,       "Mikser",   "assets://UI/blender.png" } },
                    });
            }

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
            if (e.Customer.id == m_Entity.id) {
                this->ReceiveFood(e.IsCorrect, e.HasExtraIngredients); 
            }
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
        float dt = (float)ts.GetSeconds();
        if (dt > 0.5f) dt = 0.016f;

        if (!m_PoofPlayed)
        {
            m_PoofPlayed = true;

            auto* tf = GetComponent<TransformComponent>();
            if (tf)
            {
                TransformComponent poofTf;
                glm::vec3 targetPos = tf->GetPosition() + glm::vec3(-0.5f, 1.0f, 2.5f);

                if (State != CustomerState::Spawning) {
                    targetPos = tf->GetPosition() + glm::vec3(0.0f, 1.0f, 3.5f);
                }

                poofTf.SetPosition(targetPos);
                poofTf.SetScale(glm::vec3(1.0f));

                poofTf.WorldMatrix[3][0] = targetPos.x;
                poofTf.WorldMatrix[3][1] = targetPos.y;
                poofTf.WorldMatrix[3][2] = targetPos.z;

                NativeScriptComponent poofNsc;
                poofNsc.AddScript<ParticleEmitterScript>("ParticleEmitterScript");

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
                auto* emitter = static_cast<ParticleEmitterScript*>(addedNsc->Scripts[0].Instance);

                if (emitter->GetParticles().size() > 0)
                {
                    float sizeMultiplier = (State == CustomerState::Spawning) ? 1.0f : 1.0f;
                    float spreadMultiplier = (State == CustomerState::Spawning) ? 1.0f : 4.0f;

                    emitter->ParticleTemplate.Textures.clear();
                    emitter->ParticleTemplate.Textures.push_back(AssetManager::GetTexture2D("assets://particles/PotParticle.png"));
                    emitter->ParticleTemplate.PositionOffset = { 0.0f, 0.0f, 0.0f };
                    emitter->ParticleTemplate.PositionVariation = { 0.5f * spreadMultiplier, 0.4f * spreadMultiplier, 0.5f * spreadMultiplier };
                    emitter->ParticleTemplate.Velocity = { 0.0f, 0.1f * sizeMultiplier, 0.0f };
                    emitter->ParticleTemplate.VelocityVariation = { 0.15f * sizeMultiplier, 0.0f, 0.15f * sizeMultiplier };
                    emitter->ParticleTemplate.ColorBegin = { 1.0f, 1.0f, 1.0f, 0.8f };
                    emitter->ParticleTemplate.ColorEnd = { 1.0f, 1.0f, 1.0f, 0.0f };
                    emitter->ParticleTemplate.SizeBegin = 2.5f * sizeMultiplier;
                    emitter->ParticleTemplate.SizeVariation = 0.3f * sizeMultiplier;
                    emitter->ParticleTemplate.SizeEnd = 3.5f * sizeMultiplier;
                    emitter->ParticleTemplate.LifeTime = (State == CustomerState::Spawning) ? 1.4f : 3.0f;
                    emitter->EmitRate = (State == CustomerState::Spawning) ? 0.005f : 0.015f;

                    emitter->Play();
                }
            }
        }

        if (State == CustomerState::Spawning)
        {
            if (IsGrandma && !m_CameraCaptured)
            {
                auto* camera = GetScene()->GetCamera();
                if (camera)
                {
                    m_OriginalCameraPos = camera->TargetPosition;
                    m_OriginalCameraZoom = camera->Zoom;
                    m_CameraCaptured = true;
                    GameManagerScript::s_IsCutscenePlaying = true;
                }
            }

            if (IsGrandma && m_CameraCaptured)
            {
                auto* camera = GetScene()->GetCamera();
                auto* tf = GetComponent<TransformComponent>();
                if (camera && tf)
                {
                    camera->TargetPosition = tf->GetPosition() + glm::vec3(0.0f, 1.0f, 0.0f);
                    camera->Zoom += (20.0f - camera->Zoom) * 5.0f * dt;
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

        if (State == CustomerState::LeavingReaction) {
            m_ReactionTimer -= ts.GetSeconds();

            if (m_GrandmaSuccessCutscene && m_CameraCaptured) {
                auto* camera = GetScene()->GetCamera();
                auto* tf = GetComponent<TransformComponent>();
                if (camera && tf) {
                    camera->TargetPosition = glm::mix(camera->TargetPosition, tf->GetPosition() + glm::vec3(0.0f, 1.0f, 0.0f), 5.0f * (float)ts.GetSeconds());
                    camera->Zoom += (15.0f - camera->Zoom) * 5.0f * (float)ts.GetSeconds();
                }

                if (m_ReactionTimer <= 5.5f && !m_MapUnlocked) {
                    GetScene()->GetWorld().GetEventBus().Publish(GrandmaSatisfiedEvent{});
                    m_MapUnlocked = true;

                    if (tf) {
                        TransformComponent poofTf;
                        poofTf.SetPosition(tf->GetPosition() + glm::vec3(0.0f, 1.0f, 0.0f));
                        poofTf.SetScale(glm::vec3(2.5f));
                        NativeScriptComponent poofNsc;
                        poofNsc.AddScript<PoofEmitterScript>("PoofEmitterScript");
                        Entity bigPoof = GetScene()->GetWorld().BuildEntity()
                            .With<TagComponent>({ "PoofEmitter" })
                            .With<TransformComponent>(poofTf)
                            .With<NativeScriptComponent>(poofNsc)
                            .Build();
                        auto* addedNsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(bigPoof);
                        if (addedNsc && !addedNsc->Scripts.empty() && addedNsc->Scripts[0].Instance) {
                            static_cast<PoofEmitterScript*>(addedNsc->Scripts[0].Instance)->Play();
                        }
                    }
                }
            }

            if (m_ReactionTimer <= 0.0f && !IsPendingDestroy) {
                if (!m_ExitPoofStarted) {
                    m_ExitPoofStarted = true;
                    m_ReactionTimer = 2.0f;

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

                        if (m_CameraCaptured) {
                            auto* camera = GetScene()->GetCamera();
                            if (camera) {
                                camera->TargetPosition = m_OriginalCameraPos;
                                camera->Zoom = m_OriginalCameraZoom;
                            }
                            GameManagerScript::s_IsCutscenePlaying = false;
                            m_CameraCaptured = false;
                        }

                        glm::vec3 hidePos = tf->GetPosition();
                        hidePos.y -= 100.0f;
                        tf->SetPosition(hidePos);
                    }
                }
                else {
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

        if (State == CustomerState::Seated)
        {
            if (IsGrandma && m_CameraCaptured)
            {
                m_SeatedTimer += (float)ts.GetSeconds();

                auto* camera = GetScene()->GetCamera();
                if (camera)
                {
                    camera->TargetPosition = glm::mix(camera->TargetPosition, m_OriginalCameraPos, 4.0f * (float)ts.GetSeconds());
                    camera->Zoom += (m_OriginalCameraZoom - camera->Zoom) * 4.0f * (float)ts.GetSeconds();
                }

                if (m_SeatedTimer >= 1.0f)
                {
                    if (camera)
                    {
                        camera->TargetPosition = m_OriginalCameraPos;
                        camera->Zoom = m_OriginalCameraZoom;
                    }
                    GameManagerScript::s_IsCutscenePlaying = false;
                    m_CameraCaptured = false;
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

            if (IsGrandma && m_CameraCaptured)
            {
                auto* camera = GetScene()->GetCamera();
                if (camera)
                {
                    camera->TargetPosition = tf->GetPosition() + glm::vec3(0.0f, 1.0f, 0.0f);
                    camera->Zoom += (20.0f - camera->Zoom) * 5.0f * (float)ts.GetSeconds();
                }
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

    virtual void ReceiveFood(bool isCorrectOrder = true, bool hasExtraIngredients = false)
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
            if (IsGrandma) {
                m_GrandmaSuccessCutscene = true;
                m_MapUnlocked = false;
                auto* camera = GetScene()->GetCamera();
                if (camera && !m_CameraCaptured) {
                    m_OriginalCameraPos = camera->TargetPosition;
                    m_OriginalCameraZoom = camera->Zoom;
                    m_CameraCaptured = true;
                    GameManagerScript::s_IsCutscenePlaying = true;
                }

                m_PoofPlayed = false;
                m_PoofStarted = false;
                if (m_PoofEntity.id != std::numeric_limits<std::size_t>::max()) {
                    GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_PoofEntity });
                    m_PoofEntity = { std::numeric_limits<std::size_t>::max(), 0 };
                }
            }

            if (tagComp) tagComp->Tag = "ZadowolonyKlient";
            highlightColor = { 0.1f, 1.0f, 0.2f };

            if (GameManagerScript::s_Instance) {
                float finalReward = OrderPrice;
                if (hasExtraIngredients) {
                    AwardedTip = OrderPrice * 0.5f;
                    finalReward += AwardedTip;
                }
                GetScene()->GetWorld().GetEventBus().Publish(OrderFulfilledEvent(finalReward));
            }
        }
        else {
            if (tagComp) tagComp->Tag = "ZlyKlient";
            highlightColor = { 1.0f, 0.1f, 0.1f };
            if (GameManagerScript::s_Instance) GetScene()->GetWorld().GetEventBus().Publish(OrderFulfilledEvent(0.0f));
        }

        GetScene()->GetWorld().GetEventBus().Publish(TriggerHighlightEvent{ m_Entity, highlightColor, 2.0f, false });
        m_WasCorrect = isCorrectOrder;

        State = CustomerState::LeavingReaction;
        m_ReactionTimer = IsGrandma ? 6.5f : 2.0f;
    }

    void OnDestroy() override
    {
        if (m_CameraCaptured) {
            auto* camera = GetScene()->GetCamera();
            if (camera) {
                camera->TargetPosition = m_OriginalCameraPos;
                camera->Zoom = m_OriginalCameraZoom;
            }
            GameManagerScript::s_IsCutscenePlaying = false;
        }

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
        if (m_CutsceneSmokeEntity.id != std::numeric_limits<std::size_t>::max())
        {
            GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_CutsceneSmokeEntity });
        }
    }
};