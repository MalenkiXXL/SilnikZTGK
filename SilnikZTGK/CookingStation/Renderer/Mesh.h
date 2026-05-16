#ifndef MESH_H
#define MESH_H

#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"
#include <string>
#include <memory>
#include <vector>
#include <cstddef> // DODANE: Wymagane dla makra offsetof
#include "CookingStation/Math/Geometry.h"
#include <limits>
#include <cmath>

#include "CookingStation/Renderer/VertexArray.h"
#include "CookingStation/Renderer/Buffer.h"
#include "CookingStation/Renderer/Renderer.h" // Tutaj znajduje się definicja naszej struktury InstanceData
#include "CookingStation/Renderer/RenderCommand.h"
#include "CookingStation/Renderer/Texture.h"

using namespace std;

#define MAX_BONE_INFLUENCE 4

// Struktura opisujaca pojedynczy wierzcholek 3D
struct Vertex {
    glm::vec3 Position;  // Pozycja w przestrzeni
    glm::vec3 Normal;    // Wektor normalny (kierunek, w ktorym "patrzy" wierzcholek)
    glm::vec2 TexCoords; // Wspolrzedne tekstury (U, V)
    glm::vec2 TexCoords2; // Wspolrzedne tekstury (U, V)
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    float m_Weights[MAX_BONE_INFLUENCE];
};

// Struktura przechowujaca ID zaladowanej tekstury i jej typ (diffuse/specular)
struct MeshTexture {
    std::shared_ptr<Texture2D> Texture2DPtr;
    string type;
    string path;

    uint32_t GetID() const {
        return Texture2DPtr ? Texture2DPtr->GetRendererID() : 0;
    }
};

// Klasa Mesh to pojedyncza "siatka". Model moze skladac sie z wielu takich siatek (np. ludzik: osobna siatka na cialo, osobna na bron)
class Mesh {
public:
    AABB localAABB;

    AABB GetWorldAABB(const glm::mat4& transform) const {
        // --- POPRAWKA OPTYMALIZACYJNA FPS #2: Transformacja 8 rożków zamiast całej siatki ---

        // 1. Odtwarzamy granice Min/Max w Local Space za pomocą środka (center) i wielkości (extents)
        glm::vec3 minLocal = localAABB.center - localAABB.extents;
        glm::vec3 maxLocal = localAABB.center + localAABB.extents;

        // 2. Definiujemy 8 narożników lokalnego pudełka
        glm::vec3 corners[8] = {
            glm::vec3(minLocal.x, minLocal.y, minLocal.z),
            glm::vec3(maxLocal.x, minLocal.y, minLocal.z),
            glm::vec3(minLocal.x, maxLocal.y, minLocal.z),
            glm::vec3(maxLocal.x, maxLocal.y, minLocal.z),
            glm::vec3(minLocal.x, minLocal.y, maxLocal.z),
            glm::vec3(maxLocal.x, minLocal.y, maxLocal.z),
            glm::vec3(minLocal.x, maxLocal.y, maxLocal.z),
            glm::vec3(maxLocal.x, maxLocal.y, maxLocal.z)
        };

        // 3. Transformujemy te punkty do przestrzeni świata i wyciągamy nowe skrajne współrzędne
        glm::vec3 worldMin(std::numeric_limits<float>::max());
        glm::vec3 worldMax(std::numeric_limits<float>::lowest());

        for (int i = 0; i < 8; i++) {
            glm::vec3 transformedCorner = glm::vec3(transform * glm::vec4(corners[i], 1.0f));
            worldMin = glm::min(worldMin, transformedCorner);
            worldMax = glm::max(worldMax, transformedCorner);
        }

        AABB resultAABB;
        resultAABB.center = (worldMin + worldMax) * 0.5f;
        resultAABB.extents = (worldMax - worldMin) * 0.5f;

        return resultAABB;
    }

