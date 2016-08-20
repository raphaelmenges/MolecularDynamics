//
// Created by ubundrian on 12.08.16.
//

#ifndef OPENGL_FRAMEWORK_GPUHANDLER_H
#define OPENGL_FRAMEWORK_GPUHANDLER_H

#include <cstring>
#include <GL/glew.h>
#include <Utils/Logger.h>
#include <vector>

#include "NeighborhoodSearchDefines.h"
#include "../../executables/NeighborSearch/SimpleAtom.h"

class GPUHandler {
public:
    /*
     * initialize ssbo of specified size
     */
    void initSSBOInt(GLuint* ssboHandler, int length);
    void initSSBOUInt(GLuint* ssboHandler, int length);
    void initSSBOFloat(GLuint* ssboHandler, int length);
    void initSSBOFloat3(GLuint* ssboHandler, int length);
    void initSSBOFloat4(GLuint* ssboHandler, int length);

    /*
     * fill every block of the ssbo with the provided value
     */
    void fillSSBOInt(GLuint* ssboHandler, int length, int value);
    void fillSSBOUInt(GLuint* ssboHandler, int length, uint value);

    /*
     * copy data to ssbo
     */
    void copyDataToSSBOInt(GLuint* targetHandler, int* data, int length);
    void copyDataToSSBOUInt(GLuint* targetHandler, uint* data, int length);
    void copyDataToSSBOFloat(GLuint* targetHandler, float* data, int length);
    void copyDataToSSBOFloat3(GLuint* targetHandler, glm::vec3* data, int length);

    /*
     * copy data from ssbo to ssbo
     */
    void copySSBOtoSSBOUInt(GLuint* targetHandler, GLuint* dataHandler, int length);
    void copySSBOtoSSBOFloat4(GLuint* targetHandler, GLuint* dataHandler, int length);

    /*
     * for debugging
     */
    void printSSBODataInt(GLuint* ssboHandler, int length);
    void printSSBODataUInt(GLuint* ssboHandler, int length);
    void printSSBODataFloat(GLuint* ssboHandler, int length);
    void printSSBODataFloat3(GLuint* ssboHandler, int length);
    void printSSBODataFloat4(GLuint* ssboHandler, int length);

    void assertAtomsAreInBounds(GLuint* ssboHandler, int length, glm::vec3 min, glm::vec3 max);
    void assertSum(GLuint* ssboHandler, int length, int sum);
    void assertBelowLimit(GLuint* ssboHandler, int length, int limit);
    void assertAboveLimit(GLuint* ssboHandler, int length, int limit);
    void assertAboveLimit(GLuint* ssboHandler, int length, uint limit);
    void assertAboveEqLimit(GLuint* ssboHandler, int length, int limit);
    void assertAboveEqLimit(GLuint* ssboHandler, int length, uint limit);
    void assertAllEqual(GLuint* ssboHandler, int length, int value);
    void assertAllEqual(GLuint* ssboHandler, int length, uint value);
    void assertCellContent(GLuint* cellHandler,GLuint* cellCntHandler, int length1, int length2);

    /*
     * deleting ssbo
     */
    void deleteSSBO(GLuint* ssboHandler);

};


#endif //OPENGL_FRAMEWORK_GPUHANDLER_H
