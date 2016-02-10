#ifndef TEXTURE_H
#define TEXTURE_H

#include <string>
#include <string.h>
#include <GL/glew.h>
#include <IL/ilu.h>
#include <IL/ilut.h>


class Texture
{
public:
    Texture(std::string path);
    Texture(int w, int h);
    Texture(GLuint internalFormat, GLuint format, GLuint type);
    ~Texture();

    GLuint getHandle();
    void clear();

    static GLuint load(std::string path);
    GLuint genTexture(int w, int h);
    GLuint genUimageBuffer(int size);

    Texture();
    GLuint getInternalFormat() const;

    GLuint getFormat() const;

    GLuint getType() const;

    GLuint getTarget() const;

    bool getIsImageTex() const;

protected:
    GLuint textureHandle;
    int w;
    int h;
    int byteCount;
    unsigned char* pixels;

    GLuint internalFormat;
    GLuint format;
    GLuint type;
    GLuint target;
    bool isImageTex;
};

static bool devILInitialized = false;

#endif // TEXTURE_H
