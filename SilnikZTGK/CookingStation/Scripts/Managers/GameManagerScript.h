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
    WaitingForTimer,   
    IslandArriving,    
    WaitingForAccept,  
    QuestActive,       
    IslandLeaving     
};

class GameManagerScript : public ScriptableEntity
{
public:
    float m_MapExpandProgress = 0.0f;
    bool m_IsMapExpanding = false;
    static inline bool s_IsTutorialMode = false;
    static inline bool s_IsCutscenePlaying = false;
    static inline bool s_MapExpanded = false;
    static inline bool s_ShowTutorialDialog = false;
    static inline std::string s_TutorialSpeaker = "";
    static inline glm::vec4 s_TutorialSpeakerColor = glm::vec4(1.0f);
    static inline std::string s_TutorialText = "";
    static inline int s_TutorialCharsRevealed = 0;
    static inline bool s_TutorialDialogIsBottom = false;
    static inline float s_TutorialIconAlpha = 0.0f;
    static inline Entity s_TutorialTrackedEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    static inline glm::vec3 s_TutorialTrackedOffset = glm::vec3(0.0f, 0.0f, 0.0f);
    static inline bool s_GrandmaServed = false;

    static inline bool s_SpeedUpUIHeld = false;

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
    std::vector<IngredientType> GetDishHistory(std::size_t entityId) const
    {
        auto it = m_DishMemory.find(entityId);
        if (it != m_DishMemory.end())
            return it->second.BaseIngredients;
        return {};
    }


private:
    void OnOrderFulfilled(const OrderFulfilledEvent& e);
    void UnlockNewMapArea();

    int money = 0;
    std::unordered_map<IngredientType, int> m_Inventory;

    std::unordered_map<std::size_t, DishHistory> m_DishMemory;

    std::size_t m_IngredientUsedSubId = 0;
    std::size_t m_AddIngredientSubId = 0;
    std::size_t m_OrderFulfilledSubId = 0;
    std::size_t m_DishCreatedSubId = 0;
    std::size_t m_ValidateOrderSubId = 0;
    int m_CustomersServed = 0;
    int m_TotalMoneyEarned = 0;
    const int MAX_CUSTOMERS = 22;


    //questy
    QuestEventState m_CurrentQuestState = QuestEventState::WaitingForTimer;
    float m_QuestTimer = 0.0f;
    const float QUEST_INTERVAL = 180.0f;
    std::vector<QuestData> m_AvailableQuests;
    int m_CurrentQuestIndex = 0;
    int m_SkipsLeft = 3;

    float m_AnimationProgress = 0.0f;
    std::vector<std::pair<Entity, float>> m_EventIslandGroup;
    std::vector<std::pair<Entity, float>> m_MainIslandQuestGroup;
    std::vector<std::pair<Entity, float>> m_ReplacedByQuestGroup;

    int m_CurrentQuestProgress = 0;

    int m_CollectedFlagsCount = 0;
    std::size_t m_GrandmaSatisfiedSubId = 0;

    std::vector<std::pair<Entity, glm::vec3>> m_NewMapEntities;
    Entity m_SmallFloor = { std::numeric_limits<std::size_t>::max(), 0 };
    Entity m_BigFloor = { std::numeric_limits<std::size_t>::max(), 0 };
    // finaly
    // std::atomic<bool> m_NewQuestsReady{false};
    // bool m_IsGeneratingQuests = false;
};