#pragma once
#include <string>
#include <glad/glad.h> 

class Texture {
public:
    Texture(const std::string& path);
    Texture(uint32_t width, uint32_t height); 
    ~Texture();

    void Bind(uint32_t slot = 0) const;

    void SetData(void* data, uint32_t size);

    inline uint32_t GetWidth() const { return m_Width; }
    inline uint32_t GetHeight() const { return m_Height; }

private:
    uint32_t m_RendererID;
    uint32_t m_Width, m_Height;
    GLenum m_InternalFormat, m_DataFormat; 
    std::string m_FilePath;
};