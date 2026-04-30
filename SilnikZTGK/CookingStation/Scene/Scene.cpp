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
#include "CookingStation/Scripts/ConveyorScript.h"

#include <iostream> 

Scene::Scene()
{
	m_ECSWorld.RegisterComponent<TagComponent>();
	m_ECSWorld.RegisterComponent<MeshComponent>();
	m_ECSWorld.RegisterComponent<TransformComponent>();
	m_ECSWorld.RegisterComponent<BoxColliderComponent>();
	m_ECSWorld.RegisterComponent<NativeScriptComponent>();
	m_ECSWorld.RegisterComponent<ClearColorComponent>();
	m_ECSWorld.RegisterComponent<RelationshipComponent>();
	m_ECSWorld.RegisterComponent<ConveyorComponent>();
}

Scene::~Scene() {};

AABB ComputeDynamicAABB(TransformComponent* trans, BoxColliderComponent* col)
{
	AABB box;
	glm::vec3 globalPos = glm::vec3(trans->WorldMatrix[3][0], trans->WorldMatrix[3][1], trans->WorldMatrix[3][2]);
	glm::vec3 center = globalPos + col->Offset;
	glm::vec3 extents = trans->Scale * col->Size; // odleg³oœæ od œrodka do krawêdzi

	// wyliczamy sam¹ macierz rotacji
	glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(trans->Rotation.x), { 1, 0, 0 })
		* glm::rotate(glm::mat4(1.0f), glm::radians(trans->Rotation.y), { 0, 1, 0 })
		* glm::rotate(glm::mat4(1.0f), glm::radians(trans->Rotation.z), { 0, 0, 1 });

	// przekszta³camy rozmiary przez bezwzglêdne wartoœci macierzy rotacji
	glm::vec3 rotatedExtents(
		std::abs(rotation[0][0]) * extents.x + std::abs(rotation[1][0]) * extents.y + std::abs(rotation[2][0]) * extents.z,
		std::abs(rotation[0][1]) * extents.x + std::abs(rotation[1][1]) * extents.y + std::abs(rotation[2][1]) * extents.z,
		std::abs(rotation[0][2]) * extents.x + std::abs(rotation[1][2]) * extents.y + std::abs(rotation[2][2]) * extents.z
	);

	box.Min = center - rotatedExtents;
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
	CalculateTransforms();
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

	if (!m_ConveyorCacheReady)
	{
		RebuildConveyorCache();
		m_ConveyorCacheReady = true;
	}

	// ==========================================
	// 2. KROK FIZYKI I KOLIZJI 
	// ==========================================

	auto* colliderStorage = m_ECSWorld.GetComponentVector<BoxColliderComponent>();
	auto* transformStorage = m_ECSWorld.GetComponentVector<TransformComponent>();

	if (colliderStorage && transformStorage && colliderStorage->dense.size() > 1)
	{
		struct ColliderData
		{
			Entity ent;
			AABB box;
		};

		std::vector<ColliderData> activeColliders;
		activeColliders.reserve(colliderStorage->dense.size());

		//zbieramy aktualne	pudelka wszystkich encji
		for (size_t i = 0; i < colliderStorage->dense.size(); i++)
		{
			Entity ent = colliderStorage->reverse[i];
			auto* trans = transformStorage->Get(ent);
			auto* col = &colliderStorage->dense[i];

			if (trans) {

				activeColliders.push_back({ ent, ComputeDynamicAABB(trans, col) });
			}
		} 

		// 2. sortujemy po minimalnej wartoœci osi X
		std::sort(activeColliders.begin(), activeColliders.end(), [](const ColliderData& a, const ColliderData& b) {
			return a.box.Min.x < b.box.Min.x;
			});

		for (size_t i = 0; i < activeColliders.size(); i++)
		{
			const auto& dataA = activeColliders[i];

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

					auto* scriptA = m_ECSWorld.GetComponent<NativeScriptComponent>(dataA.ent);
					if (scriptA && scriptA->Instance) {
						scriptA->Instance->OnCollision();
					}

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
	// Przeliczamy ca³e drzewo transformacji przed renderowaniem

	if (m_MainCamera)
	{
		float orthoSize = 10.0f * (m_MainCamera->Zoom / 45.0f);
		glm::mat4 projection = glm::ortho(-m_MainCamera->AspectRatio * orthoSize, m_MainCamera->AspectRatio * orthoSize, -orthoSize, orthoSize, -100.0f, 100.0f);
		glm::mat4 viewProjection = projection * m_MainCamera->GetViewMatrix();

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

				if (trans && meshComp.ModelPtr && meshComp.ShaderPtr)
				{
					// Wysy³amy do karty graficznej ostateczn¹ pozycjê w œwiecie (WorldMatrix)
					Renderer::Submit(meshComp.ShaderPtr, meshComp.ModelPtr, trans->WorldMatrix);
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

	m_ConveyorCacheReady = false;
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

// 1. Rekurencyjna funkcja schodz¹ca w g³¹b drzewa
void UpdateTransformTree(World& world, std::size_t entityId, const glm::mat4& parentGlobalMatrix) {
	// U¿ywamy GetComponentByID, by omin¹æ sztywn¹ generacjê 0
	auto* transform = world.GetComponentByID<TransformComponent>(entityId);
	if (!transform) return;

	// A. Liczymy nasz¹ pozycjê w œwiecie = Rodzic * Nasza Lokalna
	glm::mat4 localMatrix = transform->GetLocalMatrix();
	transform->WorldMatrix = parentGlobalMatrix * localMatrix;

	// B. Przekazujemy nasz¹ macierz ni¿ej, do naszych dzieci
	auto* rel = world.GetComponentByID<RelationshipComponent>(entityId);
	if (rel && rel->FirstChild != NULL_ENTITY) {

		std::size_t currentChildId = rel->FirstChild;

		while (currentChildId != NULL_ENTITY) {
			// Wywo³anie rekurencyjne
			UpdateTransformTree(world, currentChildId, transform->WorldMatrix);

			// Przechodzimy do kolejnego brata
			auto* childRel = world.GetComponentByID<RelationshipComponent>(currentChildId);
			if (childRel) {
				currentChildId = childRel->NextSibling;
			}
			else {
				break; // zabezpieczenie
			}
		}
	}
}

void Scene::CalculateTransforms() {
	auto& world = GetWorld();
	auto* transformStorage = world.GetComponentVector<TransformComponent>();
	auto* relStorage = world.GetComponentVector<RelationshipComponent>();

	if (!transformStorage) return;

	// Iterujemy po wszystkich encjach z Transform
	for (size_t i = 0; i < transformStorage->reverse.size(); i++) {
		Entity entity = transformStorage->reverse[i];

		bool isRoot = true;

		// Sprawdzamy czy encja ma rodzica
		if (relStorage) {
			// U¿ywamy GetByID, aby omin¹æ weryfikacjê generacji i oprzeæ siê na czystym indeksie
			if (auto* rel = relStorage->GetByID(entity.id)) {
				if (rel->Parent != NULL_ENTITY) {
					isRoot = false; // Ma rodzica! Zostanie przeliczona, gdy funkcja wywo³a siê dla rodzica.
				}
			}
		}

		// Jeœli to korzeñ grafu (lub samodzielny obiekt), startujemy drzewo
		if (isRoot) {
			UpdateTransformTree(world, entity.id, glm::mat4(1.0f));
		}
	}
}

void Scene::SetParent(Entity child, Entity parent) {
	auto& world = GetWorld();

	// 1. Zabezpieczenie przed zapêtleniem (Cylic Dependency Check)
	Entity currentAncestor = parent;
	while (currentAncestor.id != NULL_ENTITY) {
		if (currentAncestor.id == child.id) {
			spdlog::warn("Nie mozna podpiac: Cykl w hierarchii! Encja {} jest juz przodkiem {}.", child.id, parent.id);
			return; // Przerywamy akcjê!
		}
		auto* ancestorRel = world.GetComponent<RelationshipComponent>(currentAncestor);
		if (ancestorRel && ancestorRel->Parent != NULL_ENTITY) {
			currentAncestor.id = ancestorRel->Parent;
		}
		else {
			break;
		}
	}

	// 2. Dodajemy komponenty relacji, jeœli ich nie maj¹
	if (!world.GetComponent<RelationshipComponent>(child)) {
		world.AddComponent<RelationshipComponent>(child, {});
	}
	if (!world.GetComponent<RelationshipComponent>(parent)) {
		world.AddComponent<RelationshipComponent>(parent, {});
	}

	auto* childRel = world.GetComponent<RelationshipComponent>(child);
	auto* parentRel = world.GetComponent<RelationshipComponent>(parent);

	// 3. ODPIÊCIE OD STAREGO RODZICA (jeœli dziecko ju¿ jakiegoœ mia³o)
	if (childRel->Parent != NULL_ENTITY) {
		auto* oldParentRel = world.GetComponent<RelationshipComponent>({ childRel->Parent, 0 });
		if (oldParentRel) {
			// Szukamy dziecka na liœcie starego rodzica i je usuwamy (przepinamy wskaŸniki braci)
			if (oldParentRel->FirstChild == child.id) {
				oldParentRel->FirstChild = childRel->NextSibling;
			}
			if (childRel->PreviousSibling != NULL_ENTITY) {
				auto* prevRel = world.GetComponent<RelationshipComponent>({ childRel->PreviousSibling, 0 });
				if (prevRel) prevRel->NextSibling = childRel->NextSibling;
			}
			if (childRel->NextSibling != NULL_ENTITY) {
				auto* nextRel = world.GetComponent<RelationshipComponent>({ childRel->NextSibling, 0 });
				if (nextRel) nextRel->PreviousSibling = childRel->PreviousSibling;
			}
			oldParentRel->ChildrenCount--;
		}
	}

	// 4. NOWE PODPIÊCIE
	childRel->Parent = parent.id;
	childRel->NextSibling = parentRel->FirstChild;
	childRel->PreviousSibling = NULL_ENTITY;

	if (parentRel->FirstChild != NULL_ENTITY) {
		auto* oldFirstChildRel = world.GetComponentByID<RelationshipComponent>(parentRel->FirstChild);
		if (oldFirstChildRel) {
			oldFirstChildRel->PreviousSibling = child.id;
		}
	}

	parentRel->FirstChild = child.id;
	parentRel->ChildrenCount++;

	spdlog::info("Podpieto encje {} do rodzica {}", child.id, parent.id);
}

void Scene::RebuildConveyorCache()
{
	Conveyors.clear();

	auto* scriptStorage = m_ECSWorld.GetComponentVector<NativeScriptComponent>();
	if (!scriptStorage) return;

	for (auto& scriptComp : scriptStorage->dense)
	{
		if (scriptComp.Instance)
		{
			ConveyorScript* conveyor = dynamic_cast<ConveyorScript*>(scriptComp.Instance);
			if (conveyor)
				Conveyors.push_back(conveyor);
		}
	}

	spdlog::info("Znaleziono {} tasmy na scenie.", Conveyors.size());
}