#include "Scene.h"
#include "CookingStation/Scripts/RotationScript.h" 
#include "ScriptableEntity.h" 
#include "Entity.h"            
#include "CookingStation/Core/Physics.h"            
#include "spdlog/spdlog.h"                  
#include "CookingStation/Scene/SceneSerializer.h"
#include <memory>
#include "CookingStation/Layers/CameraLayer/Camera.h"
#include "CookingStation/Renderer/Model.h"          
#include "glm/gtc/matrix_transform.hpp"

#include <iostream> 

Scene::Scene()
{
	m_ECSWorld.RegisterComponent<TagComponent>();
	m_ECSWorld.RegisterComponent<MeshComponent>();
	m_ECSWorld.RegisterComponent<TransformComponent>();
	m_ECSWorld.RegisterComponent<BoxColliderComponent>();
	m_ECSWorld.RegisterComponent<NativeScriptComponent>();
	m_ECSWorld.RegisterComponent<ClearColorComponent>();
}

Scene::~Scene() {};


void Scene::OnRuntimeStart()
{
	std::cout << "[Scene] OnRuntimeStart\n";
}

void Scene::OnUpdateRuntime(Timestep ts)
{
	// ==========================================
	// 1. UPDATE SKRYPTÓW (Logika gracza i obiektów)
	// ==========================================
	auto* scriptStorage = m_ECSWorld.GetComponentVector<NativeScriptComponent>();

	if (scriptStorage)
	{
		for (size_t i = 0; i < scriptStorage->dense.size(); i++)
		{
			Entity entity = scriptStorage->reverse[i];
			auto& scriptComp = scriptStorage->dense[i];

			if (!scriptComp.Instance)
			{
				if (scriptComp.InstantiateScript)
				{
					scriptComp.Instance = scriptComp.InstantiateScript();
					scriptComp.Instance->m_Entity = entity;
					scriptComp.Instance->m_Scene = this;
					scriptComp.Instance->OnCreate();
				}
			}

			if (scriptComp.Instance)
			{
				scriptComp.Instance->OnUpdate(ts);
			}
		}
	}

	// ==========================================
	// 2. KROK FIZYKI I KOLIZJI 
	// ==========================================
	auto* colliderStorage = m_ECSWorld.GetComponentVector<BoxColliderComponent>();
	auto* transformStorage = m_ECSWorld.GetComponentVector<TransformComponent>();

	if (colliderStorage && transformStorage && colliderStorage->dense.size() > 1)
	{
		for (size_t i = 0; i < colliderStorage->dense.size(); i++)
		{
			Entity entA = colliderStorage->reverse[i];
			auto* transA = transformStorage->Get(entA);
			auto* colA = &colliderStorage->dense[i];

			if (!transA) continue;

			AABB boxA;
			glm::vec3 centerA = transA->Position + colA->Offset;
			glm::vec3 extensA = transA->Scale * colA->Size;
			boxA.Min = centerA - extensA;
			boxA.Max = centerA + extensA;

			for (size_t j = i + 1; j < colliderStorage->dense.size(); j++)
			{
				Entity entB = colliderStorage->reverse[j];
				auto* transB = transformStorage->Get(entB);
				auto* colB = &colliderStorage->dense[j];

				if (!transB) continue;

				AABB boxB;
				glm::vec3 centerB = transB->Position + colB->Offset;
				glm::vec3 extentsB = transB->Scale * colB->Size;
				boxB.Min = centerB - extentsB;
				boxB.Max = centerB + extentsB;

				if (Physics::Intersects(boxA, boxB))
				{
					spdlog::info("kolizja miedzy ID: {} a ID: {}", entA.id, entB.id);

					auto* scriptA = m_ECSWorld.GetComponent<NativeScriptComponent>(entA);
					if (scriptA && scriptA->Instance) {
						scriptA->Instance->OnCollision();
					}

					auto* scriptB = m_ECSWorld.GetComponent<NativeScriptComponent>(entB);
					if (scriptB && scriptB->Instance) {
						scriptB->Instance->OnCollision();
					}
				}
			}
		}
	}
	// ==========================================
	// 3. RENDEROWANIE SCENY 
	// ==========================================
	if (m_MainCamera)
	{
		// 1. Obliczamy macierz projekcji 
		glm::mat4 projection = glm::perspective(glm::radians(m_MainCamera->Zoom), 16.0f / 9.0f, 0.1f, 100.0f);

		// 2. £¹czymy Projekcjê z Widokiem z Twojej klasy Camera
		glm::mat4 viewProjection = projection * m_MainCamera->GetViewMatrix();

		// 3. Rozpoczynamy klatkê
		Renderer::BeginScene(viewProjection);

		auto* meshStorage = m_ECSWorld.GetComponentVector<MeshComponent>();
		auto* transformStorage = m_ECSWorld.GetComponentVector<TransformComponent>();

		if (meshStorage && transformStorage)
		{
			for (size_t i = 0; i < meshStorage->dense.size(); i++)
			{
				Entity entity = meshStorage->reverse[i];
				auto& meshComp = meshStorage->dense[i];
				auto* trans = transformStorage->Get(entity);

				// U¿ywamy ModelPtr z Twojego MeshComponent i sprawdzamy czy dodano ShaderPtr
				if (trans && meshComp.ModelPtr && meshComp.ShaderPtr)
				{
					// U¿ywamy GetTransformMatrix() z Twojego TransformComponent
					Renderer::Submit(meshComp.ShaderPtr, meshComp.ModelPtr, trans->GetTransformMatrix());
				}
			}
		}
		Renderer::EndScene();
	}
}

void Scene::OnRuntimeStop()
{
	std::cout << "[Scene] OnRuntimeStop\n";
}

std::shared_ptr<Scene> Scene::Copy(std::shared_ptr<Scene> other)
{
	// 1. Tworzymy now¹, pust¹ scenê ( m_RuntimeScene)
	std::shared_ptr<Scene> newScene = std::make_shared<Scene>();

	// 2. U¿ywamy serializera, aby zapisaæ obecn¹ scenê do pliku temp
	SceneSerializer serializer(other.get());

	// Zapisujemy w folderze saves jako plik tymczasowy
	serializer.Serialize("CookingStation/Assets/saves/temp_play_scene.json");

	// 3. Wczytujemy dok³adnie ten sam plik do nowej sceny
	SceneSerializer deserializer(newScene.get());
	deserializer.Deserialize("CookingStation/Assets/saves/temp_play_scene.json");

	// 4. Zwracamy now¹, gotow¹ do gry scenê, która jest dok³adnym klonem edytora
	return newScene;
}

