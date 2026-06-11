#include "Input.h"
#include "Application.h"
#include "CookingStation/Events/GamepadEvent.h"	

#include <GLFW/glfw3.h>

bool Input::s_CurrentMouseStates[8] = { false };
bool Input::s_PreviousMouseStates[8] = { false };

bool Input::s_CurrentGamepadStates[32] = { false };
bool Input::s_PreviousGamepadStates[32] = { false };

float Input::s_CurrentGamepadAxes[6] = { 0.0f };
float Input::s_PreviousGamepadAxes[6] = { 0.0f };

bool Input::s_UICapturesMouse = false;


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
	for (int i = 0; i < 8; i++)
	{
		s_PreviousMouseStates[i] = s_CurrentMouseStates[i];
		s_CurrentMouseStates[i] = IsMouseButtonPressed(i);
	}

	if (IsGamepadPresent(0))
	{
		GLFWgamepadstate state;
		if (glfwGetGamepadState(0, &state))
		{
			for (int i = 0; i <= GLFW_GAMEPAD_BUTTON_DPAD_LEFT; i++)
			{
				s_PreviousGamepadStates[i] = s_CurrentGamepadStates[i];
				s_CurrentGamepadStates[i] = state.buttons[i] == GLFW_PRESS;

				if (s_CurrentGamepadStates[i] && !s_PreviousGamepadStates[i])
				{
					GamepadButtonPressedEvent event(i, 0);
					Application::Get().OnEvent(event);
					Application::Get().GetEventBus().Publish(event);
				}
				else if (!s_CurrentGamepadStates[i] && s_PreviousGamepadStates[i])
				{
					GamepadButtonReleasedEvent event(i, 0);
					Application::Get().OnEvent(event);
					Application::Get().GetEventBus().Publish(event);
				}
			}

			for (int i = 0; i <= GLFW_GAMEPAD_AXIS_LAST; i++)
			{
				s_PreviousGamepadAxes[i] = s_CurrentGamepadAxes[i];
				s_CurrentGamepadAxes[i] = state.axes[i];

				if (std::abs(s_CurrentGamepadAxes[i] - s_PreviousGamepadAxes[i]) > 0.05f)
				{
					GamepadAxisMovedEvent event(i, s_CurrentGamepadAxes[i], 0);
					Application::Get().OnEvent(event);
					Application::Get().GetEventBus().Publish(event);
				}
			}
		}
	}
}

bool Input::IsMouseButtonJustPressed(int button) {
	return s_CurrentMouseStates[button] && !s_PreviousMouseStates[button];
}

bool Input::IsMouseButtonJustReleased(int button) {
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

