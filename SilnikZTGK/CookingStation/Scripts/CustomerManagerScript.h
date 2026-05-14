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

    // Lista dostêpnych modeli klientów (zgodna z tym co pisa³aœ)
    std::vector<std::string> m_CustomerModels = {
        "CookingStation/Assets/models/klienci/klient.gltf",
        "CookingStation/Assets/models/klienci/klient2.gltf",
        "CookingStation/Assets/models/klienci/klient3.gltf",
        "CookingStation/Assets/models/klienci/klientka1.gltf",
        "CookingStation/Assets/models/klienci/klientka2.gltf",
        "CookingStation/Assets/models/klienci/klientka3.gltf"
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

        // --- SKALOWANIE I POZYCJA ---
        // Obni¿y³em wysokoœæ spawnu (Y z 1.0 na 0.5) i zmniejszy³em skalê na 0.4.
        // Dopasuj te wartoœci tak, ¿eby ludziki ³adnie siedzia³y!
        tc.SetPosition(chairTransform->GetPosition() + glm::vec3(0.0f, 2.0f, 0.0f));
        tc.SetScale(glm::vec3(0.3f, 0.3f, 0.3f));

        tc.SetRotation(chairTransform->GetRotation());
        builder.With<TransformComponent>(tc);

        MeshComponent mesh;
        mesh.ModelPtr = AssetManager::GetModel(chosenModel);
        builder.With<MeshComponent>(mesh);

        BoxColliderComponent bc;
        bc.Size = { 1.0f, 2.0f, 1.0f };
        builder.With<BoxColliderComponent>(bc);

        NativeScriptComponent nsc;

        if (isHelper) {
            // nsc.AddScript<HelperScript>("HelperScript"); // Zrobimy potem
        }
        else {
            nsc.AddScript<CustomerScript>("CustomerScript"); // <--- BEZPOŒREDNIO!
        }

        builder.With<NativeScriptComponent>(nsc);

        builder.Build();
        spdlog::info("Zespawnowano nowego {} (Model: {})", isHelper ? "Helpera" : "Klienta", chosenModel);
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
                    return chairEntity; // Zwracamy PIERWSZE WOLNE krzes³o
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
        glm::vec2 chairPos2D = { chairPos.x, chairPos.z }; // Ignorujemy wysokoœæ Y

        auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();

        if (!tags || !transforms) return true;

        for (size_t i = 0; i < tags->dense.size(); ++i)
        {
            std::string tag = tags->dense[i].Tag;
            // Szukamy tylko wœród wygenerowanych klientów
            if (tag == "NormalCustomer" || tag == "HelperCustomer")
            {
                Entity customer = tags->reverse[i];
                auto* custTransform = transforms->Get(customer);

                if (custTransform)
                {
                    glm::vec2 custPos2D = { custTransform->GetPosition().x, custTransform->GetPosition().z };

                    // Jeœli jakiœ klient jest bli¿ej ni¿ pó³ metra od krzes³a, to krzes³o jest ZAJÊTE
                    if (glm::distance(chairPos2D, custPos2D) < 0.5f)
                    {
                        return false;
                    }
                }
            }
        }
        return true; // Krzes³o jest puste!
    }
};