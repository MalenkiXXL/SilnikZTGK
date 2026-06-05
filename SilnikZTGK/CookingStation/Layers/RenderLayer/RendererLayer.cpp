#include "RendererLayer.h"
#include "CookingStation/Renderer/Renderer.h"
#include "CookingStation/Renderer/RenderCommand.h"
#include "CookingStation/Scene/ecs.h"
#include "CookingStation/Layers/CameraLayer/Camera.h" 
#include <glm/gtc/matrix_transform.hpp>
#include "CookingStation/Layers/GuiLayer/Utils/Renderer2D.h"
#include "CookingStation/Layers/GuiLayer/Utils/Gui.h"
#include "CookingStation/Core/Input.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <thread>
#include <cstdlib>
#include <GLFW/glfw3.h>

#include "CookingStation/Scripts/ParticleEmitterScript.h"

void RendererLayer::OnAttach() {
    m_ShaderLibrary.Load("Standard", "shaders://vsShaders/shader.vert", "shaders://fragShaders/shader.frag");
    m_ShaderLibrary.Load("RAMP", "shaders://vsShaders/shader.vert", "shaders://fragShaders/RAMP.frag");
    m_ShaderLibrary.Load("FakeBRDF", "shaders://vsShaders/shader.vert", "shaders://fragShaders/FakeBRDF.frag");
    m_ShaderLibrary.Load("BlinnPhong", "shaders://vsShaders/shader.vert", "shaders://fragShaders/BlinnPhong.frag");
    m_ShaderLibrary.Load("Rim", "shaders://vsShaders/shader.vert", "shaders://fragShaders/Rim.frag");
    m_ShaderLibrary.Load("Conveyor", "shaders://vsShaders/shader.vert", "shaders://fragShaders/conveyor.frag");
    m_ShaderLibrary.Load("HighlightShader", "shaders://vsShaders/highlight.vert", "shaders://fragShaders/highlight.frag");
    m_ShaderLibrary.Load("ShadowMap", "shaders://vsShaders/shadow.vert", "shaders://fragShaders/shadow.frag");

    // --- ŁADOWANIE SHADERÓW POST-PROCESS ---
    m_ShaderLibrary.Load("BloomExtract", "shaders://vsShaders/postprocess.vert", "shaders://fragShaders/bloom_extract.frag");
    m_ShaderLibrary.Load("BloomBlur", "shaders://vsShaders/postprocess.vert", "shaders://fragShaders/bloom_blur.frag");
    m_ShaderLibrary.Load("BloomComposite", "shaders://vsShaders/postprocess.vert", "shaders://fragShaders/bloom_composite.frag");

    m_RampTexture = std::make_shared<Texture2D>("assets://textures/RAMP_texture.png");
    m_BackgroundTexture = std::make_shared<Texture2D>("assets://background/background.png");

    // --- SETUP FBO MAPY CIENI ---
    FramebufferSpecification shadowSpec;
    shadowSpec.Width = 4096;
    shadowSpec.Height = 4096;
    shadowSpec.DepthOnly = true;
    m_ShadowMapFBO = std::make_shared<Framebuffer>(shadowSpec);

    // --- SETUP FBO DLA POST-PROCESSingu ---
    // HDR = true — GL_RGBA16F pozwala przechowywać wartości > 1.0, co jest wymagane
    // do poprawnego Bloom. Bez tego wszystkie wartości są obcinane do [0,1].
    FramebufferSpecification ppSpec;
    ppSpec.Width = 1920 / 2;
    ppSpec.Height = 1080 / 2;
    ppSpec.HDR = true;
    m_PingPongFBO[0] = std::make_shared<Framebuffer>(ppSpec);
    m_PingPongFBO[1] = std::make_shared<Framebuffer>(ppSpec);

    FramebufferSpecification finalSpec;
    finalSpec.Width = 1920;
    finalSpec.Height = 1080;
    finalSpec.HDR = true;
    m_PostProcessFBO = std::make_shared<Framebuffer>(finalSpec);

    // --- QUAD EKRANOWY ---
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    glGenVertexArrays(1, &m_ScreenQuadVAO);
    glGenBuffers(1, &m_ScreenQuadVBO);
    glBindVertexArray(m_ScreenQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_ScreenQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

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

    // Rysowanie tła 2D
    glDisable(GL_DEPTH_TEST);
    glm::mat4 bgProjection = glm::ortho(0.0f, fboWidth, 0.0f, fboHeight, -1.0f, 1.0f);
    Renderer2D::BeginScene(bgProjection);
    Renderer2D::DrawQuad(
        glm::vec2(0.0f, 0.0f), glm::vec2(fboWidth, fboHeight),
        m_BackgroundTexture->GetRendererID(), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f)
    );
    Renderer2D::EndScene();
    glEnable(GL_DEPTH_TEST);

    if (activeScene->GetCamera()) {
        glm::mat4 view = activeScene->GetCamera()->GetViewMatrix();
        float safeZoom = std::max(activeScene->GetCamera()->Zoom, 1.0f);
        float orthoSize = 10.0f * (safeZoom / 45.0f);
        glm::mat4 projection = glm::ortho(-aspectRatio * orthoSize, aspectRatio * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
        glm::mat4 viewProjection = projection * view;

        activeScene->CalculateTransforms();

        // Zebranie obiektów do narysowania
        auto stdShader = m_ShaderLibrary.Get(Renderer::ActiveShader);
        auto conveyorShader = m_ShaderLibrary.Get("Conveyor");
        Frustum activeFrustum = ExtractFrustum(viewProjection);

        std::unordered_map<Model*, std::unordered_map<std::shared_ptr<Shader>, std::vector<InstanceData>>> instancedBatches;
        struct AnimatedDrawCmd {
            std::shared_ptr<Shader> shader;
            Model* model;
            InstanceData instanceData;
            AnimatorComponent* animComp;
        };
        std::vector<AnimatedDrawCmd> animatedDraws;

        if (meshStorage && transformStorage) {
            for (size_t i = 0; i < meshStorage->dense.size(); i++) {
                auto& meshComp = meshStorage->dense[i];
                Entity owner = meshStorage->reverse[i];
                TransformComponent* transform = transformStorage->Get(owner);

                if (transform && meshComp.ModelPtr) {
                    bool isVisible = false;
                    for (auto& mesh : meshComp.ModelPtr->meshes) {
                        AABB worldAABB = mesh.GetWorldAABB(transform->WorldMatrix);
                        if (IsOnFrustum(activeFrustum, worldAABB)) {
                            isVisible = true; break;
                        }
                    }

                    if (!isVisible) {
                        Renderer::GetStats().CulledObjects3D++; continue;
                    }

                    UVScrollComponent* scroll = scrollStorage ? scrollStorage->Get(owner) : nullptr;
                    float currentUVOffset = scroll ? scroll->Offset : 0.0f;
                    std::shared_ptr<Shader> shaderToUse = nullptr;

                    if (scroll) shaderToUse = conveyorShader;
                    else if (!meshComp.ShaderName.empty() && meshComp.ShaderName != "Standard") {
                        shaderToUse = m_ShaderLibrary.Exists(meshComp.ShaderName) ? m_ShaderLibrary.Get(meshComp.ShaderName) : stdShader;
                    }
                    else if (meshComp.ShaderPtr) shaderToUse = meshComp.ShaderPtr;
                    else shaderToUse = stdShader;

                    AnimatorComponent* animComp = animatorStorage ? animatorStorage->Get(owner) : nullptr;

                    if (animComp && animComp->AnimatorInstance) {
                        animatedDraws.push_back({ shaderToUse, meshComp.ModelPtr.get(), { transform->WorldMatrix, currentUVOffset, meshComp.HighlightColor }, animComp });
                    }
                    else {
                        instancedBatches[meshComp.ModelPtr.get()][shaderToUse].push_back({ transform->WorldMatrix, currentUVOffset, meshComp.HighlightColor });
                    }
                }
            }
        }

        // ===========================================================
        // FAZA 1: RENDEROWANIE MAPY CIENI (DEPTH PASS)
        // ===========================================================
        glm::vec3 sunDir = glm::normalize(glm::vec3(-0.321f, -0.766f, -0.557f));
        glm::vec3 sunTarget = activeScene->GetCamera()->Position; // Słońce podąża za kamerą!
        glm::vec3 sunPos = sunTarget - (sunDir * 25.0f);

        glm::mat4 lightView = glm::lookAt(sunPos, sunTarget, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 1.0f, 50.0f);
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        Renderer::BeginScene(viewProjection, lightSpaceMatrix, activeScene->GetCamera()->Position);

        m_ShadowMapFBO->Bind(); // Odpina TargetFBO — depth pass do oddzielnego bufora
        glClear(GL_DEPTH_BUFFER_BIT);

        auto shadowShader = m_ShaderLibrary.Get("ShadowMap");

        for (auto& [modelPtr, shaderMap] : instancedBatches) {
            std::vector<InstanceData> allInstances;
            for (auto& [shaderPtr, batchData] : shaderMap) {
                allInstances.insert(allInstances.end(), batchData.begin(), batchData.end());
            }
            shadowShader->use();
            shadowShader->setBool("u_Animated", false);
            Renderer::SubmitInstanced(shadowShader, modelPtr, allInstances);
        }

        for (auto& animDraw : animatedDraws) {
            shadowShader->use();
            shadowShader->setBool("u_Animated", true);
            shadowShader->setMat4Array("finalBonesMatrices", animDraw.animComp->AnimatorInstance->GetFinalBoneMatrices());
            std::vector<InstanceData> singleInstance = { animDraw.instanceData };
            Renderer::SubmitInstanced(shadowShader, animDraw.model, singleInstance);
        }

        // ===========================================================
        // FAZA 2: GŁÓWNE RENDEROWANIE 3D (z cieniami)
        // ===========================================================
        if (m_TargetFBO) m_TargetFBO->Bind(); // Wracamy do głównego bufora HDR
        glViewport(0, 0, fboWidth, fboHeight); // Przywracamy właściwy rozmiar okna

        if (Renderer::ActiveShader == "RAMP") m_RampTexture->Bind(10);

        // Zbindowanie tekstury cieni na slot 15
        glActiveTexture(GL_TEXTURE15);
        glBindTexture(GL_TEXTURE_2D, m_ShadowMapFBO->GetDepthAttachmentRendererID());

        for (auto& [modelPtr, shaderMap] : instancedBatches) {
            for (auto& [shaderPtr, batchData] : shaderMap) {
                shaderPtr->use();
                shaderPtr->setBool("u_Animated", false);
                shaderPtr->setInt("shadowMap", 15);
                Renderer::SubmitInstanced(shaderPtr, modelPtr, batchData);
            }
        }

        for (auto& animDraw : animatedDraws) {
            animDraw.shader->use();
            animDraw.shader->setBool("u_Animated", true);
            animDraw.shader->setInt("shadowMap", 15);
            animDraw.shader->setMat4Array("finalBonesMatrices", animDraw.animComp->AnimatorInstance->GetFinalBoneMatrices());
            std::vector<InstanceData> singleInstance = { animDraw.instanceData };
            Renderer::SubmitInstanced(animDraw.shader, animDraw.model, singleInstance);
            animDraw.shader->setBool("u_Animated", false);
        }

        Renderer::EndScene();

        // Rysowanie cząsteczek
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        Renderer2D::BeginScene(viewProjection);
        auto* scriptStorage = world.GetComponentVector<NativeScriptComponent>();
        if (scriptStorage && activeScene->GetCamera())
        {
            glm::vec3 camRight = activeScene->GetCamera()->Right;
            glm::vec3 camUp = activeScene->GetCamera()->Up;

            for (auto& scriptComp : scriptStorage->dense) {
                for (auto& scriptEl : scriptComp.Scripts) {
                    if ((scriptEl.Name == "ParticleEmitterScript" || scriptEl.Name == "SteamEmitterScript" || scriptEl.Name == "DustEmitterScript") && scriptEl.Instance) {
                        ParticleEmitterScript* emitter = dynamic_cast<ParticleEmitterScript*>(scriptEl.Instance);
                        if (emitter) {
                            for (const auto& particle : emitter->GetParticles()) {
                                if (!particle.Active || particle.LifeTime <= 0.0001f) continue;
                                float lifeRatio = particle.LifeRemaining / particle.LifeTime;
                                float currentSize = glm::mix(particle.SizeEnd, particle.SizeBegin, lifeRatio);
                                glm::vec4 currentColor = glm::mix(particle.ColorEnd, particle.ColorBegin, lifeRatio);
                                glm::mat4 transform = glm::translate(glm::mat4(1.0f), particle.Position);
                                transform[0] = glm::vec4(camRight * currentSize, 0.0f);
                                transform[1] = glm::vec4(camUp * currentSize, 0.0f);
                                transform[2] = glm::vec4(glm::cross(camRight, camUp) * currentSize, 0.0f);

                                if (particle.TextureID != 0) Renderer2D::DrawQuad(transform, particle.TextureID, currentColor);
                                else Renderer2D::DrawQuad(transform, currentColor);
                            }
                        }
                    }
                }
            }
        }
        Renderer2D::EndScene();
        glDepthMask(GL_TRUE);
    }

    // ===========================================================
    // FAZA POST-PROCESSING (BLOOM + COLOR GRADING)
    // ===========================================================
    if (m_TargetFBO) {
        // Sprawdzamy m_ResolveFBO przed użyciem — null dereference zabezpieczenie
        if (!m_ResolveFBO) {
            m_TargetFBO->Unbind();
            return;
        }

        // Zrzucamy obraz MSAA do zwykłego ResolveFBO
        m_TargetFBO->ResolveTo(m_ResolveFBO);
        m_TargetFBO->Unbind();

        // Zamykamy Blending — nie może wchodzić w interakcję z Bloomem
        glDisable(GL_BLEND);

        bool depthTestState = glIsEnabled(GL_DEPTH_TEST);
        glDisable(GL_DEPTH_TEST);

        // Dynamiczne dopasowanie rozdzielczości FBO pod obecny rozmiar
        uint32_t currentWidth = (uint32_t)fboWidth;
        uint32_t currentHeight = (uint32_t)fboHeight;

        if (m_PostProcessFBO->GetSpecification().Width != currentWidth || m_PostProcessFBO->GetSpecification().Height != currentHeight) {
            m_PostProcessFBO->Resize(currentWidth, currentHeight);
            m_PingPongFBO[0]->Resize(currentWidth / 2, currentHeight / 2);
            m_PingPongFBO[1]->Resize(currentWidth / 2, currentHeight / 2);
        }

        // 1. Wyciągamy jasne kolory do PingPongFBO[0]
        m_PingPongFBO[0]->Bind();
        auto extractShader = m_ShaderLibrary.Get("BloomExtract");
        extractShader->use();
        extractShader->setInt("sceneColor", 0);
        extractShader->setFloat("threshold", 0.8f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_ResolveFBO->GetColorAttachmentRendererID());

        glBindVertexArray(m_ScreenQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // 2. Ping-Pong Blur (10 przebiegów Gaussa)
        bool horizontal = true, first_iteration = true;
        int amount = 10;
        auto blurShader = m_ShaderLibrary.Get("BloomBlur");
        blurShader->use();
        blurShader->setInt("image", 0);

        for (unsigned int i = 0; i < amount; i++)
        {
            m_PingPongFBO[horizontal]->Bind();
            blurShader->setBool("horizontal", horizontal);
            glActiveTexture(GL_TEXTURE0);

            uint32_t textureToBind = first_iteration
                ? m_PingPongFBO[0]->GetColorAttachmentRendererID()
                : m_PingPongFBO[!horizontal]->GetColorAttachmentRendererID();
            glBindTexture(GL_TEXTURE_2D, textureToBind);

            glBindVertexArray(m_ScreenQuadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            horizontal = !horizontal;
            if (first_iteration) first_iteration = false;
        }

        // 3. Finalny kompozyt i Color Grading
        m_PostProcessFBO->Bind();
        auto compositeShader = m_ShaderLibrary.Get("BloomComposite");
        compositeShader->use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_ResolveFBO->GetColorAttachmentRendererID()); // Czysta scena
        compositeShader->setInt("sceneColor", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_PingPongFBO[!horizontal]->GetColorAttachmentRendererID()); // Rozmyty bloom
        compositeShader->setInt("bloomBlur", 1);

        compositeShader->setFloat("exposure", 1.8f);
        compositeShader->setFloat("bloomStrength", 0.25f);

        glBindVertexArray(m_ScreenQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        m_PostProcessFBO->Unbind();

        if (depthTestState) glEnable(GL_DEPTH_TEST);

        // 4. Rysowanie przetworzonego obrazu na ekran
#ifdef CS_DISTRIBUTION
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_PostProcessFBO->GetRendererID());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, currentWidth, currentHeight, 0, 0, (uint32_t)m_ViewportWidth, (uint32_t)m_ViewportHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
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