#pragma once
#include "Buffer.h"
#include <vector>
#include <memory>


//laczy vbo i ebo w jeden obiekt do rysowania
class VertexArray
{
public:
	//tworzy strukture vao
	VertexArray();
	~VertexArray();

	//aktywuje konfiguracje przed rysowaniem
	void Bind() const;
	void Unbind() const;

	//przekazuje wskaznik na VertexBuffer zeby vao go zapisalo i polaczylo z layoutem
	void AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer);
	//przekazuje wskaznik na IndexBUffer zeby wiedziec jak laczyc punkty z VertexBuffera
	void SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer);

	//zwraca liste podpietych wierzcholkow
	const std::vector<std::shared_ptr<VertexBuffer>>& GetVertexBuffers() const { return m_VertexBuffers; }
	//zwraca podpiete indexy
	const std::shared_ptr<IndexBuffer>& GetIndexBuffers() const { return m_IndexBuffer; }

private:
	//numer id tego vao
	uint32_t m_RenderID;
	//liczy dodane atrybuty (0 - pozycja, 1 - kolor itd.)
	uint32_t m_VertexBufferIndex = 0;
	//tablica przechowujaca wskazniki na pudla z danymi
	std::vector<std::shared_ptr<VertexBuffer>> m_VertexBuffers;
	//wskaznik na pudlo z instrukcja laczenia wierzcholkow
	std::shared_ptr<IndexBuffer> m_IndexBuffer;
};

