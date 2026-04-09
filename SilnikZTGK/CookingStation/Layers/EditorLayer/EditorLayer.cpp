#include "EditorLayer.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Layers/CameraLayer/Camera.h"
#include "CookingStation/Scene/ecs.h"
#include "CookingStation/Layers/GuiLayer/Gui.h" 
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Scene/SceneSerializer.h"
#include "CookingStation/Scripts/RotationScript.h" 

void EditorLayer::OnAttach() {
    // pobieramy aktualny rozmiar okna
    auto windowSize = Input::GetWindowSize();
    m_ViewportWidth = (float)windowSize.first;
    m_ViewportHeight = (float)windowSize.second;
}

void EditorLayer::OnUpdate(Timestep ts) {
    
    // sprawdzamy czy istnieje jakas aktywna scena
    if (!m_ActiveScene) return;
    m_ActiveScene->Update(ts);

    // je�eli tak, to pobieramy z niej dane
    auto& world = m_ActiveScene->GetWorld();
    auto* camera = m_ActiveScene->GetCamera();

    // pobieramy pozycje kamery 
    glm::vec3 camPos = camera ? camera->Position : glm::vec3(0.0f);

    // obliczamy sobie skal� dla okna, aby dzialalo niezaleznie od rozmiaru
    float aspectRatio = m_ViewportWidth / (m_ViewportHeight > 0 ? m_ViewportHeight : 1.0f);
    float orthoSize = 10.0f; // orthoSize jest tym samym jakie jest zdefiniowane w RendererLayer
    
    // obliczamy wymiary swiata widocznego w oknie
    float worldHeight = orthoSize * 2.0f; 
    float worldWidth = worldHeight * aspectRatio;

    // ustawiamy orto i rysujemy elementy edytora
    glm::mat4 uiProj = glm::ortho(0.0f, m_ViewportWidth, m_ViewportHeight, 0.0f);

    glDisable(GL_DEPTH_TEST);

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

        // nad myszka pokaze sie informacja o tym co stawiamy
        Gui::DrawGuiText("Stawiasz: " + m_PendingModelName, { mPos.x + 10, mPos.y }, 0.35f, { 0.2f, 1.0f, 0.2f, 1.0f });

        // jezeli klikniemy LPM (0) to..
        if (Input::IsMouseButtonPressed(0) && mPos.x > 200) {

            // tworzymy now� encj�
            Entity newEnt = world.CreateEntity();

            // dodajemy do niej komponenty
            world.AddComponent<TagComponent>(newEnt, TagComponent{ "Nowy Obiekt" });
            world.AddComponent<MeshComponent>(newEnt, MeshComponent{ AssetManager::GetModel(m_PendingModelPath) });
            world.AddComponent<TransformComponent>(newEnt, TransformComponent{});
            world.AddComponent<BoxColliderComponent>(newEnt, BoxColliderComponent{});
            world.AddComponent<NativeScriptComponent>(newEnt, NativeScriptComponent{});

            auto* script = world.GetComponent<NativeScriptComponent>(newEnt);
            if (script) {
                script->Bind<RotationScript>();
            }

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

    if (Input::IsMouseButtonPressed(0))
    {
        glm::vec2 mPos = Gui::GetMappedMousePos();
        if (mPos.x > 200)
        {
            //kamera ze swiata gry
            float aspectRatio = m_ViewportWidth / (m_ViewportHeight > 0 ? m_ViewportHeight : 1.0f);
            float orthoSize = 10.0f;
            glm::mat4 projection = glm::ortho(-aspectRatio * orthoSize, aspectRatio * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);

            // jesli kamera znie istnieje dajemy bezpieczn� macierz bazow�
            glm::mat4 view = m_ActiveScene->GetCamera() ? m_ActiveScene->GetCamera()->GetViewMatrix() : glm::mat4(1.0f);
            Ray ray = Physics::CastRayFromMouse(Input::GetMousePosition().first, Input::GetMousePosition().second, m_ViewportWidth, m_ViewportHeight, projection, view);
            auto* meshStorage = world.GetComponentVector<MeshComponent>();
            auto* transformStorage = world.GetComponentVector<TransformComponent>();

            if (meshStorage != nullptr && transformStorage != nullptr)
            {
                for (auto it = 0; it < meshStorage->dense.size(); it++)
                {
                    //wyciaganie id z encji
                    Entity entity = meshStorage->reverse[it];
                    //pozycja
                    TransformComponent* transform = transformStorage->Get(entity);
 
                    if (transform != nullptr)
                    {
                        // tworzymy karton (AABB) woko� obiektu, odejmujemy i dodajemy skale, zeby karton rosl razem z modelem
                        AABB box;
                        box.Min = transform->Position - transform->Scale;
                        box.Max = transform->Position + transform->Scale;

                        // sprawdzamy trafienie
                        if (Physics::Intersects(ray, box))
                        {
                            m_ActiveScene->SetSelectedEntity(entity);
                            break;
                        }
                    }
                }
            }
        }
    }

    //Test BoxCollidera - potem w Scene::OnUpdate()
    auto* colliderStorage = world.GetComponentVector<BoxColliderComponent>();
    auto* transformStorage = world.GetComponentVector<TransformComponent>();

    if (colliderStorage && transformStorage && colliderStorage->dense.size() > 1)
    {
        //sprawdzenie kazdego z kazdym
        for (size_t i = 0; i < colliderStorage->dense.size(); i++)
        {
            Entity entA = colliderStorage->reverse[i];
            auto* transA = transformStorage->Get(entA);
            auto* colA = &colliderStorage->dense[i];

            if (!transA) continue;

            //aabb dla obiektu a
            AABB boxA;
            glm::vec3 centerA = transA->Position + colA->Offset;
            glm::vec3 extensA = transA->Scale * colA->Size;
            boxA.Min = centerA - extensA;
            boxA.Max = centerA + extensA;

            for (size_t j = i + 1; j < colliderStorage->dense.size(); j++) {
                Entity entB = colliderStorage->reverse[j];
                auto* transB = transformStorage->Get(entB);
                auto* colB = &colliderStorage->dense[j];

                if (!transB) continue;

                // Obliczamy AABB dla obiektu B
                AABB boxB;
                glm::vec3 centerB = transB->Position + colB->Offset;
                glm::vec3 extentsB = transB->Scale * colB->Size;
                boxB.Min = centerB - extentsB;
                boxB.Max = centerB + extentsB;

                if (Physics::Intersects(boxA, boxB))
                {
                    spdlog::info("kolizja miedzy ID: {} a ID: {}", entA.id, entB.id);

                    // szukamy skryptu dla obiektu A
                    auto* scriptA = world.GetComponent<NativeScriptComponent>(entA);
                    if (scriptA && scriptA->Instance) {
                        scriptA->Instance->OnCollision(); // Wysyłamy sygnał do skryptu
                    }

                    // szukamy skryptu dla obiektu B
                    auto* scriptB = world.GetComponent<NativeScriptComponent>(entB);
                    if (scriptB && scriptB->Instance) {
                        scriptB->Instance->OnCollision(); // Wysyłamy sygnał do skryptu
                    }
                }
            }
        }
    }

    Entity selected = m_ActiveScene->GetSelectedEntity();

    if (selected.id != std::numeric_limits<std::size_t>::max()) {
        // pobieramy pozycje tego komponentu
        auto* transform = world.GetComponent<TransformComponent>(selected);
        if (transform) {
            // obliczamy pozycje �wiata na piksele ekranu, aby w dobrym miejscu ulozylo sie gizmo 
            float screenX = (((transform->Position.x - camPos.x) + (worldWidth / 2.0f)) / worldWidth) * m_ViewportWidth;
            float screenY = ((-(transform->Position.y - camPos.y) + (worldHeight / 2.0f)) / worldHeight) * m_ViewportHeight;

            // rysujemy osie x i y  
            Renderer2D::DrawQuad({ screenX, screenY }, { 50.0f, 2.0f }, { 1.0f, 0.0f, 0.0f, 1.0f });
            Renderer2D::DrawQuad({ screenX, screenY }, { 2.0f, 50.0f }, { 0.0f, 1.0f, 0.0f, 1.0f });
        }
    };

    SceneSerializer serializer(m_ActiveScene.get());
    
    if (Gui::Button("Save Scene", { m_ViewportWidth - 150.0f, m_ViewportHeight - 100.f }, { 100.f,50.f })) {
        serializer.Serialize("CookingStation/Assets/saved.json");
    }
  
   
    Renderer2D::EndScene();
    glEnable(GL_DEPTH_TEST);
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