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
        RGB, RGBA
    };

    // Constructor
    Framebuffer(int width, int height, bool superSampling = false);

    // Destructor
    virtual ~Framebuffer();

    // Bind framebuffer
    void bind() const;

    // Unbind (binds default, as long as it is zero)
    void unbind() const;

    // Resizing (needs bound framebuffer). Returns whether size changed
    bool resize(int width, int height);
    bool resize(int width, int height, bool superSampling);

    // Add attachment (needs bound framebuffer)
    void addAttachment(ColorFormat colorFormat);

    // Get texture handle of color attachment
    GLuint getAttachment(int number) const { return colorAttachments.at(number).first; }

    // Get whether using super sampling
    bool superSampling() const { return mSuperSampling; }

    // Get multiplier of super sampling
    int getSuperSamplingMultiplier() const { return mSuperSamplingMultiplier; }

    // Get pixel width and height of framebuffer. Inclusive super sampling
    int getWidth() const { return mWidth; }
    int getHeight() const { return mHeight; }

private:

    // Pair of color format and texture handle
    typedef std::pair<GLuint, ColorFormat> ColorAttachment;

    // Members
    std::vector<ColorAttachment> colorAttachments;
    GLuint mFramebuffer;
    GLuint mDepthStencil;
    int mWidth = -1;
    int mHeight = -1;
    bool mSuperSampling = false;
    const int mSuperSamplingMultiplier = 2; // more than 2 make advanced filtering at composition necessary
};

#endif // FRAMEBUFFER_H
