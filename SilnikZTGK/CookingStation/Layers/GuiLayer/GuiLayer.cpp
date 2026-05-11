#include "GuiLayer.h"
#include "CookingStation/Events/WindowEvent.h"
#include "CookingStation/Events/KeyEvent.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Scene/ecs.h"
#include "CookingStation/Events/EditorEvents.h"
#include "CookingStation/Core/Application.h"
#include <cstdlib> 
#include <cstdio>
#include <fstream>

// SYSTEM KOTWIC (VIRTUAL ANCHORING)
enum class Anchor {
    TopLeft, TopRight, BottomLeft, BottomRight, Center
};

glm::vec2 GetAnchoredPosition(Anchor anchor, float offsetX, float offsetY, float width, float height, float screenWidth, float screenHeight) {
    switch (anchor) {
    case Anchor::TopLeft:     return { offsetX, offsetY };
    case Anchor::TopRight:    return { screenWidth - width - offsetX, offsetY };
    case Anchor::BottomLeft:  return { offsetX, screenHeight - height - offsetY };
    case Anchor::BottomRight: return { screenWidth - width - offsetX, screenHeight - height - offsetY };
    case Anchor::Center:      return { (screenWidth - width) / 2.0f + offsetX, (screenHeight - height) / 2.0f + offsetY };
    }
    return { offsetX, offsetY };
}

void GuiLayer::OnAttach() {
    Renderer2D::Init();

    // pobieramy aktualny rozmiar okna
    auto windowSize = Input::GetWindowSize();
    m_ViewportWidth = (float)windowSize.first;
    m_ViewportHeight = (float)windowSize.second;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // aktualizacja rozmiaru ekranu 
    Gui::UpdateScreenSize(m_ViewportWidth, m_ViewportHeight);

    // wczytanie czcionki
    Gui::Init("CookingStation/Assets/fonts/ARIAL.ttf", 32);
}

