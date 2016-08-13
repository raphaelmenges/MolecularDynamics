//
// Created by ubundrian on 12.08.16.
//

#ifndef OPENGL_FRAMEWORK_GPUHANDLER_H
#define OPENGL_FRAMEWORK_GPUHANDLER_H

#include <cstring>
#include <GL/glew.h>
#include <Utils/Logger.h>

#include "NeighborhoodSearchDefines.h"

class GPUHandler {
public:
    /*
     * initialize ssbo of specified size
     */
    void initSSBOInt(GLuint* ssboHandler, int length);
    void initSSBOUInt(GLuint* ssboHandler, int length);
    void initSSBOFloat(GLuint* ssboHandler, int length);
    void initSSBOFloat3(GLuint* ssboHandler, int length);

    /*
     * fill every block of the ssbo with the provided value
     */
    void fillSSBOInt(GLuint* ssboHandler, int length, int value);

    /*
     * copy data to ssbo
     */
    void copyDataToSSBOInt(GLuint* targetHandler, int* data, int length);
    void copyDataToSSBOUInt(GLuint* targetHandler, uint* data, int length);
    void copyDataToSSBOFloat(GLuint* targetHandler, float* data, int length);
    void copyDataToSSBOFloat3(GLuint* targetHandler, glm::vec3* data, int length);

    void deleteSSBO(GLuint* ssboHandler);

};


#endif //OPENGL_FRAMEWORK_GPUHANDLER_H
