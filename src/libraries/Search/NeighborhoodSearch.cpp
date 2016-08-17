//
// Created by ubundrian on 23.06.16.
//

#include <GL/glew.h>
#include "NeighborhoodSearch.h"

NeighborhoodSearch::~NeighborhoodSearch()
{
    Logger::instance().print("Destroy NeighborhoodSearch object");
    freeGrid();
    deallocateBuffers();
}


/*
 * Getter and setter
 */
int NeighborhoodSearch::getNumberOfGridCells()
{
    return m_gridTotal;
}
glm::vec3 NeighborhoodSearch::getGridSize()
{
    return m_gridSize;
}
glm::ivec3 NeighborhoodSearch::getGridResolution()
{
    return m_gridRes;
}
float NeighborhoodSearch::getCellSize()
{
    return m_cellSize;
}
void NeighborhoodSearch::getGridMinMax(glm::vec3& min, glm::vec3& max)
{
    min = m_gridMin;
    max = m_gridMax;
}
int NeighborhoodSearch::getGridSearch()
{
    return m_gridSearch;
}



//-----------------------------------------------------//
//                   INITIALIZATION                    //
//-----------------------------------------------------//
void NeighborhoodSearch::init(uint numElements, glm::fvec3 min, glm::fvec3 max, glm::ivec3 resolution, float searchRadius)
{
    m_numElements = numElements;    // save number of elements for later calculations
    setupComputeShaders();
    allocateBuffers(numElements);
    preallocBlockSumsInt(1);
    setupGrid(min, max, resolution, searchRadius);
    calculateNumberOfBlocksAndThreads(numElements);
    deallocBlockSumsInt();
    preallocBlockSumsInt(m_gridTotal);
}



void NeighborhoodSearch::setupComputeShaders()
{
    m_extractElementPositionsShader = ShaderProgram("/NeighborSearch/extractElementPositions.comp");
    m_insertElementsShader = ShaderProgram("/NeighborSearch/insertElements.comp");
    m_prescanIntShader = ShaderProgram("/NeighborSearch/prescanInt.comp");
    m_uniformAddIntShader = ShaderProgram("/NeighborSearch/uniformAddInt.comp");
}



void NeighborhoodSearch::allocateBuffers(uint numElements)
{
    // init gpu buffers
    m_gpuBuffers.dp_pos     = new GLuint;
    m_gpuBuffers.dp_gcell   = new GLuint;
    m_gpuBuffers.dp_gndx    = new GLuint;
    m_gpuBuffers.dp_sortbuf = new GLuint;
    m_gpuBuffers.dp_grid    = new GLuint;
    m_gpuBuffers.dp_gridcnt = new GLuint;
    m_gpuBuffers.dp_gridoff = new GLuint;

    // init ssbo's
    m_gpuHandler.initSSBOFloat4(m_gpuBuffers.dp_pos,     numElements);
    m_gpuHandler.initSSBOUInt  (m_gpuBuffers.dp_gcell,   numElements);
    m_gpuHandler.initSSBOUInt  (m_gpuBuffers.dp_gndx,    numElements);
    m_gpuHandler.initSSBOUInt  (m_gpuBuffers.dp_sortbuf, numElements);
    m_gpuHandler.initSSBOInt   (m_gpuBuffers.dp_grid,    numElements);
    m_gpuHandler.initSSBOInt   (m_gpuBuffers.dp_gridcnt, numElements);
    m_gpuHandler.initSSBOInt   (m_gpuBuffers.dp_gridoff, numElements);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, *m_gpuBuffers.dp_pos);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, *m_gpuBuffers.dp_gcell);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, *m_gpuBuffers.dp_gndx);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, *m_gpuBuffers.dp_gridcnt);
}
void NeighborhoodSearch::deallocateBuffers()
{
    m_gpuHandler.deleteSSBO(m_gpuBuffers.dp_pos);
    m_gpuHandler.deleteSSBO(m_gpuBuffers.dp_gcell);
    m_gpuHandler.deleteSSBO(m_gpuBuffers.dp_gndx);
    m_gpuHandler.deleteSSBO(m_gpuBuffers.dp_sortbuf);
    m_gpuHandler.deleteSSBO(m_gpuBuffers.dp_grid);
    m_gpuHandler.deleteSSBO(m_gpuBuffers.dp_gridcnt);
    m_gpuHandler.deleteSSBO(m_gpuBuffers.dp_gridoff);
}



