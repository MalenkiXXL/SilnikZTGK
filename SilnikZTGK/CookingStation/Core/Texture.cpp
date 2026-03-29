#include "Texture.h"
#include <stb_image.h> 

Texture::Texture(const std::string& path) : m_FilePath(path) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);

    if (data) {
        m_Width = width;
        m_Height = height;
        m_InternalFormat = GL_RGBA8;
        m_DataFormat = GL_RGBA;

        glGenTextures(1, &m_RendererID);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height, 0, m_DataFormat, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }
}

Texture::Texture(uint32_t width, uint32_t height)
    : m_Width(width), m_Height(height)
{
    m_InternalFormat = GL_RGBA8;
    m_DataFormat = GL_RGBA;

    glGenTextures(1, &m_RendererID);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);

    // Ustawianie arametrow, ¿eby tekstura nie by³a rozmyta
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Rezerwowanie na GPU
    glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height, 0, m_DataFormat, GL_UNSIGNED_BYTE, nullptr);
}

void Texture::SetData(void* data, uint32_t size) {
    // Sprawdzenie,czy przesy³ane dane pasuj¹ do rozmiaru tekstury (np. 1x1 piksel * 4 bajty)
    uint32_t bpp = m_DataFormat == GL_RGBA ? 4 : 3;
    if (size == m_Width * m_Height * bpp) {
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, m_DataFormat, GL_UNSIGNED_BYTE, data);
    }
}

Texture::~Texture() {
    glDeleteTextures(1, &m_RendererID);
}

void Texture::Bind(uint32_t slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);
}