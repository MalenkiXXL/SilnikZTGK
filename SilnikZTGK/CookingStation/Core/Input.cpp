#include "Input.h"
#include "Application.h"

#include <GLFW/glfw3.h>

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

