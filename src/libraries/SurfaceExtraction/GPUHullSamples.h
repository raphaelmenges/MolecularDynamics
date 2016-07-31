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
        unsigned int sampleSeed,
        int sampleCountPerAtom);

    // Draw the computed samples
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

    // Shader to compute classification
    std::unique_ptr<ShaderProgram> mupComputeProgram;

    // Vector with relative positions of samples
    std::vector<glm::vec3> mSamplesRelativePosition;

    // SSBO with relative positions of samples
    GLuint mSamplesRelativePositionSSBO;

    // Vectro with information whether sample is on surface or not
    // Saved in single bits of unsigned integers in vector
    std::vector<GLuint> mSamplesSurfaceClassification;

    // SSBO with information whether sample is on surface or not
    // Saved in unsigned integers where single bits are set to indicate
    // whether a sample is it at surface or not
    GLuint mSamplesSurfaceClassificationSSBO;


};

#endif // GPU_HULL_SAMPLES_H
