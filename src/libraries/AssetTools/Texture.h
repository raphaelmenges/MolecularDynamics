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
    ~Texture();

    GLuint getHandle();
    void clear();

    static GLuint load(std::string path);
    GLuint genTexture(int w, int h);
    GLuint genUimageBuffer(int size);
    GLuint genUimageBuffer2(int size);

    Texture();
protected:
    GLuint textureHandle;
    int w;
    int h;
    int byteCount;
    unsigned char* pixels;
};

static bool devILInitialized = false;

#endif // TEXTURE_H
