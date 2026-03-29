#include "RenderCommand.h"
#include <glad/glad.h>

void RenderCommand::SetClearColor(const glm::vec4& color)
{
	glClearColor(color.r, color.g, color.b, color.a);
}

void RenderCommand::Clear()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RenderCommand::DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray)
{
	glDrawElements(GL_TRIANGLES, vertexArray->GetIndexBuffers()->GetCount(), GL_UNSIGNED_INT, nullptr);
	// 1 - rysuj trójk¹ty
	// 2 - pobierz iloœæ indeksów do narysowania z podpiêtego VAO
	// 3 - typ indeksów 
	// 4 - offset 
}
