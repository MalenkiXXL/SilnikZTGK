#include "Scene.h"
#include "CookingStation/Scripts/RotationScript.h" 
#include "ScriptableEntity.h" 
#include "Entity.h"            
#include "CookingStation/Core/Physics.h"            
#include "spdlog/spdlog.h"                  
#include "CookingStation/Scene/SceneSerializer.h"
#include <memory>
#include <algorithm>
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

// funkcja bierze pozycje, skale, rotacje obiektu i statyczne parametry kolidera a zwraca zaktualizowane pude³ko na obecn¹ klatkê gry.
AABB ComputeDynamicAABB(TransformComponent* trans, BoxColliderComponent* col)
{
	AABB box;

	// obliczamy fizyczny œrodek pude³ka w œwiecie
	glm::vec3 center = trans->Position + col->Offset;

	// obliczamy rozpiêtoœæ (odleg³oœæ od œrodka do œcianek w ka¿dej osi)
	glm::vec3 extents = trans->Scale * col->Size; 

	// wyliczamy sam¹ macierz rotacji i sk³adamy j¹
	glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(trans->Rotation.x), { 1, 0, 0 })
		* glm::rotate(glm::mat4(1.0f), glm::radians(trans->Rotation.y), { 0, 1, 0 })
		* glm::rotate(glm::mat4(1.0f), glm::radians(trans->Rotation.z), { 0, 0, 1 });

	// zachowanie wyrównania pude³ka wzglêdem obrotu. Pude³ko AABB nie mo¿e byæ obrocone - jego œciany zawsze id¹ wzd³u¿ osi X,Y,Z œwiata.
	glm::vec3 rotatedExtents(
		std::abs(rotation[0][0]) * extents.x + std::abs(rotation[1][0]) * extents.y + std::abs(rotation[2][0]) * extents.z, // Oœ X
		std::abs(rotation[0][1]) * extents.x + std::abs(rotation[1][1]) * extents.y + std::abs(rotation[2][1]) * extents.z, // Oœ Y
		std::abs(rotation[0][2]) * extents.x + std::abs(rotation[1][2]) * extents.y + std::abs(rotation[2][2]) * extents.z  // Oœ Z
	);

	// maj¹c wyliczony œrodek oraz powiêkszon¹ rozpiêtoœæ obróconego pude³ka - obliczamy jego fizyczne wierzcho³ki:
	// Min - najmniejszy wierzcho³ek (lewy dolny tylny)
	box.Min = center - rotatedExtents;
	// Max - najwiêkszy wierzcho³ek (prawy górny przedni)
	box.Max = center + rotatedExtents;

	return box;
}


void Scene::OnRuntimeStart()
{
	std::cout << "[Scene] OnRuntimeStart\n";
}

void Scene::OnUpdateRuntime(Timestep ts)
{
	// ==========================================
	// 1. update skrytpow
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
	// 2 kolizja
	// ==========================================

	// pobieramy dostêp do wszystkich komponentow kolizji
	auto* colliderStorage = m_ECSWorld.GetComponentVector<BoxColliderComponent>();
	// pobieramy dostêp do wszystkich komponentow transformacji
	auto* transformStorage = m_ECSWorld.GetComponentVector<TransformComponent>();

	if (colliderStorage && transformStorage && colliderStorage->dense.size() > 1)
	{
		// tworzymy tymczasowa strukture zeby trzymac encje razem z jej wyliczonym aktualnym pudelkiem 3D
		struct ColliderData
		{
			Entity ent;
			AABB box;
		};

		// tworzymy liste do ktorej wrzucimy wszystkie aktywne kolidery
		std::vector<ColliderData> activeColliders;
		// rezerwujemy pamiec z gory na dokladna liczbe koliderow
		activeColliders.reserve(colliderStorage->dense.size());

		//zbieramy aktualne	pudelka wszystkich encji
		for (size_t i = 0; i < colliderStorage->dense.size(); i++)
		{
			// odczytujemy ID encji na podstawie jej indeksu
			Entity ent = colliderStorage->reverse[i];
			// probujemy pobrac transformacje dla tej konkretnej encji
			auto* trans = transformStorage->Get(ent);
			// pobieramy dane samego komponentu kolizji
			auto* col = &colliderStorage->dense[i];

			if (trans) {

				activeColliders.push_back({ ent, ComputeDynamicAABB(trans, col) });
			}
		} 

		// sortujemy po minimalnej wartoœci osi X
		// optymalizacja - ustawiamy obiekty od lewej do prawej
		std::sort(activeColliders.begin(), activeColliders.end(), [](const ColliderData& a, const ColliderData& b) {
			return a.box.Min.x < b.box.Min.x; // obiekty z mniejszym minimalnym X beda pierwsze w liscie.
			});

		//sprawdzanie potencjalnych kolizji
		for (size_t i = 0; i < activeColliders.size(); i++)
		{
			const auto& dataA = activeColliders[i];

			// i sprawdzamy z kazdym nastepnym obiektem na liœcie (B)
			for (size_t j = i + 1; j < activeColliders.size(); j++)
			{
				const auto& dataB = activeColliders[j];

				// Jeœli minimalny X obiektu B jest wiêkszy ni¿ maksymalny X obiektu A,
				// to znaczy, ¿e obiekty B i WSZYSTKIE NASTÊPNE w posortowanej liœcie
				// s¹ zbyt daleko na prawo, ¿eby kolidowaæ z A.
				if (dataB.box.Min.x > dataA.box.Max.x)
				{
					break;
				}

				// W przeciwnym razie sprawdzamy pe³n¹ kolizjê AABB z Physics.h
				if (Physics::Intersects(dataA.box, dataB.box))
				{
					// Kolizja 
					spdlog::info("kolizja miedzy ID: {} a ID: {}", dataA.ent.id, dataB.ent.id);

					// Reakcja na kolizjê dla obiektu
					auto* scriptA = m_ECSWorld.GetComponent<NativeScriptComponent>(dataA.ent);
					if (scriptA && scriptA->Instance) {
						scriptA->Instance->OnCollision();
					}
					// Reakcja na kolizjê dla obiektu
					auto* scriptB = m_ECSWorld.GetComponent<NativeScriptComponent>(dataB.ent);
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

	auto* scriptStorage = m_ECSWorld.GetComponentVector<NativeScriptComponent>();
	if (scriptStorage)
	{
		for (size_t i = 0; i < scriptStorage->dense.size(); i++)
		{
			auto& scriptComp = scriptStorage->dense[i];
			if (scriptComp.Instance)
			{
				scriptComp.Instance->OnDestroy();
				delete scriptComp.Instance;       // zwalniamy pamiêæ
				scriptComp.Instance = nullptr; // zabezpieczenie przed wiszacym pointerem
			}
		}
	}
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