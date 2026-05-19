#pragma once
#include <cstdint>
#include <memory> // Wymagane dla std::shared_ptr

struct FramebufferSpecification {
    uint32_t Width, Height;
    uint32_t Samples = 1; // do MSAA
};

class Framebuffer {
public:
    Framebuffer(const FramebufferSpecification& spec);
    ~Framebuffer();

    void Invalidate();
    void Bind();
    void Unbind();
    void Resize(uint32_t width, uint32_t height);

    // Kopiowanie i uœrednianie obrazu z bufora MSAA do docelowego bufora
    void ResolveTo(const std::shared_ptr<Framebuffer>& target);
    uint32_t GetRendererID() const { return m_RendererID; }

    uint32_t GetColorAttachmentRendererID() const { return m_ColorAttachment; }
    const FramebufferSpecification& GetSpecification() const { return m_Specification; }
    void SetSamples(uint32_t samples);

private:
    uint32_t m_RendererID = 0;
    uint32_t m_ColorAttachment = 0;
    uint32_t m_DepthAttachment = 0;
    FramebufferSpecification m_Specification;
};