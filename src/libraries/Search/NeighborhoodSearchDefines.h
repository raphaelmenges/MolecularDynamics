//
// Created by ubundrian on 11.08.16.
//

#ifndef OPENGL_FRAMEWORK_NEIGHBORHOODSEARCHDEFINES_H
#define OPENGL_FRAMEWORK_NEIGHBORHOODSEARCHDEFINES_H

// project includes
#include <glm/glm.hpp>



typedef unsigned int uint;

struct GPUBuffers {
    GLuint* dp_pos;         // float3
    GLuint* dp_gcell;       // uint
    GLuint* dp_gndx;        // uint
    GLuint* dp_sortbuf;     // char
    GLuint* dp_grid;        // uint
    GLuint* dp_gridcnt;     // int
    GLuint* dp_gridoff;     // int
};

struct Grid {
    glm::vec3  min;
    glm::vec3  delta;
    glm::ivec3 res;
    glm::ivec3 scan;
};


#define GRID_UCHAR 0xFF
#define BLOCK_SIZE 256
#define NUM_BANKS  16    // TODO: make it variable

#define NHS_DEBUG true

#endif //OPENGL_FRAMEWORK_NEIGHBORHOODSEARCHDEFINES_H
