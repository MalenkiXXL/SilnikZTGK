#include "EditorLayer.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Layers/CameraLayer/Camera.h"
#include "CookingStation/Scene/ecs.h"
#include "CookingStation/Layers/GuiLayer/Gui.h" 

void EditorLayer::OnAttach() {
    auto windowSize = Input::GetWindowSize();
    m_ViewportWidth = (float)windowSize.first;
    m_ViewportHeight = (float)windowSize.second;
}

void EditorLayer::OnUpdate() {
    if (!m_ActiveScene) return;

    auto& world = m_ActiveScene->GetWorld();
    auto* camera = m_ActiveScene->GetCamera();
    glm::vec3 camPos = camera ? camera->Position : glm::vec3(0.0f);

    // 1. PRZYGOTOWANIE MACIERZY I PARAMETRÓW ŒWIATA
    float aspectRatio = m_ViewportWidth / (m_ViewportHeight > 0 ? m_ViewportHeight : 1.0f);
    float orthoSize = 10.0f; // Rozmiar identyczny jak w RendererLayer
    float worldHeight = orthoSize * 2.0f;
    float worldWidth = worldHeight * aspectRatio;

    // GUI rysujemy w osobnym Layerze, ale tutaj musimy narysowaæ elementy "œwiata" edytora
    glm::mat4 uiProj = glm::ortho(0.0f, m_ViewportWidth, m_ViewportHeight, 0.0f);
    Renderer2D::BeginScene(uiProj);

    auto& request = m_ActiveScene->GetPlacementRequest();

    // Jeœli GUI wys³a³o proœbê, przejmujemy j¹ do lokalnego stanu stawiania
    if (request.Active) {
        m_PendingModelName = request.Name;
        m_PendingModelPath = request.Path;
        m_IsPlacing = true;
        request.Active = false; // "Konsumujemy" proœbê, ¿eby nie wyzwala³a siê co klatkê
    }

    // --- LOGIKA STAWIANIA MODELU ---
    if (m_IsPlacing) {
        glm::vec2 mPos = Gui::GetMappedMousePos();
        // PodpowiedŸ przy muszce
        Gui::DrawGuiText("Stawiasz: " + m_PendingModelName, { mPos.x + 10, mPos.y }, 0.35f, { 0.2f, 1.0f, 0.2f, 1.0f });

        // Jeœli klikniemy lewym przyciskiem poza menu bocznym
        if (Input::IsMouseButtonPressed(0) && mPos.x > 200) {
            Entity newEnt = world.CreateEntity();

            world.AddComponent<TagComponent>(newEnt, TagComponent{ "Nowy Obiekt" });
            world.AddComponent<MeshComponent>(newEnt, MeshComponent{ AssetManager::GetModel(m_PendingModelPath) });
            world.AddComponent<TransformComponent>(newEnt, TransformComponent{});

            auto* trans = world.GetComponent<TransformComponent>(newEnt);
            if (trans) {
                // Pobieramy surow¹ pozycjê myszki do mapowania
                auto rawMouse = Input::GetMousePosition();
                float xRatio = rawMouse.first / m_ViewportWidth;
                float yRatio = rawMouse.second / m_ViewportHeight;

                // Mapowanie pozycji na œwiat 3D uwzglêdniaj¹c kamerê
                trans->Position.x = (xRatio * worldWidth - (worldWidth / 2.0f)) + camPos.x;
                trans->Position.y = (-(yRatio * worldHeight - (worldHeight / 2.0f))) + camPos.y;
                trans->Position.z = 0.0f;
            }

            m_IsPlacing = false; // Koñczymy stawianie po jednym klikniêciu
            spdlog::info("Editor: Umieszczono model {}", m_PendingModelName);
        }

        // Anulowanie prawym przyciskiem
        if (Input::IsMouseButtonPressed(1)) m_IsPlacing = false;
    }

    Entity selected = m_ActiveScene->GetSelectedEntity();
    // --- RYSOWANIE GIZMOS  ---
    if (selected.id != std::numeric_limits<std::size_t>::max()) {
        auto* transform = world.GetComponent<TransformComponent>(selected);
        if (transform) {
            // Przeliczamy pozycjê œwiata na piksele ekranu dla Gizmo
            float screenX = (((transform->Position.x - camPos.x) + (worldWidth / 2.0f)) / worldWidth) * m_ViewportWidth;
            float screenY = ((-(transform->Position.y - camPos.y) + (worldHeight / 2.0f)) / worldHeight) * m_ViewportHeight;

            // Rysujemy osie X (czerwona) i Y (zielona) o sta³ej d³ugoœci w pikselach
            Renderer2D::DrawQuad({ screenX, screenY }, { 50.0f, 2.0f }, { 1.0f, 0.0f, 0.0f, 1.0f });
            Renderer2D::DrawQuad({ screenX, screenY }, { 2.0f, 50.0f }, { 0.0f, 1.0f, 0.0f, 1.0f });
        }
    }

    Renderer2D::EndScene();
}

void EditorLayer::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) {
        return OnWindowResize(ev);
        });
}

bool EditorLayer::OnWindowResize(WindowResizeEvent& e) {
    m_ViewportWidth = (float)e.GetWidth();
    m_ViewportHeight = (float)e.GetHeight();
    return false;
}