    // Dane siatki
    vector<Vertex>       vertices;
    vector<unsigned int> indices;
    vector<MeshTexture> textures;
    std::shared_ptr<VertexArray> m_VertexArray;
    unsigned int m_InstanceVBO;

    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<MeshTexture> textures)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;

        // --- ZABEZPIECZONE WYLICZANIE LOKALNEGO AABB (WYKONUJE SIĘ TYLKO RAZ PRZY LOADINGU) ---
        if (vertices.empty()) {
            localAABB.center = glm::vec3(0.0f);
            localAABB.extents = glm::vec3(0.0f);
        }
        else {
            glm::vec3 minP(std::numeric_limits<float>::max());
            glm::vec3 maxP(std::numeric_limits<float>::lowest());

            for (const auto& v : vertices) {
                minP = glm::min(minP, v.Position);
                maxP = glm::max(maxP, v.Position);
            }

            localAABB.center = (minP + maxP) * 0.5f;
            localAABB.extents = (maxP - minP) * 0.5f;
        }
        // -----------------------------------------------------------------------------------

        // Po otrzymaniu danych konfigurujemy bufory OpenGL
        setupMesh();
    }

    void Draw(Shader& shader)
    {
        // 1. Zawsze aktywujemy shader przed ustawieniem jakichkolwiek uniformów
        shader.use();

        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr = 1;
        unsigned int heightNr = 1;

        // 2. Liczymy tekstury typu "texture_diffuse". 
        // Jeśli w Model.h wczytałeś map_Kd i map_Ks jako "texture_diffuse", tutaj będzie 2.
        int diffuseCount = 0;
        for (const auto& t : textures)
        {
            if (t.type == "texture_diffuse")
                diffuseCount++;
        }

        // 3. Informujemy shader, czy ma użyć drugiej tekstury w obliczeniach
        shader.SetBool("useTexture2", diffuseCount > 1);

        // 4. Przelatujemy przez wszystkie tekstury przypięte do tej siatki
        for (unsigned int i = 0; i < textures.size(); i++)
        {
            // Aktywujemy odpowiednie gniazdo tekstury (GL_TEXTURE0 + i)
            glActiveTexture(GL_TEXTURE0 + i);

            string number;
            string name = textures[i].type;

            // Logika nadawania nazw: texture_diffuse1, texture_diffuse2 itd.
            if (name == "texture_diffuse")
                number = std::to_string(diffuseNr++);
            else if (name == "texture_specular")
                number = std::to_string(specularNr++);
            else if (name == "texture_normal")
                number = std::to_string(normalNr++);
            else if (name == "texture_height")
                number = std::to_string(heightNr++);

            // Przekazujemy numer slotu (i) do shadera pod wygenerowaną nazwę
            glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);

            // Bindujemy teksturę korzystając z Twojej klasy Texture2D
            textures[i].Texture2DPtr->Bind(i);
        }

        // 5. Rysowanie siatki przy użyciu VAO i Twojego Renderera
        m_VertexArray->Bind();
        RenderCommand::DrawIndexed(m_VertexArray);
        m_VertexArray->Unbind();

        for (unsigned int i = 0; i < textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0); // "0" oznacza brak tekstury
        }
        // ------------------------------------------

        // 6. Resetujemy aktywną jednostkę teksturującą do domyślnej
        glActiveTexture(GL_TEXTURE0);
    } // koniec funkcji Draw

private:
    // Tworzenie VAO, VBO, EBO
    void setupMesh()
    {
        // tworzymy vao
        m_VertexArray = std::make_shared<VertexArray>();

        // tworzymy vbo 
        std::shared_ptr<VertexBuffer> vertexBuffer;
        // c++ musi potraktowac nasz wektor jako plaska tablice floatow
        vertexBuffer = std::make_shared<VertexBuffer>((float*)&vertices[0], vertices.size() * sizeof(Vertex));

        vertexBuffer->SetLayout(
            {
                {ShaderDataType::Float3, "aPos"},         // loc 0
                {ShaderDataType::Float3, "aNormal"},      // loc 1
                {ShaderDataType::Float2, "aTexCoords"},   // loc 2
                {ShaderDataType::Float2, "aTexCoords2"},  // loc 3
                // te 4 rzeczy zostawiamy dla Assimpa
                {ShaderDataType::Float3, "aTangent"},     // loc 4
                {ShaderDataType::Float3, "aBitangent"},   // loc 5
                {ShaderDataType::Int4, "aBoneIDs"},       // loc 6
                {ShaderDataType::Float4, "aWeights"},     // loc 7
            }
            );

        // vao przejmuje vbo
        m_VertexArray->AddVertexBuffer(vertexBuffer);

        // tworzymy ebo
        std::shared_ptr<IndexBuffer> indexBuffer;
        indexBuffer = std::make_shared<IndexBuffer>(&indices[0], indices.size());
        m_VertexArray->SetIndexBuffer(indexBuffer);

        // ==========================================
        //  LOGIKA BUFORA INSTANCJI (InstanceVBO)
        // ==========================================
        glGenBuffers(1, &m_InstanceVBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);

        // Alokujemy pamięć na max 20 000 instancji na strukturę InstanceData!
        constexpr std::size_t maxInstances = 20000;
        glBufferData(GL_ARRAY_BUFFER, maxInstances * sizeof(InstanceData), nullptr, GL_DYNAMIC_DRAW);

        m_VertexArray->Bind();

        std::size_t vec4Size = sizeof(glm::vec4);

        // 1. Ustawienie wskaźników dla MACIERZY TRANSFORMACJI (Lokacje 8, 9, 10, 11)
        for (int i = 0; i < 4; i++)
        {
            glEnableVertexAttribArray(8 + i);
            glVertexAttribPointer(8 + i, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(i * vec4Size));
            glVertexAttribDivisor(8 + i, 1);
        }

        // 2. Ustawienie wskaźnika dla UV OFFSET (Lokacja 12)
        glEnableVertexAttribArray(12);
        glVertexAttribPointer(12, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, UVOffset));
        glVertexAttribDivisor(12, 1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        m_VertexArray->Unbind();
    }
};

#endif