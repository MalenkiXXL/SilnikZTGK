#include "EditorGuiLayer.h"
#include "CookingStation/Events/WindowEvent.h"
#include "CookingStation/Events/KeyEvent.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Scene/ecs.h"
#include "CookingStation/Events/EditorEvents.h"
#include "CookingStation/Scene/PrefabSerializer.h"
#include "CookingStation/Core/Application.h"
#include "GameGuiLayer.h"
#include <cstdlib> 
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <string>
#include <limits>
#include "CookingStation/Scripts/PotScript.h"

namespace {
    enum class Anchor { TopLeft, TopRight, BottomLeft, BottomRight, Center };

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
}

void EditorGuiLayer::OnAttach() {
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

void EditorGuiLayer::OnUpdate(Timestep ts) {
    Gui::BeginFrame(); // Reset flagi przechwytywania myszy
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

    // --- GŁÓWNY PASEK ZADAŃ ---
    Gui::Panel({ 0.0f, 0.0f }, { m_ViewportWidth, 30.0f }, { 0.15f, 0.15f, 0.15f, 1.0f });

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
        Gui::Panel({ 10.0f, 30.0f }, { 150.0f, 90.0f }, { 0.2f, 0.2f, 0.2f, 0.9f });
        if (Gui::Button("Zapisz", { 15.0f, 35.0f }, { 140.0f, 25.0f })) {
            m_ShowSaveDialog = true;
            m_ShowFileMenu = false;
        }
        if (Gui::Button("Wczytaj", { 15.0f, 65.0f }, { 140.0f, 25.0f })) {
            m_ShowLoadDialog = true;
            m_ShowFileMenu = false;
        }
    }

    // --- PANEL GENERATORA QUESTÓW AI ---
    if (m_ShowQuestsPanel) {
        glm::vec2 questPanelPos = GetAnchoredPosition(Anchor::TopLeft, 10.0f, 400.0f, 180.0f, 85.0f, m_ViewportWidth, m_ViewportHeight);

        Gui::Panel(questPanelPos, { 180.0f, 85.0f }, { 0.15f, 0.15f, 0.15f, 0.9f }, 15.0f);
        Gui::DrawGuiText("Generator Questow:", { questPanelPos.x + 5.f, questPanelPos.y + 10.f }, 0.45f, { 1.0f, 0.8f, 0.2f, 1.0f });

        if (Gui::Button("Generuj (Stary Cache)", { questPanelPos.x + 5.f, questPanelPos.y + 30.f }, { 170.f, 20.f })) {
            spdlog::info("Generowanie z istniejacego cache...");
            system("python CookingStation/Tools/QuestGenerator/main.py");
            GameGuiLayer::s_NeedsQuestReload = true;
        }

        if (Gui::Button("Generuj (Nowe Newsy)", { questPanelPos.x + 5.f, questPanelPos.y + 55.f }, { 170.f, 20.f })) {
            spdlog::info("Czyszczenie cache i pobieranie nowych newsow...");
            std::remove("CookingStation/Assets/news_cache.json");
            system("python CookingStation/Tools/QuestGenerator/main.py");
            GameGuiLayer::s_NeedsQuestReload = true;
        }
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

            m_InstanceBatchesText = "Instanced Batches: " + std::to_string(stats.InstanceBatches);
            m_MatrixCalcText = "CPU Matrix Calcs: " + std::to_string(stats.MatrixCalculations);

            float totalMatrix = (float)(stats.MatrixCalculations + stats.SkippedCalculations);
            float savings = totalMatrix > 0 ? ((float)stats.SkippedCalculations / totalMatrix) * 100.0f : 0.0f;
            m_CpuSavingsText = "CPU Savings: " + formatFloat(savings) + "%";

            m_Tris3DText = "Trojkaty (3D): " + std::to_string(stats.TriangleCount3D);
            m_Culled3DText = "Odrzucone (Culled): " + std::to_string(stats.CulledObjects3D);
            m_DrawCallsUIText = "Draw Calls (UI): " + std::to_string(stats.DrawCallsUI);
            m_TrisUIText = "Trojkaty (UI): " + std::to_string(stats.TriangleCountUI);
            m_StatsUpdateTimer = 0.0f;
        }

        glm::vec2 panelSize(300.0f, 350.0f); 
        glm::vec2 panelPos = GetAnchoredPosition(Anchor::BottomRight, 0.0f, 10.0f, panelSize.x, panelSize.y, m_ViewportWidth, m_ViewportHeight);

        Gui::Panel(panelPos, panelSize, { 0.12f, 0.12f, 0.12f, 0.85f });

        float textX = panelPos.x + 15.0f; float textY = panelPos.y + 5.0f; float lineOffset = 25.0f; float scale = 0.6f;
        glm::vec4 textColor(1.0f, 1.0f, 1.0f, 1.0f);
        glm::vec4 highlightColor(1.0f, 0.8f, 0.2f, 1.0f);
        glm::vec4 optColor(0.2f, 0.9f, 0.8f, 1.0f); 

        Gui::DrawGuiText("Diagnostyka Projektu:", { textX, textY }, scale + 0.1f, { 0.2f, 0.8f, 0.2f, 1.0f }); textY += lineOffset + 5.0f;
        Gui::DrawGuiText(m_FpsText, { textX, textY }, scale, textColor); textY += lineOffset;
        Gui::DrawGuiText(m_FrameTimeText, { textX, textY }, scale, textColor); textY += lineOffset;
        Gui::DrawGuiText(m_CpuText, { textX, textY }, scale, highlightColor); textY += lineOffset;
        Gui::DrawGuiText(m_GpuText, { textX, textY }, scale, highlightColor); textY += lineOffset;

        // Wyświetlanie nowych statystyk
        Gui::DrawGuiText(m_DrawCalls3DText, { textX, textY }, scale, textColor); textY += lineOffset;
        Gui::DrawGuiText(m_InstanceBatchesText, { textX, textY }, scale, optColor); textY += lineOffset;
        Gui::DrawGuiText(m_MatrixCalcText, { textX, textY }, scale, optColor); textY += lineOffset;
        Gui::DrawGuiText(m_CpuSavingsText, { textX, textY }, scale, optColor); textY += lineOffset;

        Gui::DrawGuiText(m_Tris3DText, { textX, textY }, scale, textColor); textY += lineOffset;
        Gui::DrawGuiText(m_Culled3DText, { textX, textY }, scale, textColor); textY += lineOffset;
        Gui::DrawGuiText(m_DrawCallsUIText, { textX, textY }, scale, textColor); textY += lineOffset;
        Gui::DrawGuiText(m_TrisUIText, { textX, textY }, scale, textColor);
    }

    // --- PANEL OTOCZENIA (ANCHOR: BOTTOM LEFT) ---
    if (m_ShowEnvironmentPanel) {
        glm::vec2 envSize = { 180.f, 300.f };
        glm::vec2 envPos = GetAnchoredPosition(Anchor::BottomLeft, 10.f, 10.f, envSize.x, envSize.y, m_ViewportWidth, m_ViewportHeight);

        Gui::Panel(envPos, envSize, { 0.15f, 0.15f, 0.15f, 0.9f });
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
        if (Gui::Button("Standard", { envPos.x + 5.f, envPos.y + 165.f }, { 80.f, 20.f }, (Renderer::ActiveShader == "Standard"))) Renderer::ActiveShader = "Standard";
        if (Gui::Button("RAMP", { envPos.x + 90.f, envPos.y + 165.f }, { 80.f, 20.f }, (Renderer::ActiveShader == "RAMP"))) Renderer::ActiveShader = "RAMP";
        if (Gui::Button("Fake BRDF", { envPos.x + 5.f, envPos.y + 195.f }, { 80.f, 20.f }, (Renderer::ActiveShader == "FakeBRDF"))) Renderer::ActiveShader = "FakeBRDF";
        if (Gui::Button("Blinn-Phong", { envPos.x + 90.f, envPos.y + 195.f }, { 80.f, 20.f }, (Renderer::ActiveShader == "BlinnPhong"))) Renderer::ActiveShader = "BlinnPhong";
        if (Gui::Button("Rim", { envPos.x + 5.f, envPos.y + 225.f }, { 80.f, 20.f }, (Renderer::ActiveShader == "Rim"))) Renderer::ActiveShader = "Rim";
    }

    // --- HIERARCHIA SCENY (ANCHOR: TOP LEFT) ---
    if (m_ShowHierarchyPanel) {
        auto* tagStorage = world.GetComponentVector<TagComponent>();
        if (tagStorage) {
            glm::vec2 panelSize = { 180.0f, 300.0f };
            glm::vec2 startPos = GetAnchoredPosition(Anchor::TopLeft, 10.0f, 80.0f, panelSize.x, panelSize.y, m_ViewportWidth, m_ViewportHeight);

            Gui::Panel(startPos, panelSize, { 0.12f, 0.12f, 0.12f, 0.8f });
            Gui::DrawGuiText("Hierarchia sceny:", { startPos.x + 5.0f, startPos.y + 15.0f }, 0.5f, { 1.0f, 1.0f, 1.0f, 1.0f });

            Renderer2D::EndScene();
            glEnable(GL_SCISSOR_TEST);

            int scissorY = (int)(m_ViewportHeight - (startPos.y + panelSize.y));
            glScissor((int)startPos.x, scissorY, (int)panelSize.x, (int)panelSize.y);

            Renderer2D::BeginScene(uiProj);

            for (size_t i = 0; i < tagStorage->dense.size(); i++) {
                glm::vec2 currentItemPos = { startPos.x, startPos.y + 35.0f + (i * 30.0f) - m_HierarchyScrollY };

                auto& tagComp = tagStorage->dense[i];
                Entity owner = tagStorage->reverse[i];

                if (Gui::Button(tagComp.Tag, currentItemPos, { 180.0f, 25.0f })) {
                    activeScene->SetSelectedEntity(owner);
                }
            }

            Renderer2D::EndScene();
            glDisable(GL_SCISSOR_TEST);
            Renderer2D::BeginScene(uiProj);
        }
    }

    // --- BIBLIOTEKA MODELI ---
    if (m_ShowLibraryPanel) {
        glm::vec2 libSize = { m_ViewportWidth - 500.0f, 200.0f };
        glm::vec2 libPos = GetAnchoredPosition(Anchor::BottomLeft, 200.0f, 0.0f, libSize.x, libSize.y, m_ViewportWidth, m_ViewportHeight);

        Gui::Panel(libPos, libSize, { 0.14f, 0.14f, 0.14f, 0.95f });
        Gui::DrawGuiText("Zasoby:", { libPos.x + 10.0f, libPos.y + 15.0f }, 0.45f, { 1, 1, 1, 1 });

        Renderer2D::EndScene();
        glEnable(GL_SCISSOR_TEST);
        int scissorY = (int)(m_ViewportHeight - (libPos.y + libSize.y));
        glScissor((int)libPos.x, scissorY, (int)libSize.x, (int)libSize.y);
        Renderer2D::BeginScene(uiProj);

        float xOffset = libPos.x + 10.0f;
        float yOffset = libPos.y + 40.0f - m_LibraryScrollY;

        for (const auto& entry : AssetManager::GetLibrary()) {
            if (Gui::Button(entry.Name, { xOffset, yOffset }, { 120, 30 })) {
                auto& request = activeScene->GetPlacementRequest();
                request.Name = entry.Name; request.Path = entry.Path; request.Active = true;
            }
            xOffset += 130.0f;

            if (xOffset + 120.0f > libPos.x + libSize.x) {
                xOffset = libPos.x + 10.0f;
                yOffset += 40.0f;
            }
        }

        Renderer2D::EndScene();
        glDisable(GL_SCISSOR_TEST);
        Renderer2D::BeginScene(uiProj);
    }

    // --- PANEL PREFABÓW ---
    if (m_ShowPrefabsPanel) {
        glm::vec2 prefSize = { m_ViewportWidth - 500.0f, 120.0f };
        glm::vec2 prefPos = GetAnchoredPosition(Anchor::BottomLeft, 200.0f, 0.0f, prefSize.x, prefSize.y, m_ViewportWidth, m_ViewportHeight);

        Gui::Panel(prefPos, prefSize, { 0.15f, 0.25f, 0.3f, 0.95f });
        Gui::DrawGuiText("Gotowe Prefaby:", { prefPos.x + 10.0f, prefPos.y + 15.0f }, 0.45f, { 1.0f, 1.0f, 1.0f, 1.0f });

        float xOffset = prefPos.x + 10.0f;
        float yOffset = prefPos.y + 40.0f;

        if (std::filesystem::exists("CookingStation/Assets/prefabs")) {
            for (const auto& entry : std::filesystem::directory_iterator("CookingStation/Assets/prefabs")) {
                if (entry.path().extension() == ".json") {
                    std::string prefabName = entry.path().stem().string();
                    std::string prefabPath = entry.path().string();
                    std::replace(prefabPath.begin(), prefabPath.end(), '\\', '/');

                    if (Gui::Button(prefabName, { xOffset, yOffset }, { 120, 30 })) {
                        auto& request = activeScene->GetPlacementRequest();
                        request.Name = prefabName;
                        request.Path = prefabPath;
                        request.Active = true;
                    }

                    xOffset += 130.0f;
                    if (xOffset + 120.0f > prefPos.x + prefSize.x) {
                        xOffset = prefPos.x + 10.0f;
                        yOffset += 40.0f;
                    }
                }
            }
        }
    }

    // --- INSPEKTOR ENCJI (ANCHOR: TOP RIGHT) ---
    if (m_ShowInspectorPanel) {
        Entity selected = activeScene->GetSelectedEntity();
        if (selected.id != std::numeric_limits<std::size_t>::max()) {
            glm::vec2 inspSize = { 300.0f, 750.0f }; // Delikatnie powiększone tło na nowe sekcje
            glm::vec2 inspPos = GetAnchoredPosition(Anchor::TopRight, 10.0f, 70.0f, inspSize.x, inspSize.y, m_ViewportWidth, m_ViewportHeight);

            // Rysujemy tło panelu
            Gui::Panel(inspPos, inspSize, { 0.15f, 0.15f, 0.15f, 0.95f });

            float currentY = inspPos.y + 10.0f; // Dynamiczny kursor Y
            float padX = inspPos.x + 5.0f;
            float elementW = 290.0f;

            // --- SEKCJA TAG ---
            auto* tag = world.GetComponent<TagComponent>(selected);
            if (tag) {
                Gui::InputGuiText("Nazwa", tag->Tag, { padX, currentY }, { elementW, 25.0f });
                currentY += 35.0f;
            }

            // --- SEKCJA TRANSFORM ---
            auto* transform = world.GetComponent<TransformComponent>(selected);
            if (transform) {
                glm::vec3 pos = transform->GetPosition();
                glm::vec3 rot = transform->GetRotation();
                glm::vec3 scale = transform->GetScale();
                glm::vec3 posBeforeSliders = pos;
                float dS = 0.05f; float rS = 0.5f;

                Gui::DrawGuiText("Transform:", { padX, currentY }, 0.4f, { 1.0f, 0.8f, 0.2f, 1.0f }); currentY += 20.0f;

                // Zmniejszyłem wysokość sliderów z 30 do 20, żeby zaoszczędzić miejsce!
                Gui::DragFloat("Pos X", &pos.x, dS, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Pos Y", &pos.y, dS, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Pos Z", &pos.z, dS, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Rot X", &rot.x, rS, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Rot Y", &rot.y, rS, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Rot Z", &rot.z, rS, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Scale X", &scale.x, dS, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Scale Y", &scale.y, dS, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Scale Z", &scale.z, dS, { padX, currentY }, { elementW, 20 }); currentY += 30.0f;

                if (pos != posBeforeSliders || rot != transform->GetRotation() || scale != transform->GetScale()) {
                    transform->SetPosition(pos); transform->SetRotation(rot); transform->SetScale(scale);
                    if (pos != posBeforeSliders && !m_IsDraggingTransform) {
                        m_IsDraggingTransform = true; m_TransformStartPos = posBeforeSliders;
                    }
                }
                if (m_IsDraggingTransform && !Input::IsMouseButtonPressed(0)) {
                    EntityTransformChangedEvent e(selected, m_TransformStartPos, pos);
                    Application::Get().OnEvent(e);
                    m_IsDraggingTransform = false;
                }
            }

            // --- SEKCJA BOX COLLIDERA ---
            auto* collider = world.GetComponent<BoxColliderComponent>(selected);
            if (collider) {
                Gui::DrawGuiText("Box Collider:", { padX, currentY }, 0.4f, { 1.0f, 0.8f, 0.2f, 1.0f }); currentY += 20.0f;
                Gui::DragFloat("Size X", &collider->Size.x, 0.05f, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Size Y", &collider->Size.y, 0.05f, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Size Z", &collider->Size.z, 0.05f, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Off X", &collider->Offset.x, 0.05f, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Off Y", &collider->Offset.y, 0.05f, { padX, currentY }, { elementW, 20 }); currentY += 22.0f;
                Gui::DragFloat("Off Z", &collider->Offset.z, 0.05f, { padX, currentY }, { elementW, 20 }); currentY += 25.0f;

                if (Gui::Button("USUN COLLIDER", { padX, currentY }, { elementW, 25.0f })) {
                    world.RemoveComponent<BoxColliderComponent>(selected);
                }
                currentY += 35.0f;
            }
            else {
                if (Gui::Button("+ Dodaj Box Collider", { padX, currentY }, { elementW, 25.0f })) {
                    BoxColliderComponent bc;
                    bc.Size = { 1.0f, 1.0f, 1.0f }; bc.Offset = { 0.0f, 0.0f, 0.0f };
                    world.AddComponent<BoxColliderComponent>(selected, bc);
                }
                currentY += 35.0f;
            }

            // --- SEKCJA SKRYPTÓW ---
            auto* scriptComp = world.GetComponent<NativeScriptComponent>(selected);
            if (!scriptComp) {
                if (Gui::Button("+ Dodaj System Skryptow", { padX, currentY }, { elementW, 25.0f })) {
                    world.AddComponent<NativeScriptComponent>(selected, NativeScriptComponent{});
                }
                currentY += 35.0f;
            }
            else {
                Gui::DrawGuiText("Skrypty:", { padX, currentY }, 0.4f, { 1.0f, 0.8f, 0.2f, 1.0f }); currentY += 20.0f;

                std::string scriptList = "";
                for (const auto& s : scriptComp->Scripts) scriptList += "- " + s.Name + "\n";
                if (!scriptList.empty()) {
                    Gui::DrawGuiText(scriptList, { padX, currentY }, 0.35f, { 0.2f, 0.9f, 0.2f, 1.0f });
                    currentY += (scriptComp->Scripts.size() * 15.0f) + 10.0f;
                }

                if (Gui::Button("+ Rot", { padX, currentY }, { 65.0f, 20.0f })) scriptComp->AddScript<RotationScript>("RotationScript");
                if (Gui::Button("+ Conv", { padX + 70.0f, currentY }, { 65.0f, 20.0f })) scriptComp->AddScript<ConveyorScript>("ConveyorScript");
                if (Gui::Button("+ Item", { padX + 140.0f, currentY }, { 65.0f, 20.0f })) scriptComp->AddScript<ItemScript>("ItemScript");
                if (Gui::Button("+ Pot", { padX + 210.0f, currentY }, { 65.0f, 20.0f })) scriptComp->AddScript<PotScript>("PotScript");
                currentY += 25.0f;

                if (Gui::Button("+ BeltVis", { padX, currentY }, { 80.0f, 20.0f })) scriptComp->AddScript<BeltVisualScript>("BeltVisualScript");
                if (Gui::Button("WYCZYSC SKRYPTY", { padX + 90.0f, currentY }, { elementW - 90.0f, 20.0f })) {
                    scriptComp->Scripts.clear();
                }
                currentY += 25.0f;

                if (Gui::Button("USUN SYSTEM SKRYPTOW", { padX, currentY }, { elementW, 25.0f })) {
                    world.RemoveComponent<NativeScriptComponent>(selected);
                }
                currentY += 35.0f;
            }

            // --- PRZYCISKI ZARZĄDZANIA ---
            currentY += 10.0f;
            if (Gui::Button("ZAPISZ JAKO PREFAB", { padX, currentY }, { elementW, 30.0f })) {
                std::string prefabName = tag ? tag->Tag : "NowyPrefab";
                std::string path = "CookingStation/Assets/prefabs/" + prefabName + ".json";
                PrefabSerializer::Serialize(activeScene.get(), selected, path);
            }
            currentY += 35.0f;

            if (Gui::Button("USUN OBIEKT Z MAPY", { padX, currentY }, { elementW, 30.0f })) {
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

        Gui::Panel(dialogPos, dialogSize, { 0.2f, 0.2f, 0.25f, 1.0f });
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

        Gui::Panel(dialogPos, dialogSize, { 0.25f, 0.2f, 0.2f, 1.0f });
        Gui::DrawGuiText("Wczytaj scene:", { dialogPos.x + 10.0f, dialogPos.y + 10.0f }, 0.5f, { 1.0f, 1.0f, 1.0f, 1.0f });
        Gui::InputGuiText("Nazwa", m_LoadFileName, { dialogPos.x + 10.0f, dialogPos.y + 50.0f }, { 330.0f, 30.0f });

        if (Gui::Button("Wczytaj plik", { dialogPos.x + 10.0f, dialogPos.y + 100.0f }, { 160.0f, 30.0f })) {
            std::string path = "CookingStation/Assets/saves/" + m_LoadFileName + ".json";
            std::shared_ptr<Scene> newScene = SceneManager::NewScene();
            SceneSerializer serializer(newScene.get());
            serializer.Deserialize(path);
            m_ShowLoadDialog = false;
        }
        if (Gui::Button("Anuluj", { dialogPos.x + 180.0f, dialogPos.y + 100.0f }, { 160.0f, 30.0f })) m_ShowLoadDialog = false;
    }

    if (m_ShowViewMenu) {
        Renderer2D::DrawQuad({ 100.0f, 30.0f }, { 160.0f, 220.0f }, { 0.2f, 0.2f, 0.2f, 0.9f });
        if (Gui::Button("Panel Otoczenia", { 105.0f, 35.0f }, { 150.0f, 25.0f }, m_ShowEnvironmentPanel)) {
            m_ShowEnvironmentPanel = !m_ShowEnvironmentPanel; m_ShowViewMenu = false;
        }
        if (Gui::Button("Hierarchia", { 105.0f, 65.0f }, { 150.0f, 25.0f }, m_ShowHierarchyPanel)) {
            m_ShowHierarchyPanel = !m_ShowHierarchyPanel; m_ShowViewMenu = false;
        }
        if (Gui::Button("Biblioteka", { 105.0f, 95.0f }, { 150.0f, 25.0f }, m_ShowLibraryPanel)) {
            m_ShowLibraryPanel = !m_ShowLibraryPanel;
            if (m_ShowPrefabsPanel) m_ShowPrefabsPanel = false;
            m_ShowViewMenu = false;
        }
        if (Gui::Button("Inspektor", { 105.0f, 125.0f }, { 150.0f, 25.0f }, m_ShowInspectorPanel)) {
            m_ShowInspectorPanel = !m_ShowInspectorPanel; m_ShowViewMenu = false;
        }
        if (Gui::Button("Diagnostyka", { 105.0f, 155.0f }, { 150.0f, 25.0f }, m_ShowDiagnosticPanel)) {
            m_ShowDiagnosticPanel = !m_ShowDiagnosticPanel; m_ShowViewMenu = false;
        }
        if (Gui::Button("Questy", { 105.0f, 185.0f }, { 150.0f, 25.0f }, m_ShowQuestsPanel)) {
            m_ShowQuestsPanel = !m_ShowQuestsPanel; m_ShowViewMenu = false;
        }
        if (Gui::Button("Prefaby", { 105.0f, 215.0f }, { 150.0f, 25.0f }, m_ShowPrefabsPanel)) {
            m_ShowPrefabsPanel = !m_ShowPrefabsPanel;
            if (m_ShowLibraryPanel) m_ShowLibraryPanel = false;
            m_ShowViewMenu = false;
        }
    }

    Renderer2D::EndScene();
    glEnable(GL_DEPTH_TEST);
    Gui::EndFrame();
}

void EditorGuiLayer::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) { return OnWindowResize(ev); });
    dispatcher.Dispatch<KeyTypedEvent>([](KeyTypedEvent& ev) { Gui::OnCharTyped(ev.GetKeyCode()); return false; });
    dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& ev) { return OnMouseButtonPressed(ev); });
    dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& ev) { return OnMouseScrolled(ev); });
}

