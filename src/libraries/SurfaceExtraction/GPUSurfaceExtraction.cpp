#include "GPUSurfaceExtraction.h"
#include "Utils/AtomicCounter.h"

GPUSurfaceExtraction::GPUSurfaceExtraction()
{
    // Create shader program which is used
    mupComputeProgram = std::unique_ptr<ShaderProgram>(new ShaderProgram(GL_COMPUTE_SHADER, "/SurfaceExtraction/surface.comp"));

    // Create query object for time measurement
    glGenQueries(1, &mQuery);
}

GPUSurfaceExtraction::~GPUSurfaceExtraction()
{
    // Delete query object
    glDeleteQueries(1, &mQuery);
}

std::unique_ptr<GPUSurface> GPUSurfaceExtraction::calculateSurface(GPUProtein const * pGPUProtein, float probeRadius, bool extractLayers) const
{
    // Prepare atomic counters for writing results to unique positions in images
    AtomicCounter internalCounter;
    AtomicCounter surfaceCounter;

    // Input count
    int inputCount = pGPUProtein->getAtomCount(); // at first run, all are input

    // Create GPUSurface
    std::unique_ptr<GPUSurface> upGPUSurface = std::unique_ptr<GPUSurface>(new GPUSurface(inputCount));

    // Variable to measure elapsed time
    GLuint timeElapsed = 0;

    // Use compute shader program
    mupComputeProgram->use();

    // Probe radius
    mupComputeProgram->update("probeRadius", probeRadius);

    // Bind SSBO with atoms
    pGPUProtein->bind(0);

    // Bind atomic counter
    internalCounter.bind(1);
    surfaceCounter.bind(2);

    // Start query for time measurement
    glBeginQuery(GL_TIME_ELAPSED, mQuery);

    // Do it as often as indicated
    bool firstRun = true;
    while(firstRun || (extractLayers && (inputCount > 0)))
    {
        // Remember the first run
        firstRun = false;

        // Reset atomic counter
        internalCounter.reset();
        surfaceCounter.reset();

        // Tell shader program about count of input atoms
        mupComputeProgram->update("inputCount", inputCount);

        // Add new layer to GPUSurface with buffers which could take all indices
        // It is more reserved than later used, therefore count of internal and surface must be saved extra
        int layer = upGPUSurface->addLayer(inputCount) - 1;

        // Bind that layer
        upGPUSurface->bindForComputation(layer, 3, 4, 5);

        // Dispatch
        glDispatchCompute((inputCount / 64) + 1, 1, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);

        // Save count of internal as next count of input atoms
        inputCount = (int)internalCounter.read();

        // Tell added layer about counts calculated on graphics card
        upGPUSurface->mInternalCounts.at(layer) = inputCount;
        upGPUSurface->mSurfaceCounts.at(layer) = (int)surfaceCounter.read();
    }

    // Print time for execution
    glEndQuery(GL_TIME_ELAPSED);
    GLuint done = 0;
    while(done == 0)
    {
        glGetQueryObjectuiv(mQuery, GL_QUERY_RESULT_AVAILABLE, &done);
    }
    glGetQueryObjectuiv(mQuery, GL_QUERY_RESULT, &timeElapsed);
    float computationTime = timeElapsed / 1000000.f; // miliseconds, now

    // Fill computation time to GPUSurface
    upGPUSurface->mComputationTime = computationTime;

    // Return unique pointer
    return std::move(upGPUSurface);
}
