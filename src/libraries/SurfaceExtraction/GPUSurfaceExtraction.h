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
    std::unique_ptr<GPUSurface> calculateSurface(
        GPUProtein const * pGPUProtein,
        int frame,
        float probeRadius,
        bool extractLayers,
        bool useCPU = false,
        int CPUThreadCount = 1) const;

private:

    // More for debugging and performance purposes, therefore member of GPUSurfaceExtraction
    // Face is defined by vec4(Normal, Distance from origin)
    class CPUSurfaceExtraction
    {
    public:

        void execute(
            GPUProtein const * pGPUProtein,
            int frame,
            int executionIndex,
            int inputCount,
            float probeRadius,
            const std::vector<unsigned int>& rInputIndices,
            std::vector<unsigned int>& rInternalIndices,
            std::vector<unsigned int>& rSurfaceIndices);

    private:

        void setup();

        bool checkParallelism(
            glm::vec4 plane,
            glm::vec4 otherPlane) const;

        bool pointInHalfspaceOfPlane(
            glm::vec4 plane,
            glm::vec3 point) const;

        void intersectPlanes(
            glm::vec4 plane,
            glm::vec4 otherPlane,
            glm::vec3 &linePoint,
            glm::vec3 &lineDir) const;

        float underSQRT(
            glm::vec3 linePoint,
            glm::vec3 lineDir,
            glm::vec3 sphereCenter,
            float sphereRadius) const;

        bool testEndpoint(
            glm::vec3 endpoint,
            int excludeA,
            int excludeB) const;

        // Members
        static const int mNeighborsMaxCount = 200;
        const bool mLogging = false; // one has to remove /* */ before activating logging

        // All cutting faces, also those who gets cut away by others
        int mCuttingFaceCount = 0;
        glm::vec3 mCuttingFaceCenters[mNeighborsMaxCount];
        glm::vec4 mCuttingFaces[mNeighborsMaxCount]; // Normal + Distance

        // Selection of cutting faces which get intersected pairwaise and produce endpoints
        int mCuttingFaceIndicators[mNeighborsMaxCount]; // Indicator whether cutting face was cut away by other (1 == not cut away)
        int mCuttingFaceIndicesCount = 0; // Count of not cut away cutting faces
        int mCuttingFaceIndices[mNeighborsMaxCount]; // Indices of cutting faces which are not cut away by other
    };

    // Shader program for computation
    std::unique_ptr<ShaderProgram> mupComputeProgram;

    // Query for time measurement
    GLuint mQuery;
};

#endif // GPU_SURFACE_EXTRACTION_H
