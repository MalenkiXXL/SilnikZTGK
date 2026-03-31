#include "GuiLayer.h"
#include "CookingStation/Events/WindowEvent.h"
#include "CookingStation/Events/KeyEvent.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Scene/ecs.h"

void GuiLayer::OnAttach() {
    Renderer2D::Init();

    auto windowSize = Input::GetWindowSize();
    m_ViewportWidth = (float)windowSize.first;
    m_ViewportHeight = (float)windowSize.second;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Gui::UpdateScreenSize(m_ViewportWidth, m_ViewportHeight);
    Gui::Init("CookingStation/Assets/fonts/ARIAL.ttf", 32);
}

void GuiLayer::OnUpdate() {
    auto& world = m_ActiveScene->GetWorld();

    // 1. PRZYGOTOWANIE RENDERERA GUI
    glDisable(GL_DEPTH_TEST); // GUI rysujemy zawsze na wierzchu
    glm::mat4 uiProj = glm::ortho(0.0f, m_ViewportWidth, m_ViewportHeight, 0.0f);
    Renderer2D::BeginScene(uiProj);

    // --- SEKCJA: T£O I SUWAKI RGB ---
    Renderer2D::DrawQuad({ 10.f, 20.f }, { 110.f, 165.f }, { 0.5f, 0.5f, 0.2f, 0.1f });

    auto* colorStorage = world.GetComponentVector<ClearColorComponent>();
    if (colorStorage && !colorStorage->dense.empty()) {
        float* r = &colorStorage->dense[0].bgColor.r;
        float* g = &colorStorage->dense[0].bgColor.g;
        float* b = &colorStorage->dense[0].bgColor.b;

        Gui::SliderFloat("R", r, 0.0f, 1.0f, { 15, 50 }, { 100, 20 });
        Gui::SliderFloat("G", g, 0.0f, 1.0f, { 15, 100 }, { 100, 20 });
        Gui::SliderFloat("B", b, 0.0f, 1.0f, { 15, 150 }, { 100, 20 });
    }

    // --- SEKCJA: HIERARCHIA SCENY ---
    auto* tagStorage = world.GetComponentVector<TagComponent>();
    if (tagStorage) {
        glm::vec2 startPos = { m_ViewportWidth - 220.0f, 50.0f };
        glm::vec2 itemSize = { 200.0f, 30.0f };
        float spacing = 5.0f;

        Gui::DrawGuiText("Hierarchia sceny:", { startPos.x, startPos.y - 25.0f }, 0.5f, { 1.0f, 1.0f, 1.0f, 1.0f });

        for (size_t i = 0; i < tagStorage->dense.size(); i++) {
            float offset_y = i * (itemSize.y + spacing);
            glm::vec2 currentItemPos = { startPos.x, startPos.y + offset_y };

            auto& tagComp = tagStorage->dense[i];
            Entity owner = tagStorage->reverse[i];

            if (Gui::Button(tagComp.Tag, currentItemPos, itemSize)) {
                // Przekazujemy zaznaczenie do EditorLayer
                m_ActiveScene->SetSelectedEntity(owner);
                spdlog::info("Gui: Wybrano encje {}", tagComp.Tag);
            }
        }
    }

    // --- SEKCJA: BIBLIOTEKA MODELI ---
    Renderer2D::DrawQuad({ 10, 200 }, { 120, 400 }, { 0.1f, 0.1f, 0.1f, 0.7f });
    Gui::DrawGuiText("Biblioteka:", { 20, 225 }, 0.45f, { 1, 1, 1, 1 });

    float yOffset = 260.0f;
    for (const auto& entry : AssetManager::GetLibrary()) {
        if (Gui::Button(entry.Name, { 20, yOffset }, { 100, 25 })) {
            // Informujemy EditorLayer, ¿e chcemy zacz¹æ stawiaæ model
            auto& request = m_ActiveScene->GetPlacementRequest();
            request.Name = entry.Name;
            request.Path = entry.Path;
            request.Active = true;
        }
        yOffset += 35.0f;
    }

    // --- SEKCJA: INSPEKTOR WYBRANEJ ENCJI ---
    // Pobieramy zaznaczon¹ encjê z EditorLayer
    Entity selected = m_ActiveScene->GetSelectedEntity();
    if (selected.id != std::numeric_limits<std::size_t>::max()) {
        glm::vec2 inspPos = { m_ViewportWidth - 175.0f, 250.0f };

        auto* tag = world.GetComponent<TagComponent>(selected);
        if (tag) {
            Gui::InputGuiText("Nazwa", tag->Tag, { inspPos.x, inspPos.y - 20.0f }, { 200, 25 });
        }

        auto* transform = world.GetComponent<TransformComponent>(selected);
        if (transform) {
            Gui::SliderFloat("Pos X", &transform->Position.x, -100.0f, 100.0f, { inspPos.x, inspPos.y + 40.0f }, { 150, 15 });
            Gui::SliderFloat("Pos Y", &transform->Position.y, -100.0f, 100.0f, { inspPos.x, inspPos.y + 80.0f }, { 150, 15 });
            Gui::SliderFloat("Rot X", &transform->Rotation.x, -360.0f, 360.0f, { inspPos.x, inspPos.y + 120.0f }, { 150, 15 });
            Gui::SliderFloat("Rot Z", &transform->Rotation.z, -360.0f, 360.0f, { inspPos.x, inspPos.y + 160.0f }, { 150, 15 });
            Gui::SliderFloat("Rot Y", &transform->Rotation.y, -360.0f, 360.0f, { inspPos.x, inspPos.y + 200.0f }, { 150, 15 });
            Gui::SliderFloat("Skala", &transform->Scale.x, 0.1f, 5.0f, { inspPos.x, inspPos.y + 240.0f }, { 180, 15 });

            // Synchronizacja skali XYZ
            transform->Scale.y = transform->Scale.x;
            transform->Scale.z = transform->Scale.x;
        }

        if (Gui::Button("USUN OBIEKT", { inspPos.x, inspPos.y + 280.0f }, { 200.0f, 30.0f })) {
            world.DestroyEntity(selected);
            m_ActiveScene->SetSelectedEntity({ std::numeric_limits<std::size_t>::max(), 0 });
        }
    }

    Renderer2D::EndScene();
    glEnable(GL_DEPTH_TEST);
}

void GuiLayer::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) { return OnWindowResize(ev); });
    dispatcher.Dispatch<KeyTypedEvent>([](KeyTypedEvent& ev) { Gui::OnCharTyped(ev.GetKeyCode()); return false; });
}

bool GuiLayer::OnWindowResize(WindowResizeEvent& e) {
    m_ViewportWidth = (float)e.GetWidth();
    m_ViewportHeight = (float)e.GetHeight();
    Gui::UpdateScreenSize(m_ViewportWidth, m_ViewportHeight);
    return false;
}

GuiLayer::~GuiLayer() {}