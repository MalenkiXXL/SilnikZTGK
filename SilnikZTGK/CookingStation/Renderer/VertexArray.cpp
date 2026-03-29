#include "VertexArray.h"
#include <glad/glad.h>

//tlumaczy nasz typ na opengla
static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
{
    switch (type)
    {
        case ShaderDataType::Float:    return GL_FLOAT;
        case ShaderDataType::Float2:   return GL_FLOAT;
        case ShaderDataType::Float3:   return GL_FLOAT;
        case ShaderDataType::Float4:   return GL_FLOAT;
        case ShaderDataType::Mat3:     return GL_FLOAT;
        case ShaderDataType::Mat4:     return GL_FLOAT;
        case ShaderDataType::Int:      return GL_INT;
        case ShaderDataType::Int2:     return GL_INT;
        case ShaderDataType::Int3:     return GL_INT;
        case ShaderDataType::Int4:     return GL_INT;
        case ShaderDataType::Bool:     return GL_BOOL;
    }
    return 0;
}

VertexArray::VertexArray()
{
    //tworzy obiekt vao
    glGenVertexArrays(1, &m_RenderID);
}
VertexArray::~VertexArray()
{
    glDeleteVertexArrays(1, &m_RenderID);
}

void VertexArray::Bind() const
{
    //wczytuje caly stan wszystkich dodanych bufforow naraz
    glBindVertexArray(m_RenderID);
}

void VertexArray::Unbind() const
{
    //odpina vao
    glBindVertexArray(0);
}

//vao czyta BufferLayout i tlumaczy go
void VertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer)
{
    //aktywujemy vao
    glBindVertexArray(m_RenderID);
    //aktyrujemy przekazany buffor
    vertexBuffer->Bind();
    //przez kazdy element w Layoutcie podanego buffora
    for (const auto& element : vertexBuffer->GetLayout())
    {
        //wlacza odczyt dla danego atrybutu
        glEnableVertexAttribArray(m_VertexBufferIndex);
        //mowi jak czytac bajty dla atrybutu
        glVertexAttribPointer(
            m_VertexBufferIndex, //ktory atrybut np 0
            element.GetComponentCount(), //z ilu liczb sie sklada np 3 dla Float3
            ShaderDataTypeToOpenGLBaseType(element.Type), //jaki to typ opengla
            element.Normalized ? GL_TRUE : GL_FALSE, //czy normalizowac
            vertexBuffer->GetLayout().GetStride(), //co ile bajtow skakac do nastepnego wierzcholka
            (const void*)(size_t)element.Offset); //gdzie wewntatrz wierzcholka zaczyna sie wartosc
        m_VertexBufferIndex++;
    }
    //zapisujemy buffor do naszej wewnetrznej listy 
    m_VertexBuffers.push_back(vertexBuffer); 
}

//podpinanie buffora z indeksami do vao
void VertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer)
{
    //aktywujemy to vao
    glBindVertexArray(m_RenderID);
    //aktywujemy buffor indeksow 
    indexBuffer->Bind();
    //zapisujemy wskaznik zeby go trzymac przy zyciu
    m_IndexBuffer = indexBuffer;
}
