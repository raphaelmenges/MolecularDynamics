#include "Framebuffer.h"

Framebuffer::Framebuffer(int width, int height)
{
    // Generate framebuffer and renderbuffer for depth and stencil tests
    glGenFramebuffers(1, &mFramebuffer);
    glGenRenderbuffers(1, &mDepthStencil);

    // Bind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);

    // Create render buffer for depth and stencil and save width and height
    resize(width, height);

    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
}

Framebuffer::~Framebuffer()
{
    glDeleteFramebuffers(1, &mFramebuffer);
    glDeleteRenderbuffers(1, &mDepthStencil);

    for(const auto& rPair : colorAttachments)
    {
        glDeleteTextures(1, &rPair.first);
    }
}

void Framebuffer::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
}

void Framebuffer::unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::resize(int width, int height)
{
    if(width != mWidth || height != mHeight)
    {
        // Save width and height
        mWidth = width;
        mHeight = height;

        // Create renderbuffer
        glBindRenderbuffer(GL_RENDERBUFFER, mDepthStencil);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, mWidth, mHeight);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        // (Re)Bind depth and stencil
        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mDepthStencil);
        }

        // Do it for all color attachments
        for(const auto& rPair : colorAttachments)
        {
            glBindTexture(GL_TEXTURE_2D, rPair.first);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                rPair.second == ColorFormat::RGB ? GL_RGB8 : GL_RGBA8,
                mWidth,
                mHeight,
                0,
                rPair.second == ColorFormat::RGB ? GL_RGB : GL_RGBA,
                GL_UNSIGNED_BYTE,
                NULL);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
}

void Framebuffer::addAttachment(ColorFormat colorFormat)
{
    // Generate new texture
    GLuint texture = 0;
    glGenTextures(1, &texture);

    // Bind texture and set it up
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    // Reserve space for texture
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        colorFormat == ColorFormat::RGB ? GL_RGB8 : GL_RGBA8,
        mWidth,
        mHeight,
        0,
        colorFormat == ColorFormat::RGB ? GL_RGB : GL_RGBA,
        GL_UNSIGNED_BYTE,
        NULL);

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);

    // Find out which color attachment should be used
    int count = colorAttachments.size();
    GLenum attachmentNumber = GL_COLOR_ATTACHMENT0;
    attachmentNumber += count;

    // Bind texture to framebuffer
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, attachmentNumber, GL_TEXTURE_2D, texture, 0);

    // Tell framebuffer how many attachments to fill
    std::vector<GLenum> attachmentIdentifiers;
    for(int i = 0; i <= count; i++)
    {
        attachmentIdentifiers.push_back(GL_COLOR_ATTACHMENT0 + i);
    }
    glDrawBuffers(attachmentIdentifiers.size(), attachmentIdentifiers.data());

    // Remember that attachment
    colorAttachments.push_back(std::make_pair(texture, colorFormat));
}
