#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include <string>
#include <vector>

enum class TutorialState {
    Start,
    WaiterIntro,
    CameraResetting,
    WaitForCrateSpawn,   // NOWOå∆: Przerwa po talerzach, przed skrzynkπ
    WaitForCrateClick,   // NOWOå∆: Czekamy aø gracz weümie pomidora!
    WaitForPotPlacement,
    BurnedSaladDialog,
    WaitForCooking,
    WaitForServing,
    Outro
};

struct TutorialDialogLine {
    std::string Speaker;
    glm::vec4 SpeakerColor;
    std::string Text;
    bool IsBottom;
};

class TutorialManagerScript : public ScriptableEntity {
public:
    void OnCreate() override;
    void OnUpdate(Timestep ts) override;

private:
    TutorialState m_State = TutorialState::Start;
    float m_StateTimer = 0.0f;

    int m_DialogIndex = 0;
    float m_TypewriterTimer = 0.0f;
    std::vector<TutorialDialogLine> m_Dialogues;

    // Encje
    Entity m_Floor;         // NOWOå∆: Nasza pod≥oga do centrowania kamery
    Entity m_Pot;
    Entity m_Burner;
    Entity m_TomatoCrate;
    Entity m_Board;
    Entity m_BoardStand;
    Entity m_PlateSpawner;
    Entity m_Waiter;
    Entity m_Poof;

    glm::vec3 m_CrateOriginalPos;

    Entity FindEntityByName(const std::string& name);
    void HideUnderground(Entity e);
    void RestorePosition(Entity e, glm::vec3 originalPos);
};