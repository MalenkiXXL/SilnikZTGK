#pragma once
#include <string>
#include <glad/glad.h> 

class Texture {
public:
    Texture(const std::string& path);
    Texture(uint32_t width, uint32_t height); 
    ~Texture();
    inline uint32_t GetRendererID() const { return m_RendererID; }
    void Bind(uint32_t slot = 0) const;

    void SetData(void* data, uint32_t size);

    inline uint32_t GetWidth() const { return m_Width; }
    inline uint32_t GetHeight() const { return m_Height; }

private:
    uint32_t m_RendererID = 0; 
    uint32_t m_Width = 0, m_Height = 0;
    GLenum m_InternalFormat = 0, m_DataFormat = 0;
    std::string m_FilePath;
};