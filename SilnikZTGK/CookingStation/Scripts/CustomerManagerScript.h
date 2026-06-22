#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include <vector>
#include <string>
#include <random>
#include "CustomerScript.h"
#include "HelperCustomerScript.h" 

class CustomerManagerScript : public ScriptableEntity
{
private:
    float m_SpawnTimer = 0.0f;
    float m_SpawnInterval = 5.0f;
    int m_TotalSpawned = 0;

    std::vector<std::string> m_CustomerModels = {
        "assets://models/klienci/klient.gltf",
        "assets://models/klienci/klient2.gltf",
        "assets://models/klienci/klient3.gltf",
        "assets://models/klienci/klientka1.gltf",
        "assets://models/klienci/klientka2.gltf",
        "assets://models/klienci/klientka3.gltf",
    };

    std::vector<std::string> m_HelperModels = {
        "assets://models/warzywka/marchewka/marchewka.gltf",
        "assets://models/warzywka/pomidor/pomidor.gltf",
        "assets://models/warzywka/rzodkiewka/rzodkiewka.gltf"
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

        if (targetChair.id == std::numeric_limits<std::size_t>::max())
        {
            spdlog::info("Wszystkie krzesla zajete. Klient musi poczekac!");
            return;
        }

        m_TotalSpawned++;
        bool isGrandma = (m_TotalSpawned == 10);
        bool isHelper = (m_TotalSpawned % 3 == 0 && !isGrandma);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::string chosenModel;

        if (isGrandma) {
            chosenModel = "assets://models/klienci/babcia.gltf";
        }
        else if (isHelper) {
            std::uniform_int_distribution<> dist(0, (int)m_HelperModels.size() - 1);
            chosenModel = m_HelperModels[dist(gen)];
        }
        else {
            std::uniform_int_distribution<> dist(0, (int)m_CustomerModels.size() - 1);
            chosenModel = m_CustomerModels[dist(gen)];
        }

        glm::vec3 finalScale = isGrandma ? glm::vec3(0.3f) : glm::vec3(3.0f);
        float rotationOffsetY = 90.0f;

        float heightOffset = isGrandma ? 0.0f : 1.0f;

        std::string animPath = "";
        std::string cutAnimPath = "";
        std::string targetTag = "NormalCustomer";

        if (isGrandma)
        {
            animPath = "CookingStation/Assets/models/animacje/babcia/babcia-siedzi.gltf";
            targetTag = "GrandmaCustomer";
            rotationOffsetY = 0.0f;
        }
        else if (isHelper)
        {
            if (chosenModel.find("marchewka") != std::string::npos || chosenModel.find("test.gltf") != std::string::npos)
            {
                finalScale = glm::vec3(1.0f);
                rotationOffsetY = -90.0f;
                animPath = "CookingStation/Assets/models/animacje/klienci/marchewka-siedzi.gltf";
                cutAnimPath = "CookingStation/Assets/models/animacje/klienci/marchewka-kroi/marchewka-kroi.gltf";
                targetTag = "HelperCustomer_Marchewka";
            }
            else if (chosenModel.find("pomidor") != std::string::npos)
            {
                finalScale = glm::vec3(1.0f);
                rotationOffsetY = -90.0f;
                animPath = "CookingStation/Assets/models/animacje/klienci/pomidor-siedzi.gltf";
                cutAnimPath = "CookingStation/Assets/models/animacje/klienci/pomidor-kroi/pomidor-kroi.gltf";
                targetTag = "HelperCustomer_Pomidor";
            }
            else if (chosenModel.find("rzodkiewka") != std::string::npos)
            {
                finalScale = glm::vec3(1.2f);
                rotationOffsetY = 90.0f;
                animPath = "CookingStation/Assets/models/animacje/klienci/rzodkiewka-siedzi.gltf";
                cutAnimPath = "CookingStation/Assets/models/animacje/klienci/rzodkiewka-kroi/rzodkiewka-kroi.gltf";
                targetTag = "HelperCustomer_Rzodkiewka";
            }
        }
        else
        {
            animPath = "CookingStation/Assets/models/animacje/klienci/klient-siedzi.gltf";
        }

        auto builder = GetScene()->GetWorld().BuildEntity();
        builder.With<TagComponent>({ targetTag });

        auto* chairTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(targetChair);
        TransformComponent tc;

        glm::vec3 chairPos = chairTransform->GetPosition();

        auto* chairRel = GetScene()->GetWorld().GetComponent<RelationshipComponent>(targetChair);
        if (chairRel && chairRel->Parent != NULL_ENTITY) {
            Entity parentEntity;
            parentEntity.id = chairRel->Parent;
            auto* parentTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(parentEntity);
            if (parentTransform) {
                chairPos += parentTransform->GetPosition();
            }
        }

        glm::vec3 tablePos = FindNearestTablePosition(chairPos);
        glm::vec3 direction = tablePos - chairPos;
        float angle = glm::degrees(std::atan2(direction.x, direction.z));
        glm::vec3 finalRotation = { 0.0f, angle + rotationOffsetY, 0.0f };

        if (isGrandma) {
            tc.SetPosition(glm::vec3(-3.0f, chairPos.y + heightOffset, -37.0f));
        }
        else {
            tc.SetPosition(chairPos + glm::vec3(0.0f, heightOffset, 0.0f));
            tc.SetRotation(finalRotation);
        }

        tc.SetScale(finalScale);
        builder.With<TransformComponent>(tc);

        MeshComponent mesh;
        mesh.ModelPtr = AssetManager::GetModel(chosenModel);
        builder.With<MeshComponent>(mesh);

        BoxColliderComponent bc;
        bc.Size = { 1.0f, 2.0f, 1.0f };
        builder.With<BoxColliderComponent>(bc);

        AnimatorComponent animatorComp;
        animatorComp.AnimatorInstance = std::make_shared<Animator>();

        if (isGrandma) {
            auto walkAnim = std::make_shared<Animation>("CookingStation/Assets/models/animacje/babcia/babcia-chodzi.gltf", mesh.ModelPtr.get());
            animatorComp.AnimatorInstance->AddAnimation("Walk", walkAnim);
            animatorComp.AnimatorInstance->PlayAnimation("Walk");

            auto sitAnim = std::make_shared<Animation>(animPath, mesh.ModelPtr.get());
            animatorComp.AnimatorInstance->AddAnimation("SitIdle", sitAnim);
        }
        else {
            auto sitAnim = std::make_shared<Animation>(animPath, mesh.ModelPtr.get());
            animatorComp.AnimatorInstance->AddAnimation("SitIdle", sitAnim);
            animatorComp.AnimatorInstance->PlayAnimation("SitIdle");

            if (isHelper && !cutAnimPath.empty()) {
                auto cutAnim = std::make_shared<Animation>(cutAnimPath, mesh.ModelPtr.get());
                animatorComp.AnimatorInstance->AddAnimation("Cut", cutAnim);
            }
        }

        builder.With<AnimatorComponent>(animatorComp);

        NativeScriptComponent nsc;
        if (isHelper) {
            nsc.AddScript<HelperCustomerScript>("HelperCustomerScript");
        }
        else {
            nsc.AddScript<CustomerScript>("CustomerScript");
        }
        builder.With<NativeScriptComponent>(nsc);

        builder.Build();

        if (isGrandma) {
            CustomerScript::s_GrandmaTargetChair = targetChair;

            float grandmaSittingHeight = 0.8f;

            CustomerScript::s_GrandmaTargetPos = chairPos + glm::vec3(0.0f, grandmaSittingHeight, 0.0f);
            CustomerScript::s_GrandmaFinalRotation = finalRotation;
        }

        builder.Build();
        spdlog::info("Zespawnowano nowego {} (Model: {})", isHelper ? "Helpera" : "Klienta", chosenModel);
    }