void GuiLayer::OnUpdate(Timestep ts) {

    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();

    if (!activeScene) {
        spdlog::error("AssetLayer: Brak aktywnej sceny!");
        return;
    }

    auto& world = activeScene->GetWorld();

    glDisable(GL_DEPTH_TEST);

    glm::mat4 uiProj = glm::ortho(0.0f, m_ViewportWidth, m_ViewportHeight, 0.0f);
    Renderer2D::BeginScene(uiProj);

    if (m_ViewportFBO) {
        glm::vec2 viewportPos = { 200.0f, 30.0f };
        glm::vec2 viewportSize = { m_ViewportWidth - 500.0f, m_ViewportHeight - 230.0f };

        if (viewportSize.x > 0.0f && viewportSize.y > 0.0f) {
            if (m_ViewportFBO->GetSpecification().Width != (uint32_t)viewportSize.x ||
                m_ViewportFBO->GetSpecification().Height != (uint32_t)viewportSize.y)
            {
                m_ViewportFBO->Resize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
            }

            uint32_t textureID = m_ViewportFBO->GetColorAttachmentRendererID();
            Renderer2D::DrawQuad(viewportPos, viewportSize, textureID, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f });
        }
    }

    // --- G£ÓWNY PASEK ZADAÑ (TOP-LEFT ANCHOR IMPLICITLY) ---
    Renderer2D::DrawQuad({ 0.0f, 0.0f }, { m_ViewportWidth, 30.0f }, { 0.15f, 0.15f, 0.15f, 1.0f });

    if (Gui::Button("Plik", { 10.0f, 5.0f }, { 80.0f, 20.0f })) {
        m_ShowFileMenu = !m_ShowFileMenu;
        m_ShowViewMenu = false;
    }
    if (Gui::Button("Widok", { 100.0f, 5.0f }, { 80.0f, 20.0f })) {
        m_ShowViewMenu = !m_ShowViewMenu;
        m_ShowFileMenu = false;
    }

    glm::vec2 playButtonPos = { 190.0f, 5.0f };
    glm::vec2 playButtonSize = { 80.0f, 20.0f };

    if (activeScene->GetState() == SceneState::Edit) {
        if (Gui::Button("PLAY", playButtonPos, playButtonSize)) {
            ScenePlayEvent e;
            Application::Get().OnEvent(e);
        }
    }
    else {
        if (Gui::Button("STOP", playButtonPos, playButtonSize)) {
            SceneStopEvent e;
            Application::Get().OnEvent(e);
        }
    }

    auto& gridReq = activeScene->GetGridRequest();
    std::string gridText = gridReq.Active ? "SIATKA: WL" : "SIATKA: WYL";
    if (Gui::Button(gridText, { 280.0f, 5.0f }, { 100.0f, 20.0f }, gridReq.Active)) {
        gridReq.Active = !gridReq.Active;
    }

    if (m_ShowFileMenu) {
        Renderer2D::DrawQuad({ 10.0f, 30.0f }, { 150.0f, 90.0f }, { 0.2f, 0.2f, 0.2f, 0.9f });
        if (Gui::Button("Zapisz", { 15.0f, 35.0f }, { 140.0f, 25.0f })) {
            m_ShowSaveDialog = true;
            m_ShowFileMenu = false;
        }
        if (Gui::Button("Wczytaj", { 15.0f, 65.0f }, { 140.0f, 25.0f })) {
            m_ShowLoadDialog = true;
            m_ShowFileMenu = false;
        }
    }

    if (m_ShowViewMenu) {
        Renderer2D::DrawQuad({ 100.0f, 30.0f }, { 160.0f, 160.0f }, { 0.2f, 0.2f, 0.2f, 0.9f });
        if (Gui::Button("Panel Otoczenia", { 105.0f, 35.0f }, { 150.0f, 25.0f }, m_ShowEnvironmentPanel)) {
            m_ShowEnvironmentPanel = !m_ShowEnvironmentPanel; m_ShowViewMenu = false;
        }
        if (Gui::Button("Hierarchia", { 105.0f, 65.0f }, { 150.0f, 25.0f }, m_ShowHierarchyPanel)) {
            m_ShowHierarchyPanel = !m_ShowHierarchyPanel; m_ShowViewMenu = false;
        }
        if (Gui::Button("Biblioteka", { 105.0f, 95.0f }, { 150.0f, 25.0f }, m_ShowLibraryPanel)) {
            m_ShowLibraryPanel = !m_ShowLibraryPanel; m_ShowViewMenu = false;
        }
        if (Gui::Button("Inspektor", { 105.0f, 125.0f }, { 150.0f, 25.0f }, m_ShowInspectorPanel)) {
            m_ShowInspectorPanel = !m_ShowInspectorPanel; m_ShowViewMenu = false;
        }
        if (Gui::Button("Diagnostyka", { 105.0f, 155.0f }, { 150.0f, 25.0f }, m_ShowDiagnosticPanel)) {
            m_ShowDiagnosticPanel = !m_ShowDiagnosticPanel; m_ShowViewMenu = false;
        }
    }

    // --- PROTOTYP QUESTÓW (ANCHOR: TOP LEFT) ---
    glm::vec2 questPanelPos = GetAnchoredPosition(Anchor::TopLeft, 10.0f, 150.0f, 180.0f, 85.0f, m_ViewportWidth, m_ViewportHeight);
    Renderer2D::DrawQuad(questPanelPos, { 180.0f, 85.0f }, { 0.15f, 0.15f, 0.15f, 0.9f });
    Gui::DrawGuiText("Generator Questow:", { questPanelPos.x + 5.f, questPanelPos.y + 10.f }, 0.45f, { 1.0f, 0.8f, 0.2f, 1.0f });

    if (Gui::Button("Generuj (Stary Cache)", { questPanelPos.x + 5.f, questPanelPos.y + 30.f }, { 170.f, 20.f })) {
        spdlog::info("Generowanie z istniejacego cache...");
        system("python CookingStation/Tools/QuestGenerator/main.py");
        ReloadQuests();
    }
    if (Gui::Button("Generuj (Nowe Newsy)", { questPanelPos.x + 5.f, questPanelPos.y + 55.f }, { 170.f, 20.f })) {
        spdlog::info("Czyszczenie cache i pobieranie nowych newsow...");
        std::remove("CookingStation/Assets/news_cache.json");
        system("python CookingStation/Tools/QuestGenerator/main.py");
        ReloadQuests();
    }

    // --- PANEL DIAGNOSTYCZNY (ANCHOR: BOTTOM RIGHT) ---
    if (m_ShowDiagnosticPanel) {
        m_StatsUpdateTimer += ts.GetSeconds();
        if (m_StatsUpdateTimer >= 0.25f) {
            auto stats = Renderer::GetStats();
            float fps = 1.0f / ts.GetSeconds();
            float frameTime = ts.GetMilliSeconds();
            auto formatFloat = [](float v) {
                char buffer[32]; snprintf(buffer, sizeof(buffer), "%.2f", v); return std::string(buffer);
                };

            m_FpsText = "FPS: " + std::to_string((int)fps);
            m_FrameTimeText = "Frame Time: " + formatFloat(frameTime) + " ms";
            m_CpuText = "CPU Logika: " + formatFloat(stats.CPULogicTime) + " ms";
            m_GpuText = "GPU Render: " + formatFloat(stats.GPURenderTime) + " ms";
            m_DrawCalls3DText = "Draw Calls (3D): " + std::to_string(stats.DrawCalls3D);
            m_Tris3DText = "Trojkaty (3D): " + std::to_string(stats.TriangleCount3D);
            m_Culled3DText = "Odrzucone (Culled): " + std::to_string(stats.CulledObjects3D);
            m_DrawCallsUIText = "Draw Calls (UI): " + std::to_string(stats.DrawCallsUI);
            m_TrisUIText = "Trojkaty (UI): " + std::to_string(stats.TriangleCountUI);
            m_StatsUpdateTimer = 0.0f;
        }

        glm::vec2 panelSize(280.0f, 250.0f);
        glm::vec2 panelPos = GetAnchoredPosition(Anchor::BottomRight, 10.0f, 10.0f, panelSize.x, panelSize.y, m_ViewportWidth, m_ViewportHeight);
        Renderer2D::DrawQuad(panelPos, panelSize, { 0.12f, 0.12f, 0.12f, 0.85f });

        float textX = panelPos.x + 15.0f; float textY = panelPos.y + 5.0f; float lineOffset = 25.0f; float scale = 0.6f;
        glm::vec4 textColor(1.0f, 1.0f, 1.0f, 1.0f);
        glm::vec4 highlightColor(1.0f, 0.8f, 0.2f, 1.0f);

        Gui::DrawGuiText("Diagnostyka Projektu:", { textX, textY }, scale + 0.1f, { 0.2f, 0.8f, 0.2f, 1.0f }); textY += lineOffset + 5.0f;
        Gui::DrawGuiText(m_FpsText, { textX, textY }, scale, textColor); textY += lineOffset;
        Gui::DrawGuiText(m_FrameTimeText, { textX, textY }, scale, textColor); textY += lineOffset;
        Gui::DrawGuiText(m_CpuText, { textX, textY }, scale, highlightColor); textY += lineOffset;
        Gui::DrawGuiText(m_GpuText, { textX, textY }, scale, highlightColor); textY += lineOffset;
        Gui::DrawGuiText(m_DrawCalls3DText, { textX, textY }, scale, textColor); textY += lineOffset;
        Gui::DrawGuiText(m_Tris3DText, { textX, textY }, scale, textColor); textY += lineOffset;
        Gui::DrawGuiText(m_Culled3DText, { textX, textY }, scale, textColor); textY += lineOffset;
        Gui::DrawGuiText(m_DrawCallsUIText, { textX, textY }, scale, textColor); textY += lineOffset;
        Gui::DrawGuiText(m_TrisUIText, { textX, textY }, scale, textColor);
    }

    // --- PANEL OTOCZENIA (ANCHOR: BOTTOM LEFT) ---
    if (m_ShowEnvironmentPanel) {
        glm::vec2 envSize = { 180.f, 300.f };
        glm::vec2 envPos = GetAnchoredPosition(Anchor::BottomLeft, 10.f, 10.f, envSize.x, envSize.y, m_ViewportWidth, m_ViewportHeight);

        Renderer2D::DrawQuad(envPos, envSize, { 0.15f, 0.15f, 0.15f, 0.9f });
        Gui::DrawGuiText("Kolor tla:", { envPos.x + 5.f, envPos.y + 15.f }, 0.45f, { 1.0f, 1.0f, 1.0f, 1.0f });

        auto* colorStorage = world.GetComponentVector<ClearColorComponent>();
        if (colorStorage && !colorStorage->dense.empty()) {
            float* r = &colorStorage->dense[0].bgColor.r;
            float* g = &colorStorage->dense[0].bgColor.g;
            float* b = &colorStorage->dense[0].bgColor.b;
            Gui::SliderFloat("R", r, 0.0f, 1.0f, { envPos.x + 5.f, envPos.y + 40.f }, { 150.f, 20.f });
            Gui::SliderFloat("G", g, 0.0f, 1.0f, { envPos.x + 5.f, envPos.y + 75.f }, { 150.f, 20.f });
            Gui::SliderFloat("B", b, 0.0f, 1.0f, { envPos.x + 5.f, envPos.y + 110.f }, { 150.f, 20.f });
        }

        Gui::DrawGuiText("Model Oswietlenia:", { envPos.x + 5.f, envPos.y + 145.f }, 0.4f, { 1.0f, 1.0f, 1.0f, 1.0f });
        if (Gui::Button("Standard", { envPos.x + 5.f, envPos.y + 165.f }, { 80.f, 20.f }, (Renderer::ActiveShader == "Phong"))) Renderer::ActiveShader = "Standard";
        if (Gui::Button("RAMP", { envPos.x + 90.f, envPos.y + 165.f }, { 80.f, 20.f }, (Renderer::ActiveShader == "RAMP"))) Renderer::ActiveShader = "RAMP";
        if (Gui::Button("Fake BRDF", { envPos.x + 5.f, envPos.y + 195.f }, { 80.f, 20.f }, (Renderer::ActiveShader == "FakeBRDF"))) Renderer::ActiveShader = "FakeBRDF";
        if (Gui::Button("Blinn-Phong", { envPos.x + 90.f, envPos.y + 195.f }, { 80.f, 20.f }, (Renderer::ActiveShader == "BlinnPhong"))) Renderer::ActiveShader = "BlinnPhong";
        if (Gui::Button("Rim", { envPos.x + 5.f, envPos.y + 225.f }, { 80.f, 20.f }, (Renderer::ActiveShader == "Rim"))) Renderer::ActiveShader = "Rim";
    }

    // --- HIERARCHIA SCENY (ANCHOR: TOP LEFT) ---
    if (m_ShowHierarchyPanel) {
        auto* tagStorage = world.GetComponentVector<TagComponent>();
        if (tagStorage) {
            glm::vec2 startPos = GetAnchoredPosition(Anchor::TopLeft, 10.0f, 80.0f, 180.0f, 25.0f, m_ViewportWidth, m_ViewportHeight);
            Gui::DrawGuiText("Hierarchia sceny:", { startPos.x, startPos.y - 25.0f }, 0.5f, { 1.0f, 1.0f, 1.0f, 1.0f });

            for (size_t i = 0; i < tagStorage->dense.size(); i++) {
                glm::vec2 currentItemPos = { startPos.x, startPos.y + (i * 30.0f) };
                auto& tagComp = tagStorage->dense[i];
                Entity owner = tagStorage->reverse[i];

                if (Gui::Button(tagComp.Tag, currentItemPos, { 180.0f, 25.0f })) {
                    activeScene->SetSelectedEntity(owner);
                }
            }
        }
    }

    // --- BIBLIOTEKA MODELI (ANCHOR: BOTTOM LEFT) ---
    if (m_ShowLibraryPanel) {
        glm::vec2 libSize = { m_ViewportWidth - 500.0f, 200.0f };
        glm::vec2 libPos = GetAnchoredPosition(Anchor::BottomLeft, 200.0f, 0.0f, libSize.x, libSize.y, m_ViewportWidth, m_ViewportHeight);

        Renderer2D::DrawQuad(libPos, libSize, { 0.14f, 0.14f, 0.14f, 0.95f });
        Gui::DrawGuiText("Zasoby:", { libPos.x + 10.0f, libPos.y + 15.0f }, 0.45f, { 1, 1, 1, 1 });

        float xOffset = libPos.x + 10.0f;
        float yOffset = libPos.y + 50.0f;

        for (const auto& entry : AssetManager::GetLibrary()) {
            if (Gui::Button(entry.Name, { xOffset, yOffset }, { 120, 30 })) {
                auto& request = activeScene->GetPlacementRequest();
                request.Name = entry.Name; request.Path = entry.Path; request.Active = true;
            }
            xOffset += 130.0f;
            if (xOffset + 120.0f > libPos.x + libSize.x) {
                xOffset = libPos.x + 10.0f; yOffset += 40.0f;
            }
        }
    }

    // --- INSPEKTOR ENCJI (ANCHOR: TOP RIGHT) ---
    if (m_ShowInspectorPanel) {
        Entity selected = activeScene->GetSelectedEntity();
        if (selected.id != std::numeric_limits<std::size_t>::max()) {
            glm::vec2 inspSize = { 300.0f, 400.0f };
            glm::vec2 inspPos = GetAnchoredPosition(Anchor::TopRight, 10.0f, 70.0f, inspSize.x, inspSize.y, m_ViewportWidth, m_ViewportHeight);

            auto* tag = world.GetComponent<TagComponent>(selected);
            if (tag) Gui::InputGuiText("Nazwa", tag->Tag, { inspPos.x, inspPos.y }, { 260.0f, 25.0f });

            auto* transform = world.GetComponent<TransformComponent>(selected);
            if (transform) {
                glm::vec3 posBeforeSliders = transform->Position;
                float dS = 0.05f; float rS = 0.5f;

                Gui::DragFloat("Pos X", &transform->Position.x, dS, { inspPos.x, inspPos.y + 40.0f }, { 300, 30 });
                Gui::DragFloat("Pos Y", &transform->Position.y, dS, { inspPos.x, inspPos.y + 80.0f }, { 300, 30 });
                Gui::DragFloat("Rot X", &transform->Rotation.x, rS, { inspPos.x, inspPos.y + 120.0f }, { 300, 30 });
                Gui::DragFloat("Rot Y", &transform->Rotation.y, rS, { inspPos.x, inspPos.y + 160.0f }, { 300, 30 });
                Gui::DragFloat("Rot Z", &transform->Rotation.z, rS, { inspPos.x, inspPos.y + 200.0f }, { 300, 30 });
                Gui::DragFloat("Skala X", &transform->Scale.x, dS, { inspPos.x, inspPos.y + 240.0f }, { 300, 30 });
                Gui::DragFloat("Skala Y", &transform->Scale.y, dS, { inspPos.x, inspPos.y + 280.0f }, { 300, 30 });
                Gui::DragFloat("Skala Z", &transform->Scale.z, dS, { inspPos.x, inspPos.y + 320.0f }, { 300, 30 });

                if (posBeforeSliders != transform->Position && !m_IsDraggingTransform) {
                    m_IsDraggingTransform = true; m_TransformStartPos = posBeforeSliders;
                }
                if (m_IsDraggingTransform && !Input::IsMouseButtonPressed(0)) {
                    EntityTransformChangedEvent e(selected, m_TransformStartPos, transform->Position);
                    Application::Get().OnEvent(e);
                    m_IsDraggingTransform = false;
                }
            }
            if (Gui::Button("USUN OBIEKT", { inspPos.x, inspPos.y + 360.0f }, { 300.0f, 40.0f })) {
                EntityDeletedEvent e(selected);
                Application::Get().OnEvent(e);
                activeScene->SetSelectedEntity({ std::numeric_limits<std::size_t>::max(), 0 });
            }
        }
    }

    // --- OKNO ZAPISU (ANCHOR: CENTER) ---
    if (m_ShowSaveDialog) {
        glm::vec2 dialogSize = { 350.0f, 150.0f };
        glm::vec2 dialogPos = GetAnchoredPosition(Anchor::Center, 0.0f, 0.0f, dialogSize.x, dialogSize.y, m_ViewportWidth, m_ViewportHeight);

        Renderer2D::DrawQuad(dialogPos, dialogSize, { 0.2f, 0.2f, 0.25f, 1.0f });
        Gui::DrawGuiText("Zapisz scene jako:", { dialogPos.x + 10.0f, dialogPos.y + 10.0f }, 0.5f, { 1.0f, 1.0f, 1.0f, 1.0f });
        Gui::InputGuiText("Nazwa", m_SaveFileName, { dialogPos.x + 10.0f, dialogPos.y + 50.0f }, { 330.0f, 30.0f });

        if (Gui::Button("Zapisz plik", { dialogPos.x + 10.0f, dialogPos.y + 100.0f }, { 160.0f, 30.0f })) {
            std::string path = "CookingStation/Assets/saves/" + m_SaveFileName + ".json";
            SceneSerializer serializer(activeScene.get());
            serializer.Serialize(path);
            m_ShowSaveDialog = false;
        }
        if (Gui::Button("Anuluj", { dialogPos.x + 180.0f, dialogPos.y + 100.0f }, { 160.0f, 30.0f })) m_ShowSaveDialog = false;
    }

    // --- OKNO WCZYTYWANIA (ANCHOR: CENTER) ---
    if (m_ShowLoadDialog) {
        glm::vec2 dialogSize = { 350.0f, 150.0f };
        glm::vec2 dialogPos = GetAnchoredPosition(Anchor::Center, 0.0f, 0.0f, dialogSize.x, dialogSize.y, m_ViewportWidth, m_ViewportHeight);

        Renderer2D::DrawQuad(dialogPos, dialogSize, { 0.25f, 0.2f, 0.2f, 1.0f });
        Gui::DrawGuiText("Wczytaj scene:", { dialogPos.x + 10.0f, dialogPos.y + 10.0f }, 0.5f, { 1.0f, 1.0f, 1.0f, 1.0f });
        Gui::InputGuiText("Nazwa", m_LoadFileName, { dialogPos.x + 10.0f, dialogPos.y + 50.0f }, { 330.0f, 30.0f });

        if (Gui::Button("Wczytaj plik", { dialogPos.x + 10.0f, dialogPos.y + 100.0f }, { 160.0f, 30.0f })) {
            std::string path = "CookingStation/Assets/saves/" + m_LoadFileName + ".json";
            std::shared_ptr<Scene> newScene = SceneManager::NewScene();
            SceneSerializer serializer(newScene.get());
            serializer.Deserialize(path);
            spdlog::info("Wczytano scene z: {}", path);
            m_ShowLoadDialog = false;
        }
        if (Gui::Button("Anuluj", { dialogPos.x + 180.0f, dialogPos.y + 100.0f }, { 160.0f, 30.0f })) m_ShowLoadDialog = false;
    }

    // --- AKTUALNE QUESTY (ANCHOR: TOP RIGHT) ---
    if (!m_CurrentQuests.empty()) {
        glm::vec2 qpSize = { 380.0f, 200.0f };
        glm::vec2 qpPos = GetAnchoredPosition(Anchor::TopRight, 20.0f, 100.0f, qpSize.x, qpSize.y, m_ViewportWidth, m_ViewportHeight);

        Renderer2D::DrawQuad(qpPos, qpSize, { 0.12f, 0.12f, 0.12f, 0.95f });

        const auto& q = m_CurrentQuests[m_CurrentQuestIndex];
        std::string header = "AKTUALNE ZADANIE (" + std::to_string(m_CurrentQuestIndex + 1) + "/" + std::to_string(m_CurrentQuests.size()) + ")";
        Gui::DrawGuiText(header, { qpPos.x + 15.f, qpPos.y + 15.f }, 0.6f, { 1.0f, 0.5f, 0.2f, 1.0f });
        Gui::DrawGuiText(q.title, { qpPos.x + 15.f, qpPos.y + 45.f }, 0.55f, { 1.0f, 0.8f, 0.2f, 1.0f });

        std::string line1 = q.desc, line2 = "", line3 = "";
        if (line1.length() > 50) {
            size_t spacePos = line1.find_last_of(' ', 50);
            if (spacePos != std::string::npos) { line2 = line1.substr(spacePos + 1); line1 = line1.substr(0, spacePos); }
        }
        if (line2.length() > 50) {
            size_t spacePos = line2.find_last_of(' ', 50);
            if (spacePos != std::string::npos) { line3 = line2.substr(spacePos + 1); line2 = line2.substr(0, spacePos); }
        }

        Gui::DrawGuiText(line1, { qpPos.x + 15.f, qpPos.y + 75.f }, 0.45f, { 0.9f, 0.9f, 0.9f, 1.0f });
        if (!line2.empty()) Gui::DrawGuiText(line2, { qpPos.x + 15.f, qpPos.y + 95.f }, 0.45f, { 0.9f, 0.9f, 0.9f, 1.0f });
        if (!line3.empty()) Gui::DrawGuiText(line3, { qpPos.x + 15.f, qpPos.y + 115.f }, 0.45f, { 0.9f, 0.9f, 0.9f, 1.0f });

        Gui::DrawGuiText("Cel: " + std::to_string(q.portions) + " porcji", { qpPos.x + 15.f, qpPos.y + 145.f }, 0.5f, { 0.5f, 1.0f, 0.5f, 1.0f });
        Gui::DrawGuiText("Nagroda: " + q.reward, { qpPos.x + 15.f, qpPos.y + 170.f }, 0.45f, { 0.4f, 0.8f, 1.0f, 1.0f });

        if (Gui::Button("< Poprzednie", { qpPos.x, qpPos.y + qpSize.y + 5.f }, { 120.f, 25.f })) {
            if (m_CurrentQuestIndex > 0) m_CurrentQuestIndex--;
        }
        if (Gui::Button("Nastepne >", { qpPos.x + qpSize.x - 120.f, qpPos.y + qpSize.y + 5.f }, { 120.f, 25.f })) {
            if (m_CurrentQuestIndex < m_CurrentQuests.size() - 1) m_CurrentQuestIndex++;
        }
    }

    Renderer2D::EndScene();
    glEnable(GL_DEPTH_TEST);
    Gui::EndFrame();
}

