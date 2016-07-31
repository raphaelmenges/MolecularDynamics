#include "GPUTextureBuffer.h"

GPUTextureBuffer::GPUTextureBuffer(int size)
{
    // Remember size
    mSize = size;

    // Create texture
    glGenTextures(1, &mTexture);

    // Create buffer
    glGenBuffers(1, &mBuffer);

    // Reserve space
    glBindBuffer(GL_TEXTURE_BUFFER, mBuffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(GLuint) * mSize, 0, GL_DYNAMIC_COPY); // DYNAMIC_COPY because filled by GPU and read by GPU
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

    // Bind buffer to texture
    glBindTexture(GL_TEXTURE_BUFFER, mTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, mBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
}

GPUTextureBuffer::GPUTextureBuffer(std::vector<GLuint> data)
{
    // Remember size
    mSize = data.size();

    // Create texture
    glGenTextures(1, &mTexture);

    // Create buffer
    glGenBuffers(1, &mBuffer);

    // Reserve space
    glBindBuffer(GL_TEXTURE_BUFFER, mBuffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(GLuint) * mSize, data.data(), GL_DYNAMIC_COPY); // DYNAMIC_COPY because filled by GPU and read by GPU
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

    // Bind buffer to texture
    glBindTexture(GL_TEXTURE_BUFFER, mTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, mBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
}

GPUTextureBuffer::~GPUTextureBuffer()
{
    // Delete buffer and texture
    glDeleteBuffers(1, &mBuffer);
    glDeleteTextures(1, &mTexture);
}

void GPUTextureBuffer::bindAsImage(GLuint slot, GPUAccess access) const
{
    // Decide access
    GLenum glAccess = GL_READ_WRITE;
    switch(access)
    {
        case GPUAccess::READ_ONLY:
            glAccess = GL_READ_ONLY;
            break;
        case GPUAccess::WRITE_ONLY:
            glAccess = GL_WRITE_ONLY;
            break;
        default:
            glAccess = GL_READ_WRITE;
            break;
    }

    // Bind as image
    glBindImageTexture(
        slot,
        mTexture,
        0,
        GL_TRUE,
        0,
        glAccess,
        GL_R32UI);
}

std::vector<GLuint> GPUTextureBuffer::read(int size) const
{
    std::vector<GLuint> data;
    data.resize(size);
    glBindBuffer(GL_TEXTURE_BUFFER, mBuffer);
    glGetBufferSubData(GL_TEXTURE_BUFFER, 0, sizeof(GLuint) * size, data.data());
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
    return data;
}

void GPUTextureBuffer::fillBuffer(const std::vector<GLuint>& rData) const
{
    glBindBuffer(GL_TEXTURE_BUFFER, mBuffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(GLuint) * rData.size(), rData.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
}
