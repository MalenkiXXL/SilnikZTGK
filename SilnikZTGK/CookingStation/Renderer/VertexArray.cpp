#include "VertexArray.h"
#include <glad/glad.h>

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
    glGenVertexArrays(1, &m_RenderID);
}
VertexArray::~VertexArray()
{
    glDeleteVertexArrays(1, &m_RenderID);
}

void VertexArray::Bind() const
{
    glBindVertexArray(m_RenderID);
}

void VertexArray::Unbind() const
{
    glBindVertexArray(0);
}

void VertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer)
{
    glBindVertexArray(m_RenderID);
    vertexBuffer->Bind();

    for (const auto& element : vertexBuffer->GetLayout())
    {
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
                vertexBuffer->GetLayout().GetStride(),
                (const void*)(size_t)element.Offset);
        }
        else
        {
            glVertexAttribPointer(
                m_VertexBufferIndex, 
                element.GetComponentCount(), 
                ShaderDataTypeToOpenGLBaseType(element.Type), 
                element.Normalized ? GL_TRUE : GL_FALSE, 
                vertexBuffer->GetLayout().GetStride(), 
                (const void*)(size_t)element.Offset); 
        }

        m_VertexBufferIndex++;
    }
    m_VertexBuffers.push_back(vertexBuffer);
}

void VertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer)
{
    glBindVertexArray(m_RenderID);
    indexBuffer->Bind();
    m_IndexBuffer = indexBuffer;
}

void VertexArray::AddInstanceBuffer(const std::shared_ptr<VertexBuffer>& instanceBuffer)
{
    glBindVertexArray(m_RenderID);
    instanceBuffer->Bind();

    for (const auto& element : instanceBuffer->GetLayout())
    {
        if (element.Type == ShaderDataType::Mat4)
        {
            uint8_t count = 4;
            for (uint8_t i = 0; i < count; i++)
            {
                glEnableVertexAttribArray(m_VertexBufferIndex);
                glVertexAttribPointer(
                    m_VertexBufferIndex,
                    4, 
                    ShaderDataTypeToOpenGLBaseType(element.Type),
                    element.Normalized ? GL_TRUE : GL_FALSE,
                    instanceBuffer->GetLayout().GetStride(),
                    (const void*)(element.Offset + sizeof(float) * 4 * i) 
                );

                glVertexAttribDivisor(m_VertexBufferIndex, 1);
                m_VertexBufferIndex++;
            }
        }
        else
        {
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

            glVertexAttribDivisor(m_VertexBufferIndex, 1);
            m_VertexBufferIndex++;
        }
    }
    m_VertexBuffers.push_back(instanceBuffer);
}