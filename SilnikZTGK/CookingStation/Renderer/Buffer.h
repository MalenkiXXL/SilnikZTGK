#pragma once
#include <sstream>
#include <vector>
#include <stdint.h>

//typy danych jakie mozemy wysylac
enum class ShaderDataType
{
	None = 0, Float, Float2, Float3, Float4, Mat3, Mat4, Int, Int2, Int3, Int4, Bool
};

//zmienia typ wyliczeniowy na fizyczny
static uint32_t ShaderDataTypeSize(ShaderDataType type)
{
	switch (type)
	{
	case ShaderDataType::Float: return 4;
	case ShaderDataType::Float2: return 4 * 2;
	case ShaderDataType::Float3: return 4 * 3;
	case ShaderDataType::Float4: return 4 * 4;
	case ShaderDataType::Mat3: return 4 * 3 * 3;
	case ShaderDataType::Mat4: return 4 * 4 * 4;
	case ShaderDataType::Int: return 4;
	case ShaderDataType::Int2: return 4 * 2;
	case ShaderDataType::Int3: return 4 * 3;
	case ShaderDataType::Int4: return 4 * 4;
	case ShaderDataType::Bool: return 1;
	}
	return 0;
}
//pojedyncza cecha wierzcholka np. pozycja
struct BufferElement
{
	std::string Name; //nazwa zmiennej w shaderze np. a_Position
	ShaderDataType Type; //typ danych np float3 dla x,y,z
	uint32_t Size; //rozmiar w bajtach
	size_t Offset = 0; //odleglosc od poczatku wierzcholka 
	bool Normalized = false;

	//konstruktor elementu
	BufferElement(ShaderDataType type, const std::string& name, bool normalized = false)
		: Name(name), Type(type), Size(ShaderDataTypeSize(type)), Offset(0), Normalized(normalized)
	{
	}
	
	//ile kawalkow ma dany typ np Float3 = 3 kawalki
	uint32_t GetComponentCount() const
	{
		switch (Type)
		{
			case ShaderDataType::Float:   return 1;
			case ShaderDataType::Float2:  return 2;
			case ShaderDataType::Float3:  return 3;
			case ShaderDataType::Float4:  return 4;
			case ShaderDataType::Mat3:    return 3; 
			case ShaderDataType::Mat4:    return 4;
			case ShaderDataType::Int:     return 1;
			case ShaderDataType::Int2:    return 2;
			case ShaderDataType::Int3:    return 3;
			case ShaderDataType::Int4:    return 4;
			case ShaderDataType::Bool:    return 1;
		}
		return 0;
	}
};

//przechowuje pelny uklad danych czyli np pozycja + kolor + tekstura
class BufferLayout
{
public:
	BufferLayout() {}

	//przyjmuje liste elemetnow w klamrach {}
	BufferLayout(const std::initializer_list<BufferElement>& element)
		: m_Elements(element) 
	{
		CalculateOffsetAndStride(); //liczy odstepy po zrobieniu listy
	}

	//zwraca rozmiar jednego wierzcho³ka
	inline uint32_t GetStride() const { return m_Stride; }
	inline const std::vector<BufferElement>& GetElements() const { return m_Elements; }

	std::vector<BufferElement>::iterator begin() { return m_Elements.begin(); }
	std::vector<BufferElement>::iterator end() { return m_Elements.end(); }
	std::vector<BufferElement>::const_iterator begin() const { return m_Elements.begin(); }
	std::vector<BufferElement>::const_iterator end() const { return m_Elements.end(); }
private:
	//matemayka wyliczajaca odstepy
	void CalculateOffsetAndStride()
	{
		size_t offset = 0; 
		m_Stride = 0;
		for (auto& element : m_Elements)
		{
			element.Offset = offset; //aktualny offset
			offset += element.Size; //przesuwa offset o rozmiar tego elementu
			m_Stride += element.Size; //dodaje rozmiar elemetu do calkowitego rozmiaru wierzcholka
		}
	}

private:
	std::vector<BufferElement> m_Elements;
	uint32_t m_Stride = 0;
};

class VertexBuffer
{
public:
	//tworzy buffor i laduje dane
	VertexBuffer(float* verticies, uint32_t size);
	//niszczy buffor
	~VertexBuffer();

	//przypisuje instrukcje czytania danych
	void  SetLayout(const BufferLayout& layout) { m_Layout = layout; }
	const BufferLayout& GetLayout() const { return m_Layout; }

	//"uzywaj teraz tego buffora"
	void Bind() const; 
	void Unbind() const;

private:
	uint32_t m_RenderID;
	//obiekt przechowujacy uklad danych tego bufora
	BufferLayout m_Layout;
};

//klasa zarzadzajaca paczka indexow
class IndexBuffer
{
public:
	//tworzy buffor dla trojkatow
	IndexBuffer(uint32_t* indicies, uint32_t count);
	~IndexBuffer();

	void Bind() const;
	void Unbind() const;
	//ilosc punktow do narysowania
	inline uint32_t GetCount() const { return m_Count; }

private:
	uint32_t m_RenderID;
	uint32_t m_Count;
};
