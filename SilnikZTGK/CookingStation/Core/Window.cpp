#include "Window.h"
#include <iostream>

#include "CookingStation/Events/KeyEvent.h"
#include "CookingStation/Events/WindowEvent.h"
#include "CookingStation/Events/MouseEvent.h"

Window::Window(unsigned int width, unsigned int height, const std::string name)
	: screenWidth(width), screenHeight(height), screenName(name)
{
}

Window::~Window()
{
    glfwTerminate();
}

void Window::Init()
{
	//konfiguracja glfw
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//Inizjalizowanie okna
    window = glfwCreateWindow(screenWidth, screenHeight, screenName.c_str(), NULL, NULL);
	glfwMakeContextCurrent(window);

    glfwSetWindowUserPointer(   window, this);
    

    //// 3. Rejestracja funkcji reaguj¹cych na okno i myszkê
    //glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    //// Mówimy GLFW, ¿eby przechwyci³ i ukry³ nasz kursor myszy (tryb FPS)
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //// 4. Inicjalizacja GLAD (³aduje wskaŸniki do funkcji OpenGL dla naszej karty graficznej)
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);


    glfwSetKeyCallback(window, KeyCallBack);
    glfwSetCharCallback(window, CharCallback);
    glfwSetCursorPosCallback(window, MouseMoveCallback);
    glfwSetScrollCallback(window, MouseScrollCallback);
    glfwSetMouseButtonCallback(window, MouseButtonPressedCallback);
    glfwSetWindowCloseCallback(window, WindowCloseCallback);
    glfwSetWindowSizeCallback(window, WindowResizeCallback);

}

bool Window::ShouldClose()
{
    return glfwWindowShouldClose(window);
}

void Window::OnUpdate()
{
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void Window::SetEventCallback(const EventCallbackFn& callback)
{
    m_EventCallbackFn = callback;
}

//-----------------------------------Myszka----------------------------------------
void Window::MouseMoveCallback(GLFWwindow* window, double xPos, double yPos)
{
    Window* myWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
    myWindow->ProcessMouseMovement(xPos, yPos);
}

void Window::ProcessMouseMovement(double xPos, double yPos)
{
    MouseMovedEvent event(xPos, yPos);
    m_EventCallbackFn(event);
}

void Window::MouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
    Window* myWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
    myWindow->ProcessMouseScroll(xOffset, yOffset);
}

void Window::ProcessMouseScroll(double xOffset, double yOffset)
{
    MouseScrolledEvent event(xOffset, yOffset);
    m_EventCallbackFn(event);
}

void Window::MouseButtonPressedCallback(GLFWwindow* window, int button, int action, int mods)
{
    Window* myWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
    myWindow->ProcessMouseButtonPress(button, action, mods);
}

void Window::ProcessMouseButtonPress(int button, int action, int mods)
{
    switch (action)
    {
        case 1:
        {
            MouseButtonPressedEvent event(button);
            m_EventCallbackFn(event);
            break;
        }
        case 0:
        {
            MouseButtonReleasedEvent event(button);
            m_EventCallbackFn(event);
            break;
        }
    }
}

//---------------------------------------------------------------------------------


//-----------------------------------Klawiatura----------------------------------------
void Window::KeyCallBack(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Window* myWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
    myWindow->ProcessKeyInput(key, action);
}

void Window::ProcessKeyInput(int key, int action)
{
    switch (action)
    {
        case 1:
        {
            KeyPressedEvent event(key, 0);
            m_EventCallbackFn(event);
            break;
        }
        case 0:
        {
            KeyReleasedEvent event(key);
            m_EventCallbackFn(event);
            break;
        }
        case 2:
        {
            KeyPressedEvent event(key, 1);
            m_EventCallbackFn(event);
            break;
        }
    }
}

//---------------------------------------------------------------------------------



//-----------------------------------Okno----------------------------------------

void Window::WindowCloseCallback(GLFWwindow* window)
{
    Window* myWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
    myWindow->ProcessWindowClose();
}

void Window::ProcessWindowClose()
{
    WindowCloseEvent event;
    m_EventCallbackFn(event);
}

void Window::WindowResizeCallback(GLFWwindow* window,int width, int height)
{
    Window* myWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
    myWindow->ProcessWindowResize(width, height);
}

void Window::ProcessWindowResize(int width, int height)
{
    WindowResizeEvent event(width, height);
    m_EventCallbackFn(event);
}

//---------------------------------------------------------------------------------

// Statyczna funkcja poœrednicz¹ca
void Window::CharCallback(GLFWwindow* window, unsigned int keycode) {
    Window* myWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
    myWindow->ProcessCharInput(keycode);
}

// Metoda wysy³aj¹ca zdarzenie do silnika
void Window::ProcessCharInput(unsigned int keycode) {
    KeyTypedEvent event(keycode); 
    m_EventCallbackFn(event);
}