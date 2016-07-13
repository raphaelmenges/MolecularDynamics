// Author: Raphael Menges
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
    Framebuffer(int width, int height);

    // Destructor
    virtual ~Framebuffer();

    // Bind framebuffer
    void bind() const;

    // Unbind (binds default, as long as it is zero)
    void unbind() const;

    // Resizing (needs bound framebuffer)
    void resize(int width, int height);

    // Add attachment (needs bound framebuffer)
    void addAttachment(ColorFormat colorFormat);

    // Get texture handle of color attachment
    GLuint getAttachment(int number) const { return colorAttachments.at(number).first; }

private:

    // Pair of color format and texture handle
    typedef std::pair<GLuint, ColorFormat> ColorAttachment;

    // Members
    std::vector<ColorAttachment> colorAttachments;
    GLuint mFramebuffer;
    GLuint mDepthStencil;
    int mWidth = -1;
    int mHeight = -1;
};

#endif // FRAMEBUFFER_H
