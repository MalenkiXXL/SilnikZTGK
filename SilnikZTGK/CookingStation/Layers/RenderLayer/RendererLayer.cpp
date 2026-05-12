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
    // 1. Odbieranie powiadomienia od Pythona z w¹tku w tle
    if (m_GenerationDone) {
        LoadQuestFromFile("C:\\Inzynierka\\PlikPython\\wygenerowane_quests.json");
        m_IsGenerating = false;
        m_GenerationDone = false;
    }

    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();
    if (m_TargetFBO) m_TargetFBO->Bind();
    if (!activeScene) return;

    auto& world = activeScene->GetWorld();
    float fboWidth = m_TargetFBO ? (float)m_TargetFBO->GetSpecification().Width : m_ViewportWidth;
    float fboHeight = m_TargetFBO ? (float)m_TargetFBO->GetSpecification().Height : m_ViewportHeight;
    float aspectRatio = fboWidth / (fboHeight > 0 ? fboHeight : 1.0f);

    auto* colorStorage = world.GetComponentVector<ClearColorComponent>();
    auto* meshStorage = world.GetComponentVector<MeshComponent>();
    auto* transformStorage = world.GetComponentVector<TransformComponent>();
    auto* scrollStorage = world.GetComponentVector<UVScrollComponent>();

    glm::vec4 clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
    if (colorStorage && !colorStorage->dense.empty()) {
        clearColor = colorStorage->dense[0].bgColor;
    }

    RenderCommand::SetClearColor(clearColor);
    RenderCommand::Clear();

    glDisable(GL_DEPTH_TEST);

    // Tworzymy projekcjê ortograficzn¹, która idealnie pokrywa nasz ekran
    glm::mat4 bgProjection = glm::ortho(0.0f, fboWidth, 0.0f, fboHeight, -1.0f, 1.0f);

    Renderer2D::BeginScene(bgProjection);
    // Rysujemy quada na ca³y ekran. Rozmiar X to aspectRatio * 2, rozmiar Y to 2.0f
    Renderer2D::DrawQuad(glm::vec2(0.0f, 0.0f), glm::vec2(fboWidth, fboHeight), m_BackgroundTexture->GetRendererID());
    Renderer2D::EndScene();

    // W³¹czamy test g³êbokoœci z powrotem, aby modele 3D poprawnie siê nak³ada³y
    glEnable(GL_DEPTH_TEST);

    // ------- RYSOWANIE MODELI 3D ----------
    if (activeScene->GetCamera()) {
        glm::mat4 view = activeScene->GetCamera()->GetViewMatrix();
        float orthoSize = 10.0f * (activeScene->GetCamera()->Zoom / 45.0f);
        glm::mat4 projection = glm::ortho(-aspectRatio * orthoSize, aspectRatio * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
        glm::mat4 viewProjection = projection * view;

        // WA¯NE: Upewniamy siê, ¿e macierze œwiata s¹ aktualne przed cullingiem
        activeScene->CalculateTransforms();

        Renderer::BeginScene(viewProjection);

        // 1. POBIERAMY SHADERY
        auto stdShader = m_ShaderLibrary.Get(Renderer::ActiveShader);
        auto conveyorShader = m_ShaderLibrary.Get("Conveyor");

        // 2. USTAWIAMY WSPÓLNE DANE DLA OBU SHADERÓW
        std::vector<std::shared_ptr<Shader>> shaders = { stdShader, conveyorShader };
        for (auto& s : shaders) {
            if (!s) continue;
            s->use();
            s->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
            s->setVec3("sunDir", glm::vec3(-0.321f, -0.766f, -0.557f));
            s->setVec3("viewPos", activeScene->GetCamera()->Position);


            if (s == stdShader && Renderer::ActiveShader == "RAMP") {
                m_RampTexture->Bind(10);
            }
        }

        Frustum activeFrustum = ExtractFrustum(viewProjection);

        if (meshStorage && transformStorage) {
            // ZMIANA 1: Mapa teraz przechowuje nasz nowy wektor structów InstanceData
            std::unordered_map<Model*, std::pair<std::shared_ptr<Shader>, std::vector<InstanceData>>> instancedBatches;

            for (size_t i = 0; i < meshStorage->dense.size(); i++) {
                auto& meshComp = meshStorage->dense[i];
                Entity owner = meshStorage->reverse[i];
                TransformComponent* transform = transformStorage->Get(owner);

                if (transform && meshComp.ModelPtr) {
                    // --- FRUSTUM CULLING (zostaje jak by³o) ---
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

                    // --- NOWA LOGIKA WYBORU RENDERERA ---
                    UVScrollComponent* scroll = scrollStorage ? scrollStorage->Get(owner) : nullptr;

                    // Zbieramy offset. Jeœli obiekt nie ma komponentu scroll, wysy³amy po prostu 0.0f
                    float currentUVOffset = scroll ? scroll->Offset : 0.0f;

                    // Wybieramy shader: jeœli ma scrolla to conveyor, inaczej standardowy (lub nadpisany)
                    std::shared_ptr<Shader> shaderToUse = scroll ? conveyorShader : (meshComp.ShaderPtr ? meshComp.ShaderPtr : stdShader);

                    // ZMIANA 2: Grupujemy WSZYSTKO (taœmy i zwyk³e obiekty)
                    Model* modelKey = meshComp.ModelPtr.get();
                    instancedBatches[modelKey].first = shaderToUse;
                    instancedBatches[modelKey].second.push_back({ transform->WorldMatrix, currentUVOffset });
                }
            }

            // 3. RYSOWANIE PACZEK INSTANCJONOWANYCH (Teraz wyœle te¿ offsety!)
            for (auto& [modelPtr, batchData] : instancedBatches) {
                Renderer::SubmitInstanced(batchData.first, modelPtr, batchData.second);
            }
        }
        Renderer::EndScene();
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



// jakbyœmy kiedyœ potrzebowali znowu zmieniaæ shadery dla poszczególnych obiektów to tutaj poradnik

//if (meshStorage && transformStorage && tagStorage) { // Dodaj tagStorage do warunku
//    for (size_t i = 0; i < meshStorage->dense.size(); i++) {
//        auto& meshComp = meshStorage->dense[i];
//        Entity owner = meshStorage->reverse[i];
//        TransformComponent* transform = transformStorage->Get(owner);
//        TagComponent* tagComp = tagStorage->Get(owner); // Pobieramy tag obiektu
//
//        if (transform && meshComp.ModelPtr) {
//            // 1. ZAPAMIÊTUJEMY DOMYŒLNY SHADER
//            // (ten wybrany przez Ciebie w panelu bocznym edytora)
//            auto shaderToUse = m_ActiveShader;
//
//            // 2. LOGIKA PODMIANY TYLKO DLA BABÆ (NA POTRZEBY ZDJÊCIA)
//            if (tagComp) {
//                if (tagComp->Tag == "Babcia_Blinn")      shaderToUse = m_ShaderLibrary.Get("BlinnPhong");
//                else if (tagComp->Tag == "Babcia_BRDF")  shaderToUse = m_ShaderLibrary.Get("FakeBRDF");
//                else if (tagComp->Tag == "Babcia_RAMP")  shaderToUse = m_ShaderLibrary.Get("RAMP");
//                else if (tagComp->Tag == "Babcia_Rim")   shaderToUse = m_ShaderLibrary.Get("Rim");
//
//
//                if (shaderToUse != m_ActiveShader) {
//                    shaderToUse->use();
//                    // Przekazujemy parametry œwiat³a do nowego shadera
//                    shaderToUse->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
//                    shaderToUse->setVec3("lightPos", glm::vec3(5.0f, 5.0f, 10.0f));
//                    shaderToUse->setVec3("viewPos", activeScene->GetCamera()->Position);
//
//                    if (tagComp->Tag == "Babcia_RAMP") m_RampTexture->Bind(10);
//                }
//            }
//
//            // 3. RYSOWANIE
//            Renderer::Submit(shaderToUse, meshComp.ModelPtr, transform->WorldMatrix);
//
//            // 4. PRZYWRÓCENIE G£ÓWNEGO SHADERA (dla kolejnych obiektów w pêtli)
//            m_ActiveShader->use();
//        }
//    }
//}