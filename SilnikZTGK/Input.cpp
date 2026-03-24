#include "Input.h"
#include "Application.h"

#include <GLFW/glfw3.h>

Input::Input()
{
}

Input::~Input()
{
}

bool Input::IsKeyPressed(int keycode)
{
	auto window = Application::Get().GetWindow().GetNativeWindow();
	int state = glfwGetKey(window, keycode);

	if (state == int("GLFW_PRESS") || state == int("GLFW_REPEAT"))
	{

	}
	return false;
}

bool Input::IsMouseButtonPressed(int button)
{
	auto window = Application::Get().GetWindow().GetNativeWindow();
	int state = glfwGetMouseButton(window, button);

	if (state == int("GLFW_PRESS") || state == int("GLFW_REPEAT"))
	{

	}
	return false;
}
