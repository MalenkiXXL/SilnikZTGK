#pragma once
#include <glm/glm.hpp>
#include <memory>

#include "VertexArray.h"
#include "Shader.h"

class Model;

struct RendererStatistics {
	uint32_t DrawCalls3D = 0;
	uint32_t DrawCallsUI = 0;
	uint32_t TriangleCount3D = 0;
	uint32_t TriangleCountUI = 0;
	float CPULogicTime = 0.0f;
	float CPURenderTime = 0.0f;
	float GPURenderTime = 0.0f;

	void Reset() {
		DrawCalls3D = 0;
		DrawCallsUI = 0; 
		TriangleCount3D = 0;
		TriangleCountUI = 0;
	}
};

class Renderer
{
public:
	static void ResetStats() { s_Stats.Reset(); }
	static RendererStatistics& GetStats() { return s_Stats; }
	//rozpoczyna klatke - zapisuje pozycje i perspektywne kamery
	static void BeginScene(const glm::mat4& viewProjectionMatrix);
	//konczy klatke
	static void EndScene();
	//przyjmuje obiekt do narysowania, shader oraz pozycje w swiecie
	static void Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertexArray, const glm::mat4 transform = glm::mat4(1.0f));

	static void Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<Model>& model, const glm::mat4& transform);

private:
	//globalne dane dla calej klatki
	struct SceneData
	{
		glm::mat4 ViewProjectionMatrix;
	};

	static SceneData* s_SceneData;

	static RendererStatistics s_Stats;
	static uint32_t s_GPUQueryID;
	static bool s_GPUQueryInitialized;
};

