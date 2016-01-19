#ifndef ImpostorSpheres_H
#define ImpostorSpheres_H

#include "../VertexArrayObject.h"

class ImpostorSpheres : public VertexArrayObject {
public:
    ImpostorSpheres();
    void draw();
    void doOcclusionQuery();
    void drawInstanced(int countInstances);

    GLuint occlusionQuery;
    GLuint positionBuffer;

    std::vector<GLint> visibilityMap;
    GLuint visibilityBufferOffset;

    void updateVisibilityMap(std::vector<GLint> map);

    static const int num_balls = 16384;
};

#endif // ImpostorSpheres_H
