#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include "CookingStation/Assets/Flags/FlagScript.h"
#include "CookingStation/Scripts/Managers/IngredientType.h"
#include "CookingStation/Events/GameEvents.h"
#include <string>
#include <unordered_map>
#include <utility>
#include "CookingStation/Tools/QuestGenerator/QuestManager.h"

// finaly
// #include <thread>
// #include <atomic>

enum class QuestEventState {
    WaitingForTimer,    // odliczanie 3 minut
    IslandArriving,     // wyspa i stoisko przylatują 
    WaitingForAccept,   // stoisko stoi i zaakceptuje/pominie
    QuestActive,        // zaakceptowano - dobudowujemy taśmy i budkę
    IslandLeaving       // zrealizowano questa - wszystko odlatuje
};

class GameManagerScript : public ScriptableEntity
{
public:
    inline static GameManagerScript* s_Instance = nullptr;

    float m_MoneyWarningTimer = 0.0f;
    void TriggerMoneyWarning() { m_MoneyWarningTimer = 0.6f; }

    void OnCreate() override;
    void OnDestroy() override;
    void OnUpdate(Timestep ts) override;

    void AddIngredients(IngredientType type, int amount);
    void UseIngredient(IngredientType type, int amount);
    int GetIngredientCount(IngredientType type);

    int GetMoney();
    bool AddMoney(int amount);
    bool SpendMoney(int amount);

    void AcceptQuest();
    void SkipQuest();
    void CompleteQuest();
    void SpawnCollectibleFlag(const std::string& countryCode);

    QuestEventState GetQuestState() const { return m_CurrentQuestState; }
    int GetSkipsLeft() const { return m_SkipsLeft; }
    QuestData* GetCurrentQuest() {
        if (m_AvailableQuests.empty() || m_CurrentQuestIndex >= m_AvailableQuests.size()) return nullptr;
        return &m_AvailableQuests[m_CurrentQuestIndex];
    }

    int GetQuestProgress() const { return m_CurrentQuestProgress; }
    void DeliverQuestPortion();

private:
    void OnOrderFulfilled(const OrderFulfilledEvent& e);

    int money = 0;
    std::unordered_map<IngredientType, int> m_Inventory;

    std::unordered_map<std::size_t, DishHistory> m_DishMemory;

    std::size_t m_IngredientUsedSubId = 0;
    std::size_t m_AddIngredientSubId = 0;
    std::size_t m_OrderFulfilledSubId = 0;
    std::size_t m_DishCreatedSubId = 0;
    std::size_t m_ValidateOrderSubId = 0;

    //questy
    QuestEventState m_CurrentQuestState = QuestEventState::WaitingForTimer;
    float m_QuestTimer = 0.0f;
    const float QUEST_INTERVAL = 1.0f;
    std::vector<QuestData> m_AvailableQuests;
    int m_CurrentQuestIndex = 0;
    int m_SkipsLeft = 3;

    float m_AnimationProgress = 0.0f;
    std::vector<std::pair<Entity, float>> m_EventIslandGroup;
    std::vector<std::pair<Entity, float>> m_MainIslandQuestGroup;
    std::vector<std::pair<Entity, float>> m_ReplacedByQuestGroup;

    int m_CurrentQuestProgress = 0;

    int m_CollectedFlagsCount = 0;


    // finaly
    // std::atomic<bool> m_NewQuestsReady{false};
    // bool m_IsGeneratingQuests = false;
};