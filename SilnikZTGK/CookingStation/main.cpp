// main.cpp
#include "CookingStation/Core/Application.h"

// Jeœli kompilujemy wersjê finaln¹, powiedz linkerowi, ¿eby nie otwiera³ okna CMD
#ifdef CS_DISTRIBUTION
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

int main()
{
    Application* app = new Application();
    app->Run();
    delete app;
    return 0;
}