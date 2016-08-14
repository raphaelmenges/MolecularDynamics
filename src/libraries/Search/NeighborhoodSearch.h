//
// Created by ubundrian on 23.06.16.
//

#ifndef OPENGL_FRAMEWORK_NEIGHBORHOODSEARCH_H
#define OPENGL_FRAMEWORK_NEIGHBORHOODSEARCH_H

#include <math.h>
#include <malloc.h>
#include <string.h>
#include <ShaderTools/ShaderProgram.h>

#include "Utils/Logger.h"
#include "NeighborhoodSearchDefines.h"
#include "GPUHandler.h"
#include "../../executables/NeighborSearch/SimpleProtein.h"


class NeighborhoodSearch {
public:
    ~NeighborhoodSearch();

    // neighbor search
    void init(uint numElements, glm::fvec3 min, glm::fvec3 max, glm::ivec3 resolution, float searchRadius);
    void run(std::vector<SimpleProtein>& proteins);

    // other functions
    int getTotalGridNum();

private:
    // grid parameters
    uint*       m_grid;
    uint*       m_gridCnt;
    int         m_gridSearch;
    int         m_gridAdj[216];  // maximal size of the adjacency mask is 6x6x6
    int         m_gridAdjCnt;    // 3D search count =n^3 e.g. 2x2x2=8
    glm::fvec3  m_gridMin;
    glm::fvec3  m_gridMax;
    glm::ivec3  m_gridRes;       // 3D grid resolution
    glm::fvec3  m_gridSize;      // 3D grid sizes
    glm::fvec3  m_gridDelta;     // delta translate from world space to cell space
    int         m_gridTotal;
    Grid        m_gridDataGPU;

    int         m_numElements;

    // blocksums parameters
    uint        m_numElementsAllocated;
    uint        m_numLevelsAllocated;
    GLuint**    m_scanBlockSumsInt;

    // gpu
    GPUHandler  m_gpuHandler;
    GPUBuffers  m_gpuBuffers;
    uint        m_numBlocks;
    uint        m_numThreads;
    uint        m_gridBlocks;
    uint        m_gridThreads;
    ShaderProgram m_insertElementsShader;
    ShaderProgram m_extractElementPositionsShader;
    ShaderProgram m_prescanIntShader;


    // init helper functions
    void setupComputeShaders();
    void allocateBuffers(uint numElements);
    void deallocateBuffers();
    void preallocBlockSumsInt(uint maxNumElements);
    void deallocBlockSumsInt();
    void setupGrid(glm::fvec3 min, glm::fvec3 max, glm::ivec3 resolution, float searchRadius);
    void freeGrid();
    void calculateNumberOfBlocksAndThreads(uint numElements);
    void computeNumBlocks(int numElements, int maxThreads, uint& numBlocks, uint &numThreads);

    // run helper functions
    void updateElementPositions(SimpleProtein& protein);
    void insertElementsInGridGPU();

    void prefixSumCellsGPU();
    void prescanArrayRecursiveInt(GLuint* outArray, const GLuint* inArray, int numElements, int level);
    bool isPowerOfTwo(int n);
    int floorPow2(int n);

    void countingSort();
};


#endif //OPENGL_FRAMEWORK_NEIGHBORHOODSEARCH_H
