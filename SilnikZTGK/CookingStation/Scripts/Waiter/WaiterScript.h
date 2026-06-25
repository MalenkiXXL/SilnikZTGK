#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Core/GridSystem.h"
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Core/Input.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Scripts/Managers/GameManagerScript.h"
#include "CookingStation/Renderer/Model.h"
#include "CookingStation/Core/AudioEngine.h"
#include <spdlog/spdlog.h>
#include <vector>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <limits>
#include <fstream>
#include <algorithm>

struct AStarNode
{
    glm::ivec2 pos;
    float gCost;
    float hCost;
    glm::ivec2 parent;

    float fCost() const { return gCost + hCost; }
};

struct CompareNode
{
    bool operator()(const AStarNode& a, const AStarNode& b) const
    {
        return a.fCost() > b.fCost();
    }
};

class WaiterScript : public ScriptableEntity
{
public:
    enum class State {
        IDLE,
        MOVING_TO_TAKE_ORDER,
        TAKING_ORDER,
        MOVING_TO_FOOD,
        MOVING_TO_CUSTOMER,
        MOVING_TO_STATION_TO_WAVE,
        WAVING_AT_STATION,
        RETURNING
    };
    State m_CurrentState = State::IDLE;

    float m_TakeOrderTimer = 0.0f;
    float m_WaitAtPassTimer = 0.0f;

    glm::vec3 m_NextWaypoint;
    bool m_HasWaypoint = false;

    float m_Speed = 5.0f;
    float m_InteractRange = 2.4f;
    glm::vec3 m_HomePosition;

    Entity m_TargetCustomer = { std::numeric_limits<std::size_t>::max(), 0 };
    Entity m_TargetPlate = { std::numeric_limits<std::size_t>::max(), 0 };
    bool m_IsCarryingPlate = false;

    struct WaiterTask {
        int Type; 
        Entity Target;
    };
    std::vector<WaiterTask> m_TaskQueue;

    std::vector<Entity> m_AwaitingFoodCustomers;

    std::size_t m_CustomerSubId = 0;
    std::size_t m_PlateSubId = 0;
    std::size_t m_OrderTakenSubId = 0;
    std::size_t m_CustomerServedSubId = 0;
    std::size_t m_PlateGrabbedSubId = 0;

    std::shared_ptr<Model> m_OriginalModel = nullptr;
    std::shared_ptr<Model> m_PageModel = nullptr;
    bool m_WasWavingState = false;

    std::unordered_set<glm::ivec2, IVec2Hash> m_StaticObstacles;
    glm::vec3 m_WaveOffset = glm::vec3(-2.0f, 0.0f, 3.0f);
    glm::vec3 m_TrayOffset = glm::vec3(1.0f, 1.1f, 0.0f);


    void OnCreate() override
    {
        auto* tc = GetComponent<TransformComponent>();
        if (tc) m_HomePosition = tc->GetPosition();

        auto* meshComp = GetComponent<MeshComponent>();
        if (meshComp) {
            m_OriginalModel = meshComp->ModelPtr;
        }
        m_PageModel = AssetManager::GetModel("assets://models/animacje/grzybek/grzybek-notes.gltf");

        auto& bus = GetScene()->GetWorld().GetEventBus();

        m_CustomerSubId = bus.Subscribe<CustomerSeatedEvent>([this](const CustomerSeatedEvent& e) {
            m_TaskQueue.push_back({ 0, e.Customer });
            });

        m_PlateSubId = bus.Subscribe<PlateReadyEvent>([this](const PlateReadyEvent& e) {
            m_TaskQueue.push_back({ 1, e.Plate });
            });

        m_OrderTakenSubId = bus.Subscribe<OrderTakenEvent>([this](const OrderTakenEvent& e) {
            m_AwaitingFoodCustomers.push_back(e.Customer);
            });

        m_CustomerServedSubId = bus.Subscribe<CustomerServedEvent>([this](const CustomerServedEvent& e) {
            m_AwaitingFoodCustomers.erase(std::remove_if(m_AwaitingFoodCustomers.begin(), m_AwaitingFoodCustomers.end(),
                [&e](Entity cust) { return cust.id == e.Customer.id; }),
                m_AwaitingFoodCustomers.end());
            });

        m_PlateGrabbedSubId = bus.Subscribe<PlateGrabbedEvent>([this](const PlateGrabbedEvent& e) {
            m_TaskQueue.erase(std::remove_if(m_TaskQueue.begin(), m_TaskQueue.end(),
                [&e](const WaiterTask& t) { return t.Type == 1 && t.Target.id == e.Plate.id; }),
                m_TaskQueue.end());
            });

        BuildObstacleMap();
    }

