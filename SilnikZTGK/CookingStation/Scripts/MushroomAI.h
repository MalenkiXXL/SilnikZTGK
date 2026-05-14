#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Scene/ecs.h"
#include "CookingStation/Core/Input.h"
#include "CustomerScript.h" // POTRZEBNE DO KOMUNIKACJI Z KLIENTEM
#include <spdlog/spdlog.h>
#include <glm/glm.hpp>

// Odpowiednik Twojego enum State z Unity
enum class WaiterState { Idle, FetchingFood, DeliveringFood };

class MushroomAI : public ScriptableEntity
{
private:
    WaiterState m_State = WaiterState::Idle;

    Entity m_TargetCustomer = { std::numeric_limits<std::size_t>::max(), 0 };
    Entity m_TargetFood = { std::numeric_limits<std::size_t>::max(), 0 };

    float m_Speed = 5.0f;         // Prêdkoœæ chodzenia
    float m_InteractRange = 0.1f; // Zasiêg r¹k kelnera

public:
    void OnUpdate(Timestep ts) override
    {
        auto* animComp = GetComponent<AnimatorComponent>();
        auto* transform = GetComponent<TransformComponent>();
        if (!transform) return;

        // MASZYNA STANÓW GRZYBKA
        if (m_State == WaiterState::Idle)
        {
            // Zatrzymujemy animacjê, gdy stoi
            if (animComp && animComp->AnimatorInstance) animComp->IsPlaying = false;

            LookForOrders();
        }
        else if (m_State == WaiterState::FetchingFood)
        {
            // Odpalamy animacjê chodzenia
            if (animComp && animComp->AnimatorInstance) {
                animComp->AnimatorInstance->PlayAnimation("Walk");
                animComp->IsPlaying = true;
            }

            auto* foodTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(m_TargetFood);
            if (!foodTransform) {
                m_State = WaiterState::Idle; // Jedzenie zniknê³o, wracamy do szukania
                return;
            }

            MoveTowards(transform, foodTransform->GetPosition(), ts.GetSeconds());

            // P³aski dystans (ignorujemy oœ Y)
            glm::vec2 myPos2D = { transform->GetPosition().x, transform->GetPosition().z };
            glm::vec2 foodPos2D = { foodTransform->GetPosition().x, foodTransform->GetPosition().z };

            if (glm::distance(myPos2D, foodPos2D) <= m_InteractRange)
            {
                PickUpFood();
            }
        }
        else if (m_State == WaiterState::DeliveringFood)
        {
            auto* customerTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(m_TargetCustomer);
            if (!customerTransform) {
                m_State = WaiterState::Idle; // Klient znikn¹³ (np. wyszed³)
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
    // ODPOWIEDNIK NAVMESH AGENT (Ruch po prostej i obracanie siê)
    void MoveTowards(TransformComponent* myTransform, glm::vec3 targetPos, float dt)
    {
        glm::vec3 myPos = myTransform->GetPosition();
        glm::vec3 direction = targetPos - myPos;
        direction.y = 0; // Grzybek nie umie lataæ, porusza siê tylko po pod³odze (X i Z)

        if (glm::length(direction) > 0.01f)
        {
            // Normalizujemy kierunek (¿eby prêdkoœæ by³a zawsze taka sama)
            direction = glm::normalize(direction);
            myPos += direction * m_Speed * dt;
            myTransform->SetPosition(myPos);

            // MAGICZNA MATEMATYKA: Grzybek obraca siê twarz¹ w stronê, w któr¹ idzie
            float angle = glm::degrees(atan2(direction.x, direction.z));
            myTransform->SetRotation(glm::vec3(0.0f, angle, 0.0f));
        }
    }

    void LookForOrders()
    {
        Entity foundFood = { std::numeric_limits<std::size_t>::max(), 0 };
        Entity foundCustomer = { std::numeric_limits<std::size_t>::max(), 0 };

        auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
        auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();

        if (!tags || !scripts) return;

        // 1. Szukamy gotowego jedzenia (nasza kanapka, któr¹ stworzy³ garnek)
        for (size_t i = 0; i < tags->dense.size(); ++i)
        {
            if (tags->dense[i].Tag == "UgotowaneDanie")
            {
                foundFood = tags->reverse[i];
                break;
            }
        }

        if (foundFood.id != std::numeric_limits<std::size_t>::max())
        {
            for (size_t i = 0; i < tags->dense.size(); ++i)
            {
                if (tags->dense[i].Tag == "NormalCustomer")
                {
                    Entity custEntity = tags->reverse[i];
                    auto* nsc = scripts->Get(custEntity);
                    if (nsc)
                    {
                        // --- NOWY SPOSÓB SZUKANIA SKRYPTU ---
                        CustomerScript* custScript = nullptr;

                        // Przeszukujemy listê podpiêtych skryptów w tym kliencie
                        for (auto& s : nsc->Scripts)
                        {
                            if (s.Name == "CustomerScript")
                            {
                                // UWAGA: Za³o¿y³em, ¿e wskaŸnik nazywa siê "Instance". 
                                // Jeœli u Ciebie nazywa siê inaczej (np. Script, Ptr, Obj), zmieñ to poni¿ej!
                                custScript = (CustomerScript*)s.Instance;
                                break;
                            }
                        }

                        if (custScript && !custScript->IsServed)
                        {
                            std::vector<std::string> plateIngredients = { "Tomato" };

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

        // Jeœli mamy komplet (jedzenie + chêtny), ruszamy w trasê!
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

        // Magiczna funkcja, która odpina kanapkê od talerza i przypina do Grzybka
        GetScene()->SetParent(m_TargetFood, m_Entity);

        auto* foodTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(m_TargetFood);
        if (foodTransform)
        {
            // Unosimy kanapkê lokalnie o 2 metry do góry (na g³owê)
            foodTransform->SetPosition(glm::vec3(0.0f, 2.0f, 0.0f));
        }

        m_State = WaiterState::DeliveringFood;
    }

    void DeliverFood()
    {
        spdlog::info("Grzybek dostarczyl zamowienie! Klient szczesliwy.");

        // --- OBEJŒCIE B£ÊDU SILNIKA (Soft Deletion dla kanapki) ---
        auto* foodTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(m_TargetFood);
        if (foodTransform) foodTransform->SetPosition(glm::vec3(0.0f, -1000.0f, 0.0f));

        auto* foodTag = GetScene()->GetWorld().GetComponent<TagComponent>(m_TargetFood);
        if (foodTag) foodTag->Tag = "ZjedzoneDanie";

        m_TargetFood = { std::numeric_limits<std::size_t>::max(), 0 };

        // Wywo³ujemy u klienta ReceiveFood(), ¿eby znikn¹³ (zap³aci³)
        auto* scripts = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        if (scripts)
        {
            auto* nsc = scripts->Get(m_TargetCustomer);
            if (nsc)
            {
                CustomerScript* custScript = nullptr;
                for (auto& s : nsc->Scripts)
                {
                    if (s.Name == "CustomerScript")
                    {
                        custScript = (CustomerScript*)s.Instance;
                        break;
                    }
                }

                if (custScript)
                {
                    custScript->ReceiveFood(); // Kasa leci, klient spada pod mapê!
                }
            }
        }

        m_TargetCustomer = { std::numeric_limits<std::size_t>::max(), 0 };
        m_State = WaiterState::Idle; // Kelner wraca do szukania roboty
    }
};