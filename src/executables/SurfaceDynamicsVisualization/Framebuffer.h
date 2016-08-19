//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

// Framebuffer for own purposes.

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

class Framebuffer
{
public:

    // Enumeration of color formats
    enum ColorFormat
    {
        RED, RGB, RGBA
    };

    // Enumeration of depth and stencil modes
    enum Mode
    {
        NONE, DEPTH_STENCIL_RENDERBUFFER, DEPTH_TEXTURE
    };

    // Constructor
    Framebuffer(int width, int height, Mode mode, bool superSampling = false);

    // Destructor
    virtual ~Framebuffer();

    // Bind framebuffer
    void bind() const;

    // Unbind (binds default, as long as it is zero)
    void unbind() const;

    // Resizing (needs bound framebuffer)
    void resize(int width, int height);
    void resize(int width, int height, bool superSampling);

    // Add attachment (needs bound framebuffer)
    void addAttachment(ColorFormat colorFormat);

    // Get texture handle of color attachment
    GLuint getAttachment(int number) const { return colorAttachments.at(number).first; }

    // Get whether using super sampling
    bool superSampling() const { return mSuperSampling; }

    // Get multiplier of super sampling
    int getSuperSamplingMultiplier() const { return mSuperSamplingMultiplier; }

    // Get extra depth texture. Works only when mode is DEPTH_TEXTURE, otherwise zero is returned
    GLuint getDepthTexture() const { return mDepthTexture; }

private:

    // Decide internal format
    GLenum decideInternalPixelFormat(ColorFormat format) const;

    // Decide format
    GLenum decidePixelFormat(ColorFormat format) const;

    // Pair of color format and texture handle
    typedef std::pair<GLuint, ColorFormat> ColorAttachment;

    // Members
    std::vector<ColorAttachment> colorAttachments;
    GLuint mFramebuffer = 0;
    GLuint mDepthStencilRenderbuffer = 0;
    GLuint mDepthTexture = 0;
    int mWidth = -1;
    int mHeight = -1;
    Mode mMode;
    bool mSuperSampling = false;
    const int mSuperSamplingMultiplier = 2; // more than 2 make advanced filtering at composition necessary
};

#endif // FRAMEBUFFER_H
