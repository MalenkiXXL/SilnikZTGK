#include "Input.h"
#include "Application.h"

#include <GLFW/glfw3.h>

bool Input::s_CurrentMouseStates[8] = { false };
bool Input::s_PreviousMouseStates[8] = { false };


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
	for (int i = 0; i < 8; i++) {
		s_PreviousMouseStates[i] = s_CurrentMouseStates[i];

		// Pobieramy aktualny stan z GLFW (lub Twojego systemu)
		// Zak³adam, ¿e Twoja funkcja IsMouseButtonPressed dzia³a i ³¹czy siê z systemem okien
		s_CurrentMouseStates[i] = IsMouseButtonPressed(i);
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

