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
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mBuffer);
    if(!rData.empty())
    {
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(rData.at(0)) * rData.size(), rData.data(), access);
    }
    else
    {
        glBufferData(GL_SHADER_STORAGE_BUFFER, 0, 0, access);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void GPUBuffer::fill(const std::vector<glm::vec3>& rData, GLenum access)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mBuffer);
    if(!rData.empty())
    {
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(rData.at(0)) * rData.size(), rData.data(), access);
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
