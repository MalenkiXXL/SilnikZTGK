#pragma once
#include <glm/glm.hpp>
#include <memory>

#include "VertexArray.h"
#include "Shader.h"

class Renderer
{
public:
	//rozpoczyna klatke - zapisuje pozycje i perspektywne kamery
	static void BeginScene(const glm::mat4& viewProjectionMatrix);
	//konczy klatke
	static void EndScene();
	//przyjmuje obiekt do narysowania, shader oraz pozycje w swiecie
	static void Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertexArray, const glm::mat4 transform = glm::mat4(1.0f));

private:
	//globalne dane dla calej klatki
	struct SceneData
	{
		glm::mat4 ViewProjectionMatrix;
	};

	static SceneData* s_SceneData;
};