    void OnDestroy() override
    {
        auto* scene = GetScene();
        if (scene) {
            auto& bus = scene->GetWorld().GetEventBus();
            if (m_CustomerSubId != 0) bus.Unsubscribe<CustomerSeatedEvent>(m_CustomerSubId);
            if (m_PlateSubId != 0) bus.Unsubscribe<PlateReadyEvent>(m_PlateSubId);
            if (m_OrderTakenSubId != 0) bus.Unsubscribe<OrderTakenEvent>(m_OrderTakenSubId);
            if (m_CustomerServedSubId != 0) bus.Unsubscribe<CustomerServedEvent>(m_CustomerServedSubId);
            if (m_PlateGrabbedSubId != 0) bus.Unsubscribe<PlateGrabbedEvent>(m_PlateGrabbedSubId);
        }
    }

    void CleanCustomersList() {
        m_AwaitingFoodCustomers.erase(std::remove_if(m_AwaitingFoodCustomers.begin(), m_AwaitingFoodCustomers.end(),
            [this](Entity c) { return !IsValidEntity(c); }), m_AwaitingFoodCustomers.end());
    }

    void PlayAnimation(const std::string& name)
    {
        auto* animComp = GetComponent<AnimatorComponent>();
        if (animComp && animComp->AnimatorInstance)
        {
            animComp->AnimatorInstance->PlayAnimation(name);
            animComp->IsPlaying = true;
        }
    }