    Entity FindEmptyChair()
    {
        auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
        if (!tags) return { std::numeric_limits<std::size_t>::max() };

        for (size_t i = 0; i < tags->dense.size(); ++i)
        {
            const std::string& tagName = tags->dense[i].Tag;
            if (tagName.find("Chair") != std::string::npos || tagName.find("Krzeslo") != std::string::npos) {
                Entity chairEntity = tags->reverse[i];
                if (IsChairEmpty(chairEntity))
                {
                    return chairEntity;
                }
            }
        }
        return { std::numeric_limits<std::size_t>::max() };
    }

    Entity FindChairNear(glm::vec3 pos)
    {
        auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();
        if (!tags || !transforms) return { std::numeric_limits<std::size_t>::max() };

        for (size_t i = 0; i < tags->dense.size(); ++i) {
            const std::string& tagName = tags->dense[i].Tag;
            if (tagName.find("Chair") != std::string::npos || tagName.find("Krzeslo") != std::string::npos) {
                Entity chairEntity = tags->reverse[i];
                auto* tf = transforms->Get(chairEntity);
                if (tf) {
                    glm::vec3 chairPos = tf->GetPosition();
                    auto* chairRel = GetScene()->GetWorld().GetComponent<RelationshipComponent>(chairEntity);
                    if (chairRel && chairRel->Parent != NULL_ENTITY) {
                        Entity parentEntity;
                        parentEntity.id = chairRel->Parent;
                        auto* parentTransform = transforms->Get(parentEntity);
                        if (parentTransform) chairPos += parentTransform->GetPosition();
                    }
                    if (std::abs(chairPos.x - pos.x) < 1.0f && std::abs(chairPos.z - pos.z) < 1.0f) {
                        return chairEntity;
                    }
                }
            }
        }
        return { std::numeric_limits<std::size_t>::max() };
    }

