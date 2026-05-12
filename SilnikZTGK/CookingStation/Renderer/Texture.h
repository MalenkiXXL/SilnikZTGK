#pragma once
#include <string>
#include <stdint.h>
#include <glad/glad.h>

class Texture2D
{
public:
	Texture2D(const std::string& path, GLenum wrapMode = GL_CLAMP_TO_EDGE);
	Texture2D(const unsigned char* data, uint32_t size, GLenum wrapMode = GL_CLAMP_TO_EDGE);
	~Texture2D();

	inline uint32_t GetWidth() const { return m_Width; }
	inline uint32_t GetHeight() const { return m_Height; }
	inline uint32_t GetRendererID() const { return m_RendererID; }

	void Bind(uint32_t slot = 0) const;
	void SetWrapMode(GLenum wrapMode);
private:
	std::string m_Path;
	uint32_t m_RendererID;
	uint32_t m_Width, m_Height;
	GLenum m_CurrentWrapMode = GL_CLAMP_TO_EDGE;
};