void GuiLayer::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) { return OnWindowResize(ev); });
    dispatcher.Dispatch<KeyTypedEvent>([](KeyTypedEvent& ev) { Gui::OnCharTyped(ev.GetKeyCode()); return false; });
    dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& ev) { return OnMouseButtonPressed(ev); });
}

bool GuiLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e) {
    auto mousePos = Input::GetMousePosition();
    float mouseX = mousePos.first;
    float mouseY = mousePos.second;

    auto IsInside = [&](float x, float y, float w, float h) {
        return mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h;
        };

    // 1. Górny pasek
    if (IsInside(0.0f, 0.0f, m_ViewportWidth, 30.0f)) return true;
    if (m_ShowFileMenu && IsInside(10.0f, 30.0f, 150.0f, 90.0f)) return true;
    if (m_ShowViewMenu && IsInside(100.0f, 30.0f, 160.0f, 160.0f)) return true;

    // 2. Prototyp Questów (TopLeft)
    glm::vec2 questPanelPos = GetAnchoredPosition(Anchor::TopLeft, 10.0f, 150.0f, 180.0f, 85.0f, m_ViewportWidth, m_ViewportHeight);
    if (IsInside(questPanelPos.x, questPanelPos.y, 180.0f, 85.0f)) return true;

    // 3. Panele
    if (m_ShowDiagnosticPanel) {
        glm::vec2 size(280.0f, 250.0f);
        glm::vec2 pos = GetAnchoredPosition(Anchor::BottomRight, 10.0f, 10.0f, size.x, size.y, m_ViewportWidth, m_ViewportHeight);
        if (IsInside(pos.x, pos.y, size.x, size.y)) return true;
    }

    if (m_ShowEnvironmentPanel) {
        glm::vec2 size(180.f, 300.f);
        glm::vec2 pos = GetAnchoredPosition(Anchor::BottomLeft, 10.f, 10.f, size.x, size.y, m_ViewportWidth, m_ViewportHeight);
        if (IsInside(pos.x, pos.y, size.x, size.y)) return true;
    }

    if (m_ShowLibraryPanel) {
        glm::vec2 size(m_ViewportWidth - 500.0f, 200.0f);
        glm::vec2 pos = GetAnchoredPosition(Anchor::BottomLeft, 200.0f, 0.0f, size.x, size.y, m_ViewportWidth, m_ViewportHeight);
        if (IsInside(pos.x, pos.y, size.x, size.y)) return true;
    }

    if (m_ShowHierarchyPanel) {
        glm::vec2 pos = GetAnchoredPosition(Anchor::TopLeft, 10.0f, 80.0f, 180.0f, 250.0f, m_ViewportWidth, m_ViewportHeight);
        if (IsInside(pos.x, pos.y, 180.0f, 300.0f)) return true; // Przybli¿ona strefa klikniêæ dla listy
    }

    if (m_ShowInspectorPanel) {
        glm::vec2 size(300.0f, 400.0f);
        glm::vec2 pos = GetAnchoredPosition(Anchor::TopRight, 10.0f, 70.0f, size.x, size.y, m_ViewportWidth, m_ViewportHeight);
        if (IsInside(pos.x, pos.y, size.x, size.y)) return true;
    }

    // 4. Dialogi
    if (m_ShowSaveDialog || m_ShowLoadDialog) {
        glm::vec2 size(350.0f, 150.0f);
        glm::vec2 pos = GetAnchoredPosition(Anchor::Center, 0.0f, 0.0f, size.x, size.y, m_ViewportWidth, m_ViewportHeight);
        if (IsInside(pos.x, pos.y, size.x, size.y)) return true;
    }

    // 5. Aktualne questy
    if (!m_CurrentQuests.empty()) {
        glm::vec2 size(380.0f, 200.0f);
        glm::vec2 pos = GetAnchoredPosition(Anchor::TopRight, 20.0f, 100.0f, size.x, size.y, m_ViewportWidth, m_ViewportHeight);
        // Sprawdzamy panel i obszar przycisków pod nim
        if (IsInside(pos.x, pos.y, size.x, size.y + 40.0f)) return true;
    }

    return false;
}

bool GuiLayer::OnWindowResize(WindowResizeEvent& e) {
    m_ViewportWidth = (float)e.GetWidth();
    m_ViewportHeight = (float)e.GetHeight();
    Gui::UpdateScreenSize(m_ViewportWidth, m_ViewportHeight);
    return false;
}

void GuiLayer::ReloadQuests() {
    m_CurrentQuests.clear();
    std::ifstream file("CookingStation/Assets/wygenerowane_quests.json");
    if (file.is_open()) {
        try {
            nlohmann::json data = nlohmann::json::parse(file);
            for (auto& q : data) {
                m_CurrentQuests.push_back({
                    q.value("title", "Brak tytulu"),
                    q.value("description", "Brak opisu"),
                    q.value("portions", 0),
                    q.value("reward", "Brak nagrody")
                    });
            }
            m_CurrentQuestIndex = 0;
            spdlog::info("Questy zaladowane do interfejsu!");
        }
        catch (...) {
            spdlog::error("Blad parsowania JSONa questow.");
        }
    }
}

GuiLayer::~GuiLayer() {}