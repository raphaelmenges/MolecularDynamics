// Author: Raphael Menges
// Extraction of protein surface on GPU. Factory-like pattern.

#ifndef GPU_SURFACE_EXTRACTION_H
#define GPU_SURFACE_EXTRACTION_H

#include "ShaderTools/ShaderProgram.h"
#include "SurfaceExtraction/GPUProtein.h"
#include "SurfaceExtraction/GPUSurface.h"
#include <GL/glew.h>
#include <memory>

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