bool EditorGuiLayer::OnMouseScrolled(MouseScrolledEvent& e) {
    float scrollAmount = e.GetYOffset() * 30.0f;
    if (m_ShowLibraryPanel) {
        glm::vec2 size(m_ViewportWidth - 500.0f, 200.0f);
        glm::vec2 pos = GetAnchoredPosition(Anchor::BottomLeft, 200.0f, 0.0f, size.x, size.y, m_ViewportWidth, m_ViewportHeight);
        if (Gui::IsMouseOver(pos, size)) {
            m_LibraryScrollY -= scrollAmount;
            if (m_LibraryScrollY < 0.0f) m_LibraryScrollY = 0.0f;
            return true;
        }
    }
    if (m_ShowHierarchyPanel) {
        glm::vec2 pos = GetAnchoredPosition(Anchor::TopLeft, 10.0f, 80.0f, 180.0f, 250.0f, m_ViewportWidth, m_ViewportHeight);
        if (Gui::IsMouseOver(pos, { 180.0f, 300.0f })) {
            m_HierarchyScrollY -= scrollAmount;
            if (m_HierarchyScrollY < 0.0f) m_HierarchyScrollY = 0.0f;
            return true;
        }
    }
    return false;
}

bool EditorGuiLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e) {
    if (Gui::WantCaptureMouse()) return true;
    return false;
}

bool EditorGuiLayer::OnWindowResize(WindowResizeEvent& e) {
    m_ViewportWidth = (float)e.GetWidth();
    m_ViewportHeight = (float)e.GetHeight();
    Gui::UpdateScreenSize(m_ViewportWidth, m_ViewportHeight);
    return false;
}

EditorGuiLayer::~EditorGuiLayer() {}