    bool IsChairEmpty(Entity chair)
    {
        if (chair.id != std::numeric_limits<std::size_t>::max() && chair.id == CustomerScript::s_GrandmaTargetChair.id) {
            return false;
        }

        auto* chairTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(chair);
        if (!chairTransform) return false;

        glm::vec3 chairPos = chairTransform->GetPosition();
        auto* chairRel = GetScene()->GetWorld().GetComponent<RelationshipComponent>(chair);
        if (chairRel && chairRel->Parent != NULL_ENTITY) {
            Entity parentEntity;
            parentEntity.id = chairRel->Parent;
            auto* parentTransform = GetScene()->GetWorld().GetComponent<TransformComponent>(parentEntity);
            if (parentTransform) {
                chairPos += parentTransform->GetPosition();
            }
        }

        glm::vec2 chairPos2D = { chairPos.x, chairPos.z };

        auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();

        if (!tags || !transforms) return true;

        for (size_t i = 0; i < tags->dense.size(); ++i)
        {
            std::string tag = tags->dense[i].Tag;
            // === ZMIANA: Dodano "ZadowolonyKlient" oraz "ZlyKlient" do sprawdzania zajętości krzesła ===
            if (tag == "NormalCustomer" || tag.find("HelperCustomer") != std::string::npos ||
                tag.find("NajedzonyPomocnik") != std::string::npos || tag == "GrandmaCustomer" ||
                tag == "ZadowolonyKlient" || tag == "ZlyKlient") {

                Entity customer = tags->reverse[i];
                auto* custTransform = transforms->Get(customer);

                if (custTransform)
                {
                    glm::vec2 custPos2D = { custTransform->GetPosition().x, custTransform->GetPosition().z };
                    if (glm::distance(chairPos2D, custPos2D) < 0.5f)
                    {
                        return false; // Krzesło jest wciąż fizycznie zajęte!
                    }
                }
            }
        }
        return true;
    }

    glm::vec3 FindNearestTablePosition(glm::vec3 chairPos)
    {
        glm::vec3 nearestTablePos = chairPos + glm::vec3(0.0f, 0.0f, 1.0f);
        float closestDist = 999.0f;

        auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();

        if (tags && transforms) {
            for (size_t i = 0; i < tags->dense.size(); ++i) {
                std::string tag = tags->dense[i].Tag;
                if (tag.find("Table") != std::string::npos || tag.find("stolik") != std::string::npos)
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