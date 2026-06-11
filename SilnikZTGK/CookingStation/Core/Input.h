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

    static bool IsGamepadPresent(int gamepadId = 0); 
    static const char* GetGamepadName(int gamepadId = 0);
    static bool IsGamepadButtonPressed(int button, int gamepadId = 0);
    static bool IsGamepadButtonJustPressed(int button, int gamepadId = 0);
    static bool IsGamepadButtonJustReleased(int button, int gamepadId = 0);
    static float GetGamepadAxis(int axis, int gamepadId = 0);

    static bool IsUICapturingMouse() { return s_UICapturesMouse; }
    static void SetUICaptureMouse(bool state) { s_UICapturesMouse = state; }

    static void Update();

private:
    static bool s_CurrentMouseStates[8];
    static bool s_PreviousMouseStates[8];

    static bool s_UICapturesMouse;

    static bool s_CurrentGamepadStates[32];
    static bool s_PreviousGamepadStates[32];

    static float s_CurrentGamepadAxes[6];
    static float s_PreviousGamepadAxes[6];
};
