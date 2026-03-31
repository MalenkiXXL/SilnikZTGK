#include "Renderer.h"
#include "RenderCommand.h"
#include "Model.h"


Renderer::SceneData* Renderer::s_SceneData = new Renderer::SceneData;


void Renderer::BeginScene(const glm::mat4& viewProjectionMatrix)
{
	//zapisuje macierz kamery zeby nie podawac dla kazdego modelu
	s_SceneData->ViewProjectionMatrix = viewProjectionMatrix;
}

void Renderer::EndScene()
{

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
	RenderCommand::DrawIndexed(vertexArray);
}

void Renderer::Submit(const std::shared_ptr<Shader>& shader, const std::shared_ptr<Model>& model, const glm::mat4& transform) {
	shader->use(); //
	shader->setMat4("viewProjection", s_SceneData->ViewProjectionMatrix);
	shader->setMat4("model", transform);

	// model sam wie jak narysowac wszystkie mesh/vertexArray
	model->Draw(*shader);
}