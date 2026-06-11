#pragma once
#include "CookingStation/Renderer/Shader.h"
#include "CookingStation/Renderer/VertexArray.h"
#include "CookingStation/Renderer/Buffer.h"
#include "CookingStation/Core/Texture.h" 
#include <glm/glm.hpp>
#include <memory>

class Renderer2D {
public:
    static void Init();
    static void Shutdown();

    static void BeginScene(const glm::mat4& projection);
    static void EndScene();
    static void Flush();

    static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, float radius = 0.0f);

    static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const std::shared_ptr<Texture>& texture, const glm::vec4& color = glm::vec4(1.0f), const glm::vec2& uvMin = { 0.0f, 0.0f }, const glm::vec2& uvMax = { 1.0f, 1.0f }, float radius = 0.0f);

    static void DrawQuad(const glm::vec2& position, const glm::vec2& size, uint32_t textureID, const glm::vec4& color = glm::vec4(1.0f), const glm::vec2& uvMin = { 0.0f, 0.0f }, const glm::vec2& uvMax = { 1.0f, 1.0f }, float radius = 0.0f);

    static void DrawQuad(const glm::mat4& transform, const glm::vec4& color, float radius = 0.0f);
    static void DrawQuad(const glm::mat4& transform, uint32_t textureID, const glm::vec4& tintColor = glm::vec4(1.0f), float radius = 0.0f);

private:
    static void FlushAndReset();
    struct Renderer2DData;
    static Renderer2DData* s_Data;
};