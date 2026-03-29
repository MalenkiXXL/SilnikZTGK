#include "Buffer.h"
#include <glad/glad.h>


VertexBuffer::VertexBuffer(float* verticies, uint32_t size)
{
	//karta tworzy buffor i daje id 
	glGenBuffers(1, &m_RenderID);
	//aktywujemy buffor jako tablica wierzcholkow
	glBindBuffer(GL_ARRAY_BUFFER, m_RenderID);
	//wysylamy fizyczne bajty do karty
	glBufferData(GL_ARRAY_BUFFER, size, verticies, GL_STATIC_DRAW);
}
VertexBuffer::~VertexBuffer()
{
	//zwalnia zajmujace miejsce
	glDeleteBuffers(1, &m_RenderID);
}

void VertexBuffer::Bind() const
{
	//przypina buffor przed rysowaniem
	glBindBuffer(GL_ARRAY_BUFFER, m_RenderID);
}

void VertexBuffer::Unbind() const
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

IndexBuffer::IndexBuffer(uint32_t* indicies, uint32_t count)
	: m_Count(count)
{
	glGenBuffers(1, &m_RenderID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RenderID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indicies, GL_STATIC_DRAW);
}
IndexBuffer::~IndexBuffer()
{
	glDeleteBuffers(1, &m_RenderID);
}

void IndexBuffer::Bind() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RenderID);
}

void IndexBuffer::Unbind() const
{
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

