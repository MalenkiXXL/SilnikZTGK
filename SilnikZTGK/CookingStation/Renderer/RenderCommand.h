#pragma once
#include <glm/glm.hpp>
#include <memory>
#include "VertexArray.h"

class RenderCommand
{
public:
	//ustawia kolor tla
	static void SetClearColor(const glm::vec4& color);
	//czysci ekran
	static void Clear();
	//rysuje paczke danych vao
	static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray);

};