    void OnUpdate(Timestep ts) override
    {

        if (GameManagerScript::s_IsTutorialMode) return;
        if (GameManagerScript::s_IsCutscenePlaying) return;
        switch (m_CurrentState)
        {
        case State::IDLE:
        {
            CheckForTasks();

            if (m_CurrentState == State::IDLE)
            {
                CleanCustomersList();
                if (!m_AwaitingFoodCustomers.empty())
                {
                    m_WaitAtPassTimer += ts.GetSeconds();
                    if (m_WaitAtPassTimer > 3.0f)
                    {
                        m_CurrentState = State::MOVING_TO_STATION_TO_WAVE;
                        m_HasWaypoint = false;
                        m_WaitAtPassTimer = 0.0f;
                    }
                    else { PlayAnimation("Idle"); }
                }
                else
                {
                    m_WaitAtPassTimer = 0.0f;
                    PlayAnimation("Idle");
                }
            }
            else { m_WaitAtPassTimer = 0.0f; }
            break;
        }

        case State::MOVING_TO_STATION_TO_WAVE:
            PlayAnimation("Walk");
            CheckForTasks();
            if (m_CurrentState != State::MOVING_TO_STATION_TO_WAVE) break;

            {
                Entity station = FindPickupStation();
                if (!IsValidEntity(station)) { ReturnToIdle(); break; }

                auto* statTc = GetScene()->GetWorld().GetComponent<TransformComponent>(station);
                glm::vec3 exactTarget = statTc->GetPosition() + m_WaveOffset;

                if (FlatDistanceToPosition(exactTarget) <= 0.1f) {
                    m_CurrentState = State::WAVING_AT_STATION;

                    auto* tc = GetComponent<TransformComponent>();
                    if (tc) {
                        glm::vec3 lookDir = statTc->GetPosition() - tc->GetPosition();
                        lookDir.y = 0.0f;

                        if (glm::length(lookDir) > 0.001f) {
                            lookDir = glm::normalize(lookDir);
                            float targetAngle = glm::degrees(std::atan2(lookDir.x, lookDir.z));
                            targetAngle += 270.0f;
                            tc->SetRotation({ 0.0f, targetAngle, 0.0f });
                        }
                    }
                }
                else {
                    MoveTowardsWaypoint(ts);
                }
            }
            break;

        case State::WAVING_AT_STATION:
            PlayAnimation("Wave");
            CheckForTasks();
            break;

        case State::MOVING_TO_TAKE_ORDER:
            PlayAnimation("Walk");
            if (!IsValidEntity(m_TargetCustomer)) { ReturnToIdle(); break; }

            if (FlatDistanceTo(m_TargetCustomer) <= m_InteractRange) {
                m_CurrentState = State::TAKING_ORDER;
                AudioEngine::Play("CookingStation/Assets/sounds/writing.mp3");
                m_TakeOrderTimer = 3.0f;
            }
            else {
                MoveTowardsWaypoint(ts);
            }
            break;

        case State::TAKING_ORDER:
            PlayAnimation("Order");
            m_TakeOrderTimer -= ts.GetSeconds();

            if (m_TakeOrderTimer <= 0.0f) {
                RevealCustomerOrder(m_TargetCustomer);
                m_TargetCustomer = { std::numeric_limits<std::size_t>::max(), 0 };

                m_CurrentState = State::IDLE;
                CheckForTasks();

                if (m_CurrentState == State::IDLE) {
                    m_CurrentState = State::RETURNING;
                    m_HasWaypoint = false;
                }
            }
            break;

        case State::MOVING_TO_FOOD:
            PlayAnimation("Walk");
            if (!IsValidEntity(m_TargetPlate) || !IsValidEntity(m_TargetCustomer)) { CancelDelivery(); break; }
            if (FlatDistanceTo(m_TargetPlate) <= m_InteractRange) GrabFood();
            else MoveTowardsWaypoint(ts);
            break;

        case State::MOVING_TO_CUSTOMER:
            PlayAnimation("Walk");
            if (!IsValidEntity(m_TargetCustomer)) { CancelDelivery(); break; }
            if (FlatDistanceTo(m_TargetCustomer) <= m_InteractRange) ServeCustomer();
            else MoveTowardsWaypoint(ts);
            break;

        case State::RETURNING:
            PlayAnimation("Walk");
            if (FlatDistanceToPosition(m_HomePosition) <= 0.1f) ReturnToIdle();
            else MoveTowardsWaypoint(ts);
            break;
        }
        UpdateCarriedPlatePosition();

        bool isWavingOrNoting = (m_CurrentState == State::WAVING_AT_STATION || m_CurrentState == State::TAKING_ORDER);

        if (isWavingOrNoting && !m_WasWavingState) {
            SwapModel(true);
        }
        else if (!isWavingOrNoting && m_WasWavingState) {
            SwapModel(false);
        }

        m_WasWavingState = isWavingOrNoting;
    }

protected:
    std::unordered_set<glm::ivec2, IVec2Hash> m_WalkableTiles;

    float FlatDistance(const glm::vec3& a, const glm::vec3& b)
    {
        return glm::length(glm::vec2(a.x, a.z) - glm::vec2(b.x, b.z));
    }

    float FlatDistanceTo(Entity target)
    {
        auto* myTc = GetComponent<TransformComponent>();
        auto* targetTc = GetScene()->GetWorld().GetComponent<TransformComponent>(target);
        if (myTc && targetTc) return FlatDistance(myTc->GetPosition(), targetTc->GetPosition());
        return 9999.0f;
    }

    float FlatDistanceToPosition(const glm::vec3& pos)
    {
        auto* myTc = GetComponent<TransformComponent>();
        if (myTc) return FlatDistance(myTc->GetPosition(), pos);
        return 9999.0f;
    }

