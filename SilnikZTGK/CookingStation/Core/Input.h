#pragma once

#include <utility>

class Input
{
public:
	static bool IsKeyPressed(int keycode);
	static bool IsMouseButtonPressed(int button);
	static std::pair<float, float> GetMousePosition();
	static std::pair<float, float> GetWindowSize();
    static bool IsMouseButtonJustPressed(int button);
    static bool IsMouseButtonJustReleased(int button);

    // --- Obs³uga Gamepada ---
    static bool IsGamepadPresent(int gamepadId = 0); // 0 to domyœlnie pierwszy pad
    static const char* GetGamepadName(int gamepadId = 0);
    static bool IsGamepadButtonPressed(int button, int gamepadId = 0);
    static bool IsGamepadButtonJustPressed(int button, int gamepadId = 0);
    static bool IsGamepadButtonJustReleased(int button, int gamepadId = 0);
    static float GetGamepadAxis(int axis, int gamepadId = 0);

    // Tê funkcjê bêdziesz musia³ wywo³ywaæ raz na klatkê!
    static void Update();

private:
    static bool s_CurrentMouseStates[8];
    static bool s_PreviousMouseStates[8];

    // Gamepady standardowo maj¹ oko³o 15-20 przycisków
    static bool s_CurrentGamepadStates[32];
    static bool s_PreviousGamepadStates[32];
};
