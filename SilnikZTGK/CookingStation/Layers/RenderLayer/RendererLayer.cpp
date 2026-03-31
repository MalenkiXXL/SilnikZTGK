#include "RendererLayer.h"
#include "CookingStation/Renderer/Renderer.h"
#include "CookingStation/Renderer/RenderCommand.h"
#include "CookingStation/Scene/ecs.h"
#include "CookingStation/Layers/CameraLayer/Camera.h" 
#include <glm/gtc/matrix_transform.hpp>

void RendererLayer::OnAttach() {
    // ³adujemy shader na starcie warstwy
    m_Shader = m_ShaderLibrary.Load("Standardowy", "CookingStation/Shaders/vsShaders/shader.vs", "CookingStation/Shaders/fragShaders/shader.frag");
}

void RendererLayer::OnUpdate() {
    if (!m_ActiveScene) return;

    //pobieramy baze danych ecs ze sceny
    auto& world = m_ActiveScene->GetWorld();
    //proporcje ekranu
    float aspectRatio = m_ViewportWidth / (m_ViewportHeight > 0 ? m_ViewportHeight : 1.0f);

    //skrzynki z komponentami ze œwiata
    auto* colorStorage = world.GetComponentVector<ClearColorComponent>();
    auto* meshStorage = world.GetComponentVector<MeshComponent>();
    auto* transformStorage = world.GetComponentVector<TransformComponent>();

    //----czyszczenie ekranu-------
    glm::vec4 clearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
    //jesli istnieje encja ktora ma komponent tla - pobieramy ten kolor
    if (colorStorage && !colorStorage->dense.empty()) {
        clearColor = colorStorage->dense[0].bgColor;
    }

    RenderCommand::SetClearColor(clearColor);
    RenderCommand::Clear();

    //-------rysowanie 3d----------
    //czy scena ma kamere
    if (m_ActiveScene->GetCamera()) {
        //macierz widoku z kamery
        glm::mat4 view = m_ActiveScene->GetCamera()->GetViewMatrix();

        //kamera ortograficzna
        float orthoSize = 10.0f;
        glm::mat4 projection = glm::ortho(-aspectRatio * orthoSize, aspectRatio * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
        glm::mat4 viewProjection = projection * view;

        //rozpoczecie sceny w Renderer
        Renderer::BeginScene(viewProjection);

        //aktywujemy program na karcie
        m_Shader->use();
        m_Shader->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
        m_Shader->setVec3("lightPos", glm::vec3(5.0f, 5.0f, 10.0f));
        m_Shader->setVec3("viewPos", m_ActiveScene->GetCamera()->Position);

        if (meshStorage && transformStorage) {
            //przelatujemy przez liste obietkow ktore maja modele
            for (size_t i = 0; i < meshStorage->dense.size(); i++) {
                //wyciagamy dane modelu
                auto& meshComp = meshStorage->dense[i];
                //odwracamy index zeby sparawdzic jakie to entity
                Entity owner = meshStorage->reverse[i];
                //jak wiemy czyje to to bierzemy pozycje tego konkretnego wlasciciela
                TransformComponent* transform = transformStorage->Get(owner);

                //jesli obiekt ma i model i pozycje
                if (transform && meshComp.ModelPtr) {
                    //budujemy macierz widoku
                    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), transform->Position);
                    modelMatrix = glm::rotate(modelMatrix, glm::radians(transform->Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                    modelMatrix = glm::rotate(modelMatrix, glm::radians(transform->Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                    modelMatrix = glm::rotate(modelMatrix, glm::radians(transform->Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
                    modelMatrix = glm::scale(modelMatrix, transform->Scale);

                    m_Shader->setMat4("viewProjection", viewProjection);
                    m_Shader->setMat4("model", modelMatrix);
                    //rysujemy uzywajac tego shadera
                    meshComp.ModelPtr->Draw(*m_Shader);
                }
            }
        }
        Renderer::EndScene();
    }
}
//odbiornik zdarzen
void RendererLayer::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& ev) {
        return OnWindowResize(ev);
        });
}
//tylko wtedy gdy ktos zlapie krawedz okna
bool RendererLayer::OnWindowResize(WindowResizeEvent& e) {
    m_ViewportWidth = (float)e.GetWidth();
    m_ViewportHeight = (float)e.GetHeight();
    return false;
}
