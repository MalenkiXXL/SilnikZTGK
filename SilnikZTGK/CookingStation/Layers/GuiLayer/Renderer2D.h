#pragma once
#include <glm/glm.hpp>
#include <memory>
#include "CookingStation/Renderer/Shader.h"
#include "CookingStation/Renderer/VertexArray.h"
#include "CookingStation/Core/Texture.h"


class Renderer2D {
public:
	static void Init();
	static void Shutdown();

	static void BeginScene(const glm::mat4& projection);
	static void EndScene();

	static void DrawQuad(const glm::vec2& position, 
		const glm::vec2& size, 
		const glm::vec4& color);

	static void DrawQuad(const glm::vec2& position, 
		const glm::vec2& size, 
		const std::shared_ptr<Texture>& texture, 
		const glm::vec4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, 
		const glm::vec2& uvMin = { 0.0f, 0.0f }, 
		const glm::vec2& uvMax = { 1.0f, 1.0f });

	static void DrawQuad(const glm::vec2& position, 
		const glm::vec2& size, 
		uint32_t textureID, 
		const glm::vec4& tintColor = glm::vec4(1.0f), 
		const glm::vec2& uv0 = glm::vec2(0.0f, 1.0f), 
		const glm::vec2& uv1 = glm::vec2(1.0f, 0.0f));

	static void DrawQuad(const glm::mat4& transform, const glm::vec4& color);

private:
	struct Renderer2DData {
		std::unique_ptr<Shader> UI_Shader;
		std::shared_ptr<VertexArray> QuadVAO;
		std::shared_ptr<Texture> WhiteTexture;
	};
	
	static Renderer2DData* s_Data;
};