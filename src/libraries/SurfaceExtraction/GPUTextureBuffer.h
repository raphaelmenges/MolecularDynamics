// Author: Raphael Menges
// Texture buffer on GPU which can be bound as image.

#ifndef GPU_TEXTURE_BUFFER_H
#define GPU_TEXTURE_BUFFER_H

#include <GL/glew.h>
#include <vector>

// Class for texture buffer on GPU
class GPUTextureBuffer
{
public:

    // Accessibility in shader
    enum GPUAccess
    {
        WRITE_ONLY, READ_ONLY // no READ_WRITE yet
    };

    // Constructor
    GPUTextureBuffer(int size);
    GPUTextureBuffer(std::vector<GLuint> data);

    // Destructor
    virtual ~GPUTextureBuffer();

    // Bind as image
    void bindAsImage(GLuint slot, GPUAccess access) const;

    // Get size of buffer (not of valid elements!)
    GLuint getSize() const { return mSize; }

    // Get texture handle
    GLuint getTexture() const { return mTexture; }

    // Get buffer handle
    GLuint getBuffer() const { return mBuffer; }

    // Read values from texture buffer
    std::vector<GLuint> read(int size) const;

    // Fill buffer
    void fillBuffer(const std::vector<GLuint>& rData) const;

private:

    // Handle for texture which is defined by buffer
    GLuint mTexture;

    // Handle for buffer which holds the data
    GLuint mBuffer;

    // Size of data in buffer
    GLuint mSize;
};

#endif // GPU_TEXTURE_BUFFER_H
