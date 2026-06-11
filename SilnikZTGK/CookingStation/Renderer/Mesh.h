#ifndef MESH_H
#define MESH_H

#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"
#include <string>
#include <memory>
#include <vector>
#include <cstddef> 
#include "CookingStation/Math/Geometry.h"
#include <limits>
#include <cmath>
#include "CookingStation/Renderer/VertexArray.h"
#include "CookingStation/Renderer/Buffer.h"
#include "CookingStation/Renderer/Renderer.h" 
#include "CookingStation/Renderer/RenderCommand.h"
#include "CookingStation/Renderer/Texture2D.h"

using namespace std;

#define MAX_BONE_INFLUENCE 4

struct Vertex {
    glm::vec3 Position;  
    glm::vec3 Normal;   
    glm::vec2 TexCoords;
    glm::vec2 TexCoords2;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    float m_Weights[MAX_BONE_INFLUENCE];
};

struct MeshTexture {
    std::shared_ptr<Texture2D> Texture2DPtr;
    string type;
    string path;

    uint32_t GetID() const {
        return Texture2DPtr ? Texture2DPtr->GetRendererID() : 0;
    }
};

class Mesh {
public:
    AABB localAABB;

    AABB GetWorldAABB(const glm::mat4& transform) const {
        glm::vec3 minLocal = localAABB.center - localAABB.extents;
        glm::vec3 maxLocal = localAABB.center + localAABB.extents;

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

        setupMesh();
    }

    void Draw(Shader& shader)
    {
        shader.use();

        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr = 1;
        unsigned int heightNr = 1;

        int diffuseCount = 0;
        for (const auto& t : textures)
        {
            if (t.type == "texture_diffuse")
                diffuseCount++;
        }

        shader.SetBool("useTexture2", diffuseCount > 1);

        for (unsigned int i = 0; i < textures.size(); i++)
        {
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

            glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);

            textures[i].Texture2DPtr->Bind(i);
        }

        m_VertexArray->Bind();
        RenderCommand::DrawIndexed(m_VertexArray);
        m_VertexArray->Unbind();

        for (unsigned int i = 0; i < textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0); 
        }

        glActiveTexture(GL_TEXTURE0);
    } 

private:
    void setupMesh()
    {
        m_VertexArray = std::make_shared<VertexArray>();

        std::shared_ptr<VertexBuffer> vertexBuffer;
        vertexBuffer = std::make_shared<VertexBuffer>((float*)&vertices[0], vertices.size() * sizeof(Vertex));

        vertexBuffer->SetLayout(
            {
                {ShaderDataType::Float3, "aPos"},         
                {ShaderDataType::Float3, "aNormal"},      
                {ShaderDataType::Float2, "aTexCoords"},   
                {ShaderDataType::Float2, "aTexCoords2"}, 
                {ShaderDataType::Float3, "aTangent"},     
                {ShaderDataType::Float3, "aBitangent"},   
                {ShaderDataType::Int4, "aBoneIDs"},     
                {ShaderDataType::Float4, "aWeights"},    
            }
            );

        m_VertexArray->AddVertexBuffer(vertexBuffer);

        std::shared_ptr<IndexBuffer> indexBuffer;
        indexBuffer = std::make_shared<IndexBuffer>(&indices[0], indices.size());
        m_VertexArray->SetIndexBuffer(indexBuffer);

        glGenBuffers(1, &m_InstanceVBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_InstanceVBO);

        constexpr std::size_t maxInstances = 20000;
        glBufferData(GL_ARRAY_BUFFER, maxInstances * sizeof(InstanceData), nullptr, GL_DYNAMIC_DRAW);

        m_VertexArray->Bind();

        std::size_t vec4Size = sizeof(glm::vec4);

        for (int i = 0; i < 4; i++)
        {
            glEnableVertexAttribArray(8 + i);
            glVertexAttribPointer(8 + i, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(i * vec4Size));
            glVertexAttribDivisor(8 + i, 1);
        }

        glEnableVertexAttribArray(12);
        glVertexAttribPointer(12, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, UVOffset));
        glVertexAttribDivisor(12, 1);

        glEnableVertexAttribArray(13);
        glVertexAttribPointer(13, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)offsetof(InstanceData, HighlightColor));
        glVertexAttribDivisor(13, 1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        m_VertexArray->Unbind();
    }
};

#endif