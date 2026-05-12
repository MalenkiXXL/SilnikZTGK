#pragma once
#include <glm/glm.hpp>
#include <memory>
#include "CookingStation/Math/Geometry.h"

#include "VertexArray.h"
#include "Shader.h"

class Model;

struct RendererStatistics {
	uint32_t DrawCalls3D = 0;
	uint32_t DrawCallsUI = 0;
	uint32_t TriangleCount3D = 0;
	uint32_t TriangleCountUI = 0;
	uint32_t CulledObjects3D = 0;

	float CPULogicTime = 0.0f;
	float CPURenderTime = 0.0f;
	float GPURenderTime = 0.0f;

	uint32_t InstanceBatches = 0;  // Ile grup modeli wys³ano
	uint32_t MatrixCalculations = 0; // Ile macierzy przeliczono w tej klatce (CPU Dirty Flag)
	uint32_t SkippedCalculations = 0; // Ile obiektów pominiêto dziêki Dirty Flag

	void Reset() {
		DrawCalls3D = 0;
		DrawCallsUI = 0; 
		TriangleCount3D = 0;
		TriangleCountUI = 0;
		CulledObjects3D = 0;
		InstanceBatches = 0;
		MatrixCalculations = 0;
		SkippedCalculations = 0;
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
	static void SubmitInstanced(std::shared_ptr<Shader> shader, Model* model, const std::vector<glm::mat4>& transforms);
	static std::string ActiveShader;
private:
	//globalne dane dla calej klatki
	struct SceneData
	{
		glm::mat4 ViewProjectionMatrix;
		Frustum ActiveFrustum;
	};

	static SceneData* s_SceneData;

	static RendererStatistics s_Stats;
	static uint32_t s_GPUQueryID;
	static bool s_GPUQueryInitialized;
};

