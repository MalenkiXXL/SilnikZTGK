#include "Window.h"
#include <iostream>
#include <spdlog/spdlog.h>

#include "CookingStation/Events/KeyEvent.h"
#include "CookingStation/Events/WindowEvent.h"
#include "CookingStation/Events/MouseEvent.h"

// NOWE: Zeby móc wysłać event zmiany grafiki bezpośrednio do aplikacji
#include "CookingStation/Core/Application.h"
#include "CookingStation/Core/GraphicsSettings.h"
#include "CookingStation/Events/GameEvents.h"

Window::Window(unsigned int width, unsigned int height, const std::string name)
    : screenWidth(width), screenHeight(height), screenName(name)
{}

Window::~Window()
{}

// POPRAWKA: Callback bledow GLFW - pozwala zobaczyc PRAWDZIWA przyczyne
// (np. brak wsparcia OpenGL 3.3 Core na karcie graficznej), zamiast cichego nullptr.
static void GLFWErrorCallback(int error, const char* description)
{
    spdlog::error("GLFW Error [{}]: {}", error, description);
}

static bool s_GLFWInitialized = false;

bool Window::EnsureGLFWInitialized()
{
    if (s_GLFWInitialized)
        return true;

    glfwSetErrorCallback(GLFWErrorCallback);

    if (!glfwInit())
    {
        spdlog::critical("Window::EnsureGLFWInitialized: glfwInit() nie powiodlo sie!");
        return false;
    }

    s_GLFWInitialized = true;
    return true;
}

void Window::Init()
{
    if (!EnsureGLFWInitialized())
    {
        window = nullptr;
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(screenWidth, screenHeight, screenName.c_str(), NULL, NULL);

    if (!window)
    {
        spdlog::critical("Window::Init: glfwCreateWindow() zwrocilo nullptr! "
            "Zobacz log 'GLFW Error' powyzej - to jest prawdziwa przyczyna "
            "(najczesciej: karta graficzna/sterownik nie wspiera OpenGL 3.3 Core Profile).");
        return;
    }

    glfwMakeContextCurrent(window);

    glfwSetWindowUserPointer(window, this);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        spdlog::critical("Window::Init: gladLoadGLLoader() nie powiodlo sie! Kontekst OpenGL moze byc niepoprawny.");
    }

    glfwSetKeyCallback(window, KeyCallBack);
    glfwSetCharCallback(window, CharCallback);
    glfwSetCursorPosCallback(window, MouseMoveCallback);
    glfwSetScrollCallback(window, MouseScrollCallback);
    glfwSetMouseButtonCallback(window, MouseButtonPressedCallback);
    glfwSetWindowCloseCallback(window, WindowCloseCallback);
    glfwSetWindowSizeCallback(window, WindowResizeCallback);

    // NOWE: Podpięcie callbacku do przycisku maksymalizacji okna
    glfwSetWindowMaximizeCallback(window, WindowMaximizeCallback);
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

void Window::WindowResizeCallback(GLFWwindow* window, int width, int height)
{
    Window* myWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
    myWindow->ProcessWindowResize(width, height);
}

void Window::ProcessWindowResize(int width, int height)
{
    WindowResizeEvent event(width, height);
    m_EventCallbackFn(event);
}

// NOWE: Funkcja obsługująca maksymalizację
void Window::WindowMaximizeCallback(GLFWwindow* window, int maximized)
{
    Window* myWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
    myWindow->ProcessWindowMaximize(maximized);
}

void Window::ProcessWindowMaximize(int maximized)
{
    Application::Get().SetFullscreen(maximized != 0);
}

void Window::CharCallback(GLFWwindow* window, unsigned int keycode)
{
    Window* myWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
    myWindow->ProcessCharInput(keycode);
}

void Window::ProcessCharInput(unsigned int keycode)
{}

void Window::MouseMoveCallback(GLFWwindow* window, double xPos, double yPos)
{
    Window* myWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
    myWindow->ProcessMouseMovement(xPos, yPos);
}

void Window::ProcessMouseMovement(double xPos, double yPos)
{
    MouseMovedEvent event((float)xPos, (float)yPos);
    m_EventCallbackFn(event);
}

void Window::MouseScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
    Window* myWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
    myWindow->ProcessMouseScroll(xOffset, yOffset);
}

void Window::ProcessMouseScroll(double xOffset, double yOffset)
{
    MouseScrolledEvent event((float)xOffset, (float)yOffset);
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