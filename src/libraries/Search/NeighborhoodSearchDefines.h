//
// Created by ubundrian on 11.08.16.
//

#ifndef OPENGL_FRAMEWORK_NEIGHBORHOODSEARCHDEFINES_H
#define OPENGL_FRAMEWORK_NEIGHBORHOODSEARCHDEFINES_H

// project includes
#include <glm/glm.hpp>



typedef unsigned int uint;

struct GPUBuffers {
    // particle and grid buffers
    GLuint* dp_pos;         // float3   - particle position
    GLuint* dp_gcell;       // uint     - cell idx the particle is in
    GLuint* dp_gndx;        // uint     - insertion idx of the particle inside the cell
    GLuint* dp_grid;        // uint     - idx of the particle after sorting
    GLuint* dp_gridcnt;     // int      - number of particles per cell
    GLuint* dp_gridoff;     // int      - offset of every cell
    GLuint* dp_undx;        // uint     - get unsorted index from sorted index
    GLuint* dp_searchRes;   // int      - not part of the neighborhood search, it indicates if atoms of other proteins are inside the searchradius
    // temporary buffers
    GLuint* dp_tempPos;     // float3   - temporary particle position
    GLuint* dp_tempGcell;   // uint     - temporary cell idx
    GLuint* dp_tempGndx;    // uint     - temporary insertion idx
};

struct Grid {
    glm::vec3  min;
    glm::vec3  delta;
    glm::ivec3 res;
    glm::ivec3 scan;
};


#define GRID_UNDEF 4294967295
#define BLOCK_SIZE 256
#define NUM_BANKS  16    // if changed here, it also must be changed in prescanInt.comp
#define LOG_NUM_BANKS 4  // if changed here, it also must be changed in prescanInt.comp

#define NHS_DEBUG false

#endif //OPENGL_FRAMEWORK_NEIGHBORHOODSEARCHDEFINES_H
