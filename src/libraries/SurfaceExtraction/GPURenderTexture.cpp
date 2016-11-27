//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

#include "GPURenderTexture.h"

GPURenderTexture::GPURenderTexture(GLuint width, GLuint height, Type type)
{
    // Save type
    mType = type;

    // Generate texture object
    glGenTextures(1, &mTexture);
    glBindTexture(GL_TEXTURE_2D, mTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Delegate resize call for filling the texture
    resize(width, height);
}

GPURenderTexture::~GPURenderTexture()
{
    // Delete texture
    glDeleteTextures(1, &mTexture);
}

void GPURenderTexture::bindAsImage(GLuint slot, GPUAccess access) const
{
    // Determine format by type
    GLenum format;
    switch (mType)
    {
    case Type::R32UI:
            format = GL_R32UI;
            break;
        case Type::RG32F:
            format = GL_RG32F;
            break;
        case Type::RGBA32F:
            format = GL_RGBA32F;
            break;
    }

    // Bind as image
    glBindImageTexture(
        slot,
        mTexture,
        0,
        GL_TRUE,
        0,
        glEnum(access),
        format);
}

void GPURenderTexture::resize(GLuint width, GLuint height)
{
    // Save width and height
    mWidth = width;
    mHeight = height;

    // Resize texture
    glBindTexture(GL_TEXTURE_2D, mTexture);
    switch(mType)
    {
        case Type::R32UI:
        {
            std::vector<GLuint> zeros(mWidth * mHeight, 0);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, mWidth, mHeight, 0, GL_RED, GL_UNSIGNED_INT, &zeros[0]);
            break;
        }
        case Type::RG32F:
        {
            std::vector<GLfloat> zeros(mWidth * mHeight * 2, 0.f);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, mWidth, mHeight, 0, GL_RG, GL_FLOAT, &zeros[0]);
            break;
        }
        case Type::RGBA32F:
        {
            std::vector<GLfloat> zeros(mWidth * mHeight * 4, 0.f);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mWidth, mHeight, 0, GL_RGBA, GL_FLOAT, &zeros[0]);
            break;
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GPURenderTexture::clear()
{
    switch (mType)
    {
    case Type::R32UI:
            glClearTexImage(mTexture, 0, GL_RED, GL_UNSIGNED_INT, NULL);
            break;
        case Type::RG32F:
            glClearTexImage(mTexture, 0, GL_RG, GL_FLOAT, NULL);
            break;
        case Type::RGBA32F:
            glClearTexImage(mTexture, 0, GL_RGBA, GL_FLOAT, NULL);
            break;
    }
}
