#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Core/GridSystem.h"
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Core/Input.h"
#include <spdlog/spdlog.h>
#include <vector>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <limits>

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

    // Struktura zadań FIFO
    struct WaiterTask {
        int Type; // 0 = ZBIERZ_ZAMOWIENIE, 1 = PODAJ_TALERZ
        Entity Target;
    };
    std::vector<WaiterTask> m_TaskQueue;

    // Lista klientów z zapisanym zamówieniem, którzy czekają fizycznie na talerz.
    // DZIĘKI TEMU NIE SKANUJEMY JUŻ CAŁEJ SCENY I NIE UŻYWAMY DYNAMIC_CAST!
    std::vector<Entity> m_AwaitingFoodCustomers;

    std::size_t m_CustomerSubId = 0;
    std::size_t m_PlateSubId = 0;
    std::size_t m_OrderTakenSubId = 0;
    std::size_t m_CustomerServedSubId = 0;

    void OnCreate() override
    {
        auto* tc = GetComponent<TransformComponent>();
        if (tc) m_HomePosition = tc->GetPosition();

        auto& bus = GetScene()->GetWorld().GetEventBus();

        m_CustomerSubId = bus.Subscribe<CustomerSeatedEvent>([this](const CustomerSeatedEvent& e) {
            m_TaskQueue.push_back({ 0, e.Customer });
            });

        m_PlateSubId = bus.Subscribe<PlateReadyEvent>([this](const PlateReadyEvent& e) {
            m_TaskQueue.push_back({ 1, e.Plate });
            });

        // Gdy spiszemy zamówienie (albo inny kelner z AI to zrobi), dodajemy klienta do listy oczekujących na jedzenie
        m_OrderTakenSubId = bus.Subscribe<OrderTakenEvent>([this](const OrderTakenEvent& e) {
            m_AwaitingFoodCustomers.push_back(e.Customer);

            // Jeżeli ten klient nadal widniał u nas w kolejce jako zadanie nr 0, usuwamy to zadanie!
            m_TaskQueue.erase(std::remove_if(m_TaskQueue.begin(), m_TaskQueue.end(),
                [&e](const WaiterTask& t) { return t.Type == 0 && t.Target.id == e.Customer.id; }),
                m_TaskQueue.end());
            });

        // Gdy klient zostanie obsłużony (przez nas, albo kelnera AI), ściągamy go z naszej prywatnej listy
        m_CustomerServedSubId = bus.Subscribe<CustomerServedEvent>([this](const CustomerServedEvent& e) {
            m_AwaitingFoodCustomers.erase(std::remove_if(m_AwaitingFoodCustomers.begin(), m_AwaitingFoodCustomers.end(),
                [&e](Entity cust) { return cust.id == e.Customer.id; }),
                m_AwaitingFoodCustomers.end());
            });
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
        }
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
        switch (m_CurrentState)
        {
        case State::IDLE:
        {
            CheckForTasks();

            if (m_CurrentState == State::IDLE)
            {
                bool waitingForFood = IsValidEntity(FindCustomerWaitingForFood());

                if (waitingForFood)
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
            CheckForTasks(); // Może coś wskoczyło podczas marszu!
            if (m_CurrentState != State::MOVING_TO_STATION_TO_WAVE) break;

            {
                Entity station = FindPickupStation();
                if (!IsValidEntity(station)) { ReturnToIdle(); break; }

                if (FlatDistanceTo(station) <= m_InteractRange) {
                    m_CurrentState = State::WAVING_AT_STATION;
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
                // Notowanie zakończone - wysyłamy EVENT w świat!
                RevealCustomerOrder(m_TargetCustomer);
                m_TargetCustomer = { std::numeric_limits<std::size_t>::max(), 0 };

                // Sprawdzamy czy jest coś do zrobienia bez wracania do bazy!
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
    }

private:
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

    // Nowa, ekstremalnie wydajna funkcja. Patrzy tylko do wewn. listy!
    Entity FindCustomerWaitingForFood()
    {
        auto it = m_AwaitingFoodCustomers.begin();
        while (it != m_AwaitingFoodCustomers.end()) {
            if (IsValidEntity(*it)) return *it;
            it = m_AwaitingFoodCustomers.erase(it); // Usuń jeśli klient zniknął ze sceny
        }
        return { std::numeric_limits<std::size_t>::max(), 0 };
    }

    void CheckForTasks()
    {
        auto it = m_TaskQueue.begin();
        while (it != m_TaskQueue.end())
        {
            auto task = *it;
            bool invalid = false;

            if (task.Type == 0) // ZBIERZ ZAMÓWIENIE
            {
                if (!IsValidEntity(task.Target)) {
                    invalid = true;
                }
                else {
                    m_TargetCustomer = task.Target;
                    m_HasWaypoint = false;
                    m_CurrentState = State::MOVING_TO_TAKE_ORDER;
                    m_TaskQueue.erase(it);
                    return;
                }
            }
            else if (task.Type == 1) // WYDAJ TALERZ
            {
                auto* tag = GetScene()->GetWorld().GetComponent<TagComponent>(task.Target);
                if (!tag || tag->Tag != "PlateReady") {
                    invalid = true;
                }
                else {
                    Entity cust = FindCustomerWaitingForFood();
                    if (IsValidEntity(cust)) {
                        m_TargetPlate = task.Target;
                        m_TargetCustomer = cust;
                        m_HasWaypoint = false;
                        m_CurrentState = State::MOVING_TO_FOOD;
                        m_TaskQueue.erase(it);
                        return;
                    }
                    else {
                        // Talerz leży, ALE NIKT JESZCZE NIE ZŁOŻYŁ ZAMÓWIENIA! 
                        ++it;
                        continue;
                    }
                }
            }

            if (invalid) {
                it = m_TaskQueue.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void RevealCustomerOrder(Entity customer)
    {
        if (IsValidEntity(customer)) {
            // ZAMIAST RZUTOWAĆ NA CustomerScript -> UŻYWAMY EVENTBUSA!
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
        // EVENT BUS: Informujemy jedzenie, by samo się odpięło! Zamiast grzebać w ItemScript!
        GetScene()->GetWorld().GetEventBus().Publish(PlateGrabbedEvent{ m_TargetPlate });

        auto* tag = GetScene()->GetWorld().GetComponent<TagComponent>(m_TargetPlate);
        if (tag) tag->Tag = "PlateCarried";

        m_IsCarryingPlate = true;
        m_HasWaypoint = false;
        m_CurrentState = State::MOVING_TO_CUSTOMER;
    }

    void ServeCustomer()
    {
        m_IsCarryingPlate = false;
        m_HasWaypoint = false;
        bool isCorrect = false;

        if (IsValidEntity(m_TargetPlate))
        {
            auto* rel = GetScene()->GetWorld().GetComponent<RelationshipComponent>(m_TargetPlate);
            if (rel && rel->FirstChild != std::numeric_limits<std::size_t>::max())
            {
                isCorrect = true;
                Entity foodChild = { rel->FirstChild, 0 };
                GetScene()->GetWorld().DestroyEntity(foodChild);
            }
            GetScene()->GetWorld().DestroyEntity(m_TargetPlate);
        }

        m_TargetPlate = { std::numeric_limits<std::size_t>::max(), 0 };

        // EVENT BUS: Informujemy klienta, co dostał, żeby to on zarządzał swoimi pieniędzmi i oceną!
        if (IsValidEntity(m_TargetCustomer)) {
            GetScene()->GetWorld().GetEventBus().Publish(CustomerServedEvent{ m_TargetCustomer, isCorrect });
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
                plateTc->SetPosition(tc->GetPosition() + glm::vec3(0.0f, 1.5f, 0.0f));
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
            exactTargetPos = statTc->GetPosition();
            targetCell = GridSystem::WorldToCell(exactTargetPos);
            isObstacle = true;
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
        auto* transforms = GetScene()->GetWorld().GetComponentVector<TransformComponent>();
        auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();

        if (!transforms || !tags) return true;

        for (size_t i = 0; i < transforms->dense.size(); ++i)
        {
            glm::ivec2 entityCell = GridSystem::WorldToCell(transforms->dense[i].GetPosition());

            if (entityCell == cell)
            {
                Entity e = transforms->reverse[i];
                auto* tagComp = tags->Get(e);
                if (tagComp)
                {
                    std::string t = tagComp->Tag;
                    if (t.find("Table") != std::string::npos ||
                        t.find("Tasma") != std::string::npos ||
                        t.find("tasma") != std::string::npos ||
                        t.find("Chair") != std::string::npos ||
                        t.find("krzeslo") != std::string::npos ||
                        t.find("Krzeslo") != std::string::npos ||
                        t.find("budka") != std::string::npos ||
                        t.find("naroznik") != std::string::npos ||
                        t.find("PlateSpawner") != std::string::npos ||
                        t.find("Garnek") != std::string::npos)
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    }
};