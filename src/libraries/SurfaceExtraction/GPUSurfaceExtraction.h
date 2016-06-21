// Author: Raphael Menges
// Extraction of protein surface on GPU.

#ifndef GPU_SURFACE_EXTRACTION_H
#define GPU_SURFACE_EXTRACTION_H

#include "ShaderTools/ShaderProgram.h"
#include "SurfaceExtraction/GPUProtein.h"
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
    void bindInternalIndicesForDrawing(int layer, GLuint slot) const;
    void bindSurfaceIndicesForDrawing(int layer, GLuint slot) const;

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

private:

    // GPUTextureBuffer class
    class GPUTextureBuffer
    {
    public:

        // Accessibility in shader
        enum GPUAccess
        {
            WRITE_ONLY, READ_ONLY
        };

        // Constructor
        GPUTextureBuffer(int size);
        GPUTextureBuffer(std::vector<GLuint> data);

        // Destructor
        virtual ~GPUTextureBuffer();

        // Bind as image
        void bindAsImage(GLuint slot, GPUAccess access) const;

        // Get size of buffer
        GLuint getSize() const { return mSize; }

        // Get texture handle
        GLuint getTexture() const { return mTexture; }

        // Get buffer handle
        GLuint getBuffer() const { return mBuffer; }

    private:

        // Handle for texture which is defined by buffer
        GLuint mTexture;

        // Handle for buffer which holds the data
        GLuint mBuffer;

        // Size of data in buffer
        GLuint mSize;
    };

    // Read values from texture buffer
    std::vector<GLuint> readTextureBuffer(GLuint buffer, int size) const;

    // Internal indices
    std::vector<std::unique_ptr<GPUTextureBuffer> > mInternalIndices;

    // Surface indices
    std::vector<std::unique_ptr<GPUTextureBuffer> > mSurfaceIndices;

    // Initial input indices is extra GPUTextureBuffer
    std::unique_ptr<GPUTextureBuffer> mupInitialInputIndices;

    // Count of extracted layers (should be equal to size of mInternalIndices and mSurfaceIndices)
    int mLayerCount = 0;

    // ### SET / USED BY GPUSurfaceExtraction ###

    // Create new layer (should be called by GPUSurfaceExtraction, only. Returns count of layers
    int addLayer(int reservedSize);

     // Bind as images (input is readonly, internal and surface are writeonly)
    void bindForComputation(int layer, GLuint input, GLuint internal, GLuint surface) const;

    // Count of internal atoms (pushed back by addLayer and filled by GPUSurfaceExtraction)
    std::vector<int> mInternalCounts;

    // Count of surface atoms (pushed back by addLayer and filled by GPUSurfaceExtraction)
    std::vector<int> mSurfaceCounts;

    // Save time which was necessary for computation (has to be set by GPUSurfaceExtraction)
    float mComputationTime = 0;
};

// Factory for GPUSurface
class GPUSurfaceExtraction
{
public:

    // Constructor
    GPUSurfaceExtraction();

    // Destructor
    virtual ~GPUSurfaceExtraction();

    // Factory for GPUSurface objects
    std::unique_ptr<GPUSurface> calculateSurface(GPUProtein const * pGPUProtein, float probeRadius, bool extractLayers) const;

private:

    // Shader program for computation
    std::unique_ptr<ShaderProgram> mupComputeProgram;

    // Query for time measurement
    GLuint mQuery;
};

#endif // GPU_SURFACE_EXTRACTION_H
