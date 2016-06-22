#include "GPUSurface.h"
#include "SurfaceExtraction/GPUSurfaceExtraction.h"

GPUSurface::GPUSurface(int atomCount)
{
    // Create first input index list [0, atomCount[
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
        return mupInitialInputIndices->read(mupInitialInputIndices->getSize()); // size may be used here because completely filled with indices
    }
    else
    {
       return mInternalIndices.at(layer-1)->read(mInternalCounts.at(layer-1));
    }
}

std::vector<GLuint> GPUSurface::getInternalIndices(int layer) const
{
    return mInternalIndices.at(layer)->read(mInternalCounts.at(layer));
}

std::vector<GLuint> GPUSurface::getSurfaceIndices(int layer) const
{
    return mSurfaceIndices.at(layer)->read(mSurfaceCounts.at(layer));
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

std::vector<GLuint> GPUSurface::GPUTextureBuffer::read(int size) const
{
    std::vector<GLuint> data;
    data.resize(size);
    glBindBuffer(GL_TEXTURE_BUFFER, mBuffer);
    glGetBufferSubData(GL_TEXTURE_BUFFER, 0, sizeof(GLuint) * size, data.data());
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
    return data;
}

void GPUSurface::GPUTextureBuffer::fillBuffer(const std::vector<GLuint>& rData) const
{
    glBindBuffer(GL_TEXTURE_BUFFER, mBuffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(GLuint) * rData.size(), rData.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
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

void GPUSurface::bindForComputation(int layer, GLuint inputSlot, GLuint internalSlot, GLuint surfaceSlot) const
{
    // Bind texture as image where input indices are listed
    if(layer == 0)
    {
        mupInitialInputIndices->bindAsImage(inputSlot, GPUTextureBuffer::GPUAccess::READ_ONLY);
    }
    else
    {
        mInternalIndices.at(layer-1)->bindAsImage(inputSlot, GPUTextureBuffer::GPUAccess::READ_ONLY);
    }

    // Bind textures as images where output indices are written to
    mInternalIndices.at(layer)->bindAsImage(internalSlot, GPUTextureBuffer::GPUAccess::WRITE_ONLY);
    mSurfaceIndices.at(layer)->bindAsImage(surfaceSlot, GPUTextureBuffer::GPUAccess::WRITE_ONLY);
}

void GPUSurface::fillInternalBuffer(int layer, const std::vector<GLuint>& rData)
{
    mInternalIndices.at(layer)->fillBuffer(rData);
    mInternalCounts[layer] = rData.size();
}

void GPUSurface::fillSurfaceBuffer(int layer, const std::vector<GLuint>& rData)
{
    mSurfaceIndices.at(layer)->fillBuffer(rData);
    mSurfaceCounts[layer] = rData.size();
}
