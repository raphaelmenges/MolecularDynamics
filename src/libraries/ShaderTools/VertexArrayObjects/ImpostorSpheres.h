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



    const int num_balls = 1000;
};

#endif // ImpostorSpheres_H
