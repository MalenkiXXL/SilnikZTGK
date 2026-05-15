#include "RendererLayer.h"
#include "CookingStation/Renderer/Renderer.h"
#include "CookingStation/Renderer/RenderCommand.h"
#include "CookingStation/Scene/ecs.h"
#include "CookingStation/Layers/CameraLayer/Camera.h" 
#include <glm/gtc/matrix_transform.hpp>
// Potrzebujemy dostêpu do GUI i Renderera2D, ¿eby rysowaæ tekst jako obiekt
#include "CookingStation/Layers/GuiLayer/Renderer2D.h"
#include "CookingStation/Layers/GuiLayer/Gui.h"
#include "CookingStation/Core/Input.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <thread>
#include <cstdlib>
#include <GLFW/glfw3.h>

#include "CookingStation/Scripts/ParticleEmitterScript.h"

void RendererLayer::OnAttach() {
    m_ShaderLibrary.Load("Standard", "CookingStation/Shaders/vsShaders/shader.vs", "CookingStation/Shaders/fragShaders/shader.frag");
    m_ShaderLibrary.Load("RAMP", "CookingStation/Shaders/vsShaders/shader.vs", "CookingStation/Shaders/fragShaders/RAMP.frag");
    m_ShaderLibrary.Load("FakeBRDF", "CookingStation/Shaders/vsShaders/shader.vs", "CookingStation/Shaders/fragShaders/FakeBRDF.frag");
    m_ShaderLibrary.Load("BlinnPhong", "CookingStation/Shaders/vsShaders/shader.vs", "CookingStation/Shaders/fragShaders/BlinnPhong.frag");
    m_ShaderLibrary.Load("Rim", "CookingStation/Shaders/vsShaders/shader.vs", "CookingStation/Shaders/fragShaders/Rim.frag");
    m_ShaderLibrary.Load("Conveyor", "CookingStation/Shaders/vsShaders/shader.vs", "CookingStation/Shaders/fragShaders/conveyor.frag");

    m_RampTexture = std::make_shared<Texture2D>("CookingStation/Assets/textures/RAMP_texture.png");
    m_BackgroundTexture = std::make_shared<Texture2D>("CookingStation/Assets/background/background.png");

    auto rampShader = m_ShaderLibrary.Get("RAMP");
    rampShader->use();
    rampShader->setInt("rampTex", 10);

    LoadQuestFromFile("C:\\Inzynierka\\PlikPython\\wygenerowane_quests.json");
}

void RendererLayer::LoadQuestFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return;

    try {
        nlohmann::json data = nlohmann::json::parse(file);
        m_ActiveQuests.clear();
        for (auto& item : data) {
            Quest q;
            q.Title = item.value("title", "Brak tytulu");
            q.DishID = item.value("dish_id", "");
            q.Portions = item.value("portions", 0);
            q.Reward = item.value("reward", "");
            m_ActiveQuests.push_back(q);
        }
        spdlog::info("RendererLayer: Wczytano {} questow z AI do swiata gry!", m_ActiveQuests.size());
    }
    catch (nlohmann::json::exception& e) {
        spdlog::error("Blad parsowania JSON questow: {}", e.what());
    }
}

