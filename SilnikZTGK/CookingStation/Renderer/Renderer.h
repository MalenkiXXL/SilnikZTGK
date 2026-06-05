#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

#include "CookingStation/Math/Geometry.h"
#include "UniformBuffer.h"
#include "VertexArray.h"
#include "Shader.h"

class Model;

namespace UBOBindings {
    constexpr uint32_t Scene = 0;
}

struct SceneUBO {
    glm::mat4 ViewProjection;
    glm::vec3 SunDir;
    float      _pad0 = 0.0f;
    glm::vec3 LightColor;
    float      _pad1 = 0.0f;
    glm::vec3 ViewPos;
    float      _pad2 = 0.0f;
};

struct InstanceData {
    glm::mat4 Transform;
    float UVOffset;
    glm::vec4 HighlightColor;
};

struct RendererStatistics {
    uint32_t DrawCalls3D = 0;
    uint32_t DrawCallsUI = 0;
    uint32_t TriangleCount3D = 0;
    uint32_t TriangleCountUI = 0;
    uint32_t CulledObjects3D = 0;

    float CPULogicTime = 0.0f;
    float CPURenderTime = 0.0f;
    float GPURenderTime = 0.0f;

    uint32_t InstanceBatches = 0;
    uint32_t MatrixCalculations = 0;
    uint32_t SkippedCalculations = 0;

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
    static void Init();
    static void Shutdown();

    static void ResetStats() { s_Stats.Reset(); }
    static RendererStatistics& GetStats() { return s_Stats; }

    static void BeginScene(const glm::mat4& viewProjectionMatrix, const glm::vec3& viewPos = glm::vec3(0.0f));
    static void EndScene();

    static void Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertexArray, const glm::mat4 transform = glm::mat4(1.0f));
    static void Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<Model>& model, const glm::mat4& transform);
    static void SubmitInstanced(std::shared_ptr<Shader> shader, Model* model, const std::vector<InstanceData>& instanceData);

    static std::string ActiveShader;

private:
    struct SceneData
    {
        glm::mat4 ViewProjectionMatrix;
        Frustum ActiveFrustum;
    };

    static SceneData* s_SceneData;
    static RendererStatistics s_Stats;
    static uint32_t s_GPUQueryID;
    static bool s_GPUQueryInitialized;
    static std::unique_ptr<UniformBuffer> s_SceneUBO;
};