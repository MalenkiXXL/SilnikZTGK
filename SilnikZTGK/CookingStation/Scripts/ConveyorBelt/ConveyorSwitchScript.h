#pragma once

#include "ConveyorScript.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Events/GameEvents.h"


class ConveyorSwitchScript : public ConveyorScript {
private:
    std::size_t m_ClickSubId = 0;

public:
    virtual void OnCreate() override {
        ConveyorScript::OnCreate();

        m_ClickSubId = GetScene()->GetWorld().GetEventBus().Subscribe<EntityClickedEvent>(
                [this](const EntityClickedEvent &e) {
                    if (Input::IsUICapturingMouse()) return;
                    if (e.TargetEntity.id == this->m_Entity.id) {
                        this->HandleClick();
                    }
                }
        );
    }

    virtual void OnDestroy() override {
        GetScene()->GetWorld().GetEventBus().Unsubscribe<EntityClickedEvent>(m_ClickSubId);
        ConveyorScript::OnDestroy();
    }

    void OnUpdate(Timestep ts) override {
        if (Input::IsUICapturingMouse()) return;

        if (Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0)) {
            auto *tf = GetComponent<TransformComponent>();
            if (tf) {
                glm::vec2 cursor2D = {GetMouseWorldPosition().x, GetMouseWorldPosition().z};
                glm::vec2 my2D = {tf->GetPosition().x, tf->GetPosition().z};

                if (glm::distance(cursor2D, my2D) < 1.5f) {
                    HandleClick();
                }
            }
        }
    }


    void HandleClick() override{
        auto *transform = GetComponent<TransformComponent>();
        if (!transform) return;

        auto &conveyorMap = GetScene()->GetConveyorMap();

        float validAngles[4];
        int validCount = 0;
        int neighborCount = 0;

        glm::vec3 myPos = transform->GetPosition();

        for (auto &m: s_Mappings) {
            GridPos neighborKey{
                    (int) std::round((myPos.x + m.direction.x * 2.0f) / 2.0f),
                    (int) std::round((myPos.z + m.direction.z * 2.0f) / 2.0f)
            };

            auto it = conveyorMap.find(neighborKey);
            if (it == conveyorMap.end()) continue;

            neighborCount++;

            ConveyorScript *neighbor = it->second;
            bool isHeadOn = (glm::dot(m.direction, neighbor->PushDirection) < -0.9f);

            if (!isHeadOn) {
                validAngles[validCount++] = m.angle;
            }
        }

        if (neighborCount >= 3 && validCount > 0) {
            float currentRot = transform->GetRotation().y;
            currentRot = fmodf(currentRot, 360.0f);
            if (currentRot < 0.0f) currentRot += 360.0f;

            int currentIndex = -1;
            for (int i = 0; i < validCount; i++) {
                if (std::abs(validAngles[i] - currentRot) < 1.0f) {
                    currentIndex = i;
                    break;
                }
            }

            int nextIndex = (currentIndex + 1) % validCount;

            glm::vec3 newRot = transform->GetRotation();
            newRot.y = validAngles[nextIndex];
            transform->SetRotation(newRot);

            SetPushDirection();
            spdlog::info("Zwrotnica: Nowy kierunek: {}", newRot.y);
            AudioEngine::Play("assets://sounds/ui_click.mp3");
        }
    }

};