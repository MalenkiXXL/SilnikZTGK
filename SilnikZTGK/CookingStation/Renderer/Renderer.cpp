#include "Renderer.h"
#include "RenderCommand.h"
#include "Model.h"
#include <chrono>
#include <unordered_set>
#include <spdlog/spdlog.h>

// Inicjalizacja statycznych sk³adowych
Renderer::SceneData* Renderer::s_SceneData = nullptr;
RendererStatistics Renderer::s_Stats;
uint32_t Renderer::s_GPUQueryID = 0;
bool Renderer::s_GPUQueryInitialized = false;
std::string Renderer::ActiveShader = "Standard";
std::unique_ptr<UniformBuffer> Renderer::s_SceneUBO = nullptr;

void Renderer::Init()
{
    s_SceneData = new Renderer::SceneData();

    // Tworzymy bufor UBO o rozmiarze struktury SceneUBO na zdefiniowanym punkcie wi¹zania
    s_SceneUBO = std::make_unique<UniformBuffer>(sizeof(SceneUBO), UBOBindings::Scene);

    spdlog::info("Renderer: Bufor UBO gotowy ({} bajtów).", sizeof(SceneUBO));
}

void Renderer::Shutdown()
{
    if (s_GPUQueryInitialized)
    {
        glDeleteQueries(1, &s_GPUQueryID);
    }

    delete s_SceneData;
    s_SceneUBO.reset();
}

void Renderer::BeginScene(const glm::mat4& viewProjectionMatrix, const glm::vec3& viewPos)
{
    ResetStats();

    // 1. Obs³uga zapytañ GPU (Profiling)
    if (!s_GPUQueryInitialized)
    {
        glGenQueries(1, &s_GPUQueryID);
        s_GPUQueryInitialized = true;
    }
    glBeginQuery(GL_TIME_ELAPSED, s_GPUQueryID);

    // 2. Aktualizacja danych pomocniczych procesora
    s_SceneData->ViewProjectionMatrix = viewProjectionMatrix;
    s_SceneData->ActiveFrustum = ExtractFrustum(viewProjectionMatrix);

    // 3. WYSY£KA DANYCH DO UBO (std140)
    // Dziêki temu ka¿dy shader ma dostêp do macierzy kamery i œwiate³ bez glUniform!
    SceneUBO uboData;
    uboData.ViewProjection = viewProjectionMatrix;
    uboData.ViewPos = viewPos;
    uboData.SunDir = glm::vec3(-0.321f, -0.766f, -0.557f);
    uboData.LightColor = glm::vec3(1.0f, 1.0f, 1.0f);

    // Explicitne czyszczenie paddingu (dla bezpieczeñstwa)
    uboData._pad0 = 0.0f;
    uboData._pad1 = 0.0f;
    uboData._pad2 = 0.0f;

    s_SceneUBO->SetData(&uboData, sizeof(SceneUBO));
}

void Renderer::EndScene()
{
    glEndQuery(GL_TIME_ELAPSED);

    GLuint64 timeElapsed = 0;
    glGetQueryObjectui64v(s_GPUQueryID, GL_QUERY_RESULT, &timeElapsed);
    s_Stats.GPURenderTime = timeElapsed / 1000000.0f;
}

void Renderer::Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertexArray, const glm::mat4 transform)
{
    shader->use();

    // UWAGA: viewProjection nie jest ju¿ potrzebne - shader bierze je z UBO!
    shader->setMat4("u_Transform", transform); // Zmienione na u_Transform dla spójnoœci z shader.vs

    vertexArray->Bind();
    RenderCommand::DrawIndexed(vertexArray);

    uint32_t indexCount = vertexArray->GetIndexBuffer()->GetCount();
    s_Stats.DrawCalls3D++;
    s_Stats.TriangleCount3D += (indexCount / 3);
}

void Renderer::Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<Model>& model, const glm::mat4& transform) {
    shader->use();

    // Dane sta³e (ViewProjection) lec¹ z UBO, ustawiamy tylko macierz modelu
    shader->setMat4("u_Transform", transform);

    for (auto& mesh : model->meshes) {
        AABB worldAABB = mesh.GetWorldAABB(transform);

        if (IsOnFrustum(s_SceneData->ActiveFrustum, worldAABB)) {
            mesh.Draw(*shader);
            s_Stats.DrawCalls3D++;
            s_Stats.TriangleCount3D += (mesh.indices.size() / 3);
        }
        else {
            s_Stats.CulledObjects3D++;
        }
    }
}

void Renderer::SubmitInstanced(std::shared_ptr<Shader> shader, Model* model, const std::vector<InstanceData>& instanceData)
{
    if (instanceData.empty() || !model || !shader) return;

    shader->use();
    // Brak shader->setMat4("viewProjection", ...)! Obs³u¿one przez UBO.

    for (auto& mesh : model->meshes)
    {
        // 1. Przesy³amy paczkê danych instancji
        glBindBuffer(GL_ARRAY_BUFFER, mesh.m_InstanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, instanceData.size() * sizeof(InstanceData), instanceData.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // 2. Bindowanie tekstur
        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr = 1;
        unsigned int heightNr = 1;

        int diffuseCount = 0;
        for (const auto& t : mesh.textures) {
            if (t.type == "texture_diffuse") diffuseCount++;
        }

        shader->setBool("useTexture2", diffuseCount > 1);

        for (unsigned int i = 0; i < mesh.textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            std::string number;
            std::string name = mesh.textures[i].type;

            if (name == "texture_diffuse") number = std::to_string(diffuseNr++);
            else if (name == "texture_specular") number = std::to_string(specularNr++);
            else if (name == "texture_normal") number = std::to_string(normalNr++);
            else if (name == "texture_height") number = std::to_string(heightNr++);

            glUniform1i(glGetUniformLocation(shader->ID, (name + number).c_str()), i);
            mesh.textures[i].Texture2DPtr->Bind(i);
        }

        mesh.m_VertexArray->Bind();
        glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)mesh.indices.size(), GL_UNSIGNED_INT, 0, (GLsizei)instanceData.size());
        mesh.m_VertexArray->Unbind();

        // 4. Sprz¹tanie
        for (unsigned int i = 0; i < mesh.textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        s_Stats.DrawCalls3D++;
        s_Stats.InstanceBatches++;
        s_Stats.TriangleCount3D += (uint32_t)(mesh.indices.size() / 3) * (uint32_t)instanceData.size();
    }

    // Logowanie jednorazowe dla modelu
    static std::unordered_set<std::string> s_LoggedModels;
    if (s_LoggedModels.find(model->FilePath) == s_LoggedModels.end())
    {
        // ZMIANA: instanceData.size() w logach
        spdlog::info("Batch: Model {} ma {} siatek, rysuje {} instancji",
            model->FilePath, model->meshes.size(), instanceData.size());
        s_LoggedModels.insert(model->FilePath);
    }
}