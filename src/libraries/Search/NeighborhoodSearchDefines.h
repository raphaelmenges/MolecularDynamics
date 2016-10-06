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
    GLuint* dp_pos;         // float4   - particle position
    GLuint* dp_gcell;       // uint     - cell idx the particle is in
    GLuint* dp_gndx;        // uint     - insertion idx of the particle inside the cell
    GLuint* dp_grid;        // uint     - idx of the particle after sorting
    GLuint* dp_gridcnt;     // int      - number of particles per cell
    GLuint* dp_gridoff;     // int      - offset of every cell
    GLuint* dp_undx;        // uint     - get unsorted index from sorted index
    // temporary buffers
    GLuint* dp_tempPos;     // float4   - temporary particle position
    GLuint* dp_tempGcell;   // uint     - temporary cell idx
    GLuint* dp_tempGndx;    // uint     - temporary insertion idx
};

/*
 * Usage of the neighborhood:
 * To get the position within the grid
 * 1. i: unique thread index == ith Particle after sorting
 * 2. get the cell for the corresponding particle with
 *      uint cell = dp_particleCell[i];
 * 3. iterate over all cells that are inside the particles search radius
 *      uint startCell = cell - startCellOffset;
 *      for (int searchCellIndex = 0; searchCellIndex < numberOfSearchCells; searchCellIndex++)
 *      {
 *          uint currentCell = startCell + searchCellOffsets[cellIdx];
 *          ... (steps 4 to 6)
 *      }
 * 4. get the start position of the current cell within the grid data structure with
 *      uint cellStart = gridoff[currentCell];
 * 5. get the end position of the current cell within the grid data structure with
 *      uint cellEnd = cellStart + gridcnt[currentCell];
 * 6. then iterate over all particles j in the cell
 *      for (uint cellIndex = cellStart; cellIndex < cellEnd; cellIndex++)
 *      {
 *          uint j = grid[cellIndex];
 *          ... (compare particles i and j)
 *      }
 * 7. if you want to access your data you have to be careful, since the particles within
 *    the grid structure are sorted by the grid cell index they are in. To get the index
 *    you were using just use:
 *      uint originalParticleIndex = particleOriginalIndex[i];
 *      uint originalCurrentParticleIndex = particleOriginalIndex[j]
 */
struct Neighborhood {
    // GPU
    GLuint* dp_particleOriginalIndex;   // uint     particles original index before the counting sort
    GLuint* dp_particleCell;            // uint     cell index the particle is in
    GLuint* dp_particleCellIndex;       // uint     insertion index of the particle inside the cell
    GLuint* dp_grid;                    // uint     index of the particle after sorting
    GLuint* dp_gridCellCounts;          // int      number of particles that are in the respective cell
    GLuint* dp_gridCellOffsets;         // int      total offset of the starting point of the respective cell
    // CPU
    int*     p_searchCellOffsets;       // int[]    stores the offsets for all cells that have to be searched
    int        startCellOffset;         // int      since  the particle is always in the center of the search cells
                                        //          we need the offset of the cell with the lowest index within those
                                        //          search cells
    int        numberOfSearchCells;     // int      number of cells within the search radius
    float      searchRadius;            // float    adjusted search radius
};

struct Grid {
    glm::vec3  min;
    glm::vec3  delta;
    glm::ivec3 res;
};


#define GRID_UNDEF 4294967295
#define BLOCK_SIZE 256
#define NUM_BANKS  16    // if changed here, it also must be changed in prescanInt.comp
#define LOG_NUM_BANKS 4  // if changed here, it also must be changed in prescanInt.comp

#define NHS_DEBUG true

#endif //OPENGL_FRAMEWORK_NEIGHBORHOODSEARCHDEFINES_H