    bool IsValidEntity(Entity e)
    {
        return e.id != std::numeric_limits<std::size_t>::max() && GetScene()->GetWorld().GetComponent<TransformComponent>(e) != nullptr;
    }

    void CancelDelivery()
    {
        if (IsValidEntity(m_TargetPlate)) GetScene()->GetWorld().DestroyEntity(m_TargetPlate);

        if (IsValidEntity(m_TargetCustomer)) {
            m_AwaitingFoodCustomers.insert(m_AwaitingFoodCustomers.begin(), m_TargetCustomer);
        }

        ReturnToIdle();
    }

    void ReturnToIdle()
    {
        m_TargetPlate = { std::numeric_limits<std::size_t>::max(), 0 };
        m_TargetCustomer = { std::numeric_limits<std::size_t>::max(), 0 };
        m_IsCarryingPlate = false;
        m_HasWaypoint = false;
        m_CurrentState = State::IDLE;
        m_WaitAtPassTimer = 0.0f;
    }

    Entity FindCustomerWaitingForFood()
    {
        auto it = m_AwaitingFoodCustomers.begin();
        while (it != m_AwaitingFoodCustomers.end()) {
            if (IsValidEntity(*it)) return *it;
            it = m_AwaitingFoodCustomers.erase(it);
        }
        return { std::numeric_limits<std::size_t>::max(), 0 };
    }


    void CheckForTasks()
    {
        if (m_TaskQueue.empty()) return;

        auto task = m_TaskQueue.front();

        if (task.Type == 0) 
        {
            if (!IsValidEntity(task.Target)) {
                m_TaskQueue.erase(m_TaskQueue.begin());
                CheckForTasks(); 
                return;
            }

            m_TargetCustomer = task.Target;
            m_HasWaypoint = false;
            m_CurrentState = State::MOVING_TO_TAKE_ORDER;
            m_TaskQueue.erase(m_TaskQueue.begin());
        }
        else if (task.Type == 1) 
        {
            bool plateValid = IsValidEntity(task.Target);
            if (plateValid) {
                auto* pTag = GetScene()->GetWorld().GetComponent<TagComponent>(task.Target);
                if (!pTag || (pTag->Tag != "PlateReady" && pTag->Tag != "PlateAssigned")) {
                    plateValid = false;
                }
            }

            if (!plateValid) {
                m_TaskQueue.erase(m_TaskQueue.begin());
                CheckForTasks();
                return;
            }

            Entity cust = FindCustomerWaitingForFood();
            if (IsValidEntity(cust)) {
                auto* pTag = GetScene()->GetWorld().GetComponent<TagComponent>(task.Target);
                if (pTag) pTag->Tag = "PlateAssigned";

                m_TargetPlate = task.Target;
                m_TargetCustomer = cust;
                m_HasWaypoint = false;
                m_CurrentState = State::MOVING_TO_FOOD;
                m_TaskQueue.erase(m_TaskQueue.begin());
            }
        }
    }

    void RevealCustomerOrder(Entity customer)
    {
        if (IsValidEntity(customer)) {
            GetScene()->GetWorld().GetEventBus().Publish(OrderTakenEvent{ customer });
        }
    }

    Entity FindPickupStation()
    {
        auto* nscArray = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
        if (!nscArray) return { std::numeric_limits<std::size_t>::max(), 0 };

        for (size_t i = 0; i < nscArray->dense.size(); ++i)
        {
            auto& nsc = nscArray->dense[i];
            for (auto& script : nsc.Scripts)
            {
                if (script.Name == "WaiterPickupStationScript") return nscArray->reverse[i];
            }
        }
        return { std::numeric_limits<std::size_t>::max(), 0 };
    }

