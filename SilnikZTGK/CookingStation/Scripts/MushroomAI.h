#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scripts/ParticleEmitterScript.h"
#include "CookingStation/Scene/ecs.h"
#include "CookingStation/Core/Input.h"
#include "CustomerScript.h" 
#include <spdlog/spdlog.h>
#include <glm/glm.hpp>

enum class WaiterState { Idle, FetchingFood, DeliveringFood };

class MushroomAI : public ScriptableEntity
{
private:
    WaiterState m_State = WaiterState::Idle;

    Entity m_TargetCustomer = { std::numeric_limits<std::size_t>::max(), 0 };
    Entity m_TargetFood = { std::numeric_limits<std::size_t>::max(), 0 };

    float m_Speed = 5.0f;       
    float m_InteractRange = 0.1f;

    void SetDusting(bool state)
    {
        auto* scriptComp = GetComponent<NativeScriptComponent>();
        if (scriptComp)
        {
            for (auto& s : scriptComp->Scripts)
            {
                if (s.Name == "DustEmitterScript" && s.Instance)
                {
                    auto* emitter = static_cast<ParticleEmitterScript*>(s.Instance);
                    if (state) emitter->Play();
                    else emitter->Stop();
                    break;
                }
            }
        }
    }

public:
    void OnUpdate(Timestep ts) override
    {
        auto* animComp = GetComponent<AnimatorComponent>();
        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;

        if (m_State == WaiterState::Idle)
        {
            if (animComp && animComp->AnimatorInstance) animComp->IsPlaying = false;

            SetDusting(false);

            LookForOrders();
        }
        else if (m_State == WaiterState::FetchingFood)
        {
            if (animComp && animComp->AnimatorInstance) {
                animComp->AnimatorInstance->PlayAnimation("Walk");
                animComp->IsPlaying = true;
            }

            auto* foodTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(m_TargetFood);
            if (!foodTransform) {
                m_State = WaiterState::Idle;
                return;
            }

            MoveTowards(transform, foodTransform->GetPosition(), ts.GetSeconds());

            glm::vec2 myPos2D = { transform->GetPosition().x, transform->GetPosition().z };
            glm::vec2 foodPos2D = { foodTransform->GetPosition().x, foodTransform->GetPosition().z };

            if (glm::distance(myPos2D, foodPos2D) <= m_InteractRange)
            {
                PickUpFood();
            }
        }
        else if (m_State == WaiterState::DeliveringFood)
        {
            if (animComp && animComp->AnimatorInstance) {
                animComp->AnimatorInstance->PlayAnimation("Walk");
                animComp->IsPlaying = true;
            }
            auto* customerTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(m_TargetCustomer);
            if (!customerTransform) {
                m_State = WaiterState::Idle;
                return;
            }

            MoveTowards(transform, customerTransform->GetPosition(), ts.GetSeconds());

            glm::vec2 myPos2D = { transform->GetPosition().x, transform->GetPosition().z };
            glm::vec2 custPos2D = { customerTransform->GetPosition().x, customerTransform->GetPosition().z };

            if (glm::distance(myPos2D, custPos2D) <= m_InteractRange)
            {
                DeliverFood();
            }
        }
    }

private:
    void MoveTowards(TransformComponent* myTransform, glm::vec3 targetPos, float dt)
    {
        glm::vec3 myPos = myTransform->GetPosition();
        glm::vec3 direction = targetPos - myPos;
        direction.y = 0;

        if (glm::length(direction) > 0.01f)
        {
            direction = glm::normalize(direction);
            myPos += direction * m_Speed * dt;
            myTransform->SetPosition(myPos);

            float angle = glm::degrees(atan2(direction.x, direction.z));
            myTransform->SetRotation(glm::vec3(0.0f, angle, 0.0f));

            SetDusting(true);
        }
        else
        {
            SetDusting(false);
        }
    }

