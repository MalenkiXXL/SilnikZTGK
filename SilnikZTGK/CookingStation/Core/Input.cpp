#include "Input.h"
#include "Application.h"

#include <GLFW/glfw3.h>

bool Input::s_CurrentMouseStates[8] = { false };
bool Input::s_PreviousMouseStates[8] = { false };

bool Input::s_CurrentGamepadStates[32] = { false };
bool Input::s_PreviousGamepadStates[32] = { false };


bool Input::IsKeyPressed(int keycode)
{
	auto window = Application::Get().GetWindow().GetNativeWindow();
	int state = glfwGetKey(window, keycode);

	if (state == GLFW_PRESS || state == GLFW_REPEAT)
	{
		return true;
	}
	return false;
}

void Input::Update() {
	// Kopiujemy obecny stan do tablicy poprzedniej klatki
	for (int i = 0; i < 8; i++) 
	{
		s_PreviousMouseStates[i] = s_CurrentMouseStates[i];
		s_CurrentMouseStates[i] = IsMouseButtonPressed(i);
	}

	// Aktualizacja stanu gamepada
	if (IsGamepadPresent(0)) 
	{
		GLFWgamepadstate state;
		if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state))	// GLFW_JOYSTICK_1 ma wartoœæ 0
		{ 
			for (int i = 0; i < 32; i++) 
			{
				if (i <= GLFW_GAMEPAD_BUTTON_DPAD_LEFT)
				{
					// Zakres standardowych przycisków GLFW
					s_PreviousGamepadStates[i] = s_CurrentGamepadStates[i];
					s_CurrentGamepadStates[i] = state.buttons[i] == GLFW_PRESS;
				}
			}
		}
	}
}

bool Input::IsMouseButtonJustPressed(int button) {
	// Zwraca true TYLKO jeœli w tej klatce jest wciœniêty, a w poprzedniej nie by³
	return s_CurrentMouseStates[button] && !s_PreviousMouseStates[button];
}

bool Input::IsMouseButtonJustReleased(int button) {
	// Zwraca true TYLKO jeœli w tej klatce jest PUSZCZONY, a w poprzedniej by³ wciœniêty
	return !s_CurrentMouseStates[button] && s_PreviousMouseStates[button];
}

bool Input::IsMouseButtonPressed(int button)
{
	auto window = Application::Get().GetWindow().GetNativeWindow();
	int state = glfwGetMouseButton(window, button);

	if (state == GLFW_PRESS)
	{
		return true;
	}
	return false;
}

bool Input::IsGamepadPresent(int gamepadId) {
	// GLFW obs³uguje joystiki od GLFW_JOYSTICK_1 (0) do GLFW_JOYSTICK_16 (15)
	// glfwJoystickIsGamepad sprawdza, czy wykryty sprzêt to zmapowany gamepad (np. pad PS4/Xbox)
	return glfwJoystickPresent(gamepadId) && glfwJoystickIsGamepad(gamepadId);
}

const char* Input::GetGamepadName(int gamepadId) {
	if (IsGamepadPresent(gamepadId)) {
		return glfwGetGamepadName(gamepadId);
	}
	return "Disconnected";
}

bool Input::IsGamepadButtonPressed(int button, int gamepadId) {
	if (IsGamepadPresent(gamepadId)) {
		GLFWgamepadstate state;
		if (glfwGetGamepadState(gamepadId, &state)) {
			return state.buttons[button] == GLFW_PRESS;
		}
	}
	return false;
}

bool Input::IsGamepadButtonJustPressed(int button, int gamepadId) {
	if (gamepadId == 0) {
		return s_CurrentGamepadStates[button] && !s_PreviousGamepadStates[button];
	}
	return false;
}

bool Input::IsGamepadButtonJustReleased(int button, int gamepadId) {
	if (gamepadId == 0) {
		return !s_CurrentGamepadStates[button] && s_PreviousGamepadStates[button];
	}
	return false;
}

float Input::GetGamepadAxis(int axis, int gamepadId) {
	if (IsGamepadPresent(gamepadId)) {
		GLFWgamepadstate state;
		if (glfwGetGamepadState(gamepadId, &state)) {
			return state.axes[axis];
		}
	}
	return 0.0f;
}

std::pair<float, float> Input::GetMousePosition()
{
	auto window = Application::Get().GetWindow().GetNativeWindow();
	double xpos, ypos;

	glfwGetCursorPos(window, &xpos, &ypos);
	return { (float)xpos, (float)ypos };
}

std::pair<float, float> Input::GetWindowSize() {
	auto window = Application::Get().GetWindow().GetNativeWindow();
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	return { (float)width, (float)height };
}

