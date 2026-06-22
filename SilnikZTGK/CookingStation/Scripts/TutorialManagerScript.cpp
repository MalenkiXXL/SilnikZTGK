#include "TutorialManagerScript.h"
#include "CookingStation/Scripts/Managers/GameManagerScript.h"
#include "CookingStation/Scripts/Waiter/WaiterScript.h"
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Core/Input.h" 
#include "CookingStation/Scripts/PoofEmitterScript.h"
#include "CookingStation/Scripts/ParticleEmitterScript.h"
#include "CookingStation/Scripts/CrateScript.h" 
#include <algorithm>
#include <limits> 
#include "CookingStation/Scripts/Machines/CuttingBoardScript.h"
#include "CookingStation/Scripts/Machines/PotScript.h"
#include "CookingStation/Scripts/PlateScript.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Scene/SceneManager.h"
#include "CookingStation/Scene/SceneSerializer.h"
#include "CookingStation/Core/Application.h"
#include "CookingStation/Layers/GuiLayer/Utils/Gui.h"

bool TutorialManagerScript::s_AllowConveyorSwitch = false;
bool TutorialManagerScript::s_AllowSkip = false;

extern bool g_TriggerCloudTransition;

static glm::vec3 s_WaiterOriginalSpawnPos = glm::vec3(0.0f);

Entity TutorialManagerScript::FindEntityByName(const std::string& name) {
    auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
    if (tags) {
        for (size_t i = 0; i < tags->dense.size(); ++i) {
            if (tags->dense[i].Tag == name) return tags->reverse[i];
        }
    }
    return { std::numeric_limits<std::size_t>::max(), 0 };
}

void TutorialManagerScript::HideUnderground(Entity e) {
    if (e.id == std::numeric_limits<std::size_t>::max()) return;
    auto* tc = GetScene()->GetWorld().GetComponent<TransformComponent>(e);
    if (tc) {
        glm::vec3 pos = tc->GetPosition();
        pos.y = -999.0f;
        tc->SetPosition(pos);
    }
}

void TutorialManagerScript::RestorePosition(Entity e, glm::vec3 originalPos) {
    if (e.id == std::numeric_limits<std::size_t>::max()) return;
    auto* tc = GetScene()->GetWorld().GetComponent<TransformComponent>(e);
    if (tc) tc->SetPosition(originalPos);
}

void TutorialManagerScript::UpdateWaitingUI(float dt) {
    m_TypewriterTimer += dt;
    if (m_TypewriterTimer < 5.5f) {
        GameManagerScript::s_ShowTutorialDialog = true;
        GameManagerScript::s_TutorialSpeaker = "";
        GameManagerScript::s_TutorialDialogIsBottom = true;
        GameManagerScript::s_TutorialIconAlpha = 0.0f;

        int stage = (int)(m_TypewriterTimer / 0.8f);
        std::string offset = "                        ";
        if (stage == 0 || stage == 3) GameManagerScript::s_TutorialText = offset + ".";
        else if (stage == 1 || stage == 4) GameManagerScript::s_TutorialText = offset + ". .";
        else if (stage == 2 || stage == 5) GameManagerScript::s_TutorialText = offset + ". . .";
        else GameManagerScript::s_TutorialText = "";

        GameManagerScript::s_TutorialCharsRevealed = GameManagerScript::s_TutorialText.length();
    }
    else {
        GameManagerScript::s_ShowTutorialDialog = false;
    }
}

void TutorialManagerScript::SetHighlight(Entity target, glm::vec3 color, float duration, bool isInfinite) {
    if (target.id != std::numeric_limits<std::size_t>::max()) {
        TriggerHighlightEvent ev;
        ev.TargetEntity = target;
        ev.Color = color;
        ev.Duration = duration;
        ev.IsInfinite = isInfinite;
        GetScene()->GetWorld().GetEventBus().Publish(ev);
    }
}

glm::vec3 TutorialManagerScript::GetRaycastedMousePos(float targetY) {
    glm::vec3 floorMousePos = GetMouseWorldPosition();
    auto* camera = GetScene()->GetCamera();
    if (camera && std::abs(camera->Front.y) > 0.001f) {
        float t = (targetY - floorMousePos.y) / camera->Front.y;
        return floorMousePos + camera->Front * t;
    }
    return floorMousePos;
}

float TutorialManagerScript::UpdateLerp(bool condition, float currentLerp, float dt, float speed) {
    currentLerp += (condition ? speed : -speed) * dt;
    return std::clamp(currentLerp, 0.0f, 1.0f);
}

void TutorialManagerScript::PlayPoofAt(glm::vec3 pos) {
    RestorePosition(m_Poof, pos);
    auto* poofNsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Poof);
    if (poofNsc) {
        for (auto& s : poofNsc->Scripts) {
            if (s.Name == "PoofEmitterScript" && s.Instance) {
                static_cast<ParticleEmitterScript*>(s.Instance)->Play();
                break;
            }
        }
    }
}

Entity TutorialManagerScript::FindClosestPlate(glm::vec3 targetPos, PlateScript** outScript, bool mustBeEmpty) {
    Entity closestPlate = { std::numeric_limits<std::size_t>::max(), 0 };
    float closestDist = 999.0f;

    auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
    auto* nscVector = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();

    if (!tags || !nscVector) return closestPlate;

    for (size_t i = 0; i < tags->dense.size(); ++i) {
        const std::string& tag = tags->dense[i].Tag;
        if (tag.find("Plate") != std::string::npos || tag.find("Talerz") != std::string::npos) {
            Entity p = tags->reverse[i];
            auto* pTf = GetScene()->GetWorld().GetComponent<TransformComponent>(p);
            if (pTf) {
                float d = glm::distance(pTf->GetPosition(), targetPos);
                if (d < closestDist) {
                    PlateScript* pScript = nullptr;
                    auto* plateNsc = nscVector->Get(p);
                    if (plateNsc) {
                        for (auto& s : plateNsc->Scripts) {
                            if (s.Name == "PlateScript" && s.Instance) {
                                pScript = static_cast<PlateScript*>(s.Instance);
                                break;
                            }
                        }
                    }

                    if (mustBeEmpty && pScript) {
                        if (!pScript->m_Ingredients.empty() || pScript->m_CompletedDish != IngredientType::None) {
                            continue; 
                        }
                    }

                    closestDist = d;
                    closestPlate = p;
                    if (outScript) *outScript = pScript;
                }
            }
        }
    }
    return closestPlate;
}

bool TutorialManagerScript::IsHovering(Entity e, glm::vec3 mousePos, float radius) {
    if (Input::IsUICapturingMouse() || e.id == std::numeric_limits<std::size_t>::max()) return false;
    auto* tf = GetScene()->GetWorld().GetComponent<TransformComponent>(e);
    if (!tf) return false;
    return glm::distance(glm::vec2(mousePos.x, mousePos.z), glm::vec2(tf->GetPosition().x, tf->GetPosition().z)) < radius;
}

