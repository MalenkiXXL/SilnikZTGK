#include "Framebuffer.h"
#include <glad/glad.h> 
#include "spdlog/spdlog.h"

Framebuffer::Framebuffer(const FramebufferSpecification& spec)
    : m_Specification(spec) {
    Invalidate();
}

Framebuffer::~Framebuffer() {
    glDeleteFramebuffers(1, &m_RendererID);
    if (!m_Specification.DepthOnly) {
        glDeleteTextures(1, &m_ColorAttachment);
        glDeleteRenderbuffers(1, &m_DepthAttachment);
    }
    else {
        glDeleteTextures(1, &m_DepthAttachment); 
    }
}

void Framebuffer::Invalidate() {
    if (m_RendererID) {
        glDeleteFramebuffers(1, &m_RendererID);
        if (!m_Specification.DepthOnly) {
            glDeleteTextures(1, &m_ColorAttachment);
            glDeleteRenderbuffers(1, &m_DepthAttachment);
        }
        else {
            glDeleteTextures(1, &m_DepthAttachment);
        }
    }

    glGenFramebuffers(1, &m_RendererID);
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

    bool multisampled = m_Specification.Samples > 1;

    if (!m_Specification.DepthOnly) {
        GLenum internalFormat = m_Specification.HDR ? GL_RGBA16F : GL_RGBA8;
        GLenum dataType = m_Specification.HDR ? GL_FLOAT : GL_UNSIGNED_BYTE;

        glGenTextures(1, &m_ColorAttachment);
        if (multisampled) {
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_ColorAttachment);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_Specification.Samples,
                internalFormat, m_Specification.Width, m_Specification.Height, GL_TRUE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D_MULTISAMPLE, m_ColorAttachment, 0);
        }
        else {
            glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat,
                m_Specification.Width, m_Specification.Height, 0,
                GL_RGBA, dataType, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D, m_ColorAttachment, 0);
        }

        glGenRenderbuffers(1, &m_DepthAttachment);
        glBindRenderbuffer(GL_RENDERBUFFER, m_DepthAttachment);
        if (multisampled) {
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_Specification.Samples,
                GL_DEPTH24_STENCIL8, m_Specification.Width, m_Specification.Height);
        }
        else {
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
                m_Specification.Width, m_Specification.Height);
        }
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
            GL_RENDERBUFFER, m_DepthAttachment);
    }
    else {
        glGenTextures(1, &m_DepthAttachment);
        glBindTexture(GL_TEXTURE_2D, m_DepthAttachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
            m_Specification.Width, m_Specification.Height, 0,
            GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D, m_DepthAttachment, 0);

        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        spdlog::error("Framebuffer is incomplete!");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
    glViewport(0, 0, m_Specification.Width, m_Specification.Height);
}

void Framebuffer::Unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0 || width > 8192 || height > 8192) return;
    m_Specification.Width = width;
    m_Specification.Height = height;
    Invalidate();
}

void Framebuffer::ResolveTo(const std::shared_ptr<Framebuffer>& target) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_RendererID);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target->m_RendererID);
    glBlitFramebuffer(0, 0, m_Specification.Width, m_Specification.Height,
        0, 0, target->GetSpecification().Width, target->GetSpecification().Height,
        GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::SetSamples(uint32_t samples) {
    if (m_Specification.Samples == samples) return;
    m_Specification.Samples = samples;
    Invalidate();
}