    void GrabFood()
    {
        GetScene()->GetWorld().GetEventBus().Publish(PlateGrabbedEvent{ m_TargetPlate });

        auto* tag = GetScene()->GetWorld().GetComponent<TagComponent>(m_TargetPlate);
        if (tag) tag->Tag = "PlateCarried";

        auto* plateTc = GetScene()->GetWorld().GetComponent<TransformComponent>(m_TargetPlate);
        if (plateTc)
        {
            plateTc->SetScale(plateTc->GetScale() * 0.5f);
        }

        m_IsCarryingPlate = true;
        m_HasWaypoint = false;
        m_CurrentState = State::MOVING_TO_CUSTOMER;
    }

    void ServeCustomer()
    {
        m_IsCarryingPlate = false;
        m_HasWaypoint = false;

        Entity trueFoodChild = { std::numeric_limits<std::size_t>::max(), 0 };

        if (IsValidEntity(m_TargetPlate))
        {
            auto* rel = GetScene()->GetWorld().GetComponent<RelationshipComponent>(m_TargetPlate);
            if (rel && rel->FirstChild != std::numeric_limits<std::size_t>::max())
            {
                auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();

                if (tags) {
                    for (size_t i = 0; i < tags->dense.size(); ++i) {
                        if (tags->reverse[i].id == rel->FirstChild) {
                            trueFoodChild = tags->reverse[i];
                            break;
                        }
                    }
                }
            }
            GetScene()->GetWorld().DestroyEntity(m_TargetPlate);
        }

        m_TargetPlate = { std::numeric_limits<std::size_t>::max(), 0 };

        if (IsValidEntity(m_TargetCustomer)) {
            if (trueFoodChild.id == std::numeric_limits<std::size_t>::max()) {
                spdlog::warn("Kelner podal PUSTY talerz (Brak jedzenia jako dziecka talerza)!");
            }
            else {
                spdlog::info("Kelner podaje klientowi encje jedzenia: {}", trueFoodChild.id);
            }

            GetScene()->GetWorld().GetEventBus().Publish(CustomerServedEvent{ m_TargetCustomer, trueFoodChild });
        }

        m_TargetCustomer = { std::numeric_limits<std::size_t>::max(), 0 };

        m_CurrentState = State::IDLE;
        CheckForTasks();

        if (m_CurrentState == State::IDLE) {
            m_CurrentState = State::RETURNING;
            m_HasWaypoint = false;
        }
    }

    void UpdateCarriedPlatePosition()
    {
        if (m_IsCarryingPlate && IsValidEntity(m_TargetPlate))
        {
            auto* tc = GetComponent<TransformComponent>();
            auto* plateTc = GetScene()->GetWorld().GetComponent<TransformComponent>(m_TargetPlate);
            if (tc && plateTc)
            {
                float yaw = glm::radians(tc->GetRotation().y);

                float sinY = std::sin(yaw);
                float cosY = std::cos(yaw);

                glm::vec3 rotatedOffset;
                rotatedOffset.x = m_TrayOffset.x * cosY + m_TrayOffset.z * sinY;
                rotatedOffset.y = m_TrayOffset.y;
                rotatedOffset.z = -m_TrayOffset.x * sinY + m_TrayOffset.z * cosY;

                plateTc->SetPosition(tc->GetPosition() + rotatedOffset);

                plateTc->SetRotation(tc->GetRotation());
            }
        }
    }

    void MoveTowardsWaypoint(Timestep ts)
    {
        auto* tc = GetComponent<TransformComponent>();
        if (!tc) return;

        if (!m_HasWaypoint)
        {
            UpdateWaypoint();
            if (!m_HasWaypoint) return;
        }

        glm::vec3 currentPos = tc->GetPosition();
        glm::vec3 targetPos = m_NextWaypoint;
        targetPos.y = currentPos.y;

        glm::vec3 dir = targetPos - currentPos;
        float dist = glm::length(dir);
        float step = m_Speed * ts.GetSeconds();

        if (dist <= step + 0.05f)
        {
            tc->SetPosition(targetPos);
            m_HasWaypoint = false;
        }
        else
        {
            dir = glm::normalize(dir);
            tc->SetPosition(currentPos + dir * step);

            float targetAngle = glm::degrees(std::atan2(dir.x, dir.z));
            tc->SetRotation({ 0.0f, targetAngle, 0.0f });
        }
    }