    void LookForOrders()
    {
        Entity foundFood = { std::numeric_limits<std::size_t>::max(), 0 };
        Entity foundCustomer = { std::numeric_limits<std::size_t>::max(), 0 };

        auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
        auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        const auto& pickupPoints = GetScene()->GetPickupPoints();

        if (!tags || !scripts) return;

        for (size_t i = 0; i < tags->dense.size(); ++i)
        {
            if (tags->dense[i].Tag == "UgotowaneDanie") {
                Entity potentialFood = tags->reverse[i];

                Entity plateEntity = GetScene()->GetParent(potentialFood);
                auto* plateTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(plateEntity);
                if (!plateTransform) continue;

                glm::vec2 platePos2D = { plateTransform->GetPosition().x, plateTransform->GetPosition().z };

                bool isOnPickupPoint = false;
                for (const auto& pp : pickupPoints)
                {
                    glm::vec2 pp2D = { pp.x, pp.z };
                    if (glm::distance(platePos2D, pp2D) < 0.5f)
                    {
                        isOnPickupPoint = true;
                        break;
                    }
                }

                if (!isOnPickupPoint) continue;

                foundFood = plateEntity;;
                break;
            }
        }

        if (foundFood.id != std::numeric_limits<std::size_t>::max())
        {
            for (size_t i = 0; i < tags->dense.size(); ++i)
            {
                if (tags->dense[i].Tag == "NormalCustomer" || tags->dense[i].Tag == "HelperCustomer")
                {
                    Entity custEntity = tags->reverse[i];
                    auto* nsc = scripts->Get(custEntity);
                    if (nsc)
                    {
                        CustomerScript* custScript = nullptr;

                        for (auto& s : nsc->Scripts)
                        {
                            if (s.Name == "CustomerScript" || s.Name == "HelperCustomerScript")
                            {
                                custScript = (CustomerScript*)s.Instance;
                                break;
                            }
                        }

                        if (custScript && !custScript->IsServed)
                        {
                            std::vector<IngredientType> plateIngredients = { IngredientType::Tomato };

                            if (custScript->IsOrderMatching(plateIngredients))
                            {
                                foundCustomer = custEntity;
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (foundFood.id != std::numeric_limits<std::size_t>::max() && foundCustomer.id != std::numeric_limits<std::size_t>::max())
        {
            m_TargetFood = foundFood;
            m_TargetCustomer = foundCustomer;
            m_State = WaiterState::FetchingFood;
            spdlog::info("Grzybek zauwazyl jedzenie i ruszyl po nie!");
        }
    }

    void PickUpFood()
    {
        spdlog::info("Grzybek wrzucil jedzenie na kapelusz!");
        GetScene()->SetParent(m_TargetFood, m_Entity);

        auto* foodTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(m_TargetFood);
        auto* transform = GetComponent<TransformComponent>();

        if (foodTransform && transform)
        {
            foodTransform->SetPosition(glm::vec3(0.0f, 2.0f, 0.0f));
            foodTransform->SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));

            glm::vec3 myScale = transform->GetScale();

            if (myScale.x != 0.0f && myScale.y != 0.0f && myScale.z != 0.0f) {
                foodTransform->SetScale(glm::vec3(1.0f) / myScale);
            }
        }

        auto* foodScripts = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_TargetFood);
        if (foodScripts)
        {
            for (auto& scriptEl : foodScripts->Scripts) {
                if (scriptEl.Instance) {
                    scriptEl.Instance->OnDestroy();
                    if (scriptEl.DestroyScript) scriptEl.DestroyScript(&scriptEl);
                    scriptEl.Instance = nullptr;
                }
            }
            foodScripts->Scripts.clear();
        }

        m_State = WaiterState::DeliveringFood;
    }


    void DeliverFood()
    {
        spdlog::info("Grzybek dostarczyl zamowienie! Klient szczesliwy.");

        Entity nullEntity = { std::numeric_limits<std::size_t>::max(), 0 };

        GetScene()->RemoveParent(m_TargetFood);
        GetScene()->DestroyEntity(m_TargetFood);
        m_TargetFood = nullEntity;

        auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        if (scripts)
        {
            auto* nsc = scripts->Get(m_TargetCustomer);
            if (nsc)
            {
                CustomerScript* custScript = nullptr;
                for (auto& s : nsc->Scripts)
                {
                    if (s.Name == "CustomerScript" || s.Name == "HelperCustomerScript")
                    {
                        custScript = (CustomerScript*)s.Instance;
                        break;
                    }
                }

                if (custScript)
                {
                    custScript->ReceiveFood();
                }
            }
        }

        m_TargetCustomer = nullEntity;
        m_State = WaiterState::Idle;
    }
};