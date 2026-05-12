#include "Renderer.h"
#include "RenderCommand.h"
#include "Model.h"
#include <chrono>

Renderer::SceneData* Renderer::s_SceneData = new Renderer::SceneData;
RendererStatistics Renderer::s_Stats;
uint32_t Renderer::s_GPUQueryID = 0;
bool Renderer::s_GPUQueryInitialized = false;
std::string Renderer::ActiveShader = "Standard";

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
	s_SceneData->ActiveFrustum = ExtractFrustum(viewProjectionMatrix);
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
	//rysuje

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

	for (auto& mesh : model->meshes) {
		// Obliczamy gdzie fizycznie w œwiecie le¿y ten konkretny Mesh
		AABB worldAABB = mesh.GetWorldAABB(transform);

		// Test przeciêcia (Culling)
		if (IsOnFrustum(s_SceneData->ActiveFrustum, worldAABB)) {
			// Widaæ go Renderujemy
			mesh.Draw(*shader);
			s_Stats.DrawCalls3D++;
			s_Stats.TriangleCount3D += (mesh.indices.size() / 3);
		}
		else {
			// Ukryty, pomijamy GPU
			s_Stats.CulledObjects3D++;
		}
	}
}

void Renderer::SubmitInstanced(std::shared_ptr<Shader> shader, Model* model, const std::vector<glm::mat4>& transforms)
{
	if (transforms.empty() || !model || !shader) return;

	shader->use();
	// Wysy³amy macierz kamery, ¿eby instancje wiedzia³y jak siê wyœwietliæ na ekranie
	shader->setMat4("viewProjection", s_SceneData->ViewProjectionMatrix);

	// Dla ka¿dego sub-mesha w modelu
	for (auto& mesh : model->meshes)
	{
		// 1. Przesy³amy zaktualizowane macierze do naszego Instance VBO
		glBindBuffer(GL_ARRAY_BUFFER, mesh.m_InstanceVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, transforms.size() * sizeof(glm::mat4), transforms.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// 2. Bindowanie tekstur (Logika skopiowana z Twojego Mesh::Draw)
		unsigned int diffuseNr = 1;
		unsigned int specularNr = 1;
		unsigned int normalNr = 1;
		unsigned int heightNr = 1;

		int diffuseCount = 0;
		for (const auto& t : mesh.textures) {
			if (t.type == "texture_diffuse") diffuseCount++;
		}

		shader->SetBool("useTexture2", diffuseCount > 1);

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
		glDrawElementsInstanced(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0, transforms.size());
		mesh.m_VertexArray->Unbind();

		// 4. Sprz¹tanie jednostek teksturuj¹cych
		for (unsigned int i = 0; i < mesh.textures.size(); i++)
		{
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		// 5. Aktualizacja statystyk dla Twojego GUI
		s_Stats.DrawCalls3D++;
		s_Stats.InstanceBatches++; 
		s_Stats.TriangleCount3D += (mesh.indices.size() / 3) * transforms.size();
	}
	static std::unordered_set<std::string> s_LoggedModels;

	if (s_LoggedModels.find(model->FilePath) == s_LoggedModels.end())
	{
		spdlog::info("Batch: Model {} ma {} siatek, rysuje {} instancji",
			model->FilePath, model->meshes.size(), transforms.size());
		s_LoggedModels.insert(model->FilePath);
	}
}