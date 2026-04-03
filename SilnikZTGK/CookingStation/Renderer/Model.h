#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <spdlog/spdlog.h> 
#include "Mesh.h"
#include "Shader.h"
#include "CookingStation/Renderer/Texture.h"

#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>

using namespace std;

class Model
{
public:
    vector<MeshTexture> textures_loaded;
    vector<Mesh> meshes;
    string FilePath;
    string directory;
    bool gammaCorrection;

    Model(string const& path, bool gamma = false) : gammaCorrection(gamma)
    {
        FilePath = path;
        loadModel(path);
    }

    void Draw(Shader& shader)
    {
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

private:
    const aiScene* m_ScenePtr = nullptr;

    void loadModel(string const& path)
    {
        spdlog::info("Wczytywanie modelu: {}", path);

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            spdlog::error("Blad Assimp: {}", importer.GetErrorString());
            return;
        }

        m_ScenePtr = scene;
        directory = path.substr(0, path.find_last_of('/'));

        processNode(scene->mRootNode, scene);

        spdlog::info("Model wczytano poprawnie: {}", path);
    }

    void processNode(aiNode* node, const aiScene* scene)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene)
    {
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<MeshTexture> textures;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

            if (mesh->HasNormals())
                vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

            if (mesh->mTextureCoords[0])
                vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            if (mesh->mTextureCoords[1])
                vertex.TexCoords2 = glm::vec2(mesh->mTextureCoords[1][i].x, mesh->mTextureCoords[1][i].y);
            else
                vertex.TexCoords2 = glm::vec2(0.0f, 0.0f);

            vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // 1. Ładujemy bazowy kolor (palette.png)
        vector<MeshTexture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        // 2. Ładujemy detal (detal.png), który w glTF siedzi w Emissive
        vector<MeshTexture> emissiveMaps = loadMaterialTextures(material, aiTextureType_EMISSIVE, "texture_diffuse");
        textures.insert(textures.end(), emissiveMaps.begin(), emissiveMaps.end());

        return Mesh(vertices, indices, textures);
    }

    vector<MeshTexture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName)
    {
        vector<MeshTexture> textures;
        unsigned int texCount = mat->GetTextureCount(type);

        for (unsigned int i = 0; i < texCount; i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);

            bool skip = false;
            for (unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }

            if (!skip)
            {
                MeshTexture texture;
                const aiTexture* embeddedTex = m_ScenePtr->GetEmbeddedTexture(str.C_Str());

                if (embeddedTex)
                {
                    texture.Texture2DPtr = std::make_shared<Texture2D>(
                        reinterpret_cast<unsigned char*>(embeddedTex->pcData),
                        embeddedTex->mWidth
                    );
                }
                else
                {
                    string fullPath = this->directory + '/' + str.C_Str();
                    texture.Texture2DPtr = std::make_shared<Texture2D>(fullPath);
                }

                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
        return textures;
    }
};

#endif