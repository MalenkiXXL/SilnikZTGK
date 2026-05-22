#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include <vector>
#include <string>
#include <random>
#include "CustomerScript.h"

class CustomerManagerScript : public ScriptableEntity
{
private:
    float m_SpawnTimer = 0.0f;
    float m_SpawnInterval = 5.0f; // Co 5 sekund sprawdza, czy mo¿na stworzyæ klienta
    int m_TotalSpawned = 0;

    // Lista dostêpnych modeli klientów 
    std::vector<std::string> m_CustomerModels = {
        "CookingStation/Assets/models/klienci/klient.gltf",
        "CookingStation/Assets/models/klienci/klient2.gltf",
        "CookingStation/Assets/models/klienci/klient3.gltf",
        "CookingStation/Assets/models/klienci/klientka1.gltf",
        "CookingStation/Assets/models/klienci/klientka2.gltf",
        "CookingStation/Assets/models/klienci/klientka3.gltf",
        "CookingStation/Assets/models/klienci/babcia.gltf"

    };

public:
    void OnUpdate(Timestep ts) override
    {
        m_SpawnTimer += ts.GetSeconds();

        if (m_SpawnTimer >= m_SpawnInterval)
        {
            m_SpawnTimer = 0.0f;
            TrySpawnCustomer();
        }
    }

private:
    void TrySpawnCustomer()
    {
        Entity targetChair = FindEmptyChair();

        // Jeœli nie ma wolnych krzese³, funkcja przerywa dzia³anie i czeka kolejne 5 sekund
        if (targetChair.id == std::numeric_limits<std::size_t>::max())
        {
            spdlog::info("Wszystkie krzesla zajete. Klient musi poczekac!");
            return;
        }

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, m_CustomerModels.size() - 1);
        std::string chosenModel = m_CustomerModels[dist(gen)];

        m_TotalSpawned++;
        bool isHelper = (m_TotalSpawned % 3 == 0);

        auto builder = GetScene()->GetWorld().BuildEntity();
        builder.With<TagComponent>({ isHelper ? "HelperCustomer" : "NormalCustomer" });

        auto* chairTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(targetChair);
        TransformComponent tc;

        // --- 1. SKALOWANIE I POZYCJA ---
        glm::vec3 chairPos = chairTransform->GetPosition();
        tc.SetPosition(chairPos + glm::vec3(0.0f, 1.0f, 0.0f));
        tc.SetScale(glm::vec3(3.0f, 3.0f, 3.0f)); // Naprawa skali z Blendera

        // --- 2. OBRÓT W STRONÊ STO£U ---
        glm::vec3 tablePos = FindNearestTablePosition(chairPos);
        glm::vec3 direction = tablePos - chairPos;

        float angle = glm::degrees(std::atan2(direction.x, direction.z));
        glm::vec3 finalRotation = { 0.0f, angle, 0.0f };

        // Korekta osi modelu z Blendera
        finalRotation.y += 90.0f;

        tc.SetRotation(finalRotation); // Przekazujemy nasz¹ now¹ rotacjê!
        // -------------------------------

        builder.With<TransformComponent>(tc);

        MeshComponent mesh;
        mesh.ModelPtr = AssetManager::GetModel(chosenModel);
        builder.With<MeshComponent>(mesh);

        BoxColliderComponent bc;
        bc.Size = { 1.0f, 2.0f, 1.0f };
        builder.With<BoxColliderComponent>(bc);

        // --- 3. ANIMACJA ---
        AnimatorComponent animatorComp;
        animatorComp.AnimatorInstance = std::make_shared<Animator>();

        std::string animPath = "CookingStation/Assets/models/animacje/klienci/klient-siedzi.gltf";
        auto sitAnimation = std::make_shared<Animation>(animPath, mesh.ModelPtr.get());
        animatorComp.AnimatorInstance->AddAnimation("SitIdle", sitAnimation);
        builder.With<AnimatorComponent>(animatorComp);

        NativeScriptComponent nsc;

        if (isHelper) {
            // nsc.AddScript<HelperScript>("HelperScript");
        }
        else {
            nsc.AddScript<CustomerScript>("CustomerScript");
        }

        builder.With<NativeScriptComponent>(nsc);

        builder.Build();
        spdlog::info("Zespawnowano nowego {} (Model: {}) - Patrzy na stolik!", isHelper ? "Helpera" : "Klienta", chosenModel);
        spdlog::info("Spawn klienta: Pozycja {}, Rotacja Y: {}",
            tc.GetPosition().y,
            finalRotation.y);
    }

    Entity FindEmptyChair()
    {
        auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
        if (!tags) return { std::numeric_limits<std::size_t>::max(), 0 };

        for (size_t i = 0; i < tags->dense.size(); ++i)
        {
            if (tags->dense[i].Tag == "Chair" || tags->dense[i].Tag == "Krzeslo")
            {
                Entity chairEntity = tags->reverse[i];
                if (IsChairEmpty(chairEntity))
                {
                    return chairEntity;
                }
            }
        }
        return { std::numeric_limits<std::size_t>::max(), 0 };
    }

    bool IsChairEmpty(Entity chair)
    {
        auto* chairTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(chair);
        if (!chairTransform) return false;

        glm::vec3 chairPos = chairTransform->GetPosition();
        glm::vec2 chairPos2D = { chairPos.x, chairPos.z };

        auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();

        if (!tags || !transforms) return true;

        for (size_t i = 0; i < tags->dense.size(); ++i)
        {
            std::string tag = tags->dense[i].Tag;
            if (tag == "NormalCustomer" || tag == "HelperCustomer")
            {
                Entity customer = tags->reverse[i];
                auto* custTransform = transforms->Get(customer);

                if (custTransform)
                {
                    glm::vec2 custPos2D = { custTransform->GetPosition().x, custTransform->GetPosition().z };
                    if (glm::distance(chairPos2D, custPos2D) < 0.5f)
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    // --- NOWA FUNKCJA DO ZNAJDOWANIA STO£U ---
    glm::vec3 FindNearestTablePosition(glm::vec3 chairPos)
    {
        // Jeœli z jakiegoœ powodu nie znajdzie sto³u, klient popatrzy po prostu w inn¹ stronê
        glm::vec3 nearestTablePos = chairPos + glm::vec3(0.0f, 0.0f, 1.0f);
        float closestDist = 999.0f;

        auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();

        if (tags && transforms) {
            for (size_t i = 0; i < tags->dense.size(); ++i) {
                std::string tag = tags->dense[i].Tag;

                // U¿ywamy .find(), ¿eby z³apaæ "Table_1", "Table_2" i inne wariacje!
                if (tag.find("Table") != std::string::npos ||
                    tag.find("Stolik") != std::string::npos)
                {
                    Entity tableEntity = tags->reverse[i];
                    auto* tableTransform = transforms->Get(tableEntity);

                    if (tableTransform) {
                        float dist = glm::distance(chairPos, tableTransform->GetPosition());
                        if (dist < closestDist) {
                            closestDist = dist;
                            nearestTablePos = tableTransform->GetPosition();
                        }
                    }
                }
            }
        }
        return nearestTablePos;
    }
};