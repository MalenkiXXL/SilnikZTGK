#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include <string>
#include <vector>

enum class TutorialState {
    Start,
    WaiterIntro,
    CameraResetting,
    WaitForCrateSpawn,   // NOWOŒÆ: Przerwa po talerzach, przed skrzynk¹
    WaitForCrateClick,   // NOWOŒÆ: Czekamy a¿ gracz weŸmie pomidora!
    WaitForBoardSpawn,        // Czekamy na pojawienie siê deski po ma³ym cooldownie
    WaitForIngredientOnBoard, // Œwiecimy desk¹ i czekamy na po³o¿enie pomidora
    WaitForChopping,          // Czekamy a¿ gracz pokroi pomidora (wymagane 3 klikniêcia)
    WaitForPlateTransfer,
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
    Entity m_Floor;         // NOWOŒÆ: Nasza pod³oga do centrowania kamery
    Entity m_Pot;
    Entity m_Burner;
    Entity m_TomatoCrate;
    Entity m_Board;
    Entity m_BoardStand;
    Entity m_PlateSpawner;
    Entity m_Waiter;
    Entity m_Poof;

    glm::vec3 m_CrateOriginalPos;
    glm::vec3 m_BoardOriginalPos;      // NOWOŒÆ: Zapisana oryginalna pozycja deski
    glm::vec3 m_BoardStandOriginalPos;

    Entity FindEntityByName(const std::string& name);
    void HideUnderground(Entity e);
    void RestorePosition(Entity e, glm::vec3 originalPos);
};