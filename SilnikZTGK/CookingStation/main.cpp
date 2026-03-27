#include <iostream>

#include "CookingStation/Core/Application.h"
#include "CookingStation/Core/Layer.h"
#include "CookingStation/Core/Input.h"

class SandboxLayer : public Layer
{
public:
    SandboxLayer()
    {
        debugName = "SandboxLayer";
    }

    void OnUpdate() override
    {
        if(Input::IsKeyPressed(GLFW_KEY_SPACE))
        {
            std::cout << "Wcisnieto spacje! w " << GetName() << std::endl;
        }
    }

    void OnEvent(Event& event) override 
    {
        std::cout << GetName() << "odebral zdarzenie!" << std::endl;
    }
};

int main()
{

    Application app;
    SandboxLayer sandbox;
    app.PushLayer(&sandbox);
    app.Run();
    
    return 0;
}

 