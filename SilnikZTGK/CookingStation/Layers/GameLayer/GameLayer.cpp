#include "GameLayer.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Core/AudioEngine.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Core/Physics.h"
#include "CookingStation/Core/GridSystem.h"
#include "CookingStation/Layers/CameraLayer/Camera.h"
#include "CookingStation/Scene/ScriptableEntity.h"
#include <spdlog/spdlog.h> // Wymagane dla logów diagnostycznych
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {
    // U¿ywamy algorytmu SSA domyœlnie, tak jak w Edytorze
    static bool s_UseSSA = true;

    // Przeniesiona funkcja pomocnicza z EditorLayer do wykrywania klikniêtych obiektów
    Entity GetHoveredEntity(const Ray& ray, std::shared_ptr<Scene> activeScene, bool requireCollider)
    {
        auto& world = activeScene->GetWorld();
        auto* colliderStorage = world.GetComponentVector<BoxColliderComponent>();
        auto* transformStorage = world.GetComponentVector<TransformComponent>();

        Entity closestEntity = { std::numeric_limits<std::size_t>::max(), 0 };
        float closestDist = std::numeric_limits<float>::max();

        if (!transformStorage) return closestEntity;

        if (s_UseSSA)
        {
            if (std::abs(ray.Direction.y) > 1e-6f)
            {
                float yMax = 15.0f;
                float yMin = -5.0f;

                float tTop = (yMax - ray.Origin.y) / ray.Direction.y;
                float tBottom = (yMin - ray.Origin.y) / ray.Direction.y;

                if (tTop > tBottom) std::swap(tTop, tBottom);

                if (tBottom > 0.0f)
                {
                    if (tTop < 0.0f) tTop = 0.0f;

                    glm::vec3 hitTop = ray.Origin + tTop * ray.Direction;
                    glm::vec3 hitBottom = ray.Origin + tBottom * ray.Direction;

                    glm::ivec2 cellTop = GridSystem::WorldToCell(hitTop);
                    glm::ivec2 cellBottom = GridSystem::WorldToCell(hitBottom);

                    int minX = std::min(cellTop.x, cellBottom.x) - 1;
                    int maxX = std::max(cellTop.x, cellBottom.x) + 1;
                    int minZ = std::min(cellTop.y, cellBottom.y) - 1;
                    int maxZ = std::max(cellTop.y, cellBottom.y) + 1;

                    for (int cx = minX; cx <= maxX; cx++)
                    {
                        for (int cz = minZ; cz <= maxZ; cz++)
                        {
                            glm::ivec2 currentCell = { cx, cz };
                            const auto* entitiesInCell = activeScene->GetEntitiesInCell(currentCell);

                            if (entitiesInCell)
                            {
                                for (Entity entity : *entitiesInCell)
                                {
                                    TransformComponent* transform = transformStorage->Get(entity);
                                    if (!transform) continue;

                                    BoxColliderComponent* collider = colliderStorage ? colliderStorage->Get(entity) : nullptr;

                                    if (requireCollider && !collider) continue;

                                    glm::vec3 boundsOffset = glm::vec3(0.0f);
                                    glm::vec3 boundsSize = glm::vec3(1.0f);

                                    if (collider) {
                                        boundsOffset = collider->Offset;
                                        boundsSize = collider->Size;
                                    }

                                    glm::vec3 globalPos = { transform->WorldMatrix[3][0], transform->WorldMatrix[3][1], transform->WorldMatrix[3][2] };
                                    glm::vec3 center = globalPos + boundsOffset;
                                    glm::vec3 extents = transform->GetScale() * boundsSize;

                                    glm::vec3 rot = transform->GetRotation();
                                    glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), glm::radians(rot.x), { 1, 0, 0 })
                                        * glm::rotate(glm::mat4(1.0f), glm::radians(rot.y), { 0, 1, 0 })
                                        * glm::rotate(glm::mat4(1.0f), glm::radians(rot.z), { 0, 0, 1 });

                                    glm::mat4 obbTransform = glm::translate(glm::mat4(1.0f), center) * rotMat;
                                    glm::mat4 invTransform = glm::inverse(obbTransform);

                                    Ray localRay;
                                    localRay.Origin = glm::vec3(invTransform * glm::vec4(ray.Origin, 1.0f));
                                    localRay.Direction = glm::normalize(glm::vec3(invTransform * glm::vec4(ray.Direction, 0.0f)));

                                    AABB localAABB;
                                    localAABB.center = glm::vec3(0.0f);
                                    localAABB.extents = extents;

                                    if (Physics::Intersects(localRay, localAABB))
                                    {
                                        float dist = glm::dot(center - ray.Origin, ray.Direction);
                                        if (dist < closestDist)
                                        {
                                            closestDist = dist;
                                            closestEntity = entity;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else // Brute-Force
        {
            for (size_t it = 0; it < transformStorage->dense.size(); it++)
            {
                Entity entity = transformStorage->reverse[it];
                TransformComponent* transform = &transformStorage->dense[it];
                BoxColliderComponent* collider = colliderStorage ? colliderStorage->Get(entity) : nullptr;

                if (requireCollider && !collider) continue;

                glm::vec3 boundsOffset = glm::vec3(0.0f);
                glm::vec3 boundsSize = glm::vec3(1.0f);

                if (collider) {
                    boundsOffset = collider->Offset;
                    boundsSize = collider->Size;
                }

                glm::vec3 globalPos = { transform->WorldMatrix[3][0], transform->WorldMatrix[3][1], transform->WorldMatrix[3][2] };
                glm::vec3 center = globalPos + boundsOffset;
                glm::vec3 extents = transform->GetScale() * boundsSize;

                glm::vec3 rot = transform->GetRotation();
                glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), glm::radians(rot.x), { 1, 0, 0 })
                    * glm::rotate(glm::mat4(1.0f), glm::radians(rot.y), { 0, 1, 0 })
                    * glm::rotate(glm::mat4(1.0f), glm::radians(rot.z), { 0, 0, 1 });

                glm::mat4 obbTransform = glm::translate(glm::mat4(1.0f), center) * rotMat;
                glm::mat4 invTransform = glm::inverse(obbTransform);

                Ray localRay;
                localRay.Origin = glm::vec3(invTransform * glm::vec4(ray.Origin, 1.0f));
                localRay.Direction = glm::normalize(glm::vec3(invTransform * glm::vec4(ray.Direction, 0.0f)));

                AABB localAABB;
                localAABB.center = glm::vec3(0.0f);
                localAABB.extents = extents;

                if (Physics::Intersects(localRay, localAABB))
                {
                    float dist = glm::dot(center - ray.Origin, ray.Direction);
                    if (dist < closestDist)
                    {
                        closestDist = dist;
                        closestEntity = entity;
                    }
                }
            }
        }

        return closestEntity;
    }
}

void GameLayer::OnAttach()
{
    m_ActiveScene = SceneManager::GetActiveScene();

    if (!m_ActiveScene)
    {
        spdlog::error("GameLayer: Brak aktywnej sceny w SceneManager!");
        return;
    }

}

void GameLayer::OnDetach()
{
    if (m_ActiveScene)
        m_ActiveScene->OnRuntimeStop();
}


void GameLayer::OnUpdate(Timestep ts)
{
    m_ActiveScene = SceneManager::GetActiveScene();
    if (!m_ActiveScene) return;

    if (m_ActiveScene->GetState() != SceneState::Play)
    {
        return;
    }

    m_ActiveScene->OnUpdateRuntime(ts);

    auto& world = m_ActiveScene->GetWorld();
    auto* animatorStorage = world.GetComponentVector<AnimatorComponent>();


    if (animatorStorage && !animatorStorage->dense.empty())
    {
        // spdlog::info("Animator dziala! Liczba postaci: {}", animatorStorage->dense.size());
    }
    else
    {
        // spdlog::debug("GameLayer: Brak komponentów AnimatorComponent w aktywnej scenie.");
    }


    if (Input::IsMouseButtonJustPressed(0))
    {
        auto mousePos = Input::GetMousePosition();
        float mouseX = mousePos.first;
        float mouseY = mousePos.second;

        // Pobieramy rozmiar okna - domyœlnie pe³ny ekran
        auto windowSize = Input::GetWindowSize();
        float viewWidth = (float)windowSize.first;
        float viewHeight = (float)windowSize.second;

        // UWAGA: Jesli to Edytor (brak zdefiniowanego CS_DISTRIBUTED), 
        // przycinamy obszar i wspolrzedne myszy o panele edytora (ImGui).
#ifndef CS_DISTRIBUTED
        mouseX -= 200.0f;
        mouseY -= 30.0f;
        viewWidth -= 500.0f;
        viewHeight -= 230.0f;
#endif

        auto* camera = m_ActiveScene->GetCamera();
        if (camera)
        {
            // Aktualizujemy AspectRatio by proporcje klikniecia pokrywaly sie w 100% z kamera w grze
            float aspectRatio = viewWidth / (viewHeight > 0.0f ? viewHeight : 1.0f);
            camera->AspectRatio = aspectRatio;

            // Obliczamy rzutowanie promienia z kamery
            float orthoSize = 10.0f * (camera->Zoom / 45.0f);

            glm::mat4 proj3D = glm::ortho(-aspectRatio * orthoSize, aspectRatio * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
            glm::mat4 view3D = camera->GetViewMatrix();

            Ray ray = Physics::CastRayFromMouse(mouseX, mouseY, viewWidth, viewHeight, proj3D, view3D);

            // Wyszukujemy obiekt
            Entity closestEntity = GetHoveredEntity(ray, m_ActiveScene, true);

            // Jeœli znaleziono jakiœ obiekt, wywo³ujemy OnClick() na jego skryptach
            if (closestEntity.id != std::numeric_limits<std::size_t>::max())
            {
                auto* scriptStorage = world.GetComponentVector<NativeScriptComponent>();
                if (scriptStorage)
                {
                    auto* nsc = scriptStorage->Get(closestEntity);
                    if (nsc) {
                        for (auto& s : nsc->Scripts) {
                            if (s.Instance) {
                                s.Instance->OnClick();
                            }
                        }
                    }
                }
            }
        }
    }
}

void GameLayer::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);

    dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& event) {
        return OnKeyPressed(event);
        });
}

bool GameLayer::OnKeyPressed(KeyPressedEvent& e)
{
    std::shared_ptr<Scene> activeScene = SceneManager::GetActiveScene();

    if (!activeScene || activeScene->GetState() != SceneState::Play)
    {
        return false;
    }

    if (e.GetKeyCode() == 32 && e.GetRepeatCode() == 0) // Spacja
    {
        AudioEngine::Play("CookingStation/Assets/sounds/onion_chopping.mp3");
        return false;
    }

    return false;
}