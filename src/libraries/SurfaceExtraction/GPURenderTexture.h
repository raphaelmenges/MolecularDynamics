//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

// 2D texture which is used to render into.

#ifndef GPU_RENDER_TEXTURE_H
#define GPU_RENDER_TEXTURE_H

#include "SurfaceExtraction/GPUAccess.h"
#include <GL/glew.h>
#include <vector>

// Class for rendering texture.
class GPURenderTexture
{
public:

    // Enumeration of available types
    enum class Type { R32UI, RG32F, RGBA32F };

    // Constructor
    GPURenderTexture(GLuint width, GLuint height, Type type);

    // Destructor
    virtual ~GPURenderTexture();

    // Bind as image
    void bindAsImage(GLuint slot, GPUAccess access) const;

    // Get texture handle
    GLuint getTexture() const { return mTexture; }

    // Resize
    void resize(GLuint width, GLuint height);

    // Clear
    void clear();

private:

    // Handle for texture
    GLuint mTexture;

    // Type
    Type mType;

    // Width of texture
    GLuint mWidth = 0;

    // Height of texture
    GLuint mHeight = 0;
};

#endif // GPU_RENDER_TEXTURE_H
