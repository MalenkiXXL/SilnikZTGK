#pragma once
#include "../Utils/IUIPanel.h"
#include <vector>
#include <string>

class CreditsPanel : public IUIPanel {
public:
    CreditsPanel() = default;

    virtual void OnUpdate(float dt) override;
    virtual void Draw(float baseScale) override;

private:
    struct CreditEntry {
        std::string Name;
        std::string Role;
    };

    std::vector<CreditEntry> m_Credits = {
        { "Aleksandra Jakobik", "Leader, UX/UI Designer, Game Loop Programmer" },
        { "Amelia Garnys",       "Core Gameplay Programmer" },
        { "Oskar Konecki",       "Gameplay & Systems Architect" },
        { "Adrian Matczak",      "Core Engine Programmer" },
        { "Klaudia Adamek",      "Lead 3D Artist & Animator" },
        { "Emilia Szczerba",     "Support 3D Artist" },
    };

    std::string m_GameTitlePlaceholder = "Cook Me Back";
    std::string m_VersionPlaceholder = "v0.1.0 ";
    std::string m_FooterPlaceholder = "Made with care by the Catering Team";

    std::string m_CompetitionInfoPlaceholder = "The game was a submission to ZTGK 2026 in Game Development category.";
    std::string m_FeedbackCallPlaceholder = "Help us shape the future of the game! We'd love to hear your thoughts and suggestions in the comments.";
    std::string m_ContactLabelPlaceholder = "Contact info:";
    std::string m_ContactDetailsPlaceholder = "[ e-mail / Discord / social media ]";

    float m_BackBtnScale = 1.0f;
    float m_DeltaTime = 0.0f;
};