void NeighborhoodSearch::preallocBlockSumsInt(uint maxNumElements)
{
    m_numElementsAllocated = maxNumElements;
    uint blockSize = BLOCK_SIZE; // max size of the thread blocks
    uint numElements = maxNumElements;
    uint totalTempSpace = maxNumElements;    // space that is needed for block sums computation
    int level = 0;

    /*
     * calculate how many times the threads need to perform reduction
     * until the remaining data can be summed up in one block
     */
    do {
        uint numBlocks = std::max(1, (int)(ceil((float)numElements)/(2.f*blockSize)));
        if (numBlocks > 1) {
            level++;
            totalTempSpace += numBlocks;
        }
        numElements = numBlocks;
    } while (numElements > 1);

    /*
     * scan block sums int contains device pointers for all steps of the reduction
     */
    m_scanBlockSumsInt = (GLuint**) malloc(level * sizeof(GLuint*));
    m_numLevelsAllocated = level;

    // reset value to do the same computation again
    numElements = maxNumElements;
    level = 0;

    /*
     * now allocate an ssbo for every level and store the device pointer
     * inside scan block sums int array in the position corresponding to
     * the current level
     */
    do {
        uint numBlocks = std::max(1, (int)(ceil((float)numElements)/(2.f*blockSize)));
        if (numBlocks > 1) {
            m_gpuHandler.initSSBOInt(m_scanBlockSumsInt[level++], numBlocks);
        }
        numElements = numBlocks;
    } while (numElements > 1);



    Logger::instance().print("Prealloc block sums:"); Logger::instance().tabIn();
    Logger::instance().print("Block size: " + std::to_string(blockSize));
    Logger::instance().print("Max num elements is: " + std::to_string(maxNumElements));
    Logger::instance().print("Number of levels required for the block sums: " + std::to_string(level));
    Logger::instance().tabOut();
}
void NeighborhoodSearch::deallocBlockSumsInt()
{
    if (m_scanBlockSumsInt != 0x0) {
        for (uint i = 0; i < m_numLevelsAllocated; i++) {
            m_gpuHandler.deleteSSBO(m_scanBlockSumsInt[i]);
        }
        free(m_scanBlockSumsInt);
    }
}



void NeighborhoodSearch::setupGrid(glm::fvec3 min, glm::fvec3 max, glm::ivec3 resolution, float searchRadius)
{
    // calculate grid parameters
    m_gridMin = min;
    m_gridMax = max;
    m_gridSize = max;
    m_gridSize -= min;
    m_gridRes = resolution;
    glm::vec3 cellSizes = m_gridSize;
    cellSizes /= m_gridRes;
    m_cellSize = std::max(cellSizes.x, std::max(cellSizes.y, cellSizes.z));
    m_gridSize  = m_gridRes; // update grid size to be a multiple of the cell size
    m_gridSize *= m_cellSize;
    m_gridMax = m_gridMin + m_gridSize;
    m_gridDelta = m_gridRes;
    m_gridDelta /= m_gridSize;
    m_gridTotal = m_gridRes.x * m_gridRes.y * m_gridRes.z;

    // allocate grid
    m_grid = (uint*) malloc(sizeof(uint*)* m_gridTotal);
    m_gridCnt = (uint*) malloc(sizeof(uint*)* m_gridTotal);
    memset(m_grid, GRID_UCHAR, m_gridTotal*sizeof(uint));
    memset(m_gridCnt, GRID_UCHAR, m_gridTotal*sizeof(uint));

    // number of cells to search
    /*
     * n = (2r/w)+1,
     * n: 1D cell search count
     * r: search radius
     * w: cell width
     */
    m_gridSearch = (int) floor(2*searchRadius / m_cellSize) +1;
    Logger::instance().print("Init grid"); Logger::instance().tabIn();
    Logger::instance().print("Grid search: floor(2*searchRadius/cellsize) + 1");
    Logger::instance().print(std::to_string(2*searchRadius) + "/" + std::to_string(m_cellSize) + " + 1 = " + std::to_string(m_gridSearch));
    if (m_gridSearch < 2) m_gridSearch = 2;
    m_gridAdjCnt = m_gridSearch * m_gridSearch * m_gridSearch;
    if (m_gridSearch > 6) {
        Logger::instance().print("Warning: Neighbor search is n > 6, n is set to 6 instead", Logger::Mode::WARNING);
        m_gridSearch = 6;
    }

    /*
     * calculate scan reach
     */
    glm::ivec3 gridScanMax = m_gridRes;
    gridScanMax -= glm::ivec3(m_gridSearch, m_gridSearch, m_gridSearch);

    // setup adjacency grid
    int cell = 0;
    for (int y = 0; y < m_gridSearch; y++) {
        for (int z = 0; z < m_gridSearch; z++) {
            for (int x = 0; x < m_gridSearch; x++) {
                m_gridAdj[cell++] = (y * m_gridRes.z + z) * m_gridRes.x + x;
            }
        }
    }

    /*
     * set grid data for gpu
     */
    m_gridDataGPU.min   = m_gridMin;
    m_gridDataGPU.delta = m_gridDelta;
    m_gridDataGPU.res   = m_gridRes;
    m_gridDataGPU.scan  = gridScanMax;


    /*
     * printing debug information
     */
    Logger::instance().print("Grid total: " + std::to_string(m_gridTotal));
    Logger::instance().print("Adjacency table (CPU)"); Logger::instance().tabIn();
    for (int n = 0; n < m_gridAdjCnt; n++) {
        Logger::instance().print("ADJ: " + std::to_string(n) + ", " + std::to_string(m_gridAdj[n]));
    }
    Logger::instance().print("Grid scan max: "  + std::to_string(gridScanMax.x) +
                                           ", " + std::to_string(gridScanMax.y) +
                                           ", " + std::to_string(gridScanMax.z));
    Logger::instance().tabOut(); Logger::instance().tabOut();
}
void NeighborhoodSearch::freeGrid()
{
    if (m_grid != 0x0) free(m_grid);
    if (m_gridCnt != 0x0) free(m_gridCnt);
}



