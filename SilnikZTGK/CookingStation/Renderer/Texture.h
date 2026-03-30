#pragma once
#include <string>
#include <stdint.h>

class Texture2D
{
public:
	Texture2D(const std::string& path);
	~Texture2D();

	inline uint32_t GetWidth() const { return m_Width; }
	inline uint32_t GetHeight() const { return m_Height; }
	inline uint32_t GetRendererID() const { return m_RendererID; }

	void Bind(uint32_t slot = 0) const;
private:
	std::string m_Path;
	uint32_t m_RendererID;
	uint32_t m_Width, m_Height;
};

