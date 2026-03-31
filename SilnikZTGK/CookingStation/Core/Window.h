#pragma once
#include <string>
#include <functional>

#include "glad/glad.h" 
#include <GLFW/glfw3.h>

#include "CookingStation/Events/Event.h"


class Window
{
public:

	using EventCallbackFn = std::function<void(Event&)>;

	Window(unsigned int screenWidth, unsigned int screenHeight, const std::string name = "Window");
	~Window();
	void Init();
	bool ShouldClose();
	void OnUpdate();

	unsigned int GetWidth() const { return screenWidth; }
	unsigned int GetHeight() const { return screenHeight; }

	void SetEventCallback(const EventCallbackFn& callback);


	static void KeyCallBack(GLFWwindow* window, int key, int scancode, int action, int mods);
	void ProcessKeyInput(int key, int action);

	static void MouseMoveCallback(GLFWwindow* window, double xPos, double yPos);
	void ProcessMouseMovement(double xPos, double yPos);

	static void MouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset);
	void ProcessMouseScroll(double xOffset, double yOffset);

	static void MouseButtonPressedCallback(GLFWwindow* window, int button, int action, int mods);
	void ProcessMouseButtonPress(int button, int action, int mods);

	static void WindowCloseCallback(GLFWwindow* window);
	void ProcessWindowClose();

	static void WindowResizeCallback(GLFWwindow* window, int width, int height);
	void ProcessWindowResize(int width, int height);

	static void CharCallback(GLFWwindow* window, unsigned int keycode);
	void ProcessCharInput(unsigned int keycode);

	//statyczny dostep do okna dla inputow
	inline GLFWwindow* GetNativeWindow() const { return window; };
private:
	GLFWwindow* window;
	unsigned int screenWidth;
	unsigned int screenHeight;
	const std::string screenName;

	EventCallbackFn m_EventCallbackFn;
};