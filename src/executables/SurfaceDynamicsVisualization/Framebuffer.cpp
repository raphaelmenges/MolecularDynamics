//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

#include "Framebuffer.h"

Framebuffer::Framebuffer(int width, int height, bool supportDepthAndStencil, bool superSampling)
{
    // Save members
    mSupportDepthAndStencil = supportDepthAndStencil;

    // Generate framebuffer and renderbuffer for depth and stencil tests
    glGenFramebuffers(1, &mFramebuffer);
    if(mSupportDepthAndStencil) { glGenRenderbuffers(1, &mDepthStencil); }

    // Bind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);

    // Create render buffer for depth and stencil and save width and height
    resize(width, height, superSampling);

    // Unbind framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Framebuffer::~Framebuffer()
{
    glDeleteFramebuffers(1, &mFramebuffer);
    if(mSupportDepthAndStencil) { glDeleteRenderbuffers(1, &mDepthStencil); }

    for(const auto& rPair : colorAttachments)
    {
        glDeleteTextures(1, &rPair.first);
    }
}

void Framebuffer::bind() const
{
    glViewport(0, 0, mWidth, mHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
}

void Framebuffer::unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::resize(int width, int height)
{
    // Apply super sampling if indicated
    if(mSuperSampling)
    {
        width *= mSuperSamplingMultiplier;
        height *= mSuperSamplingMultiplier;
    }

    // Only continue when changed
    if(width != mWidth || height != mHeight)
    {
        // Save width and height
        mWidth = width;
        mHeight = height;

        // Create renderbuffer
        if(mSupportDepthAndStencil)
        {
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
            // Create empty texture
            glBindTexture(GL_TEXTURE_2D, rPair.first);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                decideInternalPixelFormat(rPair.second),
                mWidth,
                mHeight,
                0,
                decidePixelFormat(rPair.second),
                GL_UNSIGNED_BYTE,
                NULL);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
}

void Framebuffer::resize(int width, int height, bool superSampling)
{
    mSuperSampling = superSampling;
    resize(width, height);
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
        decideInternalPixelFormat(colorFormat),
        mWidth,
        mHeight,
        0,
        decidePixelFormat(colorFormat),
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

GLenum Framebuffer::decideInternalPixelFormat(ColorFormat format) const
{
    GLenum internalPixelFormat; // pixel format on graphics card
    switch(format)
    {
    case ColorFormat::RED:
        internalPixelFormat = GL_R8;
        break;
    case ColorFormat::RGB:
        internalPixelFormat = GL_RGB8;
        break;
    default:
        internalPixelFormat = GL_RGBA8;
    }
    return internalPixelFormat;
}

GLenum Framebuffer::decidePixelFormat(ColorFormat format) const
{
    GLenum pixelFormat; // pixel format of input
    switch(format)
    {
    case ColorFormat::RED:
        pixelFormat = GL_RED;
        break;
    case ColorFormat::RGB:
        pixelFormat = GL_RGB;
        break;
    default:
        pixelFormat = GL_RGBA;
    }
    return pixelFormat;
}
