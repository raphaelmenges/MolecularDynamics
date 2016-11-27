//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

#include "GPURenderTexture.h"

GPURenderTexture::GPURenderTexture(GLuint width, GLuint height, Type type, GLuint depth)
{
    // Save type
    mType = type;
    mDepth = depth;

    // Determine target
    GLenum target = mDepth <= 1 ? GL_TEXTURE_2D : GL_TEXTURE_2D_ARRAY;

    // Generate texture object
    glGenTextures(1, &mTexture);
    glBindTexture(target, mTexture);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(target, 0);

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

    // Resize texture in case of 2D
    if(mDepth <= 1)
    {
        glBindTexture(GL_TEXTURE_2D, mTexture);
        switch(mType)
        {
            case Type::R32UI:
            {
                std::vector<GLuint> zeros(mWidth * mHeight, 0);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, mWidth, mHeight, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &zeros[0]);
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
    else // it is an array of textures
    {
        glBindTexture(GL_TEXTURE_2D_ARRAY, mTexture);
        switch(mType)
        {
            case Type::R32UI:
            {
                std::vector<GLuint> zeros(mWidth * mHeight * mDepth, 0);
                glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R32UI, mWidth, mHeight, mDepth, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &zeros[0]);
                break;
            }
            case Type::RG32F:
            {
                std::vector<GLfloat> zeros(mWidth * mHeight * mDepth * 2, 0.f);
                glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RG32F, mWidth, mHeight, mDepth, 0, GL_RG, GL_FLOAT, &zeros[0]);
                break;
            }
            case Type::RGBA32F:
            {
                std::vector<GLfloat> zeros(mWidth * mHeight * mDepth * 4, 0.f);
                glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, mWidth, mHeight, mDepth, 0, GL_RGBA, GL_FLOAT, &zeros[0]);
                break;
            }
        }
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    }
}

void GPURenderTexture::clear()
{
    // Should work for both 2D and 2D_Array
    switch (mType)
    {
    case Type::R32UI:
            glClearTexImage(mTexture, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
            break;
        case Type::RG32F:
            glClearTexImage(mTexture, 0, GL_RG, GL_FLOAT, NULL);
            break;
        case Type::RGBA32F:
            glClearTexImage(mTexture, 0, GL_RGBA, GL_FLOAT, NULL);
            break;
    }
}
