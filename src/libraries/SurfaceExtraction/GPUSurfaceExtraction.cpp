#include "GPUSurfaceExtraction.h"
#include "Utils/AtomicCounter.h"

GPUSurface::GPUSurface(int atomCount)
{
    // Create first input structure
    std::vector<GLuint> inputIndices;
    inputIndices.reserve(atomCount);
    for(GLuint i = 0; i < (GLuint)atomCount; i++) { inputIndices.push_back(i); }
    mupInitialInputIndices = std::unique_ptr<GPUTextureBuffer>(new GPUTextureBuffer(inputIndices));
}

GPUSurface::~GPUSurface()
{
    // Nothing to do here
}

void GPUSurface::bindInternalIndicesForDrawing(int layer, GLuint slot) const
{
    mInternalIndices.at(layer)->bindAsImage(slot, GPUTextureBuffer::GPUAccess::READ_ONLY);
}

void GPUSurface::bindSurfaceIndicesForDrawing(int layer, GLuint slot) const
{
    mSurfaceIndices.at(layer)->bindAsImage(slot, GPUTextureBuffer::GPUAccess::READ_ONLY);
}

std::vector<GLuint> GPUSurface::getInputIndices(int layer) const
{
    if(layer == 0)
    {
        return readTextureBuffer(mupInitialInputIndices->getBuffer(), mupInitialInputIndices->getSize());
    }
    else
    {
       return readTextureBuffer(mInternalIndices.at(layer-1)->getBuffer(), mInternalCounts.at(layer-1));
    }
}

std::vector<GLuint> GPUSurface::getInternalIndices(int layer) const
{
    return readTextureBuffer(mInternalIndices.at(layer)->getBuffer(), mInternalCounts.at(layer));
}

std::vector<GLuint> GPUSurface::getSurfaceIndices(int layer) const
{
    return readTextureBuffer(mSurfaceIndices.at(layer)->getBuffer(), mSurfaceCounts.at(layer));
}

GPUSurface::GPUTextureBuffer::GPUTextureBuffer(int size)
{
    // Remember size
    mSize = size;

    // Create texture
    glGenTextures(1, &mTexture);

    // Create buffer
    glGenBuffers(1, &mBuffer);

    // Reserve space
    glBindBuffer(GL_TEXTURE_BUFFER, mBuffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(GLuint) * mSize, 0, GL_DYNAMIC_COPY); // DYNAMIC_COPY because filled by GPU and read by GPU
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

    // Bind buffer to texture
    glBindTexture(GL_TEXTURE_BUFFER, mTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, mBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
}

GPUSurface::GPUTextureBuffer::GPUTextureBuffer(std::vector<GLuint> data)
{
    // Remember size
    mSize = data.size();

    // Create texture
    glGenTextures(1, &mTexture);

    // Create buffer
    glGenBuffers(1, &mBuffer);

    // Reserve space
    glBindBuffer(GL_TEXTURE_BUFFER, mBuffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(GLuint) * mSize, data.data(), GL_DYNAMIC_COPY); // DYNAMIC_COPY because filled by GPU and read by GPU
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

    // Bind buffer to texture
    glBindTexture(GL_TEXTURE_BUFFER, mTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, mBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
}

GPUSurface::GPUTextureBuffer::~GPUTextureBuffer()
{
    // Delete buffer and texture
    glDeleteBuffers(1, &mBuffer);
    glDeleteTextures(1, &mTexture);
}

void GPUSurface::GPUTextureBuffer::bindAsImage(GLuint slot, GPUAccess access) const
{
    glBindImageTexture(
        slot,
        mTexture,
        0,
        GL_TRUE,
        0,
        access == GPUAccess::WRITE_ONLY ? GL_WRITE_ONLY : GL_READ_ONLY,
        GL_R32UI);
}

std::vector<GLuint> GPUSurface::readTextureBuffer(GLuint buffer, int size) const
{
    std::vector<GLuint> data;
    data.resize(size);
    glBindBuffer(GL_TEXTURE_BUFFER, buffer);
    glGetBufferSubData(GL_TEXTURE_BUFFER, 0, sizeof(GLuint) * size, data.data());
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
    return data;
}

int GPUSurface::addLayer(int reservedSize)
{
    mInternalIndices.push_back(std::unique_ptr<GPUTextureBuffer>(new GPUTextureBuffer(reservedSize)));
    mSurfaceIndices.push_back(std::unique_ptr<GPUTextureBuffer>(new GPUTextureBuffer(reservedSize)));
    mInternalCounts.push_back(-1); // filled by friend
    mSurfaceCounts.push_back(-1); // filled by friend
    mLayerCount++;
    return mLayerCount;
}

void GPUSurface::bindForComputation(int layer, GLuint input, GLuint internal, GLuint surface) const
{
    // Bind texture as image where input indices are listed
    if(layer == 0)
    {
        mupInitialInputIndices->bindAsImage(input, GPUTextureBuffer::GPUAccess::READ_ONLY);
    }
    else
    {
        mInternalIndices.at(layer-1)->bindAsImage(input, GPUTextureBuffer::GPUAccess::READ_ONLY);
    }

    // Bind textures as images where output indices are written to
    mInternalIndices.at(layer)->bindAsImage(internal, GPUTextureBuffer::GPUAccess::WRITE_ONLY);
    mSurfaceIndices.at(layer)->bindAsImage(surface, GPUTextureBuffer::GPUAccess::WRITE_ONLY);
}

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

std::unique_ptr<GPUSurface> GPUSurfaceExtraction::calcSurface(GPUProtein const * pGPUProtein, float probeRadius, bool extractLayers) const
{
    // Prepare atomic counter for writing results to unique position in image
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
    if(firstRun || (extractLayers && (inputCount > 0)))
    {
        // Remember that run
        firstRun = false;

        // Reset atomic counter
        internalCounter.reset();
        surfaceCounter.reset();

        // Tell shader program about count of input atoms
        mupComputeProgram->update("inputCount", inputCount);

        // Add new layer to GPUSurface with buffers which could take all input indices
        int layer = (upGPUSurface->addLayer(inputCount) - 1);

        // Bind that layer
        upGPUSurface->bindForComputation(layer, 3, 4, 5);

        // Dispatch
        glDispatchCompute((inputCount / 64) + 1, 1, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);

        // Save new count of internal as next count of input atoms
        inputCount = internalCounter.read();

        // Tell added layer about counts calculated on graphics card
        upGPUSurface->mInternalCounts.at(layer) = inputCount;
        upGPUSurface->mSurfaceCounts.at(layer) = surfaceCounter.read();
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
