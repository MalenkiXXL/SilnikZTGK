#include "TutorialManagerScript.h"
#include "CookingStation/Scripts/Managers/GameManagerScript.h"
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Core/Input.h" 
#include "CookingStation/Scripts/PoofEmitterScript.h"
#include "CookingStation/Scripts/ParticleEmitterScript.h"
#include "CookingStation/Scripts/CrateScript.h" 
#include <algorithm>
#include <limits> 
#include "CookingStation/Scripts/Machines/CuttingBoardScript.h"
#include "CookingStation/Scripts/Machines/PotScript.h"

Entity TutorialManagerScript::FindEntityByName(const std::string& name) {
    auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
    if (tags) {
        for (size_t i = 0; i < tags->dense.size(); ++i) {
            if (tags->dense[i].Tag == name) {
                return tags->reverse[i];
            }
        }
    }
    return { NULL_ENTITY };
}

void TutorialManagerScript::HideUnderground(Entity e) {
    if (e.id == NULL_ENTITY) return;
    auto* tc = GetScene()->GetWorld().GetComponent<TransformComponent>(e);
    if (tc) {
        glm::vec3 pos = tc->GetPosition();
        pos.y = -999.0f;
        tc->SetPosition(pos);
    }
}

void TutorialManagerScript::RestorePosition(Entity e, glm::vec3 originalPos) {
    if (e.id == NULL_ENTITY) return;
    auto* tc = GetScene()->GetWorld().GetComponent<TransformComponent>(e);
    if (tc) {
        tc->SetPosition(originalPos);
    }
}

