#pragma once
#include "CookingStation/Scene/ScriptableEntity.h"
#include <glm/glm.hpp>

class ConveyorScript : public ScriptableEntity
{
public:
    glm::vec3 PushDirection = { 0.0f, 0.0f, 0.0f };
    float Speed = 2.0f;

    void OnCreate() override
    {
        // Kiedy taœma siê pojawia, sprawdzamy jej obrót (Y)
        auto* transform = GetComponent<TransformComponent>();
        if (transform)
        {
            // WA¯NE: W zale¿noœci od tego, jak dok³adnie masz zrobion¹ kamerê i œwiat,
            // te wektory (X, Z) mog¹ wymagaæ odwrócenia (np. na -1.0f). 
            // Dopasuj je do swojej gry!

            float rotY = transform->Rotation.y;

            if (rotY == 90.0f)       PushDirection = { 1.0f, 0.0f, 0.0f };  // W prawo
            else if (rotY == -90.0f) PushDirection = { -1.0f, 0.0f, 0.0f };  // W lewo
            else if (rotY == 180.0f) PushDirection = { 0.0f, 0.0f, -1.0f }; // W górê (lub w dó³)
            else if (rotY == 0.0f)   PushDirection = { 0.0f, 0.0f, 1.0f };  // W dó³ (lub w górê)
        }
    }

    void OnUpdate(Timestep ts) override
    {
        // Tutaj w nastêpnym kroku zrobimy sprawdzanie, czy le¿y na nas jakiœ talerz 
        // i jeœli tak, bêdziemy modyfikowaæ jego pozycjê u¿ywaj¹c 'PushDirection' i 'Speed'!
    }
};