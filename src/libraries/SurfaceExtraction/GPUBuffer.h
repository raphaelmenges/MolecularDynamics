//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

// Buffer on GPU.

#ifndef GPU_BUFFER_H
#define GPU_BUFFER_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

// Class for buffer on GPU
template <class T>
class GPUBuffer
{
public:

    // Constructor
    GPUBuffer()
    {
        glGenBuffers(1, &mBuffer);
    }

    // Destructor
    virtual ~GPUBuffer()
    {
        glDeleteBuffers(1, &mBuffer);
    }

    // Fill
    void fill(const std::vector<T>& rData, GLenum access)
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

    // Bind
    void bind(GLuint slot) const
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot, mBuffer);
    }

    // Read values
    std::vector<T> read() const
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mBuffer);
        GLuint *ptr;
        ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        std::vector<T> result(ptr, ptr + mSize);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        return result;
    }

    // Get handle to buffer (handle carefully)
    GLuint getHandle() const
    {
        return mBuffer;
    }

private:

    // Handle for buffer which holds the data
    GLuint mBuffer;

    // Size of buffer
    int mSize;
};

#endif // GPU_BUFFER_H
