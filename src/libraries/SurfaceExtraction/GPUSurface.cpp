//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

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

void GPUSurface::bindInternalIndices(int layer, GLuint slot) const
{
    mInternalIndices.at(layer)->bindAsImage(slot, GPUAccess::READ_ONLY);
}

void GPUSurface::bindSurfaceIndices(int layer, GLuint slot) const
{
    mSurfaceIndices.at(layer)->bindAsImage(slot, GPUAccess::READ_ONLY);
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

int GPUSurface::getLayerOfAtom(GLuint index) const
{
    // TODO: make it maybe more efficient

    // Go through surface layers
    for(int i = 0; i < mSurfaceIndices.size(); i++)
    {
        // Check whether atom index is inside
        auto indices = mSurfaceIndices.at(i)->read(mSurfaceCounts.at(i));

        // Go over indices and checked for searched one
        for(int j = 0; j < indices.size(); j++)
        {
            if(index == indices.at(j))
            {
                // Return layer
                return i;
            }
        }
    }

    return -1;
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
        mupInitialInputIndices->bindAsImage(inputSlot, GPUAccess::READ_ONLY);
    }
    else
    {
        mInternalIndices.at(layer-1)->bindAsImage(inputSlot, GPUAccess::READ_ONLY);
    }

    // Bind textures as images where output indices are written to
    mInternalIndices.at(layer)->bindAsImage(internalSlot, GPUAccess::WRITE_ONLY);
    mSurfaceIndices.at(layer)->bindAsImage(surfaceSlot, GPUAccess::WRITE_ONLY);
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
