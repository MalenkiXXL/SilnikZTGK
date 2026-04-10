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

    // Tê funkcjê bêdziesz musia³ wywo³ywaæ raz na klatkê!
    static void Update();

private:
    static bool s_CurrentMouseStates[8];
    static bool s_PreviousMouseStates[8];
};
