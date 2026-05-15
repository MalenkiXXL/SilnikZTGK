#pragma once
#include <glad/glad.h>
#include <cstdint>

class UniformBuffer {
public:
    UniformBuffer(uint32_t size, uint32_t bindingPoint) {
        glGenBuffers(1, &m_RendererID);
        glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
        glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, m_RendererID);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    ~UniformBuffer() {
        glDeleteBuffers(1, &m_RendererID);
    }

    // Wysy³a dane pod wskazany offset w buforze
    void SetData(const void* data, uint32_t size, uint32_t offset = 0) {
        glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
        glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

private:
    uint32_t m_RendererID = 0;
};