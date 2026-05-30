#include "Renderer2D.h"
// ZMIANA 1: Poprawny include wskazuj¹cy na Twoj¹ uniwersaln¹ klasê Texture z VFS
#include "CookingStation/Core/Texture.h"   
#include <glm/gtc/matrix_transform.hpp>
#include "CookingStation/Renderer/Renderer.h"
#include <array>
#include <spdlog/spdlog.h>

struct QuadVertex {
    glm::vec3 Position;
    glm::vec4 Color;
    glm::vec2 TexCoord;
    float TexIndex;
    glm::vec2 QuadSize;
    float Radius;
};

struct Renderer2D::Renderer2DData {
    static const uint32_t MaxQuads = 10000;
    static const uint32_t MaxVertices = MaxQuads * 4;
    static const uint32_t MaxIndices = MaxQuads * 6;
    static const uint32_t MaxTextureSlots = 16;

    std::unique_ptr<Shader> UI_Shader;
    std::shared_ptr<VertexArray> QuadVAO;
    std::shared_ptr<VertexBuffer> QuadVBO;

    // Texture (z Texture.h) - ma konstruktor (width,height) i SetData
    std::shared_ptr<Texture> WhiteTexture;

    uint32_t QuadIndexCount = 0;
    QuadVertex* QuadVertexBufferBase = nullptr;
    QuadVertex* QuadVertexBufferPtr = nullptr;

    std::array<uint32_t, MaxTextureSlots> TextureSlots;
    uint32_t TextureSlotIndex = 1;

    glm::vec4 QuadVertexPositions[4];
};

Renderer2D::Renderer2DData* Renderer2D::s_Data = nullptr;

void Renderer2D::Init() {
    s_Data = new Renderer2DData();

    s_Data->UI_Shader = std::make_unique<Shader>(
        "shaders://vsShaders/shader2d.vs",
        "shaders://fragShaders/shader2d.frag"
    );

    if (s_Data->UI_Shader->ID == 0)
        spdlog::error("Renderer2D: Shader 2D nie zostal zaladowany!");
    else
        spdlog::info("Renderer2D: Shader 2D OK (ID: {})", s_Data->UI_Shader->ID);

    s_Data->QuadVAO = std::make_shared<VertexArray>();
    s_Data->QuadVBO = std::make_shared<VertexBuffer>(s_Data->MaxVertices * sizeof(QuadVertex));

    s_Data->QuadVBO->SetLayout({
        { ShaderDataType::Float3, "aPos" },
        { ShaderDataType::Float4, "aColor" },
        { ShaderDataType::Float2, "aTexCoord" },
        { ShaderDataType::Float,  "aTexIndex" },
        { ShaderDataType::Float2, "aQuadSize" },
        { ShaderDataType::Float,  "aRadius" }
        });

    s_Data->QuadVAO->AddVertexBuffer(s_Data->QuadVBO);

    uint32_t* quadIndices = new uint32_t[s_Data->MaxIndices];
    uint32_t offset = 0;
    for (uint32_t i = 0; i < s_Data->MaxIndices; i += 6) {
        quadIndices[i + 0] = offset + 0;
        quadIndices[i + 1] = offset + 1;
        quadIndices[i + 2] = offset + 2;
        quadIndices[i + 3] = offset + 2;
        quadIndices[i + 4] = offset + 3;
        quadIndices[i + 5] = offset + 0;
        offset += 4;
    }
    auto quadIBB = std::make_shared<IndexBuffer>(quadIndices, s_Data->MaxIndices);
    s_Data->QuadVAO->SetIndexBuffer(quadIBB);
    delete[] quadIndices;

    s_Data->QuadVertexBufferBase = new QuadVertex[s_Data->MaxVertices];

    // Tworzymy bia³¹ teksturê 1x1 przez Texture(width,height) + SetData
    // Texture u¿ywa glTexSubImage2D bezpoœrednio - bez stbi, bez b³êdów
    uint32_t whiteTextureData = 0xffffffff;
    s_Data->WhiteTexture = std::make_shared<Texture>(1, 1);
    s_Data->WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

    s_Data->TextureSlots[0] = s_Data->WhiteTexture->GetRendererID();

    s_Data->QuadVertexPositions[0] = { 0.0f, 0.0f, 0.0f, 1.0f };
    s_Data->QuadVertexPositions[1] = { 1.0f, 0.0f, 0.0f, 1.0f };
    s_Data->QuadVertexPositions[2] = { 1.0f, 1.0f, 0.0f, 1.0f };
    s_Data->QuadVertexPositions[3] = { 0.0f, 1.0f, 0.0f, 1.0f };

    s_Data->UI_Shader->use();
    for (int i = 0; i < 16; i++) {
        s_Data->UI_Shader->setInt("u_Textures[" + std::to_string(i) + "]", i);
    }
}

void Renderer2D::Shutdown() {
    delete[] s_Data->QuadVertexBufferBase;
    delete s_Data;
}

void Renderer2D::BeginScene(const glm::mat4& projection) {
    s_Data->UI_Shader->use();
    s_Data->UI_Shader->setMat4("u_ViewProjection", projection);

    s_Data->QuadIndexCount = 0;
    s_Data->QuadVertexBufferPtr = s_Data->QuadVertexBufferBase;
    s_Data->TextureSlotIndex = 1;
}

void Renderer2D::EndScene() {
    Flush();
}

