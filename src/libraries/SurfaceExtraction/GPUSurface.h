// Author: Raphael Menges
// Surface and internal atoms of protein on GPU for a single frame.

#ifndef GPU_SURFACE_H
#define GPU_SURFACE_H

#include "GPUTextureBuffer.h"
#include <GL/glew.h>
#include <vector>
#include <memory>

// Foward declaration
class GPUSurfaceExtraction;

// Class for GPUSurface
class GPUSurface
{
public:

    // Friend class
    friend class GPUSurfaceExtraction;

    // Constructor
    GPUSurface(int atomCount);

    // Destructor
    virtual ~GPUSurface();

    // Bind as image for drawing, everything read only
    void bindInternalIndices(int layer, GLuint slot) const;
    void bindSurfaceIndices(int layer, GLuint slot) const;

    // Get duration of computation
    float getComputationTime() const { return mComputationTime; }

    // Get count of layers
    int getLayerCount() const { return mLayerCount; }

    // Get copy of vector with indices
    std::vector<GLuint> getInputIndices(int layer) const;
    std::vector<GLuint> getInternalIndices(int layer) const;
    std::vector<GLuint> getSurfaceIndices(int layer) const;

    // Get count of internal atoms in specific layer
    int getCountOfInternalAtoms(int layer) const { return mInternalCounts.at(layer); }

    // Get count of surface atoms in specific layer
    int getCountOfSurfaceAtoms(int layer) const { return mSurfaceCounts.at(layer); }

    // Get layer of atom. Returns -1 if not found in any computed layer
    int getLayerOfAtom(GLuint index) const;

private:

    // Internal indices
    std::vector<std::unique_ptr<GPUTextureBuffer> > mInternalIndices;

    // Surface indices
    std::vector<std::unique_ptr<GPUTextureBuffer> > mSurfaceIndices;

    // Initial input indices is extra GPUTextureBuffer
    std::unique_ptr<GPUTextureBuffer> mupInitialInputIndices;

    // Count of extracted layers (should be equal to size of mInternalIndices and mSurfaceIndices)
    int mLayerCount = 0;

    // ### SET / USED BY GPUSurfaceExtraction ###

    // Create new layer. Returns count of layers
    int addLayer(int reservedSize);

    // Bind as images (input is readonly, internal and surface are writeonly)
    void bindForComputation(int layer, GLuint inputSlot, GLuint internalSlot, GLuint surfaceSlot) const;

    // Simple filling of internal buffer
    void fillInternalBuffer(int layer, const std::vector<GLuint>& rData);

    // Simple filling of surface buffer
    void fillSurfaceBuffer(int layer, const std::vector<GLuint>& rData);

    // Count of internal atoms (pushed back by addLayer and filled by GPUSurfaceExtraction)
    std::vector<int> mInternalCounts;

    // Count of surface atoms (pushed back by addLayer and filled by GPUSurfaceExtraction)
    std::vector<int> mSurfaceCounts;

    // Save time which was necessary for computation (has to be set by GPUSurfaceExtraction)
    float mComputationTime = 0;
};

#endif // GPU_SURFACE_H
