// Author: Raphael Menges
// Validation of Surface Extraction algorithm.

#ifndef SURFACE_EXTRACTION_H
#define SURFACE_EXTRACTION_H

#include "ShaderTools/ShaderProgram.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <memory>

// Forward declaration
class GPUProtein;
class GPUSurface;

class SurfaceValidation
{
public:

    // Constructor
    SurfaceValidation();

    // Destructor
    virtual ~SurfaceValidation();

    // Validation. rInformation is filled with information string about validation
    void validate(
        GPUProtein const * pGPUProtein,
        GPUSurface const * pGPUSurface,
        int layer,
        float probeRadius,
        unsigned int sampleSeed,
        int samplesPerAtomCount,
        std::string& rInformation,
        std::vector<GLuint> rMaybeIncorrectSurfaceAtomIndices);

    // Draw sample points (internal sample means sample that was cut away by atom)
    void drawSamples(
        float pointSize,
        glm::vec3 internalSampleColor,
        glm::vec3 surfaceSampleColor,
        const glm::mat4& rViewMatrix,
        const glm::mat4& rProjectionMatrix,
        bool drawInternalSamples = true,
        bool drawSurfaceSamples = true) const;

private:

    // VBO for samples which were validated as surface
    GLuint mInternalVBO;

    // VAO for samples which were validated as surface
    GLuint mInternalVAO;

    // VBO for samples which were validated as surface
    GLuint mSurfaceVBO;

    // VAO for samples which were validated as surface
    GLuint mSurfaceVAO;

    // Shader program for samples
    std::unique_ptr<ShaderProgram> mupShaderProgram;

    // Count of samples for drawing
    int mInternalSampleCount = 0;
    int mSurfaceSampleCount = 0;
};

#endif // SURFACE_EXTRACTION_H