void TutorialManagerScript::OnCreate() {
    GameManagerScript::s_IsTutorialMode = true;

    // Zabieramy domyœlne pomidory z magazynu gracza na start
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

    auto* crateTc = GetScene()->GetWorld().GetComponent<TransformComponent>(m_TomatoCrate);
    if (crateTc) {
        m_CrateOriginalPos = crateTc->GetPosition();
    }
    auto* boardTc = GetScene()->GetWorld().GetComponent<TransformComponent>(m_Board);
    if (boardTc) {
        m_BoardOriginalPos = boardTc->GetPosition();
    }
    auto* standTc = GetScene()->GetWorld().GetComponent<TransformComponent>(m_BoardStand);
    if (standTc) {
        m_BoardStandOriginalPos = standTc->GetPosition();
    }
    auto* potTc = GetScene()->GetWorld().GetComponent<TransformComponent>(m_Pot);
    if (potTc) {
        m_PotOriginalPos = potTc->GetPosition();
    }
    auto* burnerTc = GetScene()->GetWorld().GetComponent<TransformComponent>(m_Burner);
    if (burnerTc) {
        m_BurnerOriginalPos = burnerTc->GetPosition();
    }

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
                if (m_Floor.id != NULL_ENTITY) {
                    auto* floorTc = GetScene()->GetWorld().GetComponent<TransformComponent>(m_Floor);
                    if (floorTc) {
                        floorCenter = floorTc->GetPosition();
                    }
                }

                glm::vec3 cameraOffset = glm::vec3(0.0f, 2.0f, 0.0f);
                camera->TargetPosition = floorCenter + cameraOffset;
            }

            m_State = TutorialState::CameraResetting;
            m_StateTimer = 0.0f;
            m_DialogIndex = 0;
        }
        break;
    }

    case TutorialState::CameraResetting: {
        auto* camera = GetScene()->GetCamera();
        if (camera) {
            camera->Zoom += (32.0f - camera->Zoom) * 4.0f * ts.GetSeconds();
        }

        if (m_StateTimer > 1.5f && m_DialogIndex == 0) {
            glm::vec3 poofPos = glm::vec3(-7.0f, 2.2f, 1.0f);
            RestorePosition(m_Poof, poofPos);

            auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Poof);
            if (nsc) {
                for (auto& s : nsc->Scripts) {
                    if (s.Name == "PoofEmitterScript" && s.Instance) {
                        static_cast<ParticleEmitterScript*>(s.Instance)->Play();
                        break;
                    }
                }
            }
            m_DialogIndex = 1;
        }

        if (m_StateTimer > 1.9f && m_DialogIndex == 1) {
            glm::vec3 spawnerPos = glm::vec3(-7.0f, 1.2f, 1.0f);
            RestorePosition(m_PlateSpawner, spawnerPos);
            m_DialogIndex = 2;
        }

        if (m_StateTimer > 2.4f && m_DialogIndex == 2) {
            if (m_Poof.id != NULL_ENTITY) {
                HideUnderground(m_Poof);
            }
            m_State = TutorialState::WaitForCrateSpawn;
            m_StateTimer = 0.0f;
            m_DialogIndex = 0;
        }
        break;
    }

    case TutorialState::WaitForCrateSpawn: {
        if (m_StateTimer > 2.5f && m_DialogIndex == 0) {
            glm::vec3 poofPos = m_CrateOriginalPos + glm::vec3(0.0f, 1.0f, 0.0f);
            RestorePosition(m_Poof, poofPos);

            auto* poofNsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Poof);
            if (poofNsc) {
                for (auto& s : poofNsc->Scripts) {
                    if (s.Name == "PoofEmitterScript" && s.Instance) {
                        static_cast<ParticleEmitterScript*>(s.Instance)->Play();
                        break;
                    }
                }
            }
            m_DialogIndex = 1;
        }

        if (m_StateTimer > 2.8f && m_DialogIndex == 1) {
            RestorePosition(m_TomatoCrate, m_CrateOriginalPos);

            if (GameManagerScript::s_Instance) {
                int currentTomatoes = GameManagerScript::s_Instance->GetIngredientCount(IngredientType::Tomato);
                if (currentTomatoes > 1) {
                    GetScene()->GetWorld().GetEventBus().Publish(IngredientUsedEvent{ IngredientType::Tomato, currentTomatoes - 1 });
                }
                else if (currentTomatoes == 0) {
                    GetScene()->GetWorld().GetEventBus().Publish(AddIngredientEvent{ IngredientType::Tomato, 1 });
                }
            }

            // --- ZMIANA: Skrzynka pojawia siê na soczysty ró¿owo! ---
            TriggerHighlightEvent ev;
            ev.TargetEntity = m_TomatoCrate;
            ev.Color = glm::vec3(1.0f, 0.2f, 0.6f);
            ev.IsInfinite = true;
            GetScene()->GetWorld().GetEventBus().Publish(ev);

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
                            m_DialogIndex = 3;
                        }
                    }
                }
            }
        }

        if (m_DialogIndex == 3) {
            auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_TomatoCrate);
            if (nsc) {
                for (auto& s : nsc->Scripts) {
                    if (s.Name == "CrateScript" && s.Instance) {
                        auto* crateScript = static_cast<CrateScript*>(s.Instance);
                        if (crateScript->m_VisualFood.id != std::numeric_limits<std::size_t>::max()) {
                            // --- ZMIANA: Jedzenie w skrzynce te¿ na ró¿owo ---
                            TriggerHighlightEvent evFood;
                            evFood.TargetEntity = crateScript->m_VisualFood;
                            evFood.Color = glm::vec3(1.0f, 0.2f, 0.6f);
                            evFood.Duration = 8.0f;
                            evFood.IsInfinite = true;
                            GetScene()->GetWorld().GetEventBus().Publish(evFood);
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
        if (m_StateTimer < 0.05f) {
            crateHoverLerp = 0.0f;
        }

        bool isHoveringCrate = false;
        auto* crateTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_TomatoCrate);

        // --- NAPRAWA CELOWANIA W SKRZYNKÊ (Rzutowanie 3D zamiast wielkiego hitboxa 4.0f) ---
        if (crateTf && !Input::IsUICapturingMouse()) {
            glm::vec3 floorMousePos = GetMouseWorldPosition();
            auto* camera = GetScene()->GetCamera();
            glm::vec3 preciseMousePos = floorMousePos;

            if (camera) {
                float targetY = crateTf->GetPosition().y;
                glm::vec3 rayDir = camera->Front;
                if (std::abs(rayDir.y) > 0.001f) {
                    float t = (targetY - floorMousePos.y) / rayDir.y;
                    preciseMousePos = floorMousePos + rayDir * t;
                }
            }

            glm::vec2 mouse2D = { preciseMousePos.x, preciseMousePos.z };

            // Teraz skrzynka ³apie idealnie, tylko z bliskiej odleg³oœci 1.5f!
            if (glm::distance(mouse2D, glm::vec2(crateTf->GetPosition().x, crateTf->GetPosition().z)) < 1.5f) {
                isHoveringCrate = true;
            }
        }

        if (isHoveringCrate) {
            crateHoverLerp += ts.GetSeconds() * 8.0f;
        }
        else {
            crateHoverLerp -= ts.GetSeconds() * 8.0f;
        }
        crateHoverLerp = std::clamp(crateHoverLerp, 0.0f, 1.0f);

        glm::vec3 basePink = glm::vec3(1.0f, 0.2f, 0.6f);
        glm::vec3 hoverGold = glm::vec3(1.0f, 0.9f, 0.0f);
        glm::vec3 currentCrateColor = glm::mix(basePink, hoverGold, crateHoverLerp);

        auto* crateNsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_TomatoCrate);
        if (crateNsc) {
            for (auto& s : crateNsc->Scripts) {
                if (s.Name == "CrateScript" && s.Instance) {
                    auto* crateScript = static_cast<CrateScript*>(s.Instance);
                    if (crateScript->m_VisualFood.id != std::numeric_limits<std::size_t>::max()) {
                        TriggerHighlightEvent evFood;
                        evFood.TargetEntity = crateScript->m_VisualFood;
                        evFood.Color = currentCrateColor;
                        evFood.Duration = 8.0f;
                        evFood.IsInfinite = true;
                        GetScene()->GetWorld().GetEventBus().Publish(evFood);
                    }
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
            TriggerHighlightEvent ev;
            ev.TargetEntity = m_TomatoCrate;
            ev.Color = glm::vec3(0.0f);
            ev.Duration = 0.1f;
            ev.IsInfinite = false;
            GetScene()->GetWorld().GetEventBus().Publish(ev);

            if (crateNsc) {
                for (auto& s : crateNsc->Scripts) {
                    if (s.Name == "CrateScript" && s.Instance) {
                        auto* crateScript = static_cast<CrateScript*>(s.Instance);
                        if (crateScript->m_VisualFood.id != std::numeric_limits<std::size_t>::max()) {
                            TriggerHighlightEvent evFoodClear;
                            evFoodClear.TargetEntity = crateScript->m_VisualFood;
                            evFoodClear.Color = glm::vec3(0.0f);
                            evFoodClear.Duration = 0.1f;
                            evFoodClear.IsInfinite = false;
                            GetScene()->GetWorld().GetEventBus().Publish(evFoodClear);
                        }
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
        // Cooldown 1.5s po zabraniu pomidora
        if (m_StateTimer > 1.5f && m_DialogIndex == 0) {
            glm::vec3 poofPos = m_BoardOriginalPos + glm::vec3(0.0f, 1.0f, 0.0f);
            RestorePosition(m_Poof, poofPos);

            auto* poofNsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Poof);
            if (poofNsc) {
                for (auto& s : poofNsc->Scripts) {
                    if (s.Name == "PoofEmitterScript" && s.Instance) {
                        static_cast<ParticleEmitterScript*>(s.Instance)->Play();
                        break;
                    }
                }
            }
            m_DialogIndex = 1;
        }

        // Przywracamy deskê i szafkê u³amek sekundy po efekcie cz¹steczkowym
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
            tomatoHoverLerp = 0.0f;
            boardHoverLerp = 0.0f;
            boardTransitionLerp = 0.0f;
        }

        // --- 1. POZYCJE OBIEKTÓW I ZASIÊG ---
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

        bool inRange = false;
        auto* boardTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_Board);
        auto* tomatoTf = GetScene()->GetWorld().GetComponent<TransformComponent>(tomatoBeltItem);

        if (tomatoTf && boardTf) {
            float distToBoard = glm::distance(
                glm::vec2(boardTf->GetPosition().x, boardTf->GetPosition().z),
                glm::vec2(tomatoTf->GetPosition().x, tomatoTf->GetPosition().z)
            );
            if (distToBoard < 3.5f) {
                inRange = true;
            }
        }

        // --- 2. PRECYZYJNA LOGIKA MYSZKI (Rzutowanie z uwzglêdnieniem kamery!) ---
        glm::vec3 floorMousePos = GetMouseWorldPosition();
        auto* camera = GetScene()->GetCamera();
        glm::vec3 preciseMousePos = floorMousePos;

        if (camera && boardTf) {
            float targetY = boardTf->GetPosition().y;
            glm::vec3 rayDir = camera->Front;
            if (std::abs(rayDir.y) > 0.001f) {
                float t = (targetY - floorMousePos.y) / rayDir.y;
                preciseMousePos = floorMousePos + rayDir * t;
            }
        }

        glm::vec2 mouse2D = { preciseMousePos.x, preciseMousePos.z };

        bool isHoveringTomato = false;
        bool isHoveringBoard = false;

        if (!Input::IsUICapturingMouse()) {
            // TERAZ U¯YWAMY IDEALNEGO, MA£EGO ZASIÊGU (1.5f), bo perspektywa jest naprawiona!
            if (tomatoTf && glm::distance(mouse2D, glm::vec2(tomatoTf->GetPosition().x, tomatoTf->GetPosition().z)) < 1.5f) isHoveringTomato = true;
            if (boardTf && glm::distance(mouse2D, glm::vec2(boardTf->GetPosition().x, boardTf->GetPosition().z)) < 1.5f) isHoveringBoard = true;
        }

        if (isHoveringTomato && inRange) tomatoHoverLerp += ts.GetSeconds() * 8.0f;
        else tomatoHoverLerp -= ts.GetSeconds() * 8.0f;
        tomatoHoverLerp = std::clamp(tomatoHoverLerp, 0.0f, 1.0f);

        if (isHoveringBoard && inRange) boardHoverLerp += ts.GetSeconds() * 8.0f;
        else boardHoverLerp -= ts.GetSeconds() * 8.0f;
        boardHoverLerp = std::clamp(boardHoverLerp, 0.0f, 1.0f);

        glm::vec3 basePink = glm::vec3(1.0f, 0.2f, 0.6f);
        glm::vec3 hoverGold = glm::vec3(1.0f, 0.9f, 0.0f);

        glm::vec3 currentTomatoColor = glm::mix(basePink, hoverGold, tomatoHoverLerp);
        glm::vec3 currentBoardInteractionColor = glm::mix(basePink, hoverGold, boardHoverLerp);


        // --- 3. P£YNNE PRZEJŒCIE KOLORU DESKI ---
        if (inRange) boardTransitionLerp += ts.GetSeconds() * 3.0f;
        else boardTransitionLerp -= ts.GetSeconds() * 3.0f;
        boardTransitionLerp = std::clamp(boardTransitionLerp, 0.0f, 1.0f);

        glm::vec3 grayColor = glm::vec3(0.4f, 0.4f, 0.4f);
        glm::vec3 currentBoardColor = glm::mix(grayColor, currentBoardInteractionColor, boardTransitionLerp);
        float currentBoardDuration = glm::mix(32.0f, 8.0f, boardTransitionLerp);

        TriggerHighlightEvent evBoard;
        evBoard.TargetEntity = m_Board;
        evBoard.Color = currentBoardColor;
        evBoard.Duration = currentBoardDuration;
        evBoard.IsInfinite = true;
        GetScene()->GetWorld().GetEventBus().Publish(evBoard);

        // --- 4. HIGHLIGHT POMIDORA ---
        if (inRange) {
            TriggerHighlightEvent evTomato;
            evTomato.TargetEntity = tomatoBeltItem;
            evTomato.Color = currentTomatoColor;
            evTomato.Duration = 8.0f;
            evTomato.IsInfinite = true;
            GetScene()->GetWorld().GetEventBus().Publish(evTomato);
        }

        // --- 5. OBS£UGA UI KROPEK ---
        m_TypewriterTimer += ts.GetSeconds();

        if (m_TypewriterTimer < 5.5f) {
            GameManagerScript::s_ShowTutorialDialog = true;
            GameManagerScript::s_TutorialSpeaker = "";
            GameManagerScript::s_TutorialDialogIsBottom = true;
            GameManagerScript::s_TutorialIconAlpha = 0.0f;

            int stage = (int)(m_TypewriterTimer / 0.8f);
            std::string offset = "                        ";
            std::string waitingText = "";

            if (stage == 0 || stage == 3) waitingText = offset + ".";
            else if (stage == 1 || stage == 4) waitingText = offset + ". .";
            else if (stage == 2 || stage == 5) waitingText = offset + ". . .";
            else waitingText = "";

            GameManagerScript::s_TutorialText = waitingText;
            GameManagerScript::s_TutorialCharsRevealed = waitingText.length();
        }
        else {
            GameManagerScript::s_ShowTutorialDialog = false;
        }

        // --- 6. OBS£UGA KLIKNIÊCIA ---
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

        // --- 7. WARUNEK PRZEJŒCIA DALEJ ---
        bool hasIngredient = false;
        auto* nscBoard = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Board);
        if (nscBoard) {
            for (auto& s : nscBoard->Scripts) {
                if (s.Name == "CuttingBoardScript" && s.Instance) {
                    auto* boardScript = static_cast<CuttingBoardScript*>(s.Instance);
                    if (!boardScript->m_Ingredients.empty()) {
                        hasIngredient = true;
                    }
                }
            }
        }

        if (hasIngredient) {
            TriggerHighlightEvent ev;
            ev.TargetEntity = m_Board;
            ev.Color = glm::vec3(0.0f);
            ev.Duration = 0.1f;
            ev.IsInfinite = false;
            GetScene()->GetWorld().GetEventBus().Publish(ev);

            GameManagerScript::s_ShowTutorialDialog = false;

            m_State = TutorialState::WaitForChopping;
            m_StateTimer = 0.0f;
        }
        break;
    }

    case TutorialState::WaitForChopping: {
        // Niezawodny reset na pocz¹tku stanu
        if (m_StateTimer < 0.05f) {
            GameManagerScript::s_TutorialCharsRevealed = 0;
        }

        // --- 1. ODCZYT DANYCH Z MASZYNY W CZASIE RZECZYWISTYM ---
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

        // --- 2. IDEALNA SYNCHRONIZACJA Z MYSZK¥ ---
        // U¿ywamy prêdkoœci 4.0f - to DOK£ADNIE taka sama wartoœæ, jakiej 
        // Twój GameGuiLayer u¿ywa do rozb³ysków myszki ( std::sin(timeNow * 4.0f) )
        float wave = (std::sin(m_StateTimer * 4.0f) + 1.0f) * 0.5f;

        // Deska oddycha fioletem w idealnym rytmie UI
        glm::vec3 basePurple = glm::vec3(0.61f, 0.44f, 0.8f);
        TriggerHighlightEvent evBoard;
        evBoard.TargetEntity = m_Board;
        evBoard.Color = basePurple * wave;
        evBoard.Duration = 0.1f;
        evBoard.IsInfinite = false;
        GetScene()->GetWorld().GetEventBus().Publish(evBoard);

        // --- 3. OBS£UGA UI ---
        GameManagerScript::s_ShowTutorialDialog = true;
        GameManagerScript::s_TutorialSpeaker = "";

        GameManagerScript::s_TutorialTrackedEntity = m_Board;
        GameManagerScript::s_TutorialTrackedOffset = glm::vec3(0.0f, 3.0f, 0.0f);

        // --- NAPRAWA: Zwracamy sta³e Alpha dla Ikonki ---
        // Ikona pojawia siê w u³amek sekundy i trzyma wartoœæ 1.0f. 
        // Twój kod w GameGuiLayer sam zajmie siê teraz lataniem góra-dó³ i rozb³yskami pod ni¹!
        GameManagerScript::s_TutorialIconAlpha = std::clamp(m_StateTimer * 3.0f, 0.0f, 1.0f);

        std::string text = std::to_string(currentChops) + " / 3";
        GameManagerScript::s_TutorialText = text;
        GameManagerScript::s_TutorialCharsRevealed = text.length();

        // --- 4. WARUNEK PRZEJŒCIA DALEJ ---
        if (isReady) {
            TriggerHighlightEvent ev;
            ev.TargetEntity = m_Board;
            ev.Color = glm::vec3(0.0f);
            ev.Duration = 0.1f;
            ev.IsInfinite = false;
            GetScene()->GetWorld().GetEventBus().Publish(ev);

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
            foodHoverLerp = 0.0f;
            plateHoverLerp = 0.0f;
            transitionLerp = 0.0f;
        }

        // --- 1. ZNAJDOWANIE POKROJONEGO POMIDORA ---
        Entity foodOnBoard = { std::numeric_limits<std::size_t>::max(), 0 };
        auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
        if (tags) {
            for (size_t i = 0; i < tags->dense.size(); ++i) {
                if (tags->dense[i].Tag == "Na_Desce") {
                    foodOnBoard = tags->reverse[i];
                    break;
                }
            }
        }

        // --- 2. ZNAJDOWANIE NAJBLI¯SZEGO TALERZA ---
        Entity closestPlate = { std::numeric_limits<std::size_t>::max(), 0 };
        float closestDist = 999.0f;
        auto* boardTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_Board);

        if (tags && boardTf) {
            for (size_t i = 0; i < tags->dense.size(); ++i) {
                const std::string& tag = tags->dense[i].Tag;
                if (tag.find("Plate") != std::string::npos || tag.find("Talerz") != std::string::npos) {
                    Entity p = tags->reverse[i];
                    auto* pTf = GetScene()->GetWorld().GetComponent<TransformComponent>(p);
                    if (pTf) {
                        float d = glm::distance(pTf->GetPosition(), boardTf->GetPosition());
                        if (d < closestDist) {
                            closestDist = d;
                            closestPlate = p;
                        }
                    }
                }
            }
        }

        // --- 3. SPRAWDZANIE ZASIÊGU (Czy talerz podjecha³?) ---
        bool inRange = false;
        auto* plateTf = closestPlate.id != std::numeric_limits<std::size_t>::max() ? GetScene()->GetWorld().GetComponent<TransformComponent>(closestPlate) : nullptr;

        if (plateTf && boardTf) {
            float distToPlate = glm::distance(
                glm::vec2(boardTf->GetPosition().x, boardTf->GetPosition().z),
                glm::vec2(plateTf->GetPosition().x, plateTf->GetPosition().z)
            );
            if (distToPlate < 3.5f) {
                inRange = true;
            }
        }

        // --- 4. PRECYZYJNA LOGIKA MYSZKI (Rzutowanie z napraw¹ perspektywy) ---
        glm::vec3 floorMousePos = GetMouseWorldPosition();
        auto* camera = GetScene()->GetCamera();
        glm::vec3 preciseMousePos = floorMousePos;

        if (camera && boardTf) {
            float targetY = boardTf->GetPosition().y;
            glm::vec3 rayDir = camera->Front;
            if (std::abs(rayDir.y) > 0.001f) {
                float t = (targetY - floorMousePos.y) / rayDir.y;
                preciseMousePos = floorMousePos + rayDir * t;
            }
        }

        glm::vec2 mouse2D = { preciseMousePos.x, preciseMousePos.z };

        bool isHoveringFood = false;
        bool isHoveringPlate = false;

        if (!Input::IsUICapturingMouse()) {
            if (boardTf && glm::distance(mouse2D, glm::vec2(boardTf->GetPosition().x, boardTf->GetPosition().z)) < 1.5f) isHoveringFood = true;
            if (plateTf && glm::distance(mouse2D, glm::vec2(plateTf->GetPosition().x, plateTf->GetPosition().z)) < 1.5f) isHoveringPlate = true;
        }

        // --- 5. P£YNNA MATEMATYKA KOLORÓW (Niezale¿na!) ---
        if (isHoveringFood && inRange) foodHoverLerp += ts.GetSeconds() * 8.0f;
        else foodHoverLerp -= ts.GetSeconds() * 8.0f;
        foodHoverLerp = std::clamp(foodHoverLerp, 0.0f, 1.0f);

        if (isHoveringPlate && inRange) plateHoverLerp += ts.GetSeconds() * 8.0f;
        else plateHoverLerp -= ts.GetSeconds() * 8.0f;
        plateHoverLerp = std::clamp(plateHoverLerp, 0.0f, 1.0f);

        if (inRange) transitionLerp += ts.GetSeconds() * 3.0f;
        else transitionLerp -= ts.GetSeconds() * 3.0f;
        transitionLerp = std::clamp(transitionLerp, 0.0f, 1.0f);

        glm::vec3 baseGray = glm::vec3(0.4f, 0.4f, 0.4f);
        glm::vec3 basePink = glm::vec3(1.0f, 0.2f, 0.6f);
        glm::vec3 hoverGold = glm::vec3(1.0f, 0.9f, 0.0f);

        glm::vec3 activeFoodColor = glm::mix(basePink, hoverGold, foodHoverLerp);
        glm::vec3 activePlateColor = glm::mix(basePink, hoverGold, plateHoverLerp);

        glm::vec3 foodColor = glm::mix(baseGray, activeFoodColor, transitionLerp);
        float foodDuration = glm::mix(32.0f, 8.0f, transitionLerp);

        // Wysy³anie eventów
        if (foodOnBoard.id != std::numeric_limits<std::size_t>::max()) {
            TriggerHighlightEvent evFood;
            evFood.TargetEntity = foodOnBoard;
            evFood.Color = foodColor;
            evFood.Duration = foodDuration;
            evFood.IsInfinite = true;
            GetScene()->GetWorld().GetEventBus().Publish(evFood);
        }

        if (inRange && plateTf) {
            TriggerHighlightEvent evPlate;
            evPlate.TargetEntity = closestPlate;
            evPlate.Color = activePlateColor;
            evPlate.Duration = 8.0f;
            evPlate.IsInfinite = true;
            GetScene()->GetWorld().GetEventBus().Publish(evPlate);
        }

        // --- 6. WYMUSZENIE KLIKNIÊCIA (PERFEKCYJNE OBEJŒCIE) ---
        bool isActionPressed = Input::IsMouseButtonJustPressed(0) || (Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0));

        if (isActionPressed && inRange && (isHoveringFood || isHoveringPlate) && !Input::IsUICapturingMouse()) {
            auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Board);
            if (nsc) {
                for (auto& s : nsc->Scripts) {
                    if (s.Name == "CuttingBoardScript" && s.Instance) {
                        auto* boardScript = static_cast<CuttingBoardScript*>(s.Instance);

                        // Rozdzielamy logikê!
                        if (isHoveringFood) {
                            // Jeœli klikniêto w pomidora na desce -> normalny klik w maszynê
                            boardScript->HandleClick();
                        }
                        else if (isHoveringPlate) {
                            // Jeœli klikniêto w talerz -> BEZPOŒREDNI ROZKAZ TRANSFERU! Omija blokady z HandleClick.
                            boardScript->TryTransferToPlate();
                        }
                    }
                }
            }
        }

        // --- 7. OBS£UGA UI KROPEK ---
        m_TypewriterTimer += ts.GetSeconds();

        if (m_TypewriterTimer < 5.5f) {
            GameManagerScript::s_ShowTutorialDialog = true;
            GameManagerScript::s_TutorialSpeaker = "";
            GameManagerScript::s_TutorialDialogIsBottom = true;
            GameManagerScript::s_TutorialIconAlpha = 0.0f;

            int stage = (int)(m_TypewriterTimer / 0.8f);
            std::string offset = "                        ";
            std::string waitingText = "";

            if (stage == 0 || stage == 3) waitingText = offset + ".";
            else if (stage == 1 || stage == 4) waitingText = offset + ". .";
            else if (stage == 2 || stage == 5) waitingText = offset + ". . .";
            else waitingText = "";

            GameManagerScript::s_TutorialText = waitingText;
            GameManagerScript::s_TutorialCharsRevealed = waitingText.length();
        }
        else {
            GameManagerScript::s_ShowTutorialDialog = false;
        }

        // --- 8. WARUNEK PRZEJŒCIA (Sprawdzamy sukces) ---
        bool transferred = false;
        auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Board);
        if (nsc) {
            for (auto& s : nsc->Scripts) {
                if (s.Name == "CuttingBoardScript" && s.Instance) {
                    auto* boardScript = static_cast<CuttingBoardScript*>(s.Instance);

                    if (boardScript->m_Ingredients.empty() && !boardScript->m_IsReady) {
                        transferred = true;
                    }
                }
            }
        }

        if (transferred) {
            // Talerz b³yska na satysfakcjonuj¹cy zielony kolor!
            if (closestPlate.id != std::numeric_limits<std::size_t>::max()) {
                TriggerHighlightEvent evGreen;
                evGreen.TargetEntity = closestPlate;
                evGreen.Color = glm::vec3(0.1f, 1.0f, 0.2f);
                evGreen.Duration = 1.5f;
                evGreen.IsInfinite = false;
                GetScene()->GetWorld().GetEventBus().Publish(evGreen);
            }

            GameManagerScript::s_ShowTutorialDialog = false;

            m_State = TutorialState::WaitForPotPlacement;
            m_StateTimer = 0.0f;
            m_DialogIndex = 0;
        }
        break;
    }

    case TutorialState::WaitForPotPlacement: {
        // ZWIÊKSZONY COOLDOWN - 5 sekund
        if (m_StateTimer > 5.0f && m_DialogIndex == 0) {
            glm::vec3 poofPos = m_PotOriginalPos + glm::vec3(0.0f, 1.0f, 0.0f);
            RestorePosition(m_Poof, poofPos);

            auto* poofNsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Poof);
            if (poofNsc) {
                for (auto& s : poofNsc->Scripts) {
                    if (s.Name == "PoofEmitterScript" && s.Instance) {
                        static_cast<ParticleEmitterScript*>(s.Instance)->Play();
                        break;
                    }
                }
            }
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
            potHoverLerp = 0.0f;
            plateHoverLerp = 0.0f;
            transitionLerp = 0.0f;
        }

        // --- 1. ZNAJDOWANIE NAJBLI¯SZEGO TALERZA I JEGO SKRYPTU ---
        Entity closestPlate = { std::numeric_limits<std::size_t>::max(), 0 };
        PlateScript* closestPlateScript = nullptr;
        float closestDist = 999.0f;

        auto* potTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_Pot);
        auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
        auto* nscVector = GetScene()->GetWorld().GetComponentVector<NativeScriptComponent>();

        if (tags && potTf && nscVector) {
            for (size_t i = 0; i < tags->dense.size(); ++i) {
                const std::string& tag = tags->dense[i].Tag;
                if (tag.find("Plate") != std::string::npos || tag.find("Talerz") != std::string::npos) {
                    Entity p = tags->reverse[i];
                    auto* pTf = GetScene()->GetWorld().GetComponent<TransformComponent>(p);
                    if (pTf) {
                        float d = glm::distance(pTf->GetPosition(), potTf->GetPosition());
                        if (d < closestDist) {
                            closestDist = d;
                            closestPlate = p;
                            // Wyci¹gamy PlateScript z encji
                            auto* plateNsc = nscVector->Get(p);
                            if (plateNsc) {
                                for (auto& s : plateNsc->Scripts) {
                                    if (s.Name == "PlateScript" && s.Instance) {
                                        closestPlateScript = static_cast<PlateScript*>(s.Instance);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // --- 2. IDEALNE ZNAJDOWANIE POMIDORA (Z PlateScript) ---
        Entity foodOnPlate = { std::numeric_limits<std::size_t>::max(), 0 };
        // Bierzemy model dok³adnie tak, jak zarz¹dza nim talerz! Zero tagów, czysta pamiêæ maszyny.
        if (closestPlateScript && !closestPlateScript->m_VisualModels.empty()) {
            foodOnPlate = closestPlateScript->m_VisualModels.back();
        }

        // --- 3. ZASIÊG (Naprawiony, 3.5f zapewnia ³apanie na taœmie!) ---
        bool inRange = false;
        auto* plateTf = closestPlate.id != std::numeric_limits<std::size_t>::max() ? GetScene()->GetWorld().GetComponent<TransformComponent>(closestPlate) : nullptr;
        if (plateTf && potTf) {
            float distToPot = glm::distance(
                glm::vec2(potTf->GetPosition().x, potTf->GetPosition().z),
                glm::vec2(plateTf->GetPosition().x, plateTf->GetPosition().z)
            );
            if (distToPot < 3.5f) {
                inRange = true;
            }
        }

        // --- 4. RZUTOWANIE MYSZKI W 3D ---
        glm::vec3 floorMousePos = GetMouseWorldPosition();
        auto* camera = GetScene()->GetCamera();
        glm::vec3 preciseMousePos = floorMousePos;

        if (camera && potTf) {
            float targetY = potTf->GetPosition().y;
            glm::vec3 rayDir = camera->Front;
            if (std::abs(rayDir.y) > 0.001f) {
                float t = (targetY - floorMousePos.y) / rayDir.y;
                preciseMousePos = floorMousePos + rayDir * t;
            }
        }

        glm::vec2 mouse2D = { preciseMousePos.x, preciseMousePos.z };
        bool isHoveringPot = false;
        bool isHoveringPlate = false;

        if (!Input::IsUICapturingMouse()) {
            if (potTf && glm::distance(mouse2D, glm::vec2(potTf->GetPosition().x, potTf->GetPosition().z)) < 1.5f) isHoveringPot = true;
            if (plateTf && glm::distance(mouse2D, glm::vec2(plateTf->GetPosition().x, plateTf->GetPosition().z)) < 1.5f) isHoveringPlate = true;
        }

        // --- 5. KOLORY ---
        if (isHoveringPot && inRange) potHoverLerp += ts.GetSeconds() * 8.0f;
        else potHoverLerp -= ts.GetSeconds() * 8.0f;
        potHoverLerp = std::clamp(potHoverLerp, 0.0f, 1.0f);

        if (isHoveringPlate && inRange) plateHoverLerp += ts.GetSeconds() * 8.0f;
        else plateHoverLerp -= ts.GetSeconds() * 8.0f;
        plateHoverLerp = std::clamp(plateHoverLerp, 0.0f, 1.0f);

        if (inRange) transitionLerp += ts.GetSeconds() * 3.0f;
        else transitionLerp -= ts.GetSeconds() * 3.0f;
        transitionLerp = std::clamp(transitionLerp, 0.0f, 1.0f);

        glm::vec3 baseGray = glm::vec3(0.4f, 0.4f, 0.4f);
        glm::vec3 basePink = glm::vec3(1.0f, 0.2f, 0.6f);
        glm::vec3 hoverGold = glm::vec3(1.0f, 0.9f, 0.0f);

        glm::vec3 activePotColor = glm::mix(basePink, hoverGold, potHoverLerp);
        glm::vec3 activePlateColor = glm::mix(basePink, hoverGold, plateHoverLerp);

        glm::vec3 potColor = glm::mix(baseGray, activePotColor, transitionLerp);
        float potDuration = glm::mix(32.0f, 8.0f, transitionLerp);

        // --- 6. EVENTY (Perfekcyjne podœwietlanie 3 elementów) ---
        TriggerHighlightEvent evPot;
        evPot.TargetEntity = m_Pot;
        evPot.Color = potColor;
        evPot.Duration = potDuration;
        evPot.IsInfinite = true;
        GetScene()->GetWorld().GetEventBus().Publish(evPot);

        if (inRange && plateTf) {
            TriggerHighlightEvent evPlate;
            evPlate.TargetEntity = closestPlate;
            evPlate.Color = activePlateColor;
            evPlate.Duration = 8.0f;
            evPlate.IsInfinite = true;
            GetScene()->GetWorld().GetEventBus().Publish(evPlate);

            if (foodOnPlate.id != std::numeric_limits<std::size_t>::max()) {
                TriggerHighlightEvent evFood;
                evFood.TargetEntity = foodOnPlate;
                evFood.Color = activePlateColor;
                evFood.Duration = 8.0f;
                evFood.IsInfinite = true;
                GetScene()->GetWorld().GetEventBus().Publish(evFood);
            }
        }

        // --- 7. LOGIKA KLIKNIÊCIA SKOPIOWANA Z DRAG AND DROP SCRIPT! ---
        bool isActionPressed = Input::IsMouseButtonJustPressed(0) || (Input::IsGamepadPresent(0) && Input::IsGamepadButtonJustPressed(2, 0));

        if (isActionPressed && inRange && (isHoveringPot || isHoveringPlate) && !Input::IsUICapturingMouse()) {
            auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Pot);
            if (nsc && closestPlateScript && !closestPlateScript->m_Ingredients.empty()) {
                for (auto& s : nsc->Scripts) {
                    if (s.Name == "PotScript" && s.Instance) {
                        auto* potScript = static_cast<PotScript*>(s.Instance);

                        IngredientType topIngredient = closestPlateScript->m_Ingredients.back();

                        // Próba dodania sk³adnika do garnka
                        if (potScript->AddIngredient(topIngredient)) {
                            // Jeœli garnek przyj¹³, wykonujemy IDENTYCZN¥ sekwencjê niszczenia jak w Twoim silniku:
                            closestPlateScript->m_Ingredients.pop_back();

                            if (!closestPlateScript->m_VisualModels.empty()) {
                                Entity visualToRemove = closestPlateScript->m_VisualModels.back();

                                // Dodatkowo zabezpieczaj¹co gasimy highlight z jedzenia przed zniszczeniem
                                TriggerHighlightEvent evClear;
                                evClear.TargetEntity = visualToRemove;
                                evClear.Color = glm::vec3(0.0f);
                                evClear.Duration = 0.1f;
                                evClear.IsInfinite = false;
                                GetScene()->GetWorld().GetEventBus().Publish(evClear);

                                // Wysy³amy event zniszczenia i usuwamy œlad z talerza
                                GetScene()->GetWorld().GetEventBus().Publish(EntityDestroyRequestEvent{ visualToRemove });
                                closestPlateScript->m_VisualModels.pop_back();
                            }
                        }
                    }
                }
            }
        }

        // --- 8. OBS£UGA UI KROPEK ---
        m_TypewriterTimer += ts.GetSeconds();

        if (m_TypewriterTimer < 5.5f) {
            GameManagerScript::s_ShowTutorialDialog = true;
            GameManagerScript::s_TutorialSpeaker = "";
            GameManagerScript::s_TutorialDialogIsBottom = true;
            GameManagerScript::s_TutorialIconAlpha = 0.0f;

            int stage = (int)(m_TypewriterTimer / 0.8f);
            std::string offset = "                        ";
            std::string waitingText = "";

            if (stage == 0 || stage == 3) waitingText = offset + ".";
            else if (stage == 1 || stage == 4) waitingText = offset + ". .";
            else if (stage == 2 || stage == 5) waitingText = offset + ". . .";
            else waitingText = "";

            GameManagerScript::s_TutorialText = waitingText;
            GameManagerScript::s_TutorialCharsRevealed = waitingText.length();
        }
        else {
            GameManagerScript::s_ShowTutorialDialog = false;
        }

        // --- 9. WARUNEK PRZEJŒCIA DALEJ ---
        bool hasIngredient = false;
        auto* nscPot = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Pot);
        if (nscPot) {
            for (auto& s : nscPot->Scripts) {
                if (s.Name == "PotScript" && s.Instance) {
                    auto* potScript = static_cast<PotScript*>(s.Instance);
                    if (!potScript->m_Ingredients.empty()) {
                        hasIngredient = true;
                    }
                }
            }
        }

        if (hasIngredient) {
            // Gasimy wszystko
            TriggerHighlightEvent ev;
            ev.TargetEntity = m_Pot;
            ev.Color = glm::vec3(0.0f);
            ev.Duration = 0.1f;
            ev.IsInfinite = false;
            GetScene()->GetWorld().GetEventBus().Publish(ev);

            if (closestPlate.id != std::numeric_limits<std::size_t>::max()) {
                TriggerHighlightEvent evPlate;
                evPlate.TargetEntity = closestPlate;
                evPlate.Color = glm::vec3(0.0f);
                evPlate.Duration = 0.1f;
                evPlate.IsInfinite = false;
                GetScene()->GetWorld().GetEventBus().Publish(evPlate);
            }

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
            potHoverLerp = 0.0f;
            plateHoverLerp = 0.0f;
            transitionLerp = 0.0f;
        }

        // --- 1. POBRANIE SKRYPTU GARNKA ---
        auto* potTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_Pot);
        auto* nscPot = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Pot);
        PotScript* potScript = nullptr;
        if (nscPot) {
            for (auto& s : nscPot->Scripts) {
                if (s.Name == "PotScript" && s.Instance) {
                    potScript = static_cast<PotScript*>(s.Instance);
                }
            }
        }

        if (!potScript) break;

        // --- 2. ZABÓJCA CRASHÓW (Warunek sukcesu na samym szczycie klatki!) ---
        // Oddaliœmy klikanie Twojej natywnej grze. Jeœli klikniesz i gra przeleje zupê,
        // maszyna przestanie byæ gotowa i wyczyœci sk³adniki. Zauwa¿amy to w u³amek sekundy!
        if (!potScript->m_IsReady && potScript->m_Ingredients.empty()) {

            // Szukamy pobliskiego talerza, ¿eby radoœnie b³ysn¹æ nim na zielono
            Entity successPlate = { std::numeric_limits<std::size_t>::max(), 0 };
            float closestD = 999.0f;
            auto* tags = GetScene()->GetWorld().GetComponentVector<TagComponent>();
            if (tags && potTf) {
                for (size_t i = 0; i < tags->dense.size(); ++i) {
                    if (tags->dense[i].Tag.find("Plate") != std::string::npos || tags->dense[i].Tag.find("Talerz") != std::string::npos) {
                        Entity p = tags->reverse[i];
                        auto* pTf = GetScene()->GetWorld().GetComponent<TransformComponent>(p);
                        if (pTf) {
                            float d = glm::distance(pTf->GetPosition(), potTf->GetPosition());
                            // Talerz musi byæ blisko garnka
                            if (d < closestD && d < 3.5f) {
                                closestD = d;
                                successPlate = p;
                            }
                        }
                    }
                }
            }

            // B³yskamy na zielono TYLKO bezpiecznym talerzem. 
            // Zupy nie dotykamy, bo natywny silnik ju¿ usun¹³ z niej encjê (to powodowa³o zamra¿anie gry)!
            if (successPlate.id != std::numeric_limits<std::size_t>::max()) {
                TriggerHighlightEvent evPlateGreen;
                evPlateGreen.TargetEntity = successPlate;
                evPlateGreen.Color = glm::vec3(0.1f, 1.0f, 0.2f);
                evPlateGreen.Duration = 1.5f;
                evPlateGreen.IsInfinite = false;
                GetScene()->GetWorld().GetEventBus().Publish(evPlateGreen);
            }

            GameManagerScript::s_ShowTutorialDialog = false;

            m_State = TutorialState::WaitForDelivery;
            m_StateTimer = 0.0f;

            break; // NATYCHMIAST ucinamy dzia³anie kodu w tej klatce. Brak szans na b³¹d pamiêci.
        }

        // --- 3. CZY ZUPA WCI¥¯ SIÊ GOTUJE? ---
        if (!potScript->m_IsReady) {
            GameManagerScript::s_ShowTutorialDialog = false;
            break;
        }

        // === ZUPA JEST GOTOWA, WCHODZIMY W BEZPIECZNE PODŒWIETLANIE ===

        // --- 4. TARCZA OCHRONNA DLA PAMIÊCI ---
        // Skanujemy absolutnie ca³¹ pamiêæ gry. Jeœli encja zupy wyparowa³a - omijamy!
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

        // --- 5. ZNAJDOWANIE NAJBLI¯SZEGO TALERZA (Wersja z dzia³aj¹cego kodu) ---
        Entity closestPlate = { std::numeric_limits<std::size_t>::max(), 0 };
        float closestDist = 999.0f;
        if (tags && potTf) {
            for (size_t i = 0; i < tags->dense.size(); ++i) {
                const std::string& tag = tags->dense[i].Tag;
                if (tag.find("Plate") != std::string::npos || tag.find("Talerz") != std::string::npos) {
                    Entity p = tags->reverse[i];
                    auto* pTf = GetScene()->GetWorld().GetComponent<TransformComponent>(p);
                    if (pTf) {
                        float d = glm::distance(pTf->GetPosition(), potTf->GetPosition());
                        if (d < closestDist) {
                            closestDist = d;
                            closestPlate = p;
                        }
                    }
                }
            }
        }

        // --- 6. ZASIÊG (3.5f zapewnia bezpieczny margines na reakcjê UI) ---
        bool inRange = false;
        auto* plateTf = closestPlate.id != std::numeric_limits<std::size_t>::max() ? GetScene()->GetWorld().GetComponent<TransformComponent>(closestPlate) : nullptr;

        if (plateTf && potTf) {
            float distToPot = glm::distance(
                glm::vec2(potTf->GetPosition().x, potTf->GetPosition().z),
                glm::vec2(plateTf->GetPosition().x, plateTf->GetPosition().z)
            );
            if (distToPot < 3.5f) {
                inRange = true;
            }
        }

        // --- 7. RZUTOWANIE MYSZKI W 3D ---
        glm::vec3 floorMousePos = GetMouseWorldPosition();
        auto* camera = GetScene()->GetCamera();
        glm::vec3 preciseMousePos = floorMousePos;

        if (camera && potTf) {
            float targetY = potTf->GetPosition().y;
            glm::vec3 rayDir = camera->Front;
            if (std::abs(rayDir.y) > 0.001f) {
                float t = (targetY - floorMousePos.y) / rayDir.y;
                preciseMousePos = floorMousePos + rayDir * t;
            }
        }

        glm::vec2 mouse2D = { preciseMousePos.x, preciseMousePos.z };
        bool isHoveringSoup = false;
        bool isHoveringPlate = false;

        if (!Input::IsUICapturingMouse()) {
            if (potTf && glm::distance(mouse2D, glm::vec2(potTf->GetPosition().x, potTf->GetPosition().z)) < 1.5f) isHoveringSoup = true;
            if (plateTf && glm::distance(mouse2D, glm::vec2(plateTf->GetPosition().x, plateTf->GetPosition().z)) < 1.5f) isHoveringPlate = true;
        }

        // --- 8. P£YNNE KOLORY (Bia³y -> Ró¿owy -> Z³oty) ---
        if (isHoveringSoup && inRange) potHoverLerp += ts.GetSeconds() * 8.0f;
        else potHoverLerp -= ts.GetSeconds() * 8.0f;
        potHoverLerp = std::clamp(potHoverLerp, 0.0f, 1.0f);

        if (isHoveringPlate && inRange) plateHoverLerp += ts.GetSeconds() * 8.0f;
        else plateHoverLerp -= ts.GetSeconds() * 8.0f;
        plateHoverLerp = std::clamp(plateHoverLerp, 0.0f, 1.0f);

        if (inRange) transitionLerp += ts.GetSeconds() * 3.0f;
        else transitionLerp -= ts.GetSeconds() * 3.0f;
        transitionLerp = std::clamp(transitionLerp, 0.0f, 1.0f);

        glm::vec3 baseGray = glm::vec3(0.4f, 0.4f, 0.4f);
        glm::vec3 basePink = glm::vec3(1.0f, 0.2f, 0.6f);
        glm::vec3 hoverGold = glm::vec3(1.0f, 0.9f, 0.0f);

        glm::vec3 activePotColor = glm::mix(basePink, hoverGold, potHoverLerp);
        glm::vec3 activePlateColor = glm::mix(basePink, hoverGold, plateHoverLerp);

        glm::vec3 soupColor = glm::mix(baseGray, activePotColor, transitionLerp);
        float soupDuration = glm::mix(32.0f, 8.0f, transitionLerp);

        // --- 9. EVENTY WIZUALNE ---
        // Nak³adamy kolor TYLKO, jeœli tarcza ochronna potwierdzi³a istnienie zupy!
        if (isSoupValid) {
            TriggerHighlightEvent evSoup;
            evSoup.TargetEntity = spawnedSoup;
            evSoup.Color = soupColor;
            evSoup.Duration = soupDuration;
            evSoup.IsInfinite = true;
            GetScene()->GetWorld().GetEventBus().Publish(evSoup);
        }

        if (inRange && plateTf) {
            TriggerHighlightEvent evPlate;
            evPlate.TargetEntity = closestPlate;
            evPlate.Color = activePlateColor;
            evPlate.Duration = 8.0f;
            evPlate.IsInfinite = true;
            GetScene()->GetWorld().GetEventBus().Publish(evPlate);
        }

        // --- UWAGA: BRAK LOGIKI KLIKANIA! ---
        // Zauwa¿, ¿e nie ma tu w ogóle sprawdzania wciskania klawiszy myszki. 
        // Twój wbudowany silnik sam rozpozna myszkê, zdejmie zupê i wywo³a sukces!

        // --- 10. OBS£UGA UI KROPEK ---
        m_TypewriterTimer += ts.GetSeconds();

        if (m_TypewriterTimer < 5.5f) {
            GameManagerScript::s_ShowTutorialDialog = true;
            GameManagerScript::s_TutorialSpeaker = "";
            GameManagerScript::s_TutorialDialogIsBottom = true;
            GameManagerScript::s_TutorialIconAlpha = 0.0f;

            int stage = (int)(m_TypewriterTimer / 0.8f);
            std::string offset = "                        ";
            std::string waitingText = "";

            if (stage == 0 || stage == 3) waitingText = offset + ".";
            else if (stage == 1 || stage == 4) waitingText = offset + ". .";
            else if (stage == 2 || stage == 5) waitingText = offset + ". . .";
            else waitingText = "";

            GameManagerScript::s_TutorialText = waitingText;
            GameManagerScript::s_TutorialCharsRevealed = waitingText.length();
        }
        else {
            GameManagerScript::s_ShowTutorialDialog = false;
        }

        break;
    }

    default:
        break;
    }
}