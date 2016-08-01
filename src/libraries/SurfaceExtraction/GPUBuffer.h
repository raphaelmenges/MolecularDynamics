// Author: Raphael Menges
// Buffer on GPU which can be bound as image.

#ifndef GPU_BUFFER_H
#define GPU_BUFFER_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

// Class for buffer on GPU
class GPUBuffer
{
public:

    // Constructor
    GPUBuffer();

    // Destructor
    virtual ~GPUBuffer();

    // Fill
    void fill(const std::vector<float>& rData, GLenum access);
    void fill(const std::vector<glm::vec3>& rData, GLenum access);

    // Bind
    void bind(GLuint slot) const;

private:

    // Handle for buffer which holds the data
    GLuint mBuffer;
};

#endif // GPU_BUFFER_H
