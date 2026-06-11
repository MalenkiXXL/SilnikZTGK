#include "CookingStation/Core/Application.h"

//#ifdef CS_DISTRIBUTION
//#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
//#endif

int main()
{
    Application* app = new Application();
    app->Run();
    delete app;
    return 0;
}