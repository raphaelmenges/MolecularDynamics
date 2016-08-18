//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

// Access policy for buffers on GPU.

#ifndef GPU_ACCESS_H
#define GPU_ACCESS_H

#include <GL/glew.h>

// Accessibility of buffers in shader
enum GPUAccess
{
    WRITE_ONLY, READ_ONLY, READ_WRITE
};

static GLenum glEnum(GPUAccess access)
{
    // Decide access
    switch(access)
    {
        case GPUAccess::READ_ONLY:
            return GL_READ_ONLY;
        case GPUAccess::WRITE_ONLY:
            return GL_WRITE_ONLY;
        default:
            return GL_READ_WRITE;
    }
}

#endif // GPU_ACCESS_H
