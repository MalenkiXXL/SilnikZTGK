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
        // --- NOWOŒÆ: Logika Hovera dla samej Skrzynki! ---
        static float crateHoverLerp = 0.0f;
        if (m_StateTimer < 0.05f) {
            crateHoverLerp = 0.0f;
        }

        bool isHoveringCrate = false;
        auto* crateTf = GetScene()->GetWorld().GetComponent<TransformComponent>(m_TomatoCrate);
        if (crateTf && !Input::IsUICapturingMouse()) {
            glm::vec3 mousePos = GetMouseWorldPosition();
            glm::vec2 mouse2D = { mousePos.x, mousePos.z };
            if (glm::distance(mouse2D, glm::vec2(crateTf->GetPosition().x, crateTf->GetPosition().z)) < 4.0f) {
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

        // P³ynne przejœcie z "Oczekuj¹cego Ró¿u" na "Aktywne Z³oto"
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
            // Gasimy podœwietlenie ca³kowicie po pomyœlnym wrzuceniu na taœmê
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
        // --- NOWOŒÆ: Rozdzielenie zmiennych na dwa osobne obiekty ---
        static float tomatoHoverLerp = 0.0f;
        static float boardHoverLerp = 0.0f;
        static float boardTransitionLerp = 0.0f; // Szary -> Ró¿owy

        // Niezawodny reset na starcie stanu
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

        // --- 2. LOGIKA MYSZKI I PRZEJŒCIE KOLORÓW (Osobno dla Deski i Pomidora!) ---
        glm::vec3 mousePos = GetMouseWorldPosition();
        glm::vec2 mouse2D = { mousePos.x, mousePos.z };

        bool isHoveringTomato = false;
        bool isHoveringBoard = false;

        if (!Input::IsUICapturingMouse()) {
            // ZMNIEJSZONO PROMIEÑ z 4.0f na 2.5f, ¿eby hover puszcza³ b³yskawicznie po zjechaniu myszk¹!
            if (tomatoTf && glm::distance(mouse2D, glm::vec2(tomatoTf->GetPosition().x, tomatoTf->GetPosition().z)) < 2.5f) isHoveringTomato = true;
            if (boardTf && glm::distance(mouse2D, glm::vec2(boardTf->GetPosition().x, boardTf->GetPosition().z)) < 2.5f) isHoveringBoard = true;
        }

        // Interpolacja koloru pomidora
        if (isHoveringTomato && inRange) tomatoHoverLerp += ts.GetSeconds() * 8.0f;
        else tomatoHoverLerp -= ts.GetSeconds() * 8.0f;
        tomatoHoverLerp = std::clamp(tomatoHoverLerp, 0.0f, 1.0f);

        // Interpolacja koloru deski
        if (isHoveringBoard && inRange) boardHoverLerp += ts.GetSeconds() * 8.0f;
        else boardHoverLerp -= ts.GetSeconds() * 8.0f;
        boardHoverLerp = std::clamp(boardHoverLerp, 0.0f, 1.0f);

        glm::vec3 basePink = glm::vec3(1.0f, 0.2f, 0.6f);
        glm::vec3 hoverGold = glm::vec3(1.0f, 0.9f, 0.0f);

        glm::vec3 currentTomatoColor = glm::mix(basePink, hoverGold, tomatoHoverLerp);
        glm::vec3 currentBoardInteractionColor = glm::mix(basePink, hoverGold, boardHoverLerp);


        // --- 3. P£YNNE PRZEJŒCIE KOLORU DESKI (Szary -> Ró¿owy/Z³oty) ---
        if (inRange) boardTransitionLerp += ts.GetSeconds() * 3.0f;
        else boardTransitionLerp -= ts.GetSeconds() * 3.0f;
        boardTransitionLerp = std::clamp(boardTransitionLerp, 0.0f, 1.0f);

        glm::vec3 grayColor = glm::vec3(0.4f, 0.4f, 0.4f);
        // Do deski ³adujemy wyliczony dla NIEJ kolor interakcji
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
            evTomato.Color = currentTomatoColor; // £adujemy tylko kolor wyliczony dla pomidora!
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

        // Gracz mo¿e klikn¹æ jeœli namierzy³ pomidora LUB namierzy³ deskê
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
        // Niezawodny reset timera przy wejœciu do stanu
        if (m_StateTimer < 0.05f) {
            GameManagerScript::s_TutorialCharsRevealed = 0;
        }

        // --- 1. HIGHLIGHT DESKI ---
        // Deska ca³y czas lœni na z³oto, co informuje gracza o koniecznoœci interakcji
        TriggerHighlightEvent evBoard;
        evBoard.TargetEntity = m_Board;
        evBoard.Color = glm::vec3(1.0f, 0.9f, 0.0f);
        evBoard.Duration = 8.0f;
        evBoard.IsInfinite = true;
        GetScene()->GetWorld().GetEventBus().Publish(evBoard);

        // --- 2. ODCZYT DANYCH Z MASZYNY W CZASIE RZECZYWISTYM ---
        int currentChops = 0;
        bool isReady = false;

        auto* nsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_Board);
        if (nsc) {
            for (auto& s : nsc->Scripts) {
                if (s.Name == "CuttingBoardScript" && s.Instance) {
                    auto* boardScript = static_cast<CuttingBoardScript*>(s.Instance);
                    currentChops = boardScript->m_ChopCount; // Pobieramy na ¿ywo aktualn¹ wartoœæ z deski!
                    isReady = boardScript->m_IsReady;
                }
            }
        }

        // --- 3. OBS£UGA UI (Dynamiczny tekst i pulsuj¹ca myszka) ---
        GameManagerScript::s_ShowTutorialDialog = true;
        GameManagerScript::s_TutorialSpeaker = "";
        GameManagerScript::s_TutorialDialogIsBottom = true;

        // Pulsuj¹ca animacja ikony klikniêcia myszki (od 0% do 100% przeziernoœci z u¿yciem funkcji Sinus)
        // Szybkoœæ pulsowania (8.0f) mo¿esz ³atwo dostosowaæ!
        GameManagerScript::s_TutorialIconAlpha = (std::sin(m_StateTimer * 8.0f) + 1.0f) * 0.5f;

        // U¿ywamy tego samego offsetu co przy kropkach, ¿eby UI w tutorialu trzyma³o równy layout
        std::string offset = "                        ";
        std::string text = offset + "Krojenie: " + std::to_string(currentChops) + " / 3";

        GameManagerScript::s_TutorialText = text;
        GameManagerScript::s_TutorialCharsRevealed = text.length();

        // --- 4. WARUNEK PRZEJŒCIA DALEJ ---
        if (isReady) {
            // Natychmiastowe ugaszenie tutorialowego podœwietlenia
            TriggerHighlightEvent ev;
            ev.TargetEntity = m_Board;
            ev.Color = glm::vec3(0.0f);
            ev.Duration = 0.1f;
            ev.IsInfinite = false;
            GetScene()->GetWorld().GetEventBus().Publish(ev);

            // Wy³¹czenie i sprz¹tniêcie UI
            GameManagerScript::s_ShowTutorialDialog = false;
            GameManagerScript::s_TutorialIconAlpha = 0.0f;

            // Przejœcie do pakowania na talerz
            m_State = TutorialState::WaitForPlateTransfer;
            m_StateTimer = 0.0f;
        }
        break;
    }

    default:
        break;
    }
}