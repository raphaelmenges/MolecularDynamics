#include "GPUBuffer.h"

GPUBuffer::GPUBuffer()
{
    glGenBuffers(1, &mBuffer);
}

GPUBuffer::~GPUBuffer()
{
    glDeleteBuffers(1, &mBuffer);
}

void GPUBuffer::fill(const std::vector<float>& rData, GLenum access)
{
    mSize = rData.size();
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mBuffer);
    if(!rData.empty())
    {
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(rData.at(0)) * mSize, rData.data(), access);
    }
    else
    {
        glBufferData(GL_SHADER_STORAGE_BUFFER, 0, 0, access);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void GPUBuffer::fill(const std::vector<glm::vec3>& rData, GLenum access)
{
    mSize = rData.size();
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mBuffer);
    if(!rData.empty())
    {
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(rData.at(0)) * mSize, rData.data(), access);
    }
    else
    {
        glBufferData(GL_SHADER_STORAGE_BUFFER, 0, 0, access);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void GPUBuffer::bind(GLuint slot) const
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot, mBuffer);
}

std::vector<GLuint> GPUBuffer::read() const
{
    std::vector<GLuint> result;
    result.reserve(mSize);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mBuffer);
    GLuint *ptr;
    ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for(int i = 0; i < mSize; i++)
    {
        result.push_back(ptr[i]); // direct copy of complete data would be faster
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}