    void UpdateWaypoint()
    {
        auto* tc = GetComponent<TransformComponent>();
        if (!tc) return;

        glm::ivec2 startCell = GridSystem::WorldToCell(tc->GetPosition());
        glm::ivec2 targetCell;
        bool isObstacle = false;
        glm::vec3 exactTargetPos;

        if (m_CurrentState == State::MOVING_TO_FOOD)
        {
            auto* plateTc = GetScene()->GetWorld().GetComponent<TransformComponent>(m_TargetPlate);
            if (!plateTc) return;
            exactTargetPos = plateTc->GetPosition();
            targetCell = GridSystem::WorldToCell(exactTargetPos);
            isObstacle = true;
        }
        else if (m_CurrentState == State::MOVING_TO_CUSTOMER || m_CurrentState == State::MOVING_TO_TAKE_ORDER)
        {
            auto* custTc = GetScene()->GetWorld().GetComponent<TransformComponent>(m_TargetCustomer);
            if (!custTc) return;
            exactTargetPos = custTc->GetPosition();
            targetCell = GridSystem::WorldToCell(exactTargetPos);
            isObstacle = true;
        }
        else if (m_CurrentState == State::MOVING_TO_STATION_TO_WAVE)
        {
            Entity station = FindPickupStation();
            if (!IsValidEntity(station)) return;
            auto* statTc = GetScene()->GetWorld().GetComponent<TransformComponent>(station);
            if (!statTc) return;
            exactTargetPos = statTc->GetPosition() + m_WaveOffset;
            targetCell = GridSystem::WorldToCell(exactTargetPos);
            isObstacle = false;
        }
        else if (m_CurrentState == State::RETURNING)
        {
            exactTargetPos = m_HomePosition;
            targetCell = GridSystem::WorldToCell(exactTargetPos);
        }
        else
        {
            return;
        }

        if (startCell == targetCell)
        {
            m_NextWaypoint = exactTargetPos;
            m_HasWaypoint = true;
            return;
        }

        std::vector<glm::vec3> path = CalculatePath(startCell, targetCell, isObstacle);

        if (!path.empty())
        {
            m_NextWaypoint = path.front();
            m_HasWaypoint = true;
        }
        else
        {
            m_NextWaypoint = exactTargetPos;
            m_HasWaypoint = true;
        }
    }

    std::vector<glm::vec3> CalculatePath(glm::ivec2 start, glm::ivec2 target, bool targetIsObstacle)
    {
        std::vector<glm::vec3> emptyPath;
        if (start == target) return emptyPath;

        std::priority_queue<AStarNode, std::vector<AStarNode>, CompareNode> openSet;
        std::unordered_map<glm::ivec2, glm::ivec2, IVec2Hash> cameFrom;
        std::unordered_map<glm::ivec2, float, IVec2Hash> gScore;

        openSet.push({ start, 0.0f, Heuristic(start, target), start });
        gScore[start] = 0.0f;

        const glm::ivec2 neighbors[4] = { {0, 1}, {1, 0}, {0, -1}, {-1, 0} };
        int maxIterations = 2000;
        int iterations = 0;

        while (!openSet.empty())
        {
            if (++iterations > maxIterations) break;

            AStarNode current = openSet.top();
            openSet.pop();

            if (current.pos == target)
            {
                auto path = ReconstructPath(cameFrom, current.pos, start);
                if (targetIsObstacle && !path.empty()) path.pop_back();
                return path;
            }

            for (const auto& offset : neighbors)
            {
                glm::ivec2 neighborPos = current.pos + offset;
                if (neighborPos != target && !IsWalkable(neighborPos)) continue;

                float tentativeG = gScore[current.pos] + 1.0f;

                if (gScore.find(neighborPos) == gScore.end() || tentativeG < gScore[neighborPos])
                {
                    gScore[neighborPos] = tentativeG;
                    float h = Heuristic(neighborPos, target);
                    cameFrom[neighborPos] = current.pos;
                    openSet.push({ neighborPos, tentativeG, h, current.pos });
                }
            }
        }
        return emptyPath;
    }

