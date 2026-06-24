#pragma once
#include "CookingStation/Core/Texture.h"
#include "CookingStation/Scene/Scene.h"
#include "CookingStation/Scene/Entity.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>

class BuildModePanel {
public:
    void Init(std::shared_ptr<Texture> coinIcon);

    void DrawButton(float gameX, float gameY, float gameW, float gameH, float baseScale, float dt, bool isBlocked);
    void DrawPanel(float gameX, float gameY, float gameW, float gameH, float baseScale, float dt);

    // Zaktualizowane sygnatury - teraz w 100% zgadzają się z .cpp!
    void DrawOverlay(float gameX, float gameY, float gameW, float gameH, float baseScale);
    void DrawGrid(const glm::mat4& viewProj3D, const glm::vec3& camPos, const glm::vec3& hoverPos, int hoverState, float gameX, float gameY, float gameW, float gameH);
    void DrawActiveGrid(std::shared_ptr<Scene>& activeScene, float gameX, float gameY, float gameW, float gameH, float baseScale);

    void UpdatePlacement(std::shared_ptr<Scene>& activeScene, float gameX, float gameY, float gameW, float gameH, float baseScale);

    void Activate();
    void Deactivate();
    void ForceReset();

    void Toggle() { if (m_IsActive) Deactivate(); else Activate(); }
    bool IsActive() const { return m_IsActive; }

private:
    struct MachineEntry {
        std::string Label;
        std::string PrefabPath;
        std::shared_ptr<Texture> Icon;
        int Price;
    };

    bool m_IsActive = false;
    float m_SlideY = 0.0f;
    float m_ButtonScale = 1.0f;
    int m_HeldMachineIndex = -1;
    bool m_JustSelectedFromPanel = false;

    // --- NOWE ZMIENNE DO PODPOWIEDZI ---
    float m_GameTime = 0.0f;
    float m_BuildHintTimer = 0.0f;
    bool m_HasShownBuildHint = false;
    // -----------------------------------

    std::shared_ptr<Texture> m_CoinIcon;
    std::shared_ptr<Scene> m_CurrentScene;

    std::shared_ptr<Texture> m_LeftMouseIcon;
    std::shared_ptr<Texture> m_RightMouseIcon;
    std::shared_ptr<Texture> m_TabIcon;

    std::vector<MachineEntry> m_MachineEntries;
    std::vector<std::pair<Entity, glm::vec3>> m_PreviewGroup;
    Entity m_MovingMachineEntity = { std::numeric_limits<std::size_t>::max(), 0 };
    glm::vec3 m_MovingMachineOriginalPos = glm::vec3(0.0f);
    std::vector<std::pair<Entity, glm::vec3>> m_MovingGroup;

    bool IsPlacementValid(std::shared_ptr<Scene>& activeScene, const glm::vec3& snappedPos);
    int GetCellState(std::shared_ptr<Scene>& activeScene, const glm::vec3& snappedPos, Entity& outMachine);
};