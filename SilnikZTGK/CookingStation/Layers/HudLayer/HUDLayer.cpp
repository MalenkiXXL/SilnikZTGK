#include "HUDLayer.h"
#include "CookingStation/Layers/AssetLayer/AssetManager.h"
#include "CookingStation/Core/Input.h"
#include <glm/gtc/matrix_transform.hpp>

void HUDLayer::OnAttach() {
	Renderer2D::Init(); 
}

void HUDLayer::OnUpdate(Timestep ts) {
	// 1. Pobieramy rozmiar okna tradycyjn¹ metod¹ 
	std::pair<float, float> windowSize = Input::GetWindowSize(); 
	float width = windowSize.first;
	float height = windowSize.second;

	// 2. Tworzymy macierz projekcji z 6 argumentami 
	glm::mat4 projection = glm::ortho(0.0f, width, 0.0f, height, -1.0f, 1.0f);

	// 3. Rozpoczêcie rysowania UI
	Renderer2D::BeginScene(projection); 

	// przykladowe tlo
	Renderer2D::DrawQuad({ 110.0f, height - 50.0f }, { 200.0f, 30.0f }, { 0.1f, 0.1f, 0.1f, 0.8f });

	Renderer2D::EndScene();
}