void NeighborhoodSearch::calculateNumberOfBlocksAndThreads(uint numElements)
{
    int threadsPerBlock = 256; //TODO make it adjustable by the user
    computeNumBlocks(numElements, threadsPerBlock, m_numBlocks, m_numThreads);
    computeNumBlocks(m_gridTotal, threadsPerBlock, m_gridBlocks, m_gridThreads);
    Logger::instance().print("GPU infos:"); Logger::instance().tabIn();
    Logger::instance().print("Number of blocks:       " + std::to_string(m_numBlocks));
    Logger::instance().print("Number of threads:      " + std::to_string(m_numThreads));
    Logger::instance().print("Threads per block:      " + std::to_string(threadsPerBlock));
    Logger::instance().print("Number of grid blocks:  " + std::to_string(m_gridBlocks));
    Logger::instance().print("Number of grid threads: " + std::to_string(m_gridThreads));
    Logger::instance().tabOut();
}
void NeighborhoodSearch::computeNumBlocks(int numElements, int maxThreads, uint& numBlocks, uint &numThreads)
{
    numThreads = std::min(maxThreads, numElements);
    numBlocks = (numElements % numThreads != 0) ? (numElements/numThreads + 1) : (numElements/numThreads);
}



//-----------------------------------------------------//
//                NEIGHBORHOODSEARCH                   //
//-----------------------------------------------------//
void NeighborhoodSearch::run()
{
    updateElementPositions();
    insertElementsInGridGPU();
    prefixSumCellsGPU();
    //countingSort();
}

void NeighborhoodSearch::updateElementPositions()
{
    m_extractElementPositionsShader.use();
    m_extractElementPositionsShader.update("pnum", m_numElements);
    glDispatchCompute(m_numBlocks, 1, 1);
    glMemoryBarrier (GL_ALL_BARRIER_BITS);
}


