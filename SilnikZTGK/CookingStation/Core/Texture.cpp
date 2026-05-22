#include "Texture.h"
#include <stb_image.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <glad/glad.h> // ZMIANA: Doda³em jawny nag³ówek OpenGL dla sta³ych GL_TEXTURE_2D itd.

// ZMIANA VFS: Dodajemy includa do systemu wirtualnego systemu plików
#include "CookingStation/Core/VFS/VFS.h"

// Sta³e anizotropii — definiujemy rêcznie jeœli GLAD ich nie eksponuje
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT     0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

Texture::Texture(const std::string& path) : m_FilePath(path) {
    int width, height, channels;

    // Wa¿ne dla uk³adu 2D (ortograficznego)
    stbi_set_flip_vertically_on_load(1);

    // VFS: Zamiast szukaæ na dysku, wczytujemy plik do pamiêci RAM
    std::vector<uint8_t> fileData = VFS::ReadFile(path);
    if (fileData.empty()) {
        spdlog::error("[Texture_GUI] Brak pliku lub blad VFS dla tekstury: {}", path);
        return;
    }

    // NAPRAWA PRZEZROCZYSTOŒCI: Zawsze wymuszamy 4 kana³y (ostatni argument), 
    // aby niezale¿nie od typu pliku zawsze mieæ format RGBA (Alpha = przezroczystoœæ).
    unsigned char* data = stbi_load_from_memory(
        fileData.data(),
        static_cast<int>(fileData.size()),
        &width, &height, &channels, 4);

    if (data) {
        m_Width = width;
        m_Height = height;
        m_InternalFormat = GL_RGBA8;
        m_DataFormat = GL_RGBA;

        glGenTextures(1, &m_RendererID);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);

        glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height,
            0, m_DataFormat, GL_UNSIGNED_BYTE, data);

        glGenerateMipmap(GL_TEXTURE_2D);

        // NAPRAWA PIKSELOZY: Miêkkie, dwuliniowe filtrowanie wyg³adza ikony, fonty i wektory GUI
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // NAPRAWA BIA£YCH OBWÓDEK: CLAMP_TO_EDGE pilnuje, aby piksele z przeciwleg³ej 
        // strony obrazka nie "zawija³y siê" na krawêdziach elementów GUI
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Opcjonalne: Filtrowanie anizotropowe, jeœli wspierane
        const char* extensions = (const char*)glGetString(GL_EXTENSIONS);
        bool hasAniso = extensions && strstr(extensions, "GL_EXT_texture_filter_anisotropic");
        if (hasAniso) {
            float maxAniso = 0.0f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, glm::min(maxAniso, 8.0f));
        }

        stbi_image_free(data);
    }
    else {
        spdlog::error("[Texture_GUI] Blad STB Image: {} (Plik: {})", stbi_failure_reason(), path);
    }
}

// Konstruktor puste tekstury (Framebuffery itp.)
Texture::Texture(uint32_t width, uint32_t height)
    : m_Width(width), m_Height(height)
{
    m_InternalFormat = GL_RGBA8;
    m_DataFormat = GL_RGBA;

    glGenTextures(1, &m_RendererID);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height,
        0, m_DataFormat, GL_UNSIGNED_BYTE, nullptr);
}

void Texture::SetData(void* data, uint32_t size) {
    uint32_t bpp = m_DataFormat == GL_RGBA ? 4 : 3;
    if (size == m_Width * m_Height * bpp) {
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height,
            m_DataFormat, GL_UNSIGNED_BYTE, data);
    }
}

Texture::~Texture() {
    glDeleteTextures(1, &m_RendererID);
}

void Texture::Bind(uint32_t slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_RendererID);
}