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

        // ZABEZPIECZENIE DLA TYPÓW CA£KOWITOLICZBOWYCH (np. Koœci dla animacji)
        if (element.Type == ShaderDataType::Int ||
            element.Type == ShaderDataType::Int2 ||
            element.Type == ShaderDataType::Int3 ||
            element.Type == ShaderDataType::Int4 ||
            element.Type == ShaderDataType::Bool)
        {
            // Funkcja z 'I' (IPointer) wzywana dla œcis³ych intów
            glVertexAttribIPointer(
                m_VertexBufferIndex,
                element.GetComponentCount(),
                ShaderDataTypeToOpenGLBaseType(element.Type),
                vertexBuffer->GetLayout().GetStride(),
                (const void*)(size_t)element.Offset);
        }
        else
        {
            // Standardowe floaty
            glVertexAttribPointer(
                m_VertexBufferIndex, //ktory atrybut np 0
                element.GetComponentCount(), //z ilu liczb sie sklada np 3 dla Float3
                ShaderDataTypeToOpenGLBaseType(element.Type), //jaki to typ opengla
                element.Normalized ? GL_TRUE : GL_FALSE, //czy normalizowac
                vertexBuffer->GetLayout().GetStride(), //co ile bajtow skakac do nastepnego wierzcholka
                (const void*)(size_t)element.Offset); //gdzie wewntatrz wierzcholka zaczyna sie wartosc
        }

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

void VertexArray::AddInstanceBuffer(const std::shared_ptr<VertexBuffer>& instanceBuffer)
{
    glBindVertexArray(m_RenderID);
    instanceBuffer->Bind();

    for (const auto& element : instanceBuffer->GetLayout())
    {
        // Macierz 4x4 w OpenGL to specjalny przypadek - nie mieœci siê w 1 slocie (max vec4).
        // Musimy rozbiæ j¹ na 4 osobne atrybuty (wiersze macierzy).
        if (element.Type == ShaderDataType::Mat4)
        {
            uint8_t count = 4;
            for (uint8_t i = 0; i < count; i++)
            {
                glEnableVertexAttribArray(m_VertexBufferIndex);
                glVertexAttribPointer(
                    m_VertexBufferIndex,
                    4, // Ka¿dy wektor w macierzy ma 4 floaty
                    ShaderDataTypeToOpenGLBaseType(element.Type),
                    element.Normalized ? GL_TRUE : GL_FALSE,
                    instanceBuffer->GetLayout().GetStride(),
                    (const void*)(element.Offset + sizeof(float) * 4 * i) // Przesuniêcie dla ka¿dego wiersza
                );

                // MAGIA INSTANCJONOWANIA: ten atrybut zmienia siê co 1 instancjê
                glVertexAttribDivisor(m_VertexBufferIndex, 1);
                m_VertexBufferIndex++;
            }
        }
        else
        {
            // Dla innych atrybutów instancji (np. nasz float UVOffset)
            glEnableVertexAttribArray(m_VertexBufferIndex);

            if (element.Type == ShaderDataType::Int ||
                element.Type == ShaderDataType::Int2 ||
                element.Type == ShaderDataType::Int3 ||
                element.Type == ShaderDataType::Int4 ||
                element.Type == ShaderDataType::Bool)
            {
                glVertexAttribIPointer(
                    m_VertexBufferIndex,
                    element.GetComponentCount(),
                    ShaderDataTypeToOpenGLBaseType(element.Type),
                    instanceBuffer->GetLayout().GetStride(),
                    (const void*)(size_t)element.Offset
                );
            }
            else
            {
                glVertexAttribPointer(
                    m_VertexBufferIndex,
                    element.GetComponentCount(),
                    ShaderDataTypeToOpenGLBaseType(element.Type),
                    element.Normalized ? GL_TRUE : GL_FALSE,
                    instanceBuffer->GetLayout().GetStride(),
                    (const void*)(size_t)element.Offset
                );
            }

            // MAGIA INSTANCJONOWANIA cd.
            glVertexAttribDivisor(m_VertexBufferIndex, 1);
            m_VertexBufferIndex++;
        }
    }
    m_VertexBuffers.push_back(instanceBuffer);
}