void TutorialManagerScript::OnCreate() {
    GameManagerScript::s_IsTutorialMode = true;
    TutorialManagerScript::s_AllowConveyorSwitch = false;
    GetScene()->GetWorld().GetEventBus().Publish(IngredientUsedEvent{ IngredientType::Tomato, 5 });

    glm::vec4 walterColor = glm::vec4(0.75f, 0.4f, 0.9f, 1.0f);
    glm::vec4 playerColor = glm::vec4(1.0f, 0.4f, 0.8f, 1.0f);

    m_Dialogues = {
        { "Walter:", walterColor, "Our restaurant has been closed for so long, my cap is drooping just thinking about a good meal... But now you're here!", false },
        { "You:", playerColor, "You must be dreaming... I got fired yesterday for burning a salad.", true },
        { "Walter:", walterColor, "Maybe I'm not the one dreaming, huh? Anyway, nothing burns here. You better prepare me a tomato soup.", false }
    };

    m_Floor = FindEntityByName("podloga");
    m_Pot = FindEntityByName("TutorialPot");
    m_Burner = FindEntityByName("Palnik_24");
    m_TomatoCrate = FindEntityByName("SkrzynkaPomidor");
    m_Board = FindEntityByName("TutorialCuttingBoard");
    m_BoardStand = FindEntityByName("TutorialBoardStand");
    m_PlateSpawner = FindEntityByName("PlateSpawner_62_3");
    m_Waiter = FindEntityByName("Pan Grzybek_1");
    m_Poof = FindEntityByName("TutorialPoof");

    Entity upgradedWaiter = FindEntityByName("Pan Grzybek_Kelner");
    if (upgradedWaiter.id != std::numeric_limits<std::size_t>::max()) {
        HideUnderground(upgradedWaiter);
    }

    auto* crateTc = GetScene()->GetWorld().GetComponent<TransformComponent>(m_TomatoCrate);
    if (crateTc) m_CrateOriginalPos = crateTc->GetPosition();

    auto* boardTc = GetScene()->GetWorld().GetComponent<TransformComponent>(m_Board);
    if (boardTc) m_BoardOriginalPos = boardTc->GetPosition();

    auto* standTc = GetScene()->GetWorld().GetComponent<TransformComponent>(m_BoardStand);
    if (standTc) m_BoardStandOriginalPos = standTc->GetPosition();

    auto* potTc = GetScene()->GetWorld().GetComponent<TransformComponent>(m_Pot);
    if (potTc) m_PotOriginalPos = potTc->GetPosition();

    auto* burnerTc = GetScene()->GetWorld().GetComponent<TransformComponent>(m_Burner);
    if (burnerTc) m_BurnerOriginalPos = burnerTc->GetPosition();

    auto* waiterStartTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_Waiter);
    if (waiterStartTf) s_WaiterOriginalSpawnPos = waiterStartTf->GetPosition();

    HideUnderground(m_Pot);
    HideUnderground(m_Burner);
    HideUnderground(m_TomatoCrate);
    HideUnderground(m_Board);
    HideUnderground(m_BoardStand);
    HideUnderground(m_PlateSpawner);
    HideUnderground(m_Poof);

    m_State = TutorialState::WaiterIntro;
    m_StateTimer = 0.0f;
}

