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

    m_RampTexture = std::make_shared<Texture2D>("CookingStation/Assets/textures/RAMP_texture.png");

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

    glm::vec4 clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
    if (colorStorage && !colorStorage->dense.empty()) {
        clearColor = colorStorage->dense[0].bgColor;
    }

    RenderCommand::SetClearColor(clearColor);
    RenderCommand::Clear();

    // ------- RYSOWANIE MODELI 3D ----------
    if (activeScene->GetCamera()) {
        glm::mat4 view = activeScene->GetCamera()->GetViewMatrix();
        float orthoSize = 10.0f * (activeScene->GetCamera()->Zoom / 45.0f);
        glm::mat4 projection = glm::ortho(-aspectRatio * orthoSize, aspectRatio * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
        glm::mat4 viewProjection = projection * view;

        Renderer::BeginScene(viewProjection);

        // Wybieramy shader z biblioteki na podstawie zmiennej z GUI
        m_ActiveShader = m_ShaderLibrary.Get(Renderer::ActiveShader);

        m_ActiveShader->use();
        m_ActiveShader->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
        m_ActiveShader->setVec3("lightPos", glm::vec3(5.0f, 5.0f, 10.0f));
        m_ActiveShader->setVec3("viewPos", activeScene->GetCamera()->Position);

        // Bindujemy LUT jeœli aktywny jest RAMP
        if (Renderer::ActiveShader == "RAMP") {
            m_RampTexture->Bind(10);
        }

        // 1. Wyci¹gamy Frustum z aktualnej macierzy widoku-projekcji
        Frustum activeFrustum = ExtractFrustum(viewProjection);

        if (meshStorage && transformStorage) {
            // Mapa do grupowania obiektów
            std::unordered_map<Model*, std::pair<std::shared_ptr<Shader>, std::vector<glm::mat4>>> instancedBatches;

            for (size_t i = 0; i < meshStorage->dense.size(); i++) {
                auto& meshComp = meshStorage->dense[i];
                Entity owner = meshStorage->reverse[i];
                TransformComponent* transform = transformStorage->Get(owner);

                if (transform && meshComp.ModelPtr) {
                    bool isVisible = false;

                    // Sprawdzamy widocznoœæ modelu (testujemy ka¿d¹ jego siatkê)
                    for (auto& mesh : meshComp.ModelPtr->meshes) {
                        // U¿ywamy Twojej istniej¹cej metody GetWorldAABB z klasy Mesh
                        AABB worldAABB = mesh.GetWorldAABB(transform->WorldMatrix);

                        if (IsOnFrustum(activeFrustum, worldAABB)) {
                            isVisible = true;
                            break; // Wystarczy, ¿e jedna siatka modelu jest widoczna
                        }
                    }

                    if (isVisible) {
                        Model* modelKey = meshComp.ModelPtr.get();
                        instancedBatches[modelKey].first = meshComp.ShaderPtr ? meshComp.ShaderPtr : m_ActiveShader;
                        instancedBatches[modelKey].second.push_back(transform->WorldMatrix);
                    }
                    else {
                        // Aktualizujemy statystyki, które wyœwietlasz w GUI
                        Renderer::GetStats().CulledObjects3D++;
                    }
                }
            }

            // Rysowanie paczek
            for (auto& [modelPtr, batchData] : instancedBatches) {
                Renderer::SubmitInstanced(batchData.first, modelPtr, batchData.second);
            }
        }

        Renderer::EndScene();


        // =================================================================
        // RYSOWANIE QUESTÓW JAKO OBIEKT FIZYCZNY W ŒWIECIE GRY
        // =================================================================
        auto* tagStorage = world.GetComponentVector<TagComponent>();
        bool boardFound = false;
        glm::vec3 boardPos = { 0.0f, 0.0f, 0.0f };

        // Szukamy w œwiecie obiektu, nad którym ma wisieæ nasz tekst (Tablica/Marchewa itp.)
        if (tagStorage && transformStorage) {
            for (size_t i = 0; i < tagStorage->dense.size(); i++) {
                // TUTAJ WPISZ NAZWÊ OBIEKTU Z EDYTORA, np. "Tablica"
                if (tagStorage->dense[i].Tag == "Tablica") {
                    Entity e = tagStorage->reverse[i];
                    if (auto* trans = transformStorage->Get(e)) {
                        boardPos = { trans->WorldMatrix[3][0], trans->WorldMatrix[3][1], trans->WorldMatrix[3][2] };
                        boardFound = true;
                        break;
                    }
                }
            }
        }

        if (boardFound) {
            // Skoro jesteœmy w 3D, w³¹czamy Depth Test! 
            // Tekst schowa siê np. za grzybkiem, jeœli grzybek bêdzie bli¿ej kamery.
            glEnable(GL_DEPTH_TEST);

            // Wyci¹gamy rotacjê z widoku kamery i ODWRACAMY J¥.
            // Dziêki temu tablica z tekstem bêdzie ZAWSZE patrzyæ przodem prosto do kamery gracza! (tzw. Billboarding)
            glm::mat4 invView = glm::inverse(view);
            glm::mat4 textRotation = glm::mat4(glm::mat3(invView));

            // Przesuwamy obiekt o 2 jednostki do góry w osi Y wzglêdem modelu tablicy
            glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), boardPos + glm::vec3(0.0f, 2.0f, 0.0f)) * textRotation;

            // Zbudowanie specjalnej macierzy dla Renderer2D
            glm::mat4 custom3DViewProj = projection * view * modelMatrix;

            Renderer2D::BeginScene(custom3DViewProj);

            // Skala jest u³amkowa, bo w przestrzeni 3D rysujemy w metrach (1.0 = 1 metr wielkoœci liter)
            float scale = 0.015f;

            // Rysujemy fizyczne t³o tablicy (wymiary w metrach)
            Renderer2D::DrawQuad({ -1.5f, -0.5f }, { 3.0f, 2.5f }, { 0.15f, 0.15f, 0.15f, 0.85f });

            // Rysowanie poszczególnych linijek
            float textY = 0.0f;
            Gui::DrawGuiText("ZLECENIA AI:", { -1.3f, textY }, scale + 0.005f, { 1.0f, 0.8f, 0.2f, 1.0f });
            textY += 0.3f;

            for (const auto& quest : m_ActiveQuests) {
                Gui::DrawGuiText(quest.Title, { -1.3f, textY }, scale, { 1.0f, 1.0f, 1.0f, 1.0f });
                textY += 0.2f;
                Gui::DrawGuiText(quest.DishID + " (x" + std::to_string(quest.Portions) + ")", { -1.2f, textY }, scale * 0.8f, { 0.7f, 0.7f, 0.7f, 1.0f });
                textY += 0.3f;
            }

            // Oznaczenie statusu i przycisk klawiaturowy
            std::string statusText = m_IsGenerating ? "Generowanie..." : "[Nacisnij ENTER aby pobrac nowe]";
            glm::vec4 statusColor = m_IsGenerating ? glm::vec4(1.0f, 0.2f, 0.2f, 1.0f) : glm::vec4(0.4f, 1.0f, 0.4f, 1.0f);

            Gui::DrawGuiText(statusText, { -1.3f, textY + 0.1f }, scale, statusColor);

            Renderer2D::EndScene();

            // POBIERANIE W TLE (Poniewa¿ tablica jest w œwiecie 3D, zrezygnowaliœmy z klikania na ni¹ myszk¹. U¿ywamy klawisza ENTER).
            if (Input::IsKeyPressed(GLFW_KEY_ENTER) && !m_IsGenerating) {
                m_IsGenerating = true;
                spdlog::info("Rozpoczeto asynchroniczne generowanie questow z News API...");

                std::thread([this]() {
                    std::system("cd C:\\Inzynierka\\PlikPython && .venv\\Scripts\\python.exe main.py");
                    m_GenerationDone = true;
                    }).detach();
            }
        }
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