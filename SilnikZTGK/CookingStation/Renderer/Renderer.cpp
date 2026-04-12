#include "Renderer.h"
#include "RenderCommand.h"
#include "Model.h"


Renderer::SceneData* Renderer::s_SceneData = new Renderer::SceneData;
RendererStatistics Renderer::s_Stats;
uint32_t Renderer::s_GPUQueryID = 0;
bool Renderer::s_GPUQueryInitialized = false;


void Renderer::BeginScene(const glm::mat4& viewProjectionMatrix)
{
	ResetStats(); // reset statystyk na pocz¹tku klatki

	//  jeœli nie wygenerowaliœmy jeszcze zapytania w OpenGL, robimy to raz
	if (!s_GPUQueryInitialized)
	{
		glGenQueries(1, &s_GPUQueryID);
		s_GPUQueryInitialized = true;
	}

	// startujemy stoper na karcie graficznej
	glBeginQuery(GL_TIME_ELAPSED, s_GPUQueryID);

	//zapisuje macierz kamery zeby nie podawac dla kazdego modelu
	s_SceneData->ViewProjectionMatrix = viewProjectionMatrix;
}

void Renderer::EndScene()
{
	// zatrzymujemy stoper
	glEndQuery(GL_TIME_ELAPSED);

	// wyci¹gamy czas w nanosekundach i zamieniamy na milisekundy
	GLuint64 timeElapsed = 0;
	glGetQueryObjectui64v(s_GPUQueryID, GL_QUERY_RESULT, &timeElapsed);
	s_Stats.GPURenderTime = timeElapsed / 1000000.0f;
}

void Renderer::Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<VertexArray>& vertexArray, const glm::mat4 transform)
{
	//uzywaj tego konkretnego shaedra
	shader->use();
	//wysylamy kamere
	shader->setMat4("viewProjection", s_SceneData->ViewProjectionMatrix);
	//wysylamy pozycje obrot i skale
	shader->setMat4("model", transform);
	//mowimy vao zeby przygotowal buffory i instrukcje
	vertexArray->Bind();
	//rysuje#include <chrono>

	class ProfileTimer {
	public:
		ProfileTimer(float& outTime) : m_OutTime(outTime) {
			m_StartTime = std::chrono::high_resolution_clock::now();
		}
		~ProfileTimer() {
			auto endTime = std::chrono::high_resolution_clock::now();
			m_OutTime = std::chrono::duration<float, std::chrono::milliseconds::period>(endTime - m_StartTime).count();
		}
	private:
		float& m_OutTime;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
	};
	RenderCommand::DrawIndexed(vertexArray);

	uint32_t indexCount = vertexArray->GetIndexBuffer()->GetCount();
	s_Stats.DrawCalls3D++;
	s_Stats.TriangleCount3D += (indexCount / 3);
}

void Renderer::Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<Model>& model, const glm::mat4& transform) {
	shader->use(); 
	shader->setMat4("viewProjection", s_SceneData->ViewProjectionMatrix);
	shader->setMat4("model", transform);

	// model sam wie jak narysowac wszystkie mesh/vertexArray
	model->Draw(*shader);

	for (const auto& mesh : model->meshes)
	{
		s_Stats.DrawCalls3D++; 
		s_Stats.TriangleCount3D += (mesh.indices.size() / 3);
	}

}