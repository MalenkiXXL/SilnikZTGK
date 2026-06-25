#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include <string>
#include <vector>
#include "CookingStation/Scripts/PlateScript.h"

enum class TutorialState {
    Start,
    WaiterIntro,
    CameraResetting,
    WaitForCrateSpawn,   
    WaitForCrateClick,   
    WaitForBoardSpawn,       
    WaitForIngredientOnBoard, 
    WaitForChopping,          
    WaitForPlateTransfer,
    BurnedSaladDialog,
    WaitForPotPlacement, 
    WaitForIngredientInPot,   
	WaitForCooking,
    WaitForDelivery,
    WaitForEnd,
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
    static bool s_AllowConveyorSwitch;
    static bool s_AllowSkip;

private:
    TutorialState m_State = TutorialState::Start;
    float m_StateTimer = 0.0f;

    int m_DialogIndex = 0;
    float m_TypewriterTimer = 0.0f;
    std::vector<TutorialDialogLine> m_Dialogues;
    bool m_DialogAudioPlayed = false;
    Entity m_Floor;        
    Entity m_Pot;
    Entity m_Burner;
    Entity m_TomatoCrate;
    Entity m_Board;
    Entity m_BoardStand;
    Entity m_PlateSpawner;
    Entity m_Waiter;
    Entity m_Poof;

    glm::vec3 m_CrateOriginalPos;
    glm::vec3 m_BoardOriginalPos;     
    glm::vec3 m_BoardStandOriginalPos;
    glm::vec3 m_PotOriginalPos;
    glm::vec3 m_BurnerOriginalPos;

    bool m_WalkAnimPlayed = false;

    Entity FindEntityByName(const std::string& name);
    void HideUnderground(Entity e);
    void RestorePosition(Entity e, glm::vec3 originalPos);

    void UpdateWaitingUI(float dt);
    void SetHighlight(Entity target, glm::vec3 color, float duration = 0.1f, bool isInfinite = false);
    glm::vec3 GetRaycastedMousePos(float targetY);
    float UpdateLerp(bool condition, float currentLerp, float dt, float speed = 8.0f);
    void PlayPoofAt(glm::vec3 pos);
    Entity FindClosestPlate(glm::vec3 targetPos, PlateScript** outScript = nullptr, bool mustBeEmpty = false);
    bool IsHovering(Entity e, glm::vec3 mousePos, float radius = 1.5f);

    void ResetTutorial();
  
};