#include "Texture.h"
#include "stb_image.h"
#include <glad/glad.h>
#include <iostream>

Texture2D::Texture2D(const std::string& path)
{
	stbi_set_flip_vertically_on_load(1);
	int width, height, channels;

	stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

	if (data)
	{
		m_Width = width;
		m_Height = height;

		GLenum internalFormat = 0, dataFormat = 0;
		if (channels == 4)
		{
			internalFormat = GL_RGBA8;
			dataFormat = GL_RGBA;
		}
		else if (channels == 3)
		{
			internalFormat = GL_RGB8;
			dataFormat = GL_RGB;
		}

		glGenTextures(1, &m_RendererID);
		glBindTexture(GL_TEXTURE_2D, m_RendererID);

		// Ustawienia filtrowania i powtarzania
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		// Przes³anie pikseli na kartê graficzn¹
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Nie udalo sie wczytac tekstury z pliku: " << path << std::endl;
	}
}

Texture2D::Texture2D(const unsigned char* data, uint32_t size)
{
	// glTF/GLB zazwyczaj nie wymagaj¹ flipowania UV, ale jeœli Twoje UV 
	// s¹ odwrócone, zostawiamy to zgodnie z Twoim projektem.
	stbi_set_flip_vertically_on_load(1);

	int width, height, channels;

	// ZMIANA: Prosimy stbi o za³adowanie obrazu zawsze jako 4 kana³y (RGBA), 
	// aby unikn¹æ problemów z czarnym t³em detali.
	unsigned char* pixels = stbi_load_from_memory(data, size, &width, &height, &channels, 4);

	if (pixels)
	{
		m_Width = width;
		m_Height = height;

		// Skoro wymusiliœmy 4 kana³y w stbi_load_from_memory, 
		// u¿ywamy formatów wspieraj¹cych przezroczystoœæ (Alpha).
		GLenum internalFormat = GL_RGBA8;
		GLenum dataFormat = GL_RGBA;

		glGenTextures(1, &m_RendererID);
		glBindTexture(GL_TEXTURE_2D, m_RendererID);

		// Ustawienia filtrowania
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Ustawienia zawijania - wa¿ne dla tekstur detali (buŸki)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, dataFormat, GL_UNSIGNED_BYTE, pixels);
		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(pixels);

		// Debug: potwierdzenie wczytania w konsoli
		// std::cout << "Wczytano teksture wbudowana: " << m_Width << "x" << m_Height << std::endl;
	}
	else
	{
		std::cout << "B£¥D: Nie uda³o siê wczytaæ wbudowanej tekstury z pamiêci! Powód: " << stbi_failure_reason() << std::endl;
	}
}

Texture2D::~Texture2D()
{
	glDeleteTextures(1, &m_RendererID);
}

void Texture2D::Bind(uint32_t slot) const
{
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, m_RendererID);
}