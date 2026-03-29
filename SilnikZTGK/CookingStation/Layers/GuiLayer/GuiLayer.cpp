#include "GuiLayer.h"
#include "CookingStation/Events/WindowEvent.h"

void GuiLayer::OnAttach() {
    Renderer2D::Init();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Gui::UpdateScreenSize(m_ViewportWidth, m_ViewportHeight);
    Gui::Init("CookingStation/Assets/fonts/ARIAL.ttf", 32);
}

void GuiLayer::OnUpdate() {
    auto& world = m_ActiveScene->GetWorld();
    auto* colorStorage = world.GetComponentVector<ClearColorComponent>();
    // Pobieramy wskaŸnik do koloru t³a z ECS, aby suwak móg³ go modyfikowaæ
    float* redChan = nullptr;
    float* greenChan = nullptr;
    float* blueChan = nullptr;
    if (colorStorage && !colorStorage->dense.empty()) {
        redChan = &colorStorage->dense[0].bgColor.r;
        greenChan = &colorStorage->dense[0].bgColor.g;
        blueChan = &colorStorage->dense[0].bgColor.b;
    }

    // Wy³¹czamy test g³êbi, aby GUI nie by³o "przecinane" przez modele 3D
    glDisable(GL_DEPTH_TEST);

    // Tworzymy macierz ortogonaln¹ dopasowan¹ do aktualnego rozmiaru okna
    glm::mat4 uiProj = glm::ortho(0.0f, m_ViewportWidth, m_ViewportHeight, 0.0f);

    Renderer2D::BeginScene(uiProj);

    Renderer2D::DrawQuad({ 45.f,20.f }, { 210.f,165.f }, { 0.5f,0.5f,0.2f,0.1f });

    if (redChan) {
        // Suwak modyfikuje bezpoœrednio wartoœæ w komponencie ECS
        Gui::SliderFloat("R", redChan, 0.0f, 1.0f, { 50, 50 }, { 200, 20 });
        Gui::SliderFloat("G", greenChan, 0.0f, 1.0f, { 50, 100 }, { 200, 20 });
        Gui::SliderFloat("B", blueChan, 0.0f, 1.0f, { 50, 150 }, { 200, 20 });
    }

    auto* tagStorage = world.GetComponentVector<TagComponent>();
    if (tagStorage) {
        glm::vec2 startPos = { m_ViewportWidth - 220.0f, 50.0f };
        glm::vec2 itemSize = { 200.0f, 30.0f };
        float spacing = 5.0f;

        Gui::DrawGuiText("hierarchia sceny:", { startPos.x, startPos.y - 25.0f }, 0.5f, { 1.0f, 1.0f, 1.0f, 1.0f });

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

    // --- SEKCJA INSPEKTORA ---
// Sprawdzamy, czy jakakolwiek encja jest wybrana
    if (m_SelectedEntity.id != std::numeric_limits<std::size_t>::max()) {

        // Ustalamy pozycjê panelu inspektora (np. pod hierarchi¹)
        glm::vec2 inspPos = { m_ViewportWidth - 220.0f, 400.0f };

        // 1. Edycja Transformacji (Pozycja i Skala)
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
    }

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