#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Events/GameEvents.h"
#include <unordered_map>
#include <algorithm>
#include <spdlog/spdlog.h>
#include <cmath> // potrzebne do std::sin

struct HighlightData {
    Entity Target;
    float Timer;
    float MaxTime;
    glm::vec3 Color;
    std::string OriginalShader;
    bool IsInfinite;
    bool PingedThisFrame;
};

class HighlightManagerScript : public ScriptableEntity {
public:
    void OnCreate() override {
        spdlog::info("HighlightManagerScript: URUCHOMIONY na scenie!");

        m_SubId = GetScene()->GetWorld().GetEventBus().Subscribe<TriggerHighlightEvent>(
                [this](const TriggerHighlightEvent& e) {
                    auto* mesh = GetScene()->GetWorld().GetComponent<MeshComponent>(e.TargetEntity);
                    if (mesh) {
                        if (m_ActiveHighlights.find(e.TargetEntity.id) == m_ActiveHighlights.end()) {
                            m_ActiveHighlights[e.TargetEntity.id].OriginalShader = mesh->ShaderName;
                        }

                        auto& data = m_ActiveHighlights[e.TargetEntity.id];
                        data.Target = e.TargetEntity;
                        data.Color = e.Color;
                        data.IsInfinite = e.IsInfinite;

                        if (e.IsInfinite) {
                            data.PingedThisFrame = true;
                        } else {
                            data.Timer = e.Duration;
                            data.MaxTime = e.Duration;
                        }
                    }
                }
        );
    }

    void OnDestroy() override {
        GetScene()->GetWorld().GetEventBus().Unsubscribe<TriggerHighlightEvent>(m_SubId);
    }

    void OnUpdate(Timestep ts) override {
        m_GlobalTime += ts.GetSeconds();

        for (auto it = m_ActiveHighlights.begin(); it != m_ActiveHighlights.end(); ) {
            auto& data = it->second;
            auto* mesh = GetScene()->GetWorld().GetComponent<MeshComponent>(data.Target);

            if (!mesh) {
                it = m_ActiveHighlights.erase(it);
                continue;
            }

            if (data.IsInfinite) {
                if (!data.PingedThisFrame) {
                    mesh->ShaderName = data.OriginalShader;
                    it = m_ActiveHighlights.erase(it);
                    continue;
                }

                float wave = std::sin(m_GlobalTime * 4.0f);
                float currentOpacity = (wave + 1.0f) * 0.5f * 0.7f;

                mesh->ShaderName = "HighlightShader";
                mesh->HighlightColor = glm::vec4(data.Color, currentOpacity);

                data.PingedThisFrame = false;
                ++it;

            } else {
                data.Timer -= ts.GetSeconds();

                if (data.Timer <= 0.0f) {
                    mesh->ShaderName = data.OriginalShader;
                    it = m_ActiveHighlights.erase(it);
                } else {
                    float progress = 1.0f - (std::max(data.Timer, 0.0f) / data.MaxTime);
                    float wave = std::sin(progress * 3.14159f);
                    float currentOpacity = wave * 0.7f;

                    mesh->ShaderName = "HighlightShader";
                    mesh->HighlightColor = glm::vec4(data.Color, currentOpacity);
                    ++it;
                }
            }
        }
    }

private:
    std::size_t m_SubId = 0;
    std::unordered_map<std::size_t, HighlightData> m_ActiveHighlights;

    float m_GlobalTime = 0.0f;
};