void NeighborhoodSearch::insertElementsInGridGPU()
{
    m_gpuHandler.fillSSBOInt(m_gpuBuffers.dp_gridcnt, m_gridTotal, 0);

    m_insertElementsShader.use();
    m_insertElementsShader.update("grid.min",   glm::vec4(m_gridDataGPU.min, 0));
    m_insertElementsShader.update("grid.delta", glm::vec4(m_gridDataGPU.delta, 0));
    m_insertElementsShader.update("grid.res",   glm::ivec4(m_gridDataGPU.res, 0));
    //m_insertElementsShader.update("grid.scan",  glm::ivec4(m_gridDataGPU.scan, 0));
    m_insertElementsShader.update("pnum",       m_numElements);
    glDispatchCompute(m_numBlocks, 1, 1);
    glMemoryBarrier (GL_ALL_BARRIER_BITS);


    /*
     * sanity check
     */
    if (NHS_DEBUG) {
        Logger::instance().print("Checking data for insertElementsInGridGPU:"); Logger::instance().tabIn();
        m_gpuHandler.assertAtomsAreInBounds(m_gpuBuffers.dp_pos, m_numElements, m_gridMin, m_gridMax);
        m_gpuHandler.assertSum(m_gpuBuffers.dp_gridcnt, m_gridTotal, m_numElements);
        m_gpuHandler.assertBelowLimit(m_gpuBuffers.dp_gcell, m_numElements, m_gridTotal);
        m_gpuHandler.assertCellContent(m_gpuBuffers.dp_gcell, m_gpuBuffers.dp_gridcnt, m_numElements, m_gridTotal);
        Logger::instance().print("Checks successful");
        Logger::instance().tabOut();
    }
}


