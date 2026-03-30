#include "GuiLayer.h"
#include "CookingStation/Events/WindowEvent.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Scene/ecs.h"

void GuiLayer::OnAttach() {
    Renderer2D::Init();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Gui::UpdateScreenSize(m_ViewportWidth, m_ViewportHeight);
    Gui::Init("CookingStation/Assets/fonts/ARIAL.ttf", 32);
}

void GuiLayer::OnUpdate() {
    auto& world = m_ActiveScene->GetWorld();

    // 1. PRZYGOTOWANIE RENDERERA (Raz na pocz¹tku klatki)
    glDisable(GL_DEPTH_TEST); // GUI rysujemy bez testu g³êbi
    glm::mat4 uiProj = glm::ortho(0.0f, m_ViewportWidth, m_ViewportHeight, 0.0f);
    Renderer2D::BeginScene(uiProj);

    // --- SEKCJA: T£O I SUWAKI RGB ---
    Renderer2D::DrawQuad({ 45.f, 20.f }, { 210.f, 165.f }, { 0.5f, 0.5f, 0.2f, 0.1f });

    auto* colorStorage = world.GetComponentVector<ClearColorComponent>();
    if (colorStorage && !colorStorage->dense.empty()) {
        float* r = &colorStorage->dense[0].bgColor.r;
        float* g = &colorStorage->dense[0].bgColor.g;
        float* b = &colorStorage->dense[0].bgColor.b;

        Gui::SliderFloat("R", r, 0.0f, 1.0f, { 50, 50 }, { 200, 20 });
        Gui::SliderFloat("G", g, 0.0f, 1.0f, { 50, 100 }, { 200, 20 });
        Gui::SliderFloat("B", b, 0.0f, 1.0f, { 50, 150 }, { 200, 20 });
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
                m_SelectedEntity = owner;
                spdlog::info("Wybrano obiekt: {} (ID: {})", tagComp.Tag, owner.id);
            }
        }
    }

    // --- SEKCJA: BIBLIOTEKA MODELI (Zawsze widoczna) ---
    Renderer2D::DrawQuad({ 10, 200 }, { 180, 400 }, { 0.1f, 0.1f, 0.1f, 0.7f });
    Gui::DrawGuiText("Biblioteka:", { 20, 225 }, 0.45f, { 1, 1, 1, 1 });

    float yOffset = 260.0f;
    for (const auto& entry : AssetManager::GetLibrary()) {
        if (Gui::Button(entry.Name, { 20, yOffset }, { 160, 25 })) {
            m_PendingModelPath = entry.Path;
            m_IsPlacing = true; // Aktywujemy tryb stawiania
        }
        yOffset += 35.0f;
    }

    // --- SEKCJA: LOGIKA STAWIANIA ---
    if (m_IsPlacing) {
        glm::vec2 mPos = Gui::GetMappedMousePos();
        Gui::DrawGuiText("Stawiasz: " + m_PendingModelPath, { mPos.x + 10, mPos.y }, 0.35f, { 0.2f, 1.0f, 0.2f, 1.0f });

        // Lewy przycisk stawia obiekt (blokada klikniêcia nad menu po lewej)
        if (Input::IsMouseButtonPressed(0) && mPos.x > 200) {
            Entity newEnt = world.CreateEntity();

            // Dodajemy komponenty
            world.AddComponent<TagComponent>(newEnt, TagComponent{ "Nowy Obiekt" });
            world.AddComponent<MeshComponent>(newEnt, MeshComponent{});
            world.AddComponent<TransformComponent>(newEnt, TransformComponent{});

            // Pobieramy i ustawiamy dane
            auto* mesh = world.GetComponent<MeshComponent>(newEnt);
            auto* trans = world.GetComponent<TransformComponent>(newEnt);

            if (mesh) mesh->ModelPtr = AssetManager::GetModel(m_PendingModelPath);
            if (trans) {
                trans->Position = { (mPos.x / m_ViewportWidth) * 20.f - 10.f, -(mPos.y / m_ViewportHeight) * 20.f + 10.f, 0.0f };
                trans->Scale = { 1.0f, 1.0f, 1.0f };
            }
            m_IsPlacing = false; // Opcjonalne: wy³¹czenie po jednym postawieniu
        }

        // Prawy przycisk anuluje stawianie
        if (Input::IsMouseButtonPressed(1)) m_IsPlacing = false;
    }

    // --- SEKCJA: INSPEKTOR WYBRANEJ ENCJI ---
    if (m_SelectedEntity.id != std::numeric_limits<std::size_t>::max()) {
        glm::vec2 inspPos = { m_ViewportWidth - 220.0f, 400.0f };

        auto* tag = world.GetComponent<TagComponent>(m_SelectedEntity);
        if (tag) {
            Gui::InputGuiText("Nazwa", tag->Tag, { inspPos.x, inspPos.y - 40.0f }, { 200, 25 });
        }

        auto* transform = world.GetComponent<TransformComponent>(m_SelectedEntity);
        if (transform) {
            Gui::SliderFloat("X", &transform->Position.x, -10.0f, 10.0f, { inspPos.x, inspPos.y + 50.0f }, { 200, 15 });
            Gui::SliderFloat("Y", &transform->Position.y, -10.0f, 10.0f, { inspPos.x, inspPos.y + 90.0f }, { 200, 15 });
            Gui::SliderFloat("Z", &transform->Position.z, -10.0f, 10.0f, { inspPos.x, inspPos.y + 130.0f }, { 200, 15 });

            if (Gui::SliderFloat("Skala", &transform->Scale.x, 0.1f, 5.0f, { inspPos.x, inspPos.y + 170.0f }, { 200, 15 })) {
                transform->Scale.y = transform->Scale.x;
                transform->Scale.z = transform->Scale.x;
            }
        }

        if (Gui::Button("USUN OBIEKT", { inspPos.x, inspPos.y }, { 200.0f, 30.0f })) {
            world.DestroyEntity(m_SelectedEntity);
            m_SelectedEntity = { std::numeric_limits<std::size_t>::max(), 0 };
        }
    }

    // 2. KOÑCZENIE RENDEROWANIA (Raz na koñcu klatki)
    Renderer2D::EndScene();
    glEnable(GL_DEPTH_TEST);
}

GuiLayer::~GuiLayer() {};

void GuiLayer::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) {
        return OnWindowResize(ev);
        });
};

bool GuiLayer::OnWindowResize(WindowResizeEvent& e) {
    m_ViewportWidth = (float)e.GetWidth();
    m_ViewportHeight = (float)e.GetHeight();

    Gui::UpdateScreenSize(m_ViewportWidth, m_ViewportHeight);

    return false;
}