    std::vector<glm::vec3> ReconstructPath(std::unordered_map<glm::ivec2, glm::ivec2, IVec2Hash>& cameFrom, glm::ivec2 current, glm::ivec2 start)
    {
        std::vector<glm::vec3> path;
        while (current != start)
        {
            path.push_back(GridSystem::CellToWorld(current, 0.0f));
            current = cameFrom[current];
        }
        std::reverse(path.begin(), path.end());
        return path;
    }

    float Heuristic(glm::ivec2 a, glm::ivec2 b)
    {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y);
    }

    bool IsWalkable(glm::ivec2 cell)
    {
        bool hasFloor = m_WalkableTiles.find(cell) != m_WalkableTiles.end();
        bool isObstacle = m_StaticObstacles.find(cell) != m_StaticObstacles.end();

        return hasFloor && !isObstacle;
    }

    void SwapModel(bool usePageModel)
    {
        auto* meshComp = GetComponent<MeshComponent>();
        if (!meshComp) return;

        if (usePageModel && m_PageModel) {
            meshComp->ModelPtr = m_PageModel;
        }
        else if (!usePageModel && m_OriginalModel) {
            meshComp->ModelPtr = m_OriginalModel;
        }
    }

    void BuildObstacleMap()
    {
        m_StaticObstacles.clear();
        m_WalkableTiles.clear();

        auto& world = GetScene()->GetWorld();
        auto* transforms = world.GetComponentVector<TransformComponent>();
        auto* tags = world.GetComponentVector<TagComponent>();
        auto* rels = world.GetComponentVector<RelationshipComponent>();
        auto* colliders = world.GetComponentVector<BoxColliderComponent>();

        if (!transforms || !tags)
        {
            return;
        }

        for (size_t i = 0; i < transforms->dense.size(); ++i)
        {
            Entity e = transforms->reverse[i];
            auto* tagComp = tags->Get(e);

            if (tagComp)
            {
                std::string t = tagComp->Tag;

                if (t.find("Podloga") != std::string::npos ||
                    t.find("podloga") != std::string::npos ||
                    t.find("chmura") != std::string::npos ||
                    t.find("Bridge") != std::string::npos)
                {
                    glm::vec3 globalPos = transforms->dense[i].GetPosition();
                    glm::vec3 globalScale = transforms->dense[i].GetScale();
                    glm::vec3 globalRot = transforms->dense[i].GetRotation();

                    glm::vec3 colSize = globalScale;
                    glm::vec3 colOffset = glm::vec3(0.0f);

                    if (colliders && colliders->Get(e) != nullptr) {
                        auto* boxColl = colliders->Get(e);
                        colSize = boxColl->Size * globalScale;
                        colOffset = boxColl->Offset * globalScale;
                    }

                    float angleY = std::abs(globalRot.y);
                    float remainder = std::fmod(angleY, 180.0f);

                    if (remainder > 45.0f && remainder < 135.0f) {
                        std::swap(colSize.x, colSize.z);
                        std::swap(colOffset.x, colOffset.z);
                        colOffset.x = -colOffset.x;
                    }

                    glm::vec3 center = globalPos + colOffset;
                    glm::vec3 halfSize = colSize;

                    glm::vec3 minBound = center - halfSize;
                    glm::vec3 maxBound = center + halfSize;

                    glm::ivec2 minCell = GridSystem::WorldToCell(minBound);
                    glm::ivec2 maxCell = GridSystem::WorldToCell(maxBound);

                    for (int x = minCell.x; x <= maxCell.x; ++x)
                    {
                        for (int z = minCell.y; z <= maxCell.y; ++z)
                        {
                            m_WalkableTiles.insert(glm::ivec2(x, z));
                        }
                    }
                }

                if (t.find("Table") != std::string::npos ||
                    t.find("Tasma") != std::string::npos ||
                    t.find("tasma") != std::string::npos ||
                    t.find("Chair") != std::string::npos ||
                    t.find("krzeslo") != std::string::npos ||
                    t.find("Krzeslo") != std::string::npos ||
                    t.find("wydawka") != std::string::npos ||
                    t.find("Wydawka") != std::string::npos ||
                    t.find("naroznik") != std::string::npos ||
                    t.find("PlateSpawner") != std::string::npos ||
                    t.find("Garnek") != std::string::npos)
                {
                    glm::vec3 globalPos = transforms->dense[i].GetPosition();

                    if (rels)
                    {
                        auto* relComp = rels->Get(e);
                        if (relComp && relComp->Parent != std::numeric_limits<std::size_t>::max())
                        {
                            for (size_t j = 0; j < transforms->dense.size(); ++j)
                            {
                                if (transforms->reverse[j].id == relComp->Parent)
                                {
                                    globalPos += transforms->dense[j].GetPosition();
                                    break;
                                }
                            }
                        }
                    }

                    glm::ivec2 entityCell = GridSystem::WorldToCell(globalPos);

                    if (t.find("wydawka") != std::string::npos || t.find("Wydawka") != std::string::npos)
                    {
                        m_StaticObstacles.insert(entityCell);
                        m_StaticObstacles.insert(entityCell + glm::ivec2(1, 0));
                        m_StaticObstacles.insert(entityCell + glm::ivec2(-1, 0));
                    }
                    else
                    {
                        m_StaticObstacles.insert(entityCell);
                    }
                }
            }
        }
        ExportGridToFile();
    }

    void ExportGridToFile()
    {
#ifndef CS_DISTRIBUTION
        if (m_WalkableTiles.empty() && m_StaticObstacles.empty()) return;

    int minX = 9999, maxX = -9999;
    int minZ = 9999, maxZ = -9999;

    for (const auto& tile : m_WalkableTiles) {
        minX = std::min(minX, tile.x);
        maxX = std::max(maxX, tile.x);
        minZ = std::min(minZ, tile.y);
        maxZ = std::max(maxZ, tile.y);
    }
    for (const auto& tile : m_StaticObstacles) {
        minX = std::min(minX, tile.x);
        maxX = std::max(maxX, tile.x);
        minZ = std::min(minZ, tile.y);
        maxZ = std::max(maxZ, tile.y);
    }

    minX -= 2; maxX += 2;
    minZ -= 2; maxZ += 2;

    std::ofstream file("MapaNawigacji.txt");
    if (!file.is_open()) {
        spdlog::error("Nie mozna utworzyc pliku MapaNawigacji.txt!");
        return;
    }

    file << "LEGENDA:\n";
    file << "@ = Podloga / Most (Chodliwe)\n";
    file << "X = Przeszkoda (Zablokowane)\n";
    file << "- = Przepasc (Pustka)\n\n";

    file << "OS Z (od " << minZ << " do " << maxZ << ")\n";
    file << "------------------------------------------\n";

    for (int z = minZ; z <= maxZ; ++z) {
        if (z >= 0 && z < 10) file << " ";
        file << "Z:" << z << "\t| ";

        for (int x = minX; x <= maxX; ++x) {
            glm::ivec2 cell(x, z);

            if (m_StaticObstacles.find(cell) != m_StaticObstacles.end()) {
                file << "X ";
            }
            else if (m_WalkableTiles.find(cell) != m_WalkableTiles.end()) {
                file << "@ ";
            }
            else {
                file << "- ";
            }
        }
        file << "\n";
    }

    file.close();
    spdlog::info("Zapisano wizualizacje mapy do 'MapaNawigacji.txt'!");
#endif
    }

};