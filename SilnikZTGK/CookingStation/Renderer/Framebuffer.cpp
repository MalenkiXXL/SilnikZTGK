#include "Framebuffer.h"
#include <glad/glad.h> 
#include "spdlog/spdlog.h"

Framebuffer::Framebuffer(const FramebufferSpecification& spec)
    : m_Specification(spec) {
    Invalidate();
}

Framebuffer::~Framebuffer() {
    glDeleteFramebuffers(1, &m_RendererID);
    glDeleteTextures(1, &m_ColorAttachment);
    glDeleteRenderbuffers(1, &m_DepthAttachment);
}

void Framebuffer::Invalidate() {
    if (m_RendererID) {
        glDeleteFramebuffers(1, &m_RendererID);
        glDeleteTextures(1, &m_ColorAttachment);
        glDeleteRenderbuffers(1, &m_DepthAttachment);
    }

    glGenFramebuffers(1, &m_RendererID);
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

    // 1. Za³¹cznik koloru (tekstura, któr¹ zobaczymy w GUI)
    glGenTextures(1, &m_ColorAttachment);
    glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Specification.Width, m_Specification.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Ustawienia filtrowania (wa¿ne dla poprawnego wyœwietlania skalowanego okna)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0);

    // 2. Za³¹cznik g³êbokoœci (Depth Test dla 3D)
    glGenRenderbuffers(1, &m_DepthAttachment);
    glBindRenderbuffer(GL_RENDERBUFFER, m_DepthAttachment);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Specification.Width, m_Specification.Height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthAttachment);

    // Weryfikacja
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        spdlog::error("Framebuffer is incomplete!");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
    // Framebuffer ma swój w³asny rozmiar, musimy o nim poinformowaæ OpenGL
    glViewport(0, 0, m_Specification.Width, m_Specification.Height);
}

void Framebuffer::Unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0 || width > 8192 || height > 8192) {
        return;
    }
    m_Specification.Width = width;
    m_Specification.Height = height;
    Invalidate();
}