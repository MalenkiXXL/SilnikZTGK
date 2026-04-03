#ifndef MESH_H
#define MESH_H

#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"
#include <string>
#include <memory>
#include <vector>

#include "CookingStation/Renderer/VertexArray.h"
#include "CookingStation/Renderer/Buffer.h"
#include "CookingStation/Renderer/Renderer.h"
#include "CookingStation/Renderer/RenderCommand.h"
#include "CookingStation/Renderer/Texture.h"

using namespace std;

#define MAX_BONE_INFLUENCE 4

// Struktura opisuj�ca pojedynczy wierzcho�ek 3D
struct Vertex {
    glm::vec3 Position;  // Pozycja w przestrzeni
    glm::vec3 Normal;    // Wektor normalny (kierunek, w kt�rym "patrzy" wierzcho�ek)
    glm::vec2 TexCoords; // Wsp�rz�dne tekstury (U, V)
    glm::vec2 TexCoords2; // Wsp�rz�dne tekstury (U, V)
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    float m_Weights[MAX_BONE_INFLUENCE];
};

// Struktura przechowuj�ca ID za�adowanej tekstury i jej typ (diffuse/specular)
struct MeshTexture {
    std::shared_ptr<Texture2D> Texture2DPtr;
    string type;
    string path;
};

// Klasa Mesh to pojedyncza "siatka". Model mo�e sk�ada� si� z wielu takich siatek (np. ludzik: osobna siatka na cia�o, osobna na bro�)
class Mesh {
public:
    // Dane siatki
    vector<Vertex>       vertices;
    vector<unsigned int> indices;
    vector<MeshTexture> textures;
    std::shared_ptr<VertexArray> m_VertexArray;

    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<MeshTexture> textures)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;
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

        // 3. Informujemy shader, czy ma użyć drugiej tekstury w obliczeniach [cite: 8, 10]
        shader.SetBool("useTexture2", diffuseCount > 1);

        // 4. Przelatujemy przez wszystkie tekstury przypięte do tej siatki
        for (unsigned int i = 0; i < textures.size(); i++)
        {
            // Aktywujemy odpowiednie gniazdo tekstury (GL_TEXTURE0 + i)
            glActiveTexture(GL_TEXTURE0 + i);

            string number;
            string name = textures[i].type;

            // Logika nadawania nazw: texture_diffuse1, texture_diffuse2 itd. [cite: 7]
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

        // 6. Resetujemy aktywną jednostkę teksturującą do domyślnej
        glActiveTexture(GL_TEXTURE0);
    }

private:
    // Tworzenie VAO, VBO, EBO - tak samo jak przy rysowaniu zwyk�ej kostki, tylko z u�yciem p�tli dla wektor�w
    void setupMesh()
    {
        //tworzymy vao
        m_VertexArray = std::make_shared<VertexArray>();

        //tworzymy vbo 
        std::shared_ptr<VertexBuffer> vertexBuffer;
        //c++ musi potraktowac nasz wektor jako plaska tablice floatow
        vertexBuffer = std::make_shared<VertexBuffer>((float*)&vertices[0], vertices.size() * sizeof(Vertex));

        vertexBuffer->SetLayout(
            {
                {ShaderDataType::Float3, "aPos"},
                {ShaderDataType::Float3, "aNormal"},
                {ShaderDataType::Float2, "aTexCoords"},
                {ShaderDataType::Float2, "aTexCoords2"},
                //te 4 rzeczy zostawiamy dla Assimpa
                {ShaderDataType::Float3, "aTangent"},
                {ShaderDataType::Float3, "aBitangent"},
                {ShaderDataType::Int4, "aBoneIDs"},
                {ShaderDataType::Float4, "aWeights"},
            }
        );

        //vao przejmuje vbo
        m_VertexArray->AddVertexBuffer(vertexBuffer);

        //tworzymy ebo
        std::shared_ptr<IndexBuffer> indexBuffer;
        indexBuffer = std::make_shared<IndexBuffer>(&indices[0], indices.size());

        m_VertexArray->SetIndexBuffer(indexBuffer);
        //glGenVertexArrays(1, &VAO);
        //glGenBuffers(1, &VBO);
        //glGenBuffers(1, &EBO);

        //glBindVertexArray(VAO);
        //glBindBuffer(GL_ARRAY_BUFFER, VBO);

        //glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        //glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        //// Pozycje
        //glEnableVertexAttribArray(0);
        //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        //// Normalne
        //glEnableVertexAttribArray(1);
        //glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        //// Tekstury
        //glEnableVertexAttribArray(2);
        //glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

        //glBindVertexArray(0);
    }
};
#endif