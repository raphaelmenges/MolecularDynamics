// Author: Raphael Menges
// Samples of atoms' hull.

#ifndef GPU_HULL_SAMPLES_H
#define GPU_HULL_SAMPLES_H

#include "ShaderTools/ShaderProgram.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

// Forward declaration
class GPUProtein;
class GPUSurface;
class GPUTextureBuffer;

class GPUHullSamples
{
public:

    // Constructor
    GPUHullSamples();

    // Destructor
    virtual ~GPUHullSamples();

    // Computation. End frame determined by count of surfaces
    void compute(
        GPUProtein const * pGPUProtein,
        std::vector<std::unique_ptr<GPUSurface> > const * pGPUSurfaces,
        int startFrame,
        float probeRadius,
        int sampleCountPerAtom,
        unsigned int sampleSeed);

    // Draw the computed samples
    // Radii must be bound to slot 0 and trajectory to slot 1
     void drawSamples(
        int frame, // absolute frame
        float pointSize,
        glm::vec3 internalSampleColor,
        glm::vec3 surfaceSampleColor,
        const glm::mat4& rViewMatrix,
        const glm::mat4& rProjectionMatrix,
        float clippingPlane) const;

private:

    // Remember start frame of computation
    int mStartFrame;

    // Count of atoms
    int mAtomCount;

    // Count of frames (not complete animation but given calculated surfaces)
    int mLocalFrameCount;

    // Count of samples
    int mSampleCount;

    // Count of unsigned integers necessary for each sample
    int mIntegerCountPerSample;

    // Shader to compute classification
    std::unique_ptr<ShaderProgram> mupComputeProgram;

    // Vector with relative positions of samples
    // Is relative to atoms' centers
    std::vector<glm::vec3> mSamplesRelativePosition;

    // SSBO with relative positions of samples
    GLuint mSamplesRelativePositionSSBO;

    // Texture buffer with information whether sample is on surface or not
    // Saved in single bits of unsigned integers in vector
    std::unique_ptr<GPUTextureBuffer> mupClassification;

    // Drawing shader
    std::unique_ptr<ShaderProgram> mupShaderProgram;

    // Vertex array object for drawing
    GLuint mVAO;
};

#endif // GPU_HULL_SAMPLES_H