void NeighborhoodSearch::prefixSumCellsGPU()
{
    prescanArrayRecursiveInt(m_gpuBuffers.dp_gridoff, m_gpuBuffers.dp_gridcnt, m_gridTotal, 0);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    Logger::instance().print("Prefixsumcells"); Logger::instance().tabIn();
    m_gpuHandler.printSSBODataInt(m_gpuBuffers.dp_gridoff, m_gridTotal);
    Logger::instance().tabOut();

    /*
     * sanity check
     */
    if (NHS_DEBUG) {
        Logger::instance().print("Checking data for prefixSumCellsGPU:"); Logger::instance().tabIn();
        m_gpuHandler.assertAboveLimit(m_gpuBuffers.dp_gridoff, m_gridTotal, -1);
        m_gpuHandler.assertBelowLimit(m_gpuBuffers.dp_gridoff, m_gridTotal, m_numElements);
        Logger::instance().print("Checks successful");
        Logger::instance().tabOut();
    }
}
void NeighborhoodSearch::prescanArrayRecursiveInt(GLuint* outArray, GLuint* inArray, int numElements, int level)
{
    uint numBlocks = std::max(1, (int)(ceil((float)numElements/(2.f*BLOCK_SIZE))));
    uint numThreads;

    /*
     * determine number of threads necessary to compute the scan
     */
    if (numBlocks > 1) {
        numThreads = BLOCK_SIZE;
    } else if (isPowerOfTwo(numElements)) {
        numThreads = numElements / 2;
    } else {
        numThreads = floorPow2(numElements);
    }

    uint numElementsPerBlock = numThreads * 2;

    /*
     * if the array is not power of two, then the last block will be non-full
     * then we compute for the last block the smallest power of two,
     * so that it completely fits in
     */
    uint numElementsLastBlock = numElements - (numBlocks-1)*numElementsPerBlock;
    uint numThreadsLastBlock  = (uint)std::max(1, (int)numElementsLastBlock/2);
    uint np2LastBlock = 0;
    uint sharedMemoryLastBlock = 0;
    if (numElementsLastBlock != numElementsPerBlock) {
        np2LastBlock = 1;
        if(!isPowerOfTwo(numElementsLastBlock)) {
            numThreadsLastBlock = floorPow2(numElementsLastBlock);
        }
        uint extraSpace = (2*numThreadsLastBlock) / NUM_BANKS;  //TODO: compute the number of banks on the fly
        sharedMemoryLastBlock = sizeof(float) * (2*numThreadsLastBlock + extraSpace);
    }

    /*
     * use padding space to avoid shared memory bank conflicts
     */
    uint extraSpace = numElementsPerBlock / NUM_BANKS;
    uint sharedMemorySize = sizeof(float) * (numElementsPerBlock + extraSpace);

    /*
     * setup the execution parameters
     * if the data is not power of two then process the last block separately
     */
    glm::vec3 grid(std::max(1, (int)(numBlocks - np2LastBlock)), 1, 1);
    glm::vec3 threads(numThreads, 1, 1);
    int actualNumBlocks = std::max(1, (int)(numBlocks - np2LastBlock));

    /*
     * execute scan
     */
    if(numBlocks > 1) {
        prescanInt(numThreads, actualNumBlocks, sharedMemorySize, true, false, outArray, inArray, level, numThreads * 2, 0, 0);
        if(np2LastBlock) {
            prescanInt(numThreadsLastBlock, 1, sharedMemoryLastBlock, true, true, outArray, inArray, level, numElementsLastBlock, numBlocks-1, numElements-numElementsLastBlock);
        }

        /*
         * after the scan all last values of the subblocks need to be scaned
         * The result needs to be added to each block to get the final result
         * this is done by calling this function recursively
         */
        prescanArrayRecursiveInt(m_scanBlockSumsInt[level], m_scanBlockSumsInt[level], numBlocks, level+1);
        uniformAddInt(numThreads, actualNumBlocks, outArray, level, numElements-numElementsLastBlock, 0, 0);
        if (np2LastBlock) {
            uniformAddInt(numThreadsLastBlock, 1, outArray, level, numElementsLastBlock, numBlocks-1, numElements-numElementsLastBlock);
        }
    } else if(isPowerOfTwo(numElements)) {
        prescanInt(numThreads, actualNumBlocks, sharedMemorySize, false, false, outArray, inArray, 0, numThreads * 2, 0, 0);
    } else {
        prescanInt(numThreads, actualNumBlocks, sharedMemorySize, false, true, outArray, inArray, 0, numElements, 0, 0);
    }
}
void NeighborhoodSearch::prescanInt(int numThreads, int numBlocks, int sharedMemSize, bool storeSum, bool isNP2, GLuint* outArray, GLuint* inArray, int level, int n, int blockIndex, int baseIndex)
{
    Logger::instance().print("Prescanint: "); Logger::instance().tabIn();
    Logger::instance().print("Number of threads is: " + std::to_string(numThreads));
    Logger::instance().print("Shared memory size is: " + std::to_string(sharedMemSize));
    Logger::instance().tabOut();

    m_prescanIntShader.use();
    m_prescanIntShader.update("n", n);
    m_prescanIntShader.update("blockIndex", blockIndex);
    m_prescanIntShader.update("baseIndex", baseIndex);
    m_prescanIntShader.update("isNP2", isNP2);
    m_prescanIntShader.update("storeSum", storeSum);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, *outArray);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, *inArray);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, *(m_scanBlockSumsInt[level]));
    glDispatchCompute(numBlocks, 1, 1);
    glMemoryBarrier (GL_ALL_BARRIER_BITS);
}
void NeighborhoodSearch::uniformAddInt(int numThreads, int numBlocks, GLuint* outArray, int level, int n, int blockOffset, int baseIndex)
{
    Logger::instance().print("Uniformaddint: "); Logger::instance().tabIn();
    Logger::instance().print("Number of threads is: " + std::to_string(numThreads));
    Logger::instance().print("Number of blocks is: " + std::to_string(numBlocks));
    Logger::instance().print("Level is: " + std::to_string(level));
    Logger::instance().print("n is: " + std::to_string(n));
    Logger::instance().print("Blockoffset is: " + std::to_string(blockOffset));
    Logger::instance().print("Baseindex is: " + std::to_string(baseIndex));
    Logger::instance().tabOut();

    m_uniformAddIntShader.use();
    m_uniformAddIntShader.update("n", n);
    m_uniformAddIntShader.update("blockOffset", blockOffset);
    m_uniformAddIntShader.update("baseIndex", baseIndex);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, *outArray);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, *(m_scanBlockSumsInt[level]));
    glDispatchCompute(numBlocks, 1, 1);
    glMemoryBarrier (GL_ALL_BARRIER_BITS);
}
bool NeighborhoodSearch::isPowerOfTwo(int n)
{
    return ((n&(n-1)) == 0);
}
int NeighborhoodSearch::floorPow2(int n)
{
    return 1 << (int)logb((float)n);
}


void NeighborhoodSearch::countingSort()
{
    int n = m_numElements;
    /*
     * TODO: check if it is possible to copy different data types to an ssbo
     * TODO: is it possible to specify an offset for copying the data?
     * TODO: alternatively try to understand where sortbuf is used, maybe all data can be wrapped into a struct
     */

    m_gpuHandler.fillSSBOInt(m_gpuBuffers.dp_grid, n, GRID_UCHAR);

    // TODO: call shader countingSortFull
    // TODO: synchronize threads here if necessary
}



int NeighborhoodSearch::getTotalGridNum()
{
    return m_gridTotal;
}