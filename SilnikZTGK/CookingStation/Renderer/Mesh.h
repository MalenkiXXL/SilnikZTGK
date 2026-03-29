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

using namespace std;

#define MAX_BONE_INFLUENCE 4

// Struktura opisuj¹ca pojedynczy wierzcho³ek 3D
struct Vertex {
    glm::vec3 Position;  // Pozycja w przestrzeni
    glm::vec3 Normal;    // Wektor normalny (kierunek, w którym "patrzy" wierzcho³ek)
    glm::vec2 TexCoords; // Wspó³rzêdne tekstury (U, V)
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    float m_Weights[MAX_BONE_INFLUENCE];
};

// Struktura przechowuj¹ca ID za³adowanej tekstury i jej typ (diffuse/specular)
struct MeshTexture {
    unsigned int id;
    string type;
    string path;
};

// Klasa Mesh to pojedyncza "siatka". Model mo¿e sk³adaæ siê z wielu takich siatek (np. ludzik: osobna siatka na cia³o, osobna na broñ)
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

    // Funkcja wywo³ywana z g³ównej pêtli. Wi¹¿e tekstury i zleca karcie graficznej rysowanie
    void Draw(Shader& shader)
    {
        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr = 1;
        unsigned int heightNr = 1;

        // Przelatujemy przez wszystkie tekstury przypiête do tej siatki
        for (unsigned int i = 0; i < textures.size(); i++)
        {
            // Aktywujemy odpowiednie gniazdo tekstury
            glActiveTexture(GL_TEXTURE0 + i);

            string number;
            string name = textures[i].type;
            if (name == "texture_diffuse")
                number = std::to_string(diffuseNr++);
            else if (name == "texture_specular")
                number = std::to_string(specularNr++);
            else if (name == "texture_normal")
                number = std::to_string(normalNr++);
            else if (name == "texture_height")
                number = std::to_string(heightNr++);

            // Przekazujemy numer gniazda do shadera
            glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }

        // Ostateczne rysowanie siatki z przypiêtymi teksturami
        // recznie bindujemy vao
        m_VertexArray->Bind();         
        RenderCommand::DrawIndexed(m_VertexArray);
        m_VertexArray->Unbind();

        
        glActiveTexture(GL_TEXTURE0);

       /* glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glActiveTexture(GL_TEXTURE0);*/
    }

private:
    // Tworzenie VAO, VBO, EBO - tak samo jak przy rysowaniu zwyk³ej kostki, tylko z u¿yciem pêtli dla wektorów
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