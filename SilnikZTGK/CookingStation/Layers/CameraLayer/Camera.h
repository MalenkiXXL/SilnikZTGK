#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "CookingStation/Math/Geometry.h"

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 15.0f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

class Camera
{
public:
    float AspectRatio = 1.7777f;

    // rozmiar obszaru widocznego w rzucie ortograficznym
    // Wiekszy = widac wiecej swiata, mniejszy = zoom in
    float OrthoSize = 10.0f;
    float NearPlane = 0.1f;
    float FarPlane = 100.0f;

    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    float Yaw;
    float Pitch;
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom; // zachowujemy dla kompatybilnosci, ale ortho uzywa OrthoSize

    glm::vec3 TargetPosition;
    float     TargetOrthoSize = 10.0f;


    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
        float yaw = YAW, float pitch = PITCH)
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)),
        MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = position;
        TargetPosition = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    Camera(float posX, float posY, float posZ,
        float upX, float upY, float upZ,
        float yaw, float pitch)
        : Front(glm::vec3(0.0f, 0.0f, -1.0f)),
        MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = glm::vec3(posX, posY, posZ);
        TargetPosition = Position;
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    glm::mat4 GetProjectionMatrix() const
    {
        float left = -OrthoSize * AspectRatio;
        float right = OrthoSize * AspectRatio;
        float bottom = -OrthoSize;
        float top = OrthoSize;
        return glm::ortho(left, right, bottom, top, NearPlane, FarPlane);
    }

    const Frustum& GetFrustum() const
    {
        return m_Frustum;
    }

    void UpdateLerp(float deltaTime, float lerpSpeed = 5.0f)
    {
        Position  = glm::mix(Position,  TargetPosition,  glm::clamp(lerpSpeed * deltaTime, 0.0f, 1.0f));
        OrthoSize = glm::mix(OrthoSize, TargetOrthoSize, glm::clamp(lerpSpeed * deltaTime, 0.0f, 1.0f));
        UpdateFrustum();
    }


    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        if (direction == FORWARD)  TargetPosition += Front * deltaTime;
        if (direction == BACKWARD) TargetPosition -= Front * deltaTime;
        if (direction == LEFT)     TargetPosition -= Right * deltaTime;
        if (direction == RIGHT)    TargetPosition += Right * deltaTime;
        if (direction == UP)       TargetPosition += Up * deltaTime;
        if (direction == DOWN)     TargetPosition -= Up * deltaTime;
        UpdateFrustum();
    }

    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;
        Yaw += xoffset;
        Pitch += yoffset;
        if (constrainPitch)
        {
            if (Pitch > 89.0f) Pitch = 89.0f;
            if (Pitch < -89.0f) Pitch = -89.0f;
        }
        updateCameraVectors();
    }

    void ProcessMouseScroll(float yoffset)
    {
        TargetOrthoSize -= yoffset * 0.8f;
        if (TargetOrthoSize <  2.0f) TargetOrthoSize =  2.0f;
        if (TargetOrthoSize > 40.0f) TargetOrthoSize = 40.0f;
        UpdateFrustum();
    }

    void updateCameraVectors()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));

        UpdateFrustum();
    }
private:
    Frustum m_Frustum;
    void UpdateFrustum()
    {
        // Generujemy po��czon� macierz P * V i wyci�gamy z niej p�aszczyzny
        glm::mat4 viewProj = GetProjectionMatrix() * GetViewMatrix();
        m_Frustum = ExtractFrustum(viewProj);
    }
};
#endif