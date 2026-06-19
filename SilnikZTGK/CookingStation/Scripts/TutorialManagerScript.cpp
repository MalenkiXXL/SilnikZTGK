#include "TutorialManagerScript.h"
#include "CookingStation/Scripts/Managers/GameManagerScript.h"
#include "CookingStation/Events/GameEvents.h"
#include "CookingStation/Core/Input.h" 
#include "CookingStation/Scripts/PoofEmitterScript.h"
#include "CookingStation/Scripts/ParticleEmitterScript.h"
#include "CookingStation/Scripts/CrateScript.h" 
#include <algorithm>
#include <limits> 

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

            TriggerHighlightEvent ev;
            ev.TargetEntity = m_TomatoCrate;
            ev.Color = glm::vec3(1.0f, 0.8f, 0.0f);
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
                            TriggerHighlightEvent evFood;
                            evFood.TargetEntity = crateScript->m_VisualFood;
                            evFood.Color = glm::vec3(1.0f, 0.9f, 0.0f);
                            // ZMIANA: Wolniejsze przejœcie animacji (cykl 2 sekundy zamiast natychmiastowego)
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

        auto* crateNsc = GetScene()->GetWorld().GetComponent<NativeScriptComponent>(m_TomatoCrate);
        if (crateNsc) {
            for (auto& s : crateNsc->Scripts) {
                if (s.Name == "CrateScript" && s.Instance) {
                    auto* crateScript = static_cast<CrateScript*>(s.Instance);
                    if (crateScript->m_VisualFood.id != std::numeric_limits<std::size_t>::max()) {
                        TriggerHighlightEvent evFood;
                        evFood.TargetEntity = crateScript->m_VisualFood;
                        evFood.Color = glm::vec3(1.0f, 0.9f, 0.0f);
                        // ZMIANA: Zsynchronizowane wolniejsze tempo pulsowania
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

            m_State = TutorialState::WaitForPotPlacement;
            m_StateTimer = 0.0f;
        }
        break;
    }

    default:
        break;
    }
}