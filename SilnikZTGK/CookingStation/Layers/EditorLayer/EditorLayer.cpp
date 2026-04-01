#include "EditorLayer.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Layers/CameraLayer/Camera.h"
#include "CookingStation/Scene/ecs.h"
#include "CookingStation/Layers/GuiLayer/Gui.h" 

void EditorLayer::OnAttach() {
    // pobieramy aktualny rozmiar okna
    auto windowSize = Input::GetWindowSize();
    m_ViewportWidth = (float)windowSize.first;
    m_ViewportHeight = (float)windowSize.second;
}

void EditorLayer::OnUpdate() {
    
    // sprawdzamy czy istnieje jakas aktywna scena
    if (!m_ActiveScene) return;

    // je¿eli tak, to pobieramy z niej dane
    auto& world = m_ActiveScene->GetWorld();
    auto* camera = m_ActiveScene->GetCamera();

    // pobieramy pozycje kamery 
    glm::vec3 camPos = camera ? camera->Position : glm::vec3(0.0f);

    // obliczamy sobie skalê dla okna, aby dzialalo niezaleznie od rozmiaru
    float aspectRatio = m_ViewportWidth / (m_ViewportHeight > 0 ? m_ViewportHeight : 1.0f);
    float orthoSize = 10.0f; // orthoSize jest tym samym jakie jest zdefiniowane w RendererLayer
    
    // obliczamy wymiary swiata widocznego w oknie
    float worldHeight = orthoSize * 2.0f; 
    float worldWidth = worldHeight * aspectRatio;

    // ustawiamy orto i rysujemy elementy edytora
    glm::mat4 uiProj = glm::ortho(0.0f, m_ViewportWidth, m_ViewportHeight, 0.0f);
    Renderer2D::BeginScene(uiProj);

    // sprawdzamy, czy jest jakas prosba o postawienie modelu
    auto& request = m_ActiveScene->GetPlacementRequest();

    // jezeli istnieje prosba, pobieramy nazwe i sciezke kladzionego modelu
    if (request.Active) {
        m_PendingModelName = request.Name;
        m_PendingModelPath = request.Path;
        m_IsPlacing = true;
        request.Active = false; // konsumujemy prosbe by wykonala sie tylko raz 
    }

    // jezeli kladziemy to...
    if (m_IsPlacing) {

        // pobieramy pozycje myszki zmapowana na ekran gui
        glm::vec2 mPos = Gui::GetMappedMousePos();

        // nad myszk¹ poka¿e siê informacja o tym co stawiamy
        Gui::DrawGuiText("Stawiasz: " + m_PendingModelName, { mPos.x + 10, mPos.y }, 0.35f, { 0.2f, 1.0f, 0.2f, 1.0f });

        // jezeli klikniemy LPM (0) to..
        if (Input::IsMouseButtonPressed(0) && mPos.x > 200) {

            // tworzymy now¹ encjê
            Entity newEnt = world.CreateEntity();

            // dodajemy do niej komponenty
            world.AddComponent<TagComponent>(newEnt, TagComponent{ "Nowy Obiekt" });
            world.AddComponent<MeshComponent>(newEnt, MeshComponent{ AssetManager::GetModel(m_PendingModelPath) });
            world.AddComponent<TransformComponent>(newEnt, TransformComponent{});

            // nadajemy jej pozycje
            auto* trans = world.GetComponent<TransformComponent>(newEnt);
            if (trans) {
                // tutaj potrzebujemy surowych danych myszki, aby poprawnie rozmiescic modele w 3D
                auto rawMouse = Input::GetMousePosition();
                float xRatio = rawMouse.first / m_ViewportWidth;
                float yRatio = rawMouse.second / m_ViewportHeight;

                trans->Position.x = (xRatio * worldWidth - (worldWidth / 2.0f)) + camPos.x;
                trans->Position.y = (-(yRatio * worldHeight - (worldHeight / 2.0f))) + camPos.y;
                trans->Position.z = 0.0f;
            }

            // po polozeniu przestajemy rozmieszczac model
            m_IsPlacing = false; 

            spdlog::info("Editor: Umieszczono model {}", m_PendingModelName);
        }

        // prawym przyciskiem anulujemy
        if (Input::IsMouseButtonPressed(1)) m_IsPlacing = false;
    }

    // pobieramy informacje o zaznaczonym obiekcie ze sceny
    Entity selected = m_ActiveScene->GetSelectedEntity();

    if (selected.id != std::numeric_limits<std::size_t>::max()) {
        // pobieramy pozycje tego komponentu
        auto* transform = world.GetComponent<TransformComponent>(selected);
        if (transform) {
            // obliczamy pozycje œwiata na piksele ekranu, aby w dobrym miejscu ulozylo sie gizmo 
            float screenX = (((transform->Position.x - camPos.x) + (worldWidth / 2.0f)) / worldWidth) * m_ViewportWidth;
            float screenY = ((-(transform->Position.y - camPos.y) + (worldHeight / 2.0f)) / worldHeight) * m_ViewportHeight;

            // rysujemy osie x i y  
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