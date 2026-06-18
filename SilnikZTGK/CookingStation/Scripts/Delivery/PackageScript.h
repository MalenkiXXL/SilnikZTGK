#ifndef SILNIKZTGK_PACKAGESCRIPT_H
#define SILNIKZTGK_PACKAGESCRIPT_H
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Scripts/PoofEmitterScript.h"

class PackageScript : public ScriptableEntity{
public:
    inline static std::vector<Entity> s_ActivePackages;

    void SetHovered(bool isHovered) { m_IsHovered = isHovered; }

    void OnCreate() override {
        spdlog::info("[PackageScript] Utworzono paczke typu {}", (int)m_Type);

        s_ActivePackages.push_back(m_Entity);

        m_ClickSubId = GetScene()->GetWorld().GetEventBus().Subscribe<EntityClickedEvent>(
                [this](const EntityClickedEvent& e) {
                    if (e.TargetEntity.id == m_Entity.id) {
                        this->HandleClick();
                    }
                }
        );

        m_ConfigSubId = GetScene()->GetWorld().GetEventBus().Subscribe<ConfigurePackageEvent>(
                [this](const ConfigurePackageEvent& e) {
                    if (e.TargetEntity.id == this->m_Entity.id && !this->m_IsConfigured) {
                        this->m_Type = e.Type;
                        this->m_IngredientAmount = e.Amount;
                        this->m_IsConfigured = true;
                        spdlog::info("[PackageScript] Złapałem event! Jestem typem: {}", (int)m_Type);
                    }
                }
        );

        GetScene()->GetWorld().GetEventBus().Publish(PackageSpawnedEvent{ m_Entity });
    }


    void OnUpdate(Timestep ts) override {
        m_TimeAlive += (float)ts;

        if (m_IsPoofing)
        {
            m_PoofTimer -= ts.GetSeconds();

            if (m_PoofTimer < 1.7f)
            {
                auto* mesh = GetComponent<MeshComponent>();

                if (mesh && mesh->ModelPtr != nullptr)
                {
                    mesh->ModelPtr = nullptr;

                    auto* nsc = GetComponent<NativeScriptComponent>();
                    if (nsc) {
                        for (auto& s : nsc->Scripts) {
                            if (s.Name == "PoofEmitterScript" && s.Instance) {
                                auto* emitter = static_cast<ParticleEmitterScript*>(s.Instance);
                                emitter->Stop();
                                break;
                            }
                        }
                    }
                }
            }

            if (m_PoofTimer <= 0.0f)
            {
                GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ m_Entity });
            }
        }
        auto* transform = GetComponent<TransformComponent>();
        auto* mesh = GetComponent<MeshComponent>();

        if (transform && mesh) {
            if (!m_BaseScaleInitialized) {
                m_BaseScale = transform->GetScale();
                m_BaseScaleInitialized = true;
            }

            float wave = std::sin(m_TimeAlive * 4.0f);

            transform->SetScale(m_BaseScale + glm::vec3(wave * 0.15f));

            float currentOpacity = (wave + 1.0f) * 0.5f;

            float maxOpacity = 0.6f;
            currentOpacity *= maxOpacity;

            mesh->ShaderName = "HighlightShader";
            if (m_IsHovered) {
                mesh->HighlightColor = glm::vec4(1.0f, 0.9f, 0.0f, currentOpacity);
            } else {
                mesh->HighlightColor = glm::vec4(0.513f, 0.109f, 0.364f, currentOpacity);
            }

            m_IsHovered = false;
        }
    };

    void HandleClick();

    void OnDestroy() override {
        GetScene()->GetWorld().GetEventBus().Unsubscribe<EntityClickedEvent>(m_ClickSubId);
        GetScene()->GetWorld().GetEventBus().Unsubscribe<ConfigurePackageEvent>(m_ConfigSubId);
        auto it = std::find_if(s_ActivePackages.begin(), s_ActivePackages.end(),
                               [this](const Entity& e) { return e.id == this->m_Entity.id; });

        if (it != s_ActivePackages.end()) {
            s_ActivePackages.erase(it);
        }
    }

    IngredientType getType() const;
    int getIngredientAmount() const { return m_IngredientAmount; }

    void OnHoverCursor() override {
        m_IsHovered = true;
    }

    void StartPoof();

private:
    float m_TimeAlive = 0.0f;
    bool m_IsHovered = false;
    glm::vec3 m_BaseScale = glm::vec3(0.0f);
    bool m_BaseScaleInitialized = false;

    std::size_t m_ClickSubId = 0;
    bool m_IsCollected = false;

    std::size_t m_ConfigSubId = 0;
    bool m_IsConfigured = false;

    IngredientType m_Type = IngredientType::Tomato;
    int m_IngredientAmount = 5;

    bool m_IsPoofing = false;
    float m_PoofTimer = 0.0f;

};


#endif //SILNIKZTGK_PACKAGESCRIPT_H
