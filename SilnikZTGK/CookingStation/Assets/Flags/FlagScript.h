#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Renderer/Texture2D.h"
#include "CookingStation/Renderer/Model.h" 
#include <memory>
#include <string>
#include <algorithm>
#include <cctype>

class FlagScript : public ScriptableEntity
{
public:
    std::string CountryCode;

    void OnCreate() override
    {
        // 1. Odczytujemy kod kraju z przypisanego tagu
        auto* tagComp = GetComponent<TagComponent>();
        if (tagComp) {
            std::string tag = tagComp->Tag;
            size_t pos = tag.find("_");
            if (pos != std::string::npos) {
                CountryCode = tag.substr(pos + 1);
            }
        }

        // 2. Klonowanie modelu i podmiana tekstury na ¿ywo z pliku PNG
        auto* meshComp = GetComponent<MeshComponent>();
        if (meshComp && meshComp->ModelPtr && !CountryCode.empty())
        {
            // Konwersja kodu kraju na ma³e litery, aby pasowa³ do nazwy pliku (np. "US" -> "us")
            std::string fileName = CountryCode;
            std::transform(fileName.begin(), fileName.end(), fileName.begin(), [](unsigned char c) { return std::tolower(c); });

            // Definiujemy œcie¿kê w VFS do pliku z flag¹
            std::string texturePath = "assets://textures/flags/" + fileName + ".png";

            // £adujemy now¹ teksturê bezpoœrednio z dysku
            auto newTexture = std::make_shared<Texture2D>(texturePath);

            auto instancedModel = std::make_shared<Model>(*meshComp->ModelPtr);

            spdlog::info("Model flagi ma {} sub-siatek.", instancedModel->meshes.size());

            if (!instancedModel->meshes.empty())
            {
                int targetMeshIndex = 2;

                if (!instancedModel->meshes[targetMeshIndex].textures.empty()) {
                    instancedModel->meshes[targetMeshIndex].textures[0].Texture2DPtr = newTexture;
                }
                else {
                    MeshTexture flagTex;
                    flagTex.type = "texture_diffuse";
                    flagTex.path = texturePath;
                    flagTex.Texture2DPtr = newTexture;
                    instancedModel->meshes[targetMeshIndex].textures.push_back(flagTex);
                }
            }

            meshComp->ModelPtr = instancedModel;
        }

        // 3. Zapis bazowej wysokoœci do animacji falowania
        auto* tf = GetComponent<TransformComponent>();
        if (tf) {
            m_BaseY = tf->GetPosition().y;
        }
    }

    void OnUpdate(Timestep ts) override
    {
        m_WaveTimer += ts.GetSeconds();
        auto* tf = GetComponent<TransformComponent>();
        if (tf) {
            glm::vec3 pos = tf->GetPosition();
            pos.y = m_BaseY + std::sin(m_WaveTimer * 1.5f) * 0.08f;
            tf->SetPosition(pos);
        }
    }

    void SetBaseY(float y) { m_BaseY = y; }

private:
    float m_WaveTimer = 0.0f;
    float m_BaseY = 0.0f;
};