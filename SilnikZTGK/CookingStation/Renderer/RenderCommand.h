#pragma once
#include <glm/glm.hpp>
#include <memory>
#include "VertexArray.h"

class RenderCommand
{
public:
	static void SetClearColor(const glm::vec4& color);
	static void Clear();
	static void DrawIndexed(const std::shared_ptr<VertexArray>& vertexArray);

};