void RendererLayer::OnUpdate(Timestep ts) {
    if (m_GenerationDone) {
        LoadQuestFromFile("C:\\Inzynierka\\PlikPython\\wygenerowane_quests.json");
        m_IsGenerating = false;
        m_GenerationDone = false;
    }

    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();

    if (m_TargetFBO) m_TargetFBO->Bind();

    if (!activeScene) {
        if (m_TargetFBO) m_TargetFBO->Unbind();
        return;
    }

    auto& world = activeScene->GetWorld();
    float fboWidth = m_TargetFBO ? (float)m_TargetFBO->GetSpecification().Width : m_ViewportWidth;
    float fboHeight = m_TargetFBO ? (float)m_TargetFBO->GetSpecification().Height : m_ViewportHeight;
    float aspectRatio = fboWidth / (fboHeight > 0 ? fboHeight : 1.0f);

    auto* colorStorage = world.GetComponentVector<ClearColorComponent>();
    auto* meshStorage = world.GetComponentVector<MeshComponent>();
    auto* transformStorage = world.GetComponentVector<TransformComponent>();
    auto* scrollStorage = world.GetComponentVector<UVScrollComponent>();
    auto* animatorStorage = world.GetComponentVector<AnimatorComponent>();

    glm::vec4 clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
    if (colorStorage && !colorStorage->dense.empty()) {
        clearColor = colorStorage->dense[0].bgColor;
    }

    RenderCommand::SetClearColor(clearColor);
    RenderCommand::Clear();

    glDisable(GL_DEPTH_TEST);

    glm::mat4 bgProjection = glm::ortho(0.0f, fboWidth, 0.0f, fboHeight, -1.0f, 1.0f);
    Renderer2D::BeginScene(bgProjection);
    Renderer2D::DrawQuad(glm::vec2(0.0f, 0.0f), glm::vec2(fboWidth, fboHeight), m_BackgroundTexture->GetRendererID());
    Renderer2D::EndScene();

    glEnable(GL_DEPTH_TEST);

    if (activeScene->GetCamera()) {
        glm::mat4 view = activeScene->GetCamera()->GetViewMatrix();
        float orthoSize = 10.0f * (activeScene->GetCamera()->Zoom / 45.0f);
        glm::mat4 projection = glm::ortho(-aspectRatio * orthoSize, aspectRatio * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
        glm::mat4 viewProjection = projection * view;

        activeScene->CalculateTransforms();

        // Wysy³amy viewProjection + pozycjê kamery do UBO - shadery dostaj¹ dane automatycznie
        Renderer::BeginScene(viewProjection, activeScene->GetCamera()->Position);

        auto stdShader = m_ShaderLibrary.Get(Renderer::ActiveShader);
        auto conveyorShader = m_ShaderLibrary.Get("Conveyor");

        // Jedyna rzecz wymagaj¹ca rêcznego ustawienia per-shader: tekstura rampy (slot tekstury, nie UBO)
        if (Renderer::ActiveShader == "RAMP") {
            m_RampTexture->Bind(10);
        }

        Frustum activeFrustum = ExtractFrustum(viewProjection);

        if (meshStorage && transformStorage) {

            std::unordered_map<Model*, std::pair<std::shared_ptr<Shader>, std::vector<InstanceData>>> instancedBatches;

            struct AnimatedDrawCmd {
                std::shared_ptr<Shader> shader;
                Model* model;
                InstanceData instanceData;
                AnimatorComponent* animComp;
            };
            std::vector<AnimatedDrawCmd> animatedDraws;

            for (size_t i = 0; i < meshStorage->dense.size(); i++) {
                auto& meshComp = meshStorage->dense[i];
                Entity owner = meshStorage->reverse[i];
                TransformComponent* transform = transformStorage->Get(owner);

                if (transform && meshComp.ModelPtr) {
                    bool isVisible = false;
                    for (auto& mesh : meshComp.ModelPtr->meshes) {
                        AABB worldAABB = mesh.GetWorldAABB(transform->WorldMatrix);
                        if (IsOnFrustum(activeFrustum, worldAABB)) {
                            isVisible = true;
                            break;
                        }
                    }

                    if (!isVisible) {
                        Renderer::GetStats().CulledObjects3D++;
                        continue;
                    }

                    UVScrollComponent* scroll = scrollStorage ? scrollStorage->Get(owner) : nullptr;
                    float currentUVOffset = scroll ? scroll->Offset : 0.0f;
                    std::shared_ptr<Shader> shaderToUse = scroll ? conveyorShader : (meshComp.ShaderPtr ? meshComp.ShaderPtr : stdShader);

                    AnimatorComponent* animComp = animatorStorage ? animatorStorage->Get(owner) : nullptr;

                    if (animComp && animComp->AnimatorInstance) {
                        animatedDraws.push_back({ shaderToUse, meshComp.ModelPtr.get(), { transform->WorldMatrix, currentUVOffset }, animComp });
                    }
                    else {
                        Model* modelKey = meshComp.ModelPtr.get();
                        instancedBatches[modelKey].first = shaderToUse;
                        instancedBatches[modelKey].second.push_back({ transform->WorldMatrix, currentUVOffset });
                    }
                }
            }

            for (auto& [modelPtr, batchData] : instancedBatches) {
                batchData.first->use();
                batchData.first->setBool("u_Animated", false);
                Renderer::SubmitInstanced(batchData.first, modelPtr, batchData.second);
            }

            for (auto& animDraw : animatedDraws) {
                animDraw.shader->use();
                animDraw.shader->setBool("u_Animated", true);

                const auto& finalBones = animDraw.animComp->AnimatorInstance->GetFinalBoneMatrices();
                for (int j = 0; j < (int)finalBones.size(); ++j) {
                    animDraw.shader->setMat4("finalBonesMatrices[" + std::to_string(j) + "]", finalBones[j]);
                }

                std::vector<InstanceData> singleInstance = { animDraw.instanceData };
                Renderer::SubmitInstanced(animDraw.shader, animDraw.model, singleInstance);

                animDraw.shader->setBool("u_Animated", false);
            }
        }
        Renderer::EndScene();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        Renderer2D::BeginScene(viewProjection);

        auto* scriptStorage = world.GetComponentVector<NativeScriptComponent>();
        if (scriptStorage && activeScene->GetCamera())
        {
            glm::vec3 camRight = activeScene->GetCamera()->Right;
            glm::vec3 camUp = activeScene->GetCamera()->Up;

            for (auto& scriptComp : scriptStorage->dense)
            {
                for (auto& scriptEl : scriptComp.Scripts)
                {
                    if ((scriptEl.Name == "ParticleEmitterScript" || scriptEl.Name == "SteamEmitterScript") && scriptEl.Instance)
                    {
                        ParticleEmitterScript* emitter = static_cast<ParticleEmitterScript*>(scriptEl.Instance);

                        for (const auto& particle : emitter->GetParticles())
                        {
                            if (!particle.Active) continue;

                            float lifeRatio = particle.LifeRemaining / particle.LifeTime;
                            float currentSize = glm::mix(particle.SizeEnd, particle.SizeBegin, lifeRatio);
                            glm::vec4 currentColor = glm::mix(particle.ColorEnd, particle.ColorBegin, lifeRatio);

                            glm::mat4 transform = glm::translate(glm::mat4(1.0f), particle.Position);
                            transform[0] = glm::vec4(camRight * currentSize, 0.0f);
                            transform[1] = glm::vec4(camUp * currentSize, 0.0f);
                            transform[2] = glm::vec4(glm::cross(camRight, camUp) * currentSize, 0.0f);

                            if (particle.TextureID != 0)
                                Renderer2D::DrawQuad(transform, particle.TextureID, currentColor);
                            else
                                Renderer2D::DrawQuad(transform, currentColor);
                        }
                    }
                }
            }
        }

        Renderer2D::EndScene();
        glDepthMask(GL_TRUE);
    }

    if (m_TargetFBO) {
        if (m_ResolveFBO) {
            m_TargetFBO->ResolveTo(m_ResolveFBO);
        }
        m_TargetFBO->Unbind();
    }
}

void RendererLayer::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) {
        return OnWindowResize(ev);
        });
}

bool RendererLayer::OnWindowResize(WindowResizeEvent& e) {
    m_ViewportWidth = (float)e.GetWidth();
    m_ViewportHeight = (float)e.GetHeight();
    glViewport(0, 0, e.GetWidth(), e.GetHeight());
    return false;
}