void Renderer2D::Flush() {
    if (s_Data->QuadIndexCount == 0) return;

    uint32_t dataSize = (uint32_t)((uint8_t*)s_Data->QuadVertexBufferPtr - (uint8_t*)s_Data->QuadVertexBufferBase);
    s_Data->QuadVBO->SetData(s_Data->QuadVertexBufferBase, dataSize);

    for (uint32_t i = 0; i < s_Data->TextureSlotIndex; i++) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, s_Data->TextureSlots[i]);
    }

    s_Data->QuadVAO->Bind();
    glDrawElements(GL_TRIANGLES, s_Data->QuadIndexCount, GL_UNSIGNED_INT, nullptr);

    Renderer::GetStats().DrawCallsUI++;
}

void Renderer2D::FlushAndReset() {
    EndScene();
    s_Data->QuadIndexCount = 0;
    s_Data->QuadVertexBufferPtr = s_Data->QuadVertexBufferBase;
    s_Data->TextureSlotIndex = 1;
}

void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, float radius) {
    DrawQuad(position, size, 0, color, { 0.0f, 0.0f }, { 1.0f, 1.0f }, radius);
}

// ZMIANA 2: Zast¹piono Texture2D na std::shared_ptr<Texture>
void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const std::shared_ptr<Texture>& texture, const glm::vec4& color, const glm::vec2& uvMin, const glm::vec2& uvMax, float radius) {
    DrawQuad(position, size, texture->GetRendererID(), color, uvMin, uvMax, radius);
}

void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, uint32_t textureID, const glm::vec4& color, const glm::vec2& uvMin, const glm::vec2& uvMax, float radius) {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f))
        * glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

    if (s_Data->QuadIndexCount >= Renderer2DData::MaxIndices) FlushAndReset();

    float textureIndex = 0.0f;
    if (textureID != 0) {
        for (uint32_t i = 1; i < s_Data->TextureSlotIndex; i++) {
            if (s_Data->TextureSlots[i] == textureID) {
                textureIndex = (float)i;
                break;
            }
        }
        if (textureIndex == 0.0f) {
            if (s_Data->TextureSlotIndex >= Renderer2DData::MaxTextureSlots) FlushAndReset();
            textureIndex = (float)s_Data->TextureSlotIndex;
            s_Data->TextureSlots[s_Data->TextureSlotIndex] = textureID;
            s_Data->TextureSlotIndex++;
        }
    }

    glm::vec2 uvs[4] = { {uvMin.x, uvMin.y}, {uvMax.x, uvMin.y}, {uvMax.x, uvMax.y}, {uvMin.x, uvMax.y} };

    for (int i = 0; i < 4; i++) {
        s_Data->QuadVertexBufferPtr->Position = transform * s_Data->QuadVertexPositions[i];
        s_Data->QuadVertexBufferPtr->Color = color;
        s_Data->QuadVertexBufferPtr->TexCoord = uvs[i];
        s_Data->QuadVertexBufferPtr->TexIndex = textureIndex;
        s_Data->QuadVertexBufferPtr->QuadSize = size;
        s_Data->QuadVertexBufferPtr->Radius = radius;
        s_Data->QuadVertexBufferPtr++;
    }

    s_Data->QuadIndexCount += 6;
    Renderer::GetStats().TriangleCountUI += 2;
}

// =========================================================================================
// ZMIANA 3: To jest najwa¿niejsza zmiana na samym dole! Zwróæ na to uwagê.
// Poni¿sze funkcje przekazuj¹ radius do GPU.
// =========================================================================================

void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color, float radius) {
    DrawQuad(transform, 0, color, radius);
}

void Renderer2D::DrawQuad(const glm::mat4& transform, uint32_t textureID, const glm::vec4& tintColor, float radius) {
    if (s_Data->QuadIndexCount >= Renderer2DData::MaxIndices) FlushAndReset();

    float textureIndex = 0.0f;
    if (textureID != 0) {
        for (uint32_t i = 1; i < s_Data->TextureSlotIndex; i++) {
            if (s_Data->TextureSlots[i] == textureID) {
                textureIndex = (float)i;
                break;
            }
        }
        if (textureIndex == 0.0f) {
            if (s_Data->TextureSlotIndex >= Renderer2DData::MaxTextureSlots) FlushAndReset();
            textureIndex = (float)s_Data->TextureSlotIndex;
            s_Data->TextureSlots[s_Data->TextureSlotIndex] = textureID;
            s_Data->TextureSlotIndex++;
        }
    }

    glm::vec2 uvs[4] = { {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f} };

    for (int i = 0; i < 4; i++) {
        s_Data->QuadVertexBufferPtr->Position = transform * s_Data->QuadVertexPositions[i];
        s_Data->QuadVertexBufferPtr->Color = tintColor;
        s_Data->QuadVertexBufferPtr->TexCoord = uvs[i];
        s_Data->QuadVertexBufferPtr->TexIndex = textureIndex;

        // Wydobywamy skalê bezpoœrednio z macierzy transformacji, bo Shader tego wymaga 
        // do wyliczenia dystansu (SDF - Signed Distance Field) dla zaokr¹gleñ
        glm::vec2 scale = { glm::length(glm::vec3(transform[0])), glm::length(glm::vec3(transform[1])) };
        s_Data->QuadVertexBufferPtr->QuadSize = scale;

        s_Data->QuadVertexBufferPtr->Radius = radius; // Tego tutaj wczeœniej brakowa³o!
        s_Data->QuadVertexBufferPtr++;
    }

    s_Data->QuadIndexCount += 6;
    Renderer::GetStats().TriangleCountUI += 2;
}