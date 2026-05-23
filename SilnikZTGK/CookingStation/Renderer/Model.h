#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/IOSystem.hpp> // <-- DODANE
#include <assimp/IOStream.hpp> // <-- DODANE
#include <spdlog/spdlog.h> 
#include "Mesh.h"
#include "Shader.h"
#include "CookingStation/Renderer/Texture2D.h"
#include "CookingStation/Core/VFS/VFS.h" // <-- DODANE

#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>
#include <filesystem>

using namespace std;

// ====================================================================
// WŁASNY SYSTEM WEJŚCIA/WYJŚCIA DLA ASSIMP (PEŁNA INTEGRACJA Z VFS)
// ====================================================================
class VfsIOStream : public Assimp::IOStream {
private:
    std::vector<uint8_t> m_Data;
    size_t m_Position;
public:
    VfsIOStream(std::vector<uint8_t> data) : m_Data(std::move(data)), m_Position(0) {}
    ~VfsIOStream() override = default;

    size_t Read(void* pvBuffer, size_t pSize, size_t pCount) override {
        size_t bytesToRead = pSize * pCount;
        if (m_Position + bytesToRead > m_Data.size()) {
            bytesToRead = m_Data.size() - m_Position;
        }
        if (bytesToRead > 0) {
            std::memcpy(pvBuffer, m_Data.data() + m_Position, bytesToRead);
            m_Position += bytesToRead;
        }
        return bytesToRead / pSize;
    }

    size_t Write(const void* pvBuffer, size_t pSize, size_t pCount) override { return 0; } // Tylko do odczytu

    aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override {
        if (pOrigin == aiOrigin_SET) m_Position = pOffset;
        else if (pOrigin == aiOrigin_CUR) m_Position += pOffset;
        else if (pOrigin == aiOrigin_END) m_Position = m_Data.size() - pOffset;

        if (m_Position > m_Data.size()) m_Position = m_Data.size();
        return aiReturn_SUCCESS;
    }

    size_t Tell() const override { return m_Position; }
    size_t FileSize() const override { return m_Data.size(); }
    void Flush() override {}
};
class VfsIOSystem : public Assimp::IOSystem {
public:
    bool Exists(const char* pFile) const override {
        std::string path = pFile;
        std::replace(path.begin(), path.end(), '\\', '/');

        // 1. Niezawodne sprawdzenie fizyczne (często używane przez pliki .bin podpinane w glTF)
        std::ifstream f(path);
        if (f.good()) return true;

        // 2. Jeśli nie ma fizycznie, zakładamy, że to plik wirtualny
        return path.find("assets://") == 0;
    }

    char getOsSeparator() const override { return '/'; }

    Assimp::IOStream* Open(const char* pFile, const char* pMode = "rb") override {
        std::string path = pFile;
        std::replace(path.begin(), path.end(), '\\', '/');

        std::vector<uint8_t> data;

        // 1. Jeśli to poprawny zasób wirtualny, prosimy VFS
        if (path.find("assets://") == 0) {
            data = VFS::ReadFile(path);
        }
        // 2. Jeśli to "goła" ścieżka (np. CookingStation/Assets/...), odpalamy Fallback
        else {
            data = VFS::ReadFile(path); // Sprawdzamy, czy VFS może to ogarnie

            if (data.empty()) {
                // Bezpośrednie czytanie bajtów przez standardowe biblioteki C++
                std::ifstream file(path, std::ios::binary | std::ios::ate);
                if (file.is_open()) {
                    std::streamsize size = file.tellg();
                    file.seekg(0, std::ios::beg);
                    data.resize(size);
                    file.read(reinterpret_cast<char*>(data.data()), size);
                }
            }
        }

        // KRYTYCZNE: Jeśli pliku nie ma nigdzie, musimy zwrócić nullptr!
        if (data.empty()) {
            return nullptr;
        }

        return new VfsIOStream(std::move(data));
    }

    void Close(Assimp::IOStream* pFile) override {
        delete pFile;
    }
};

// ==========================================
// NOWE STRUKTURY DLA ANIMACJI SZKIELETOWEJ
// ==========================================
struct BoneInfo
{
    int id;           // Unikalne ID wysyłane do Vertex Shadera
    glm::mat4 offset; // Offset Matrix (przekształca z przestrzeni modelu do przestrzeni kości)
};

static inline glm::mat4 AssimpMatToGLM(const aiMatrix4x4& from)
{
    glm::mat4 to;
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}

class Model
{
public:
    vector<MeshTexture> textures_loaded;
    vector<Mesh> meshes;
    string FilePath;
    string directory;
    bool gammaCorrection;

    std::map<string, BoneInfo> m_BoneInfoMap;
    int m_BoneCounter = 0;

    auto& GetBoneInfoMap() { return m_BoneInfoMap; }
    int GetBoneCount() { return m_BoneCounter; }

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

    void loadModel(string path) // <-- ZMIANA: Usunięto const&, aby móc edytować ścieżkę
    {
        // --- SANITIZER ŚCIEŻEK (Naprawia czarne modele) ---
        // Jeśli w level01.json (lub gdzie indziej) wciąż jest twarda ścieżka,
        // w locie zamieniamy ją na prawowitą ścieżkę VFS.
        const std::string prefix = "CookingStation/Assets/";
        if (path.find(prefix) == 0) {
            path = "assets://" + path.substr(prefix.length());
        }
        // --------------------------------------------------

        spdlog::info("Wczytywanie modelu z VFS: {}", path);

        Assimp::Importer importer;

        // Zastrzyk naszego VFS! Assimp teraz myśli, że VFS to jego dysk twardy.
        importer.SetIOHandler(new VfsIOSystem());

        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            spdlog::error("Blad Assimp przy wczytywaniu {}: {}", path, importer.GetErrorString());
            return;
        }

        m_ScenePtr = scene;

        // Teraz 'directory' bezpiecznie przechowa format "assets://models/..."
        directory = path.substr(0, path.find_last_of('/'));

        processNode(scene->mRootNode, scene);

        spdlog::info("Model wczytano poprawnie z VFS: {}", path);
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

            for (int j = 0; j < MAX_BONE_INFLUENCE; j++)
            {
                vertex.m_BoneIDs[j] = -1;
                vertex.m_Weights[j] = 0.0f;
            }

            vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        vector<MeshTexture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        vector<MeshTexture> emissiveMaps = loadMaterialTextures(material, aiTextureType_EMISSIVE, "texture_diffuse");
        textures.insert(textures.end(), emissiveMaps.begin(), emissiveMaps.end());

        ExtractBoneWeightForVertices(vertices, mesh, scene);

        return Mesh(vertices, indices, textures);
    }

    void SetVertexBoneData(Vertex& vertex, int boneID, float weight)
    {
        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
        {
            if (vertex.m_BoneIDs[i] < 0)
            {
                vertex.m_Weights[i] = weight;
                vertex.m_BoneIDs[i] = boneID;
                break;
            }
        }
    }

    void ExtractBoneWeightForVertices(vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene)
    {
        for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
        {
            int boneID = -1;
            string boneName = mesh->mBones[boneIndex]->mName.C_Str();

            if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end())
            {
                BoneInfo newBoneInfo;
                newBoneInfo.id = m_BoneCounter;
                newBoneInfo.offset = AssimpMatToGLM(mesh->mBones[boneIndex]->mOffsetMatrix);
                m_BoneInfoMap[boneName] = newBoneInfo;

                boneID = m_BoneCounter;
                m_BoneCounter++;
            }
            else
            {
                boneID = m_BoneInfoMap[boneName].id;
            }

            auto weights = mesh->mBones[boneIndex]->mWeights;
            int numWeights = mesh->mBones[boneIndex]->mNumWeights;

            for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
            {
                int vertexId = weights[weightIndex].mVertexId;
                float weight = weights[weightIndex].mWeight;
                SetVertexBoneData(vertices[vertexId], boneID, weight);
            }
        }
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
                    // Budowanie pełnej ścieżki na podstawie folderu VFS (bez psującego std::filesystem)
                    std::string texPath = str.C_Str();
                    std::replace(texPath.begin(), texPath.end(), '\\', '/');

                    std::string finalPath = this->directory + "/" + texPath;

                    // --- NORMALIZACJA ŚCIEŻEK (POPRAWKA VFS) ---
                    std::string prefix = "assets://";
                    if (finalPath.find(prefix) == 0) {
                        // Odcinamy "assets://"
                        std::string subPath = finalPath.substr(prefix.length());
                        // Normalizujemy sam środek (np. models/warzywka/../../textures/palette.png)
                        subPath = std::filesystem::path(subPath).lexically_normal().generic_string();
                        // Doklejamy nienaruszony przedrostek
                        finalPath = prefix + subPath;
                    }
                    else {
                        // Dla zwykłych ścieżek normalizujemy całość
                        finalPath = std::filesystem::path(finalPath).lexically_normal().generic_string();
                    }

                    texture.Texture2DPtr = std::make_shared<Texture2D>(finalPath);
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