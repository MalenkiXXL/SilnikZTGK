#include "Renderer2D.h"
#include <glm/gtc/matrix_transform.hpp>
#include "CookingStation/Renderer/Renderer.h" 

// Definicja statycznego wskaŸnika danych
Renderer2D::Renderer2DData* Renderer2D::s_Data = nullptr;

void Renderer2D::Init() {
    s_Data = new Renderer2DData();

    // 1. £adowanie shadera UI
    s_Data->UI_Shader = std::make_unique<Shader>(
        "CookingStation/Shaders/vsShaders/shader2d.vs",
        "CookingStation/Shaders/fragShaders/shader2d.frag"
    );

    float vertices[] = {
        // Pozycja (x, y, z)    // UV (u, v)
        0.0f, 0.0f, 0.0f,       0.0f, 0.0f, // Lewy górny
        1.0f, 0.0f, 0.0f,       1.0f, 0.0f, // Prawy górny
        1.0f, 1.0f, 0.0f,       1.0f, 1.0f, // Prawy dolny
        0.0f, 1.0f, 0.0f,       0.0f, 1.0f  // Lewy dolny
    };

    uint32_t indices[] = { 0, 1, 2, 2, 3, 0 };

    // 3. Tworzenie VAO/VBO 
    s_Data->QuadVAO = std::make_shared<VertexArray>();
    auto vbo = std::make_shared<VertexBuffer>(vertices, sizeof(vertices));

    vbo->SetLayout({
     { ShaderDataType::Float3, "aPos" },      // location 0
     { ShaderDataType::Float2, "aTexCoord" }  // location 1
        });

    s_Data->QuadVAO->AddVertexBuffer(vbo);

    auto ebo = std::make_shared<IndexBuffer>(indices, sizeof(indices) / sizeof(uint32_t));
    s_Data->QuadVAO->SetIndexBuffer(ebo);

    uint32_t whiteTextureData = 0xffffffff;
    s_Data->WhiteTexture = std::make_shared<Texture>(1, 1); 
    s_Data->WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));
}

void Renderer2D::BeginScene(const glm::mat4& projection) {
    s_Data->UI_Shader->use();
    s_Data->UI_Shader->setMat4("u_ViewProjection", projection);

}
// Wersja 1: Tylko kolor 
void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, float radius) {
    // Przekazujemy 'radius' dalej
    DrawQuad(position, size, s_Data->WhiteTexture, color, { 0.0f, 0.0f }, { 1.0f, 1.0f }, radius);
}

// Wersja 2: Pe³na (u¿ywana do tekstu, ikon)
void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const std::shared_ptr<Texture>& texture, const glm::vec4& color, const glm::vec2& uvMin, const glm::vec2& uvMax, float radius) {
    texture->Bind();

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f))
        * glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

    s_Data->UI_Shader->setMat4("u_Transform", transform);
    s_Data->UI_Shader->setVec4("u_Color", color);
    s_Data->UI_Shader->setVec2("u_UVMin", uvMin);
    s_Data->UI_Shader->setVec2("u_UVMax", uvMax);

    // DYNAMICZNE ZAOKR¥GLENIE
    s_Data->UI_Shader->setVec2("u_QuadSize", size);
    s_Data->UI_Shader->setFloat("u_Radius", radius); // <--- Czyta z argumentu!
    s_Data->UI_Shader->setFloat("u_Softness", 1.5f);

    s_Data->QuadVAO->Bind();
    glDrawElements(GL_TRIANGLES, s_Data->QuadVAO->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
    Renderer::GetStats().DrawCallsUI++;
    Renderer::GetStats().TriangleCountUI += 2;
}

// Wersja 3: ID Tekstury (u¿ywana do rysowania Viewport FBO)
void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, uint32_t textureID, const glm::vec4& color, const glm::vec2& uvMin, const glm::vec2& uvMax, float radius) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f))
        * glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

    s_Data->UI_Shader->setMat4("u_Transform", transform);
    s_Data->UI_Shader->setVec4("u_Color", color);
    s_Data->UI_Shader->setVec2("u_UVMin", uvMin);
    s_Data->UI_Shader->setVec2("u_UVMax", uvMax);

    // DYNAMICZNE ZAOKR¥GLENIE
    s_Data->UI_Shader->setVec2("u_QuadSize", size);
    s_Data->UI_Shader->setFloat("u_Radius", radius); // <--- Czyta z argumentu!
    s_Data->UI_Shader->setFloat("u_Softness", 1.5f);

    s_Data->QuadVAO->Bind();
    glDrawElements(GL_TRIANGLES, s_Data->QuadVAO->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
    Renderer::GetStats().DrawCallsUI++;
    Renderer::GetStats().TriangleCountUI += 2;
}

void Renderer2D::EndScene() {}

void Renderer2D::Shutdown() {
    delete s_Data;
}

void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color) {
    s_Data->WhiteTexture->Bind();
    s_Data->UI_Shader->setMat4("u_Transform", transform);
    s_Data->UI_Shader->setVec4("u_Color", color);
    s_Data->UI_Shader->setVec2("u_UVMin", { 0.0f, 0.0f });
    s_Data->UI_Shader->setVec2("u_UVMax", { 1.0f, 1.0f });
    s_Data->QuadVAO->Bind();
    glDrawElements(GL_TRIANGLES,
        s_Data->QuadVAO->GetIndexBuffer()->GetCount(),
        GL_UNSIGNED_INT, nullptr);
    Renderer::GetStats().DrawCallsUI++;
    Renderer::GetStats().TriangleCountUI += 2;
}

void Renderer2D::DrawQuad(const glm::mat4& transform, uint32_t textureID, const glm::vec4& tintColor)
{
    //aktywowanie odpowiedniego slota i zbindowanie tekstury
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);

    //uniformy dla ui_shadera/shadera2d
    s_Data->UI_Shader->use();
    s_Data->UI_Shader->setMat4("u_Transform", transform);
    s_Data->UI_Shader->setVec4("u_Color", tintColor);
    
    //rysowanie quada
    s_Data->QuadVAO->Bind();
    glDrawElements(GL_TRIANGLES, s_Data->QuadVAO->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
}