void TutorialManagerScript::OnUpdate(Timestep ts) {
    m_StateTimer += ts.GetSeconds();

    bool isEarlyState = (m_State == TutorialState::WaiterIntro && m_DialogIndex > 0) ||
        m_State == TutorialState::CameraResetting ||
        m_State == TutorialState::WaitForCrateSpawn ||
        m_State == TutorialState::WaitForCrateClick;

    if (isEarlyState) {
        s_AllowSkip = true; 
    }
    else {
        s_AllowSkip = false; 
    }

    glm::vec3 baseGray = glm::vec3(0.4f, 0.4f, 0.4f);
    glm::vec3 basePink = glm::vec3(1.0f, 0.2f, 0.6f);
    glm::vec3 hoverGold = glm::vec3(1.0f, 0.9f, 0.0f);

    switch (m_State) {

    case TutorialState::WaiterIntro: {
        auto* camera = GetScene()->GetCamera();
        auto* waiterTc = GetScene()->GetWorld().GetComponent<TransformComponent>(m_Waiter);

        if (camera && waiterTc) {
            camera->TargetPosition = waiterTc->GetPosition() + glm::vec3(0.0f, 0.8f, 0.0f);
            waiterTc->SetRotation(glm::vec3(0.0f, 45.0f, 0.0f));
            camera->Zoom += (8.0f - camera->Zoom) * 3.0f * ts.GetSeconds();
        }

        if (m_DialogIndex < m_Dialogues.size()) {
            GameManagerScript::s_ShowTutorialDialog = true;
            auto& line = m_Dialogues[m_DialogIndex];

            GameManagerScript::s_TutorialSpeaker = line.Speaker;
            GameManagerScript::s_TutorialSpeakerColor = line.SpeakerColor;
            GameManagerScript::s_TutorialText = line.Text;
            GameManagerScript::s_TutorialDialogIsBottom = line.IsBottom;

            int fullLength = line.Text.length();
            m_TypewriterTimer += ts.GetSeconds();

            if (m_TypewriterTimer > 0.05f) {
                m_TypewriterTimer = 0.0f;
                if (GameManagerScript::s_TutorialCharsRevealed < fullLength) {
                    GameManagerScript::s_TutorialCharsRevealed++;
                }
            }

            if (GameManagerScript::s_TutorialCharsRevealed >= fullLength) {
                GameManagerScript::s_TutorialIconAlpha = std::min(1.0f, GameManagerScript::s_TutorialIconAlpha + (float)ts.GetSeconds() * 3.0f);
            }
            else {
                GameManagerScript::s_TutorialIconAlpha = 0.0f;
            }

            if (Input::IsMouseButtonJustPressed(0)) {
                if (GameManagerScript::s_TutorialCharsRevealed < fullLength) {
                    GameManagerScript::s_TutorialCharsRevealed = fullLength;
                }
                else {
                    m_DialogIndex++;
                    GameManagerScript::s_TutorialCharsRevealed = 0;
                    GameManagerScript::s_TutorialIconAlpha = 0.0f;
                    m_TypewriterTimer = 0.0f;
                }
            }
        }
        else {
            GameManagerScript::s_ShowTutorialDialog = false;
            if (camera) {
                glm::vec3 floorCenter = glm::vec3(0.0f, 0.0f, 0.0f);
                if (m_Floor.id != std::numeric_limits<std::size_t>::max()) {
                    auto* floorTc = GetScene()->GetWorld().GetComponent<TransformComponent>(m_Floor);
                    if (floorTc) floorCenter = floorTc->GetPosition();
                }
                camera->TargetPosition = floorCenter + glm::vec3(0.0f, 2.0f, 0.0f);
            }

            m_State = TutorialState::CameraResetting;
            m_StateTimer = 0.0f;
            m_DialogIndex = 0;
        }
        break;
    }

    case TutorialState::CameraResetting: {
        auto* camera = GetScene()->GetCamera();
        if (camera) camera->Zoom += (32.0f - camera->Zoom) * 4.0f * ts.GetSeconds();

        if (m_StateTimer > 1.5f && m_DialogIndex == 0) {
            PlayPoofAt(glm::vec3(-7.0f, 2.2f, 1.0f));
            m_DialogIndex = 1;
        }

        if (m_StateTimer > 1.9f && m_DialogIndex == 1) {
            RestorePosition(m_PlateSpawner, glm::vec3(-7.0f, 1.2f, 1.0f));
            m_DialogIndex = 2;
        }

        if (m_StateTimer > 2.4f && m_DialogIndex == 2) {
            HideUnderground(m_Poof);
            m_State = TutorialState::WaitForCrateSpawn;
            m_StateTimer = 0.0f;
            m_DialogIndex = 0;
        }
        break;
    }

    case TutorialState::WaitForCrateSpawn: {
        if (m_StateTimer > 2.5f && m_DialogIndex == 0) {
            PlayPoofAt(m_CrateOriginalPos + glm::vec3(0.0f, 1.0f, 0.0f));
            m_DialogIndex = 1;
        }

        if (m_StateTimer > 2.8f && m_DialogIndex == 1) {
            RestorePosition(m_TomatoCrate, m_CrateOriginalPos);

            if (GameManagerScript::s_Instance) {
                int currentTomatoes = GameManagerScript::s_Instance->GetIngredientCount(IngredientType::Tomato);
                if (currentTomatoes > 1) GetScene()->GetWorld().GetEventBus().Publish(IngredientUsedEvent{ IngredientType::Tomato, currentTomatoes - 1 });
                else if (currentTomatoes == 0) GetScene()->GetWorld().GetEventBus().Publish(AddIngredientEvent{ IngredientType::Tomato, 1 });
            }

            SetHighlight(m_TomatoCrate, basePink, 0.1f, true);
            m_DialogIndex = 2;
        }

        if (m_StateTimer > 2.8f && m_DialogIndex == 2) {
            auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_TomatoCrate);
            if (nsc) {
                for (auto& s : nsc->Scripts) {
                    if (s.Name == "CrateScript" && s.Instance) {
                        auto* crateScript = static_cast<CrateScript*>(s.Instance);
                        if (crateScript->m_VisualFood.id != std::numeric_limits<std::size_t>::max()) {
                            RestorePosition(crateScript->m_VisualFood, m_CrateOriginalPos + glm::vec3(0.0f, 0.4f, 0.0f));
                            SetHighlight(crateScript->m_VisualFood, basePink, 8.0f, true);
                            m_DialogIndex = 3;
                        }
                    }
                }
            }
        }

        if (m_StateTimer > 3.3f && m_DialogIndex == 3) {
            HideUnderground(m_Poof);
            m_State = TutorialState::WaitForCrateClick;
            m_StateTimer = 0.0f;
            m_DialogIndex = 0;
        }
        break;
    }

    case TutorialState::WaitForCrateClick: {
        static float crateHoverLerp = 0.0f;
        if (m_StateTimer < 0.05f) crateHoverLerp = 0.0f;

        auto* crateTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_TomatoCrate);
        glm::vec3 preciseMousePos = GetRaycastedMousePos(crateTf ? crateTf->GetPosition().y : 0.0f);

        bool isHoveringCrate = IsHovering(m_TomatoCrate, preciseMousePos, 1.5f);
        crateHoverLerp = UpdateLerp(isHoveringCrate, crateHoverLerp, ts.GetSeconds());

        glm::vec3 currentCrateColor = glm::mix(basePink, hoverGold, crateHoverLerp);

        auto* crateNsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_TomatoCrate);
        if (crateNsc) {
            for (auto& s : crateNsc->Scripts) {
                if (s.Name == "CrateScript" && s.Instance) {
                    SetHighlight(static_cast<CrateScript*>(s.Instance)->m_VisualFood, currentCrateColor, 8.0f, true);
                }
            }
        }

        bool tomatoFound = false;
        auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
        if (tags) {
            for (const auto& tagComp : tags->dense) {
                if (tagComp.Tag.find("BeltItem") != std::string::npos) {
                    tomatoFound = true;
                    break;
                }
            }
        }

        if (tomatoFound) {
            SetHighlight(m_TomatoCrate, glm::vec3(0.0f), 0.1f, false);
            if (crateNsc) {
                for (auto& s : crateNsc->Scripts) {
                    if (s.Name == "CrateScript" && s.Instance) {
                        SetHighlight(static_cast<CrateScript*>(s.Instance)->m_VisualFood, glm::vec3(0.0f), 0.1f, false);
                    }
                }
            }

            m_State = TutorialState::WaitForBoardSpawn;
            m_StateTimer = 0.0f;
            m_DialogIndex = 0;
        }
        break;
    }

    case TutorialState::WaitForBoardSpawn: {
        if (m_StateTimer > 1.5f && m_DialogIndex == 0) {
            PlayPoofAt(m_BoardOriginalPos + glm::vec3(0.0f, 1.0f, 0.0f));
            m_DialogIndex = 1;
        }

        if (m_StateTimer > 1.8f && m_DialogIndex == 1) {
            RestorePosition(m_BoardStand, m_BoardStandOriginalPos);
            RestorePosition(m_Board, m_BoardOriginalPos);
            HideUnderground(m_Poof);
            m_State = TutorialState::WaitForIngredientOnBoard;
            m_StateTimer = 0.0f;
        }
        break;
    }

    case TutorialState::WaitForIngredientOnBoard: {
        static float tomatoHoverLerp = 0.0f;
        static float boardHoverLerp = 0.0f;
        static float boardTransitionLerp = 0.0f;

        if (m_StateTimer < 0.05f) {
            m_TypewriterTimer = 0.0f;
            tomatoHoverLerp = boardHoverLerp = boardTransitionLerp = 0.0f;
        }

        Entity tomatoBeltItem = { std::numeric_limits<std::size_t>::max(), 0 };
        auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
        if (tags) {
            for (size_t i = 0; i < tags->dense.size(); ++i) {
                if (tags->dense[i].Tag.find("BeltItem") != std::string::npos) {
                    tomatoBeltItem = tags->reverse[i];
                    break;
                }
            }
        }

        auto* boardTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_Board);
        auto* tomatoTf = GetScene()->GetWorld().GetComponent<TransformComponent>(tomatoBeltItem);

        bool inRange = false;
        if (tomatoTf && boardTf) {
            if (glm::distance(glm::vec2(boardTf->GetPosition().x, boardTf->GetPosition().z),
                glm::vec2(tomatoTf->GetPosition().x, tomatoTf->GetPosition().z)) < 3.5f) {
                inRange = true;
            }
        }

        glm::vec3 preciseMousePos = GetRaycastedMousePos(boardTf ? boardTf->GetPosition().y : 0.0f);
        bool isHoveringTomato = IsHovering(tomatoBeltItem, preciseMousePos, 1.5f);
        bool isHoveringBoard = IsHovering(m_Board, preciseMousePos, 1.5f);

        tomatoHoverLerp = UpdateLerp(isHoveringTomato && inRange, tomatoHoverLerp, ts.GetSeconds());
        boardHoverLerp = UpdateLerp(isHoveringBoard && inRange, boardHoverLerp, ts.GetSeconds());
        boardTransitionLerp = UpdateLerp(inRange, boardTransitionLerp, ts.GetSeconds(), 3.0f);

        glm::vec3 currentTomatoColor = glm::mix(basePink, hoverGold, tomatoHoverLerp);
        glm::vec3 currentBoardColor = glm::mix(baseGray, glm::mix(basePink, hoverGold, boardHoverLerp), boardTransitionLerp);

        SetHighlight(m_Board, currentBoardColor, glm::mix(32.0f, 8.0f, boardTransitionLerp), true);
        if (inRange) SetHighlight(tomatoBeltItem, currentTomatoColor, 8.0f, true);

        UpdateWaitingUI(ts.GetSeconds());

        bool isActionPressed = Input::IsMouseButtonJustPressed(0) || (Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0));
        if (isActionPressed && inRange && (isHoveringTomato || isHoveringBoard)) {
            auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Board);
            if (nsc) {
                for (auto& s : nsc->Scripts) {
                    if (s.Name == "CuttingBoardScript" && s.Instance) {
                        auto* boardScript = static_cast<CuttingBoardScript*>(s.Instance);
                        if (boardScript->m_Ingredients.empty()) {
                            boardScript->AddIngredient(IngredientType::Tomato);
                            GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ tomatoBeltItem });
                        }
                    }
                }
            }
        }

        bool hasIngredient = false;
        auto* nscBoard = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Board);
        if (nscBoard) {
            for (auto& s : nscBoard->Scripts) {
                if (s.Name == "CuttingBoardScript" && s.Instance) {
                    if (!static_cast<CuttingBoardScript*>(s.Instance)->m_Ingredients.empty()) hasIngredient = true;
                }
            }
        }

        if (hasIngredient) {
            SetHighlight(m_Board, glm::vec3(0.0f), 0.1f, false);
            GameManagerScript::s_ShowTutorialDialog = false;
            m_State = TutorialState::WaitForChopping;
            m_StateTimer = 0.0f;
        }
        break;
    }

    case TutorialState::WaitForChopping: {
        if (m_StateTimer < 0.05f) GameManagerScript::s_TutorialCharsRevealed = 0;

        int currentChops = 0;
        bool isReady = false;

        auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Board);
        if (nsc) {
            for (auto& s : nsc->Scripts) {
                if (s.Name == "CuttingBoardScript" && s.Instance) {
                    auto* boardScript = static_cast<CuttingBoardScript*>(s.Instance);
                    currentChops = boardScript->m_ChopCount;
                    isReady = boardScript->m_IsReady;
                }
            }
        }

        float wave = (std::sin(m_StateTimer * 4.0f) + 1.0f) * 0.5f;
        SetHighlight(m_Board, glm::vec3(0.61f, 0.44f, 0.8f) * wave, 0.1f, false);

        GameManagerScript::s_ShowTutorialDialog = true;
        GameManagerScript::s_TutorialSpeaker = "";
        GameManagerScript::s_TutorialTrackedEntity = m_Board;
        GameManagerScript::s_TutorialTrackedOffset = glm::vec3(0.0f, 3.0f, 0.0f);
        GameManagerScript::s_TutorialIconAlpha = std::clamp(m_StateTimer * 3.0f, 0.0f, 1.0f);

        std::string text = std::to_string(currentChops) + " / 3";
        GameManagerScript::s_TutorialText = text;
        GameManagerScript::s_TutorialCharsRevealed = text.length();

        if (isReady) {
            SetHighlight(m_Board, glm::vec3(0.0f), 0.1f, false);
            GameManagerScript::s_ShowTutorialDialog = false;
            GameManagerScript::s_TutorialIconAlpha = 0.0f;
            GameManagerScript::s_TutorialTrackedEntity = { std::numeric_limits<std::size_t>::max(), 0 };

            m_State = TutorialState::WaitForPlateTransfer;
            m_StateTimer = 0.0f;
        }
        break;
    }

    case TutorialState::WaitForPlateTransfer: {
        static float foodHoverLerp = 0.0f;
        static float plateHoverLerp = 0.0f;
        static float transitionLerp = 0.0f;

        if (m_StateTimer < 0.05f) {
            m_TypewriterTimer = 0.0f;
            foodHoverLerp = plateHoverLerp = transitionLerp = 0.0f;
        }

        Entity foodOnBoard = FindEntityByName("Na_Desce");
        auto* boardTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_Board);

        Entity closestPlate = FindClosestPlate(boardTf ? boardTf->GetPosition() : glm::vec3(0.0f));
        auto* plateTf = closestPlate.id != std::numeric_limits<std::size_t>::max() ? GetScene()->GetWorld().GetComponent<TransformComponent>(closestPlate) : nullptr;

        bool inRange = (plateTf && boardTf && glm::distance(glm::vec2(boardTf->GetPosition().x, boardTf->GetPosition().z), glm::vec2(plateTf->GetPosition().x, plateTf->GetPosition().z)) < 3.5f);

        glm::vec3 preciseMousePos = GetRaycastedMousePos(boardTf ? boardTf->GetPosition().y : 0.0f);

        bool isHoveringFood = IsHovering(m_Board, preciseMousePos, 1.5f);
        bool isHoveringPlate = IsHovering(closestPlate, preciseMousePos, 1.5f);

        foodHoverLerp = UpdateLerp(isHoveringFood && inRange, foodHoverLerp, ts.GetSeconds());
        plateHoverLerp = UpdateLerp(isHoveringPlate && inRange, plateHoverLerp, ts.GetSeconds());
        transitionLerp = UpdateLerp(inRange, transitionLerp, ts.GetSeconds(), 3.0f);

        glm::vec3 activeFoodColor = glm::mix(basePink, hoverGold, foodHoverLerp);
        glm::vec3 activePlateColor = glm::mix(basePink, hoverGold, plateHoverLerp);
        glm::vec3 foodColor = glm::mix(baseGray, activeFoodColor, transitionLerp);

        SetHighlight(foodOnBoard, foodColor, glm::mix(32.0f, 8.0f, transitionLerp), true);
        if (inRange) SetHighlight(closestPlate, activePlateColor, 8.0f, true);

        UpdateWaitingUI(ts.GetSeconds());

        bool isActionPressed = Input::IsMouseButtonJustPressed(0) || (Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0));
        if (isActionPressed && inRange && (isHoveringFood || isHoveringPlate) && !Input::IsUICapturingMouse()) {
            auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Board);
            if (nsc) {
                for (auto& s : nsc->Scripts) {
                    if (s.Name == "CuttingBoardScript" && s.Instance) {
                        auto* boardScript = static_cast<CuttingBoardScript*>(s.Instance);
                        if (isHoveringFood) boardScript->HandleClick();
                        else if (isHoveringPlate) boardScript->TryTransferToPlate();
                    }
                }
            }
        }

        bool transferred = false;
        auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Board);
        if (nsc) {
            for (auto& s : nsc->Scripts) {
                if (s.Name == "CuttingBoardScript" && s.Instance) {
                    auto* boardScript = static_cast<CuttingBoardScript*>(s.Instance);
                    if (boardScript->m_Ingredients.empty() && !boardScript->m_IsReady) transferred = true;
                }
            }
        }

        if (transferred) {
            SetHighlight(closestPlate, glm::vec3(0.1f, 1.0f, 0.2f), 1.5f, false);
            GameManagerScript::s_ShowTutorialDialog = false;
            m_State = TutorialState::WaitForPotPlacement;
            m_StateTimer = 0.0f;
            m_DialogIndex = 0;
        }
        break;
    }

    case TutorialState::WaitForPotPlacement: {
        if (m_StateTimer > 5.0f && m_DialogIndex == 0) {
            PlayPoofAt(m_PotOriginalPos + glm::vec3(0.0f, 1.0f, 0.0f));
            m_DialogIndex = 1;
        }

        if (m_StateTimer > 5.3f && m_DialogIndex == 1) {
            RestorePosition(m_Burner, m_BurnerOriginalPos);
            RestorePosition(m_Pot, m_PotOriginalPos);
            HideUnderground(m_Poof);
            m_State = TutorialState::WaitForIngredientInPot;
            m_StateTimer = 0.0f;
        }
        break;
    }

    case TutorialState::WaitForIngredientInPot: {
        static float potHoverLerp = 0.0f;
        static float plateHoverLerp = 0.0f;
        static float transitionLerp = 0.0f;

        if (m_StateTimer < 0.05f) {
            m_TypewriterTimer = 0.0f;
            potHoverLerp = plateHoverLerp = transitionLerp = 0.0f;
        }

        auto* potTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_Pot);
        PlateScript* closestPlateScript = nullptr;
        Entity closestPlate = FindClosestPlate(potTf ? potTf->GetPosition() : glm::vec3(0.0f), &closestPlateScript);

        Entity foodOnPlate = (closestPlateScript && !closestPlateScript->m_VisualModels.empty()) ? closestPlateScript->m_VisualModels.back() : Entity{ std::numeric_limits<std::size_t>::max(), 0 };
        auto* plateTf = closestPlate.id != std::numeric_limits<std::size_t>::max() ? GetScene()->GetWorld().GetComponent<TransformComponent>(closestPlate) : nullptr;

        bool inRange = (plateTf && potTf && glm::distance(glm::vec2(potTf->GetPosition().x, potTf->GetPosition().z), glm::vec2(plateTf->GetPosition().x, plateTf->GetPosition().z)) < 3.5f);

        glm::vec3 preciseMousePos = GetRaycastedMousePos(potTf ? potTf->GetPosition().y : 0.0f);
        bool isHoveringPot = IsHovering(m_Pot, preciseMousePos, 1.5f);
        bool isHoveringPlate = IsHovering(closestPlate, preciseMousePos, 1.5f);

        potHoverLerp = UpdateLerp(isHoveringPot && inRange, potHoverLerp, ts.GetSeconds());
        plateHoverLerp = UpdateLerp(isHoveringPlate && inRange, plateHoverLerp, ts.GetSeconds());
        transitionLerp = UpdateLerp(inRange, transitionLerp, ts.GetSeconds(), 3.0f);

        glm::vec3 activePotColor = glm::mix(basePink, hoverGold, potHoverLerp);
        glm::vec3 activePlateColor = glm::mix(basePink, hoverGold, plateHoverLerp);
        glm::vec3 potColor = glm::mix(baseGray, activePotColor, transitionLerp);

        SetHighlight(m_Pot, potColor, glm::mix(32.0f, 8.0f, transitionLerp), true);

        if (inRange && plateTf) {
            SetHighlight(closestPlate, activePlateColor, 8.0f, true);
            if (foodOnPlate.id != std::numeric_limits<std::size_t>::max()) {
                SetHighlight(foodOnPlate, activePlateColor, 8.0f, true);
            }
        }

        bool isActionPressed = Input::IsMouseButtonJustPressed(0) || (Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0));
        if (isActionPressed && inRange && (isHoveringPot || isHoveringPlate) && !Input::IsUICapturingMouse()) {
            auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Pot);
            if (nsc && closestPlateScript && !closestPlateScript->m_Ingredients.empty()) {
                for (auto& s : nsc->Scripts) {
                    if (s.Name == "PotScript" && s.Instance) {
                        auto* potScript = static_cast<PotScript*>(s.Instance);
                        IngredientType topIngredient = closestPlateScript->m_Ingredients.back();
                        if (potScript->AddIngredient(topIngredient)) {
                            closestPlateScript->m_Ingredients.pop_back();
                            if (!closestPlateScript->m_VisualModels.empty()) {
                                Entity visualToRemove = closestPlateScript->m_VisualModels.back();
                                SetHighlight(visualToRemove, glm::vec3(0.0f), 0.1f, false);
                                GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ visualToRemove });
                                closestPlateScript->m_VisualModels.pop_back();
                            }
                        }
                    }
                }
            }
        }

        UpdateWaitingUI(ts.GetSeconds());

        bool hasIngredient = false;
        auto* nscPot = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Pot);
        if (nscPot) {
            for (auto& s : nscPot->Scripts) {
                if (s.Name == "PotScript" && s.Instance) {
                    if (!static_cast<PotScript*>(s.Instance)->m_Ingredients.empty()) hasIngredient = true;
                }
            }
        }

        if (hasIngredient) {
            SetHighlight(m_Pot, glm::vec3(0.0f), 0.1f, false);
            if (closestPlate.id != std::numeric_limits<std::size_t>::max()) SetHighlight(closestPlate, glm::vec3(0.0f), 0.1f, false);

            GameManagerScript::s_ShowTutorialDialog = false;
            m_State = TutorialState::WaitForCooking;
            m_StateTimer = 0.0f;
        }
        break;
    }

    case TutorialState::WaitForCooking: {
        static float potHoverLerp = 0.0f;
        static float plateHoverLerp = 0.0f;
        static float transitionLerp = 0.0f;

        if (m_StateTimer < 0.05f) {
            m_TypewriterTimer = 0.0f;
            potHoverLerp = plateHoverLerp = transitionLerp = 0.0f;
        }

        auto* potTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_Pot);
        auto* nscPot = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Pot);
        PotScript* potScript = nullptr;
        if (nscPot) {
            for (auto& s : nscPot->Scripts) {
                if (s.Name == "PotScript" && s.Instance) potScript = static_cast<PotScript*>(s.Instance);
            }
        }

        if (!potScript) break;

        if (!potScript->m_IsReady && potScript->m_Ingredients.empty()) {
            Entity successPlate = FindClosestPlate(potTf ? potTf->GetPosition() : glm::vec3(0.0f));

            auto* pTf = successPlate.id != std::numeric_limits<std::size_t>::max() ? GetScene()->GetWorld().GetComponent<TransformComponent>(successPlate) : nullptr;
            if (pTf && potTf && glm::distance(pTf->GetPosition(), potTf->GetPosition()) < 3.5f) {
                SetHighlight(successPlate, glm::vec3(0.1f, 1.0f, 0.2f), 1.5f, false);
            }

            GameManagerScript::s_ShowTutorialDialog = false;
            m_State = TutorialState::WaitForDelivery;
            m_StateTimer = 0.0f;
            break;
        }

        if (!potScript->m_IsReady) {
            GameManagerScript::s_ShowTutorialDialog = false;
            break;
        }

        Entity spawnedSoup = potScript->m_SpawnedFood;
        bool isSoupValid = false;
        auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();

        if (spawnedSoup.id != std::numeric_limits<std::size_t>::max() && tags) {
            for (size_t i = 0; i < tags->dense.size(); ++i) {
                if (tags->reverse[i].id == spawnedSoup.id) {
                    isSoupValid = true;
                    break;
                }
            }
        }

        Entity closestPlate = FindClosestPlate(potTf ? potTf->GetPosition() : glm::vec3(0.0f), nullptr, true);
        auto* plateTf = closestPlate.id != std::numeric_limits<std::size_t>::max() ? GetScene()->GetWorld().GetComponent<TransformComponent>(closestPlate) : nullptr;
        bool inRange = (plateTf && potTf && glm::distance(glm::vec2(potTf->GetPosition().x, potTf->GetPosition().z), glm::vec2(plateTf->GetPosition().x, plateTf->GetPosition().z)) < 3.5f);

        auto* soupTf = isSoupValid ? GetScene()->GetWorld().GetComponent<TransformComponent>(spawnedSoup) : nullptr;
        glm::vec3 preciseMousePos = GetRaycastedMousePos(soupTf ? soupTf->GetPosition().y : (potTf ? potTf->GetPosition().y + 0.5f : 0.0f));

        bool isHoveringSoup = IsHovering(spawnedSoup, preciseMousePos, 1.8f) || IsHovering(m_Pot, preciseMousePos, 1.8f);
        bool isHoveringPlate = IsHovering(closestPlate, preciseMousePos, 1.8f);

        potHoverLerp = UpdateLerp(isHoveringSoup && inRange, potHoverLerp, ts.GetSeconds());
        plateHoverLerp = UpdateLerp(isHoveringPlate && inRange, plateHoverLerp, ts.GetSeconds());
        transitionLerp = UpdateLerp(inRange, transitionLerp, ts.GetSeconds(), 3.0f);

        glm::vec3 activePotColor = glm::mix(basePink, hoverGold, potHoverLerp);
        glm::vec3 activePlateColor = glm::mix(basePink, hoverGold, plateHoverLerp);
        glm::vec3 soupColor = glm::mix(baseGray, activePotColor, transitionLerp);

        if (isSoupValid) SetHighlight(spawnedSoup, soupColor, glm::mix(32.0f, 8.0f, transitionLerp), true);
        if (inRange) SetHighlight(closestPlate, activePlateColor, 8.0f, true);

        bool isActionPressed = Input::IsMouseButtonJustPressed(0) || (Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0));
        if (isActionPressed && inRange && !Input::IsUICapturingMouse()) {
            if (isHoveringPlate && !isHoveringSoup) potScript->TryTransferToPlate();
        }

        UpdateWaitingUI(ts.GetSeconds());
        break;
    }
    case TutorialState::WaitForDelivery: {
        static float switchHoverLerp = 0.0f;
        static float transitionLerp = 0.0f;
        static float initialRotY = -999.0f; 

        if (m_StateTimer < 0.05f) {
            m_TypewriterTimer = 0.0f;
            switchHoverLerp = 0.0f;
            transitionLerp = 0.0f;
            TutorialManagerScript::s_AllowConveyorSwitch = true;
        }

        Entity switchEntity = { std::numeric_limits<std::size_t>::max(), 0 };
        auto* nscVector = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();

        if (nscVector) {
            for (size_t i = 0; i < nscVector->dense.size(); ++i) {
                for (auto& s : nscVector->dense[i].Scripts) {
                    if (s.Name == "ConveyorSwitchScript") {
                        switchEntity = nscVector->reverse[i];
                        break;
                    }
                }
                if (switchEntity.id != std::numeric_limits<std::size_t>::max()) break;
            }
        }

        auto* switchTf = switchEntity.id != std::numeric_limits<std::size_t>::max() ? GetScene()->GetWorld().GetComponent<TransformComponent>(switchEntity) : nullptr;

        if (!switchTf) break;

        if (m_StateTimer < 0.05f) {
            initialRotY = switchTf->GetRotation().y;
        }

        glm::vec3 preciseMousePos = GetRaycastedMousePos(switchTf->GetPosition().y);
        bool isHoveringSwitch = IsHovering(switchEntity, preciseMousePos, 1.5f);

        switchHoverLerp = UpdateLerp(isHoveringSwitch, switchHoverLerp, ts.GetSeconds());
        transitionLerp = UpdateLerp(true, transitionLerp, ts.GetSeconds(), 3.0f); // Pulsowanie w��cza si� od razu

        glm::vec3 baseGray = glm::vec3(0.4f, 0.4f, 0.4f);
        glm::vec3 basePink = glm::vec3(1.0f, 0.2f, 0.6f);
        glm::vec3 hoverGold = glm::vec3(1.0f, 0.9f, 0.0f);

        glm::vec3 activeSwitchColor = glm::mix(basePink, hoverGold, switchHoverLerp);
        glm::vec3 switchColor = glm::mix(baseGray, activeSwitchColor, transitionLerp);

        SetHighlight(switchEntity, switchColor, glm::mix(32.0f, 8.0f, transitionLerp), true);

        UpdateWaitingUI(ts.GetSeconds());

        if (std::abs(switchTf->GetRotation().y - initialRotY) > 1.0f) {
            SetHighlight(switchEntity, glm::vec3(0.1f, 1.0f, 0.2f), 1.5f, false);
            GameManagerScript::s_ShowTutorialDialog = false;

            m_State = TutorialState::WaitForEnd;
            m_StateTimer = 0.0f;

            m_WalkAnimPlayed = false;
        }
        break;
    }
    case TutorialState::WaitForEnd: {
        static int endPhase = 0;
        static glm::vec3 stationPos = glm::vec3(0.0f);
        static Entity targetPlate = { std::numeric_limits<std::size_t>::max(), 0 };
        static Entity upgradedWaiter = { std::numeric_limits<std::size_t>::max(), 0 };

        auto* waiterTf = m_Waiter.id != std::numeric_limits<std::size_t>::max() ? GetScene()->GetWorld().GetComponent<TransformComponent>(m_Waiter) : nullptr;
        if (!waiterTf) break;

        if (m_StateTimer <= ts.GetSeconds() && endPhase == 0) {
            endPhase = 0;
            m_TypewriterTimer = 0.0f;
            GameManagerScript::s_TutorialCharsRevealed = 0;
            m_WalkAnimPlayed = false;

            upgradedWaiter = FindEntityByName("Pan Grzybek_Kelner");
        }

        // --- FAZA 0: Czekamy aż zupa przyjedzie na wydawkę ---
        if (endPhase == 0) {
            Entity stationEntity = { std::numeric_limits<std::size_t>::max(), 0 };
            auto* nscVector = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();

            if (nscVector) {
                for (size_t i = 0; i < nscVector->dense.size(); ++i) {
                    for (auto& s : nscVector->dense[i].Scripts) {
                        if (s.Name == "WaiterPickupStationScript") {
                            stationEntity = nscVector->reverse[i];
                            break;
                        }
                    }
                    if (stationEntity.id != std::numeric_limits<std::size_t>::max()) break;
                }
            }

            auto* stationTf = stationEntity.id != std::numeric_limits<std::size_t>::max() ? GetScene()->GetWorld().GetComponent<TransformComponent>(stationEntity) : nullptr;
            if (stationTf) {
                stationPos = stationTf->GetPosition();
                targetPlate = FindClosestPlate(stationPos, nullptr, false);

                if (targetPlate.id != std::numeric_limits<std::size_t>::max()) {
                    auto* plateTf = GetScene()->GetWorld().GetComponent<TransformComponent>(targetPlate);
                    if (plateTf && glm::distance(plateTf->GetPosition(), stationPos) < 1.5f) {
                        endPhase = 1;
                    }
                }
            }
        }
        // --- FAZA 1: Kelner idzie w stronę wydawki (zatrzymuje się DUŻO wcześniej) ---
        else if (endPhase == 1) {
            if (!m_WalkAnimPlayed) {
                auto* animComp = GetScene()->GetWorld().GetComponent<AnimatorComponent>(m_Waiter);
                if (animComp && animComp->AnimatorInstance) animComp->AnimatorInstance->PlayAnimation("Walk");
                m_WalkAnimPlayed = true;
            }

            glm::vec3 currentPos = waiterTf->GetPosition();

            // Cel: Wydawka, ale przesunięta troszkę w LEWO kelnera (prawo gracza)
            glm::vec3 targetWalkPos = stationPos;
            targetWalkPos.y = currentPos.y;
            targetWalkPos.x -= 1.5f; // Minus w osi X to dokładnie lewo kelnera!

            float dist = glm::distance(currentPos, targetWalkPos);

            // BARDZO DUŻY ZASIĘG: Zatrzymuje się aż 3 metry przed celem (czyli na najbliższym mu końcu blatu!)
            if (dist > 2.0f) {
                glm::vec3 dir = glm::normalize(targetWalkPos - currentPos);
                waiterTf->SetPosition(currentPos + dir * 3.0f * (float)ts.GetSeconds());

                float targetAngle = glm::degrees(glm::atan(dir.x, dir.z));
                waiterTf->SetRotation(glm::vec3(0.0f, targetAngle, 0.0f));
            }
            else {
                // Po zatrzymaniu obraca się kulturalnie przodem do zupy
                glm::vec3 lookDir = glm::normalize(stationPos - currentPos);
                float targetAngle = glm::degrees(glm::atan(lookDir.x, lookDir.z));
                waiterTf->SetRotation(glm::vec3(0.0f, targetAngle, 0.0f));

                if (targetPlate.id != std::numeric_limits<std::size_t>::max()) {
                    auto* plateTf = GetScene()->GetWorld().GetComponent<TransformComponent>(targetPlate);
                    if (plateTf) {
                        GetScene()->SetParent(targetPlate, m_Waiter);

                        plateTf->SetPosition(glm::vec3(0.0f, 0.0f, 0.6f));
                        plateTf->SetScale(glm::vec3(0.22f, 0.22f, 0.22f));
                    }
                }
                m_WalkAnimPlayed = false;
                auto* animComp2 = GetScene()->GetWorld().GetComponent<AnimatorComponent>(m_Waiter);
                if (animComp2 && animComp2->AnimatorInstance) animComp2->AnimatorInstance->PlayAnimation("Idle");

                endPhase = 2;
            }
        }
        // --- FAZA 2: Kelner wraca na swoje miejsce ---
        else if (endPhase == 2) {
            if (!m_WalkAnimPlayed) {
                auto* animComp = GetScene()->GetWorld().GetComponent<AnimatorComponent>(m_Waiter);
                if (animComp && animComp->AnimatorInstance) animComp->AnimatorInstance->PlayAnimation("Walk");
                m_WalkAnimPlayed = true;
            }

            glm::vec3 currentPos = waiterTf->GetPosition();
            float dist = glm::distance(currentPos, s_WaiterOriginalSpawnPos);

            if (dist > 0.1f) {
                glm::vec3 dir = glm::normalize(s_WaiterOriginalSpawnPos - currentPos);
                waiterTf->SetPosition(currentPos + dir * 3.0f * (float)ts.GetSeconds());

                float targetAngle = glm::degrees(glm::atan(dir.x, dir.z));
                waiterTf->SetRotation(glm::vec3(0.0f, targetAngle, 0.0f));
            }
            else {
                auto* animComp2 = GetScene()->GetWorld().GetComponent<AnimatorComponent>(m_Waiter);
                if (animComp2 && animComp2->AnimatorInstance) animComp2->AnimatorInstance->PlayAnimation("Idle");

                waiterTf->SetPosition(s_WaiterOriginalSpawnPos);
                m_TypewriterTimer = 0.0f;
                GameManagerScript::s_TutorialCharsRevealed = 0;
                endPhase = 3;
            }
        }
        // --- FAZA 3: Zbliżenie kamery i dialog ---
        else if (endPhase == 3) {
            auto* camera = GetScene()->GetCamera();
            if (camera) {
                camera->TargetPosition = waiterTf->GetPosition() + glm::vec3(0.0f, 0.8f, 0.0f);
                waiterTf->SetRotation(glm::vec3(0.0f, 45.0f, 0.0f));
                camera->Zoom += (8.0f - camera->Zoom) * 3.0f * ts.GetSeconds();
            }

            if (targetPlate.id != std::numeric_limits<std::size_t>::max()) {
                Entity soupModel = { std::numeric_limits<std::size_t>::max(), 0 };

                auto* nscVector = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();
                if (nscVector) {
                    auto* plateNsc = nscVector->Get(targetPlate);
                    if (plateNsc) {
                        for (auto& s : plateNsc->Scripts) {
                            if (s.Name == "PlateScript" && s.Instance) {
                                auto* pScript = static_cast<PlateScript*>(s.Instance);
                                if (!pScript->m_VisualModels.empty()) soupModel = pScript->m_VisualModels.back();
                            }
                        }
                    }
                }

                if (soupModel.id != std::numeric_limits<std::size_t>::max()) {
                    auto* soupTf = GetScene()->GetWorld().GetComponent<TransformComponent>(soupModel);
                    if (soupTf) {
                        glm::vec3 scale = soupTf->GetScale();
                        scale -= glm::vec3(1.0f) * 0.2f * (float)ts.GetSeconds();
                        scale = glm::max(scale, glm::vec3(0.0f));
                        soupTf->SetScale(scale);
                    }
                }
            }

            GameManagerScript::s_ShowTutorialDialog = true;
            GameManagerScript::s_TutorialSpeaker = "Walter:";
            GameManagerScript::s_TutorialSpeakerColor = glm::vec4(0.75f, 0.4f, 0.9f, 1.0f);

            std::string text = "Such dishes will certainly please the customers! Need a waiter? I'll be one!";
            GameManagerScript::s_TutorialText = text;
            GameManagerScript::s_TutorialDialogIsBottom = false;

            m_TypewriterTimer += ts.GetSeconds();
            if (m_TypewriterTimer > 0.05f) {
                m_TypewriterTimer = 0.0f;
                if (GameManagerScript::s_TutorialCharsRevealed < (int)text.length()) {
                    GameManagerScript::s_TutorialCharsRevealed++;
                }
            }

            if (GameManagerScript::s_TutorialCharsRevealed >= (int)text.length()) {
                GameManagerScript::s_TutorialIconAlpha = std::min(1.0f, GameManagerScript::s_TutorialIconAlpha + (float)ts.GetSeconds() * 3.0f);
            }
            else {
                GameManagerScript::s_TutorialIconAlpha = 0.0f;
            }

            bool isActionPressed = Input::IsMouseButtonJustPressed(0) || (Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0));
            if (isActionPressed) {
                if (GameManagerScript::s_TutorialCharsRevealed < (int)text.length()) {
                    GameManagerScript::s_TutorialCharsRevealed = (int)text.length();
                }
                else {
                    GameManagerScript::s_ShowTutorialDialog = false;
                    endPhase = 4;
                    m_StateTimer = 0.0f;
                    PlayPoofAt(waiterTf->GetPosition() + glm::vec3(0.0f, 1.0f, 0.0f));
                }
            }
        }
        // --- FAZA 4: Zamiana w kelnera ---
        else if (endPhase == 4) {
            static bool swapped = false;

            Entity activeWaiter = swapped ? upgradedWaiter : m_Waiter;
            auto* activeTf = activeWaiter.id != std::numeric_limits<std::size_t>::max() ? GetScene()->GetWorld().GetComponent<TransformComponent>(activeWaiter) : nullptr;

            if (activeTf) {
                glm::vec3 rot = activeTf->GetRotation();
                rot.y += 1500.0f * (float)ts.GetSeconds();
                activeTf->SetRotation(rot);
            }

            if (m_StateTimer > 0.25f && !swapped) {
                if (upgradedWaiter.id != std::numeric_limits<std::size_t>::max()) {
                    auto* newTf = GetScene()->GetWorld().GetComponent<TransformComponent>(upgradedWaiter);
                    if (newTf && waiterTf) {
                        newTf->SetPosition(waiterTf->GetPosition());
                        newTf->SetRotation(waiterTf->GetRotation());
                    }
                    HideUnderground(m_Waiter);
                }

                if (targetPlate.id != std::numeric_limits<std::size_t>::max()) {
                    GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ targetPlate });
                }
                swapped = true;
            }

            if (m_StateTimer > 0.6f) {
                if (upgradedWaiter.id != std::numeric_limits<std::size_t>::max()) {
                    auto* newTf = GetScene()->GetWorld().GetComponent<TransformComponent>(upgradedWaiter);
                    if (newTf) newTf->SetRotation(glm::vec3(0.0f, 45.0f, 0.0f));
                }

                HideUnderground(m_Poof);
                m_StateTimer = 0.0f;
                m_TypewriterTimer = 0.0f;
                GameManagerScript::s_TutorialCharsRevealed = 0;
                swapped = false;
                endPhase = 5;
            }
        }
        // --- FAZA 5: Monolog Walter the Waiter ---
        else if (endPhase == 5) {
            GameManagerScript::s_ShowTutorialDialog = true;
            GameManagerScript::s_TutorialSpeaker = "Walter the Waiter:";
            GameManagerScript::s_TutorialSpeakerColor = glm::vec4(0.75f, 0.4f, 0.9f, 1.0f); // Ten sam fioletowy!

            std::string text = "Now you are ready! Just remember to serve customers in order. Maybe some will even join your kitchen as employees!";
            GameManagerScript::s_TutorialText = text;
            GameManagerScript::s_TutorialDialogIsBottom = false;

            m_TypewriterTimer += ts.GetSeconds();
            if (m_TypewriterTimer > 0.05f) {
                m_TypewriterTimer = 0.0f;
                if (GameManagerScript::s_TutorialCharsRevealed < (int)text.length()) {
                    GameManagerScript::s_TutorialCharsRevealed++;
                }
            }

            if (GameManagerScript::s_TutorialCharsRevealed >= (int)text.length()) {
                GameManagerScript::s_TutorialIconAlpha = std::min(1.0f, GameManagerScript::s_TutorialIconAlpha + (float)ts.GetSeconds() * 3.0f);
            }
            else {
                GameManagerScript::s_TutorialIconAlpha = 0.0f;
            }

            bool isActionPressed = Input::IsMouseButtonJustPressed(0) || (Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0));
            if (isActionPressed) {
                if (GameManagerScript::s_TutorialCharsRevealed < (int)text.length()) {
                    GameManagerScript::s_TutorialCharsRevealed = (int)text.length();
                }
                else {
                    GameManagerScript::s_ShowTutorialDialog = false;
                    m_StateTimer = 0.0f;

                    // Odpalamy pożegnalny wybuch gwiazdek/dymu na cześć końca tutoriala!
                    if (upgradedWaiter.id != std::numeric_limits<std::size_t>::max()) {
                        auto* newTf = GetScene()->GetWorld().GetComponent<TransformComponent>(upgradedWaiter);
                        if (newTf) PlayPoofAt(newTf->GetPosition() + glm::vec3(0.0f, 1.0f, 0.0f));
                    }
                    endPhase = 6;
                }
            }
        }
        // --- FAZA 6: Przejście do gry właściwej ---
        else if (endPhase == 6) {
            auto* camera = GetScene()->GetCamera();
            if (camera) {
                camera->Zoom += (32.0f - camera->Zoom) * 4.0f * ts.GetSeconds();
            }

            if (m_StateTimer > 1.5f) {
                g_TriggerCloudTransition = true; // ODPALAMY CHMURKĘ!
                m_StateTimer = -9999.0f;
                endPhase = 7;
            }
        }
        break;
    }

    default:
        break;
    }
}