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

// Struktura dopasowana do layout(std140) w GLSL.
// W std140 typ vec3 jest wyrównywany do 16 bajtów (rozmiar vec4).
struct SceneUBO {
    glm::mat4 ViewProjection;   // 64 bajty (offset 0)
    glm::vec3 SunDir;           // 12 bajtów (offset 64)
    float      _pad0 = 0.0f;    // 4 bajty paddingu -> suma 16 (offset 76)
    glm::vec3 LightColor;       // 12 bajtów (offset 80)
    float      _pad1 = 0.0f;    // 4 bajty paddingu -> suma 16 (offset 92)
    glm::vec3 ViewPos;          // 12 bajtów (offset 96)
    float      _pad2 = 0.0f;    // 4 bajty paddingu -> suma 16 (offset 108)
};

struct InstanceData {
    glm::mat4 Transform;
    float UVOffset;
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

    uint32_t InstanceBatches = 0;     // Ile grup modeli wys³ano
    uint32_t MatrixCalculations = 0;  // Ile macierzy przeliczono (Dirty Flag)
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
    // Inicjalizuje systemy rendera, w tym tworzy bufor UBO
    static void Init();
    static void Shutdown();

    static void ResetStats() { s_Stats.Reset(); }
    static RendererStatistics& GetStats() { return s_Stats; }

    // Rozpoczyna klatkê - aktualizuje dane w UBO
    static void BeginScene(const glm::mat4& viewProjectionMatrix, const glm::vec3& viewPos = glm::vec3(0.0f));
    static void EndScene();

    // Metody rysowania geometrii
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

    // Statyczny wskaŸnik do bufora UBO
    static std::unique_ptr<UniformBuffer> s_SceneUBO;
};