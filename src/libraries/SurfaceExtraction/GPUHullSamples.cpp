#include "GPUHullSamples.h"
#include "GPUProtein.h"
#include "GPUSurface.h"

GPUHullSamples::GPUHullSamples()
{
    // Generate SSBOs
    glGenBuffers(1, &mSamplesRelativePositionSSBO);
    glGenBuffers(1, &mSamplesSurfaceClassificationSSBO);

    // Create compute shader which is used to classify samples
    mupComputeProgram = std::unique_ptr<ShaderProgram>(new ShaderProgram(GL_COMPUTE_SHADER, "/SurfaceExtraction/surfacesamples.comp"));

}

GPUHullSamples::~GPUHullSamples()
{
    // Delete SSBOs
    glDeleteBuffers(1, &mSamplesRelativePositionSSBO);
    glDeleteBuffers(1, &mSamplesSurfaceClassificationSSBO);
}

void GPUHullSamples::compute(
    GPUProtein const * pGPUProtein,
    std::vector<std::unique_ptr<GPUSurface> > const * pGPUSurfaces,
    int startFrame,
    float probeRadius,
    unsigned int sampleSeed,
    int sampleCountPerAtom)
{
    // Fill members
    mStartFrame = startFrame;
    mAtomCount = pGPUProtein->getAtomCount();
    mLocalFrameCount = pGPUSurfaces->size(); // not over complete animation but calculated surfaces!
    mSampleCount = sampleCountPerAtom;

    // ### RELATIVE POSITIONS ###

    // Create vector with relative positions
    // This is unchanged during all frames since the position
    // is saved relative to atom's center
    mSamplesRelativePosition.clear();
    mSamplesRelativePosition.reserve(mAtomCount * mSampleCount);
    std::srand(sampleSeed); // initialize random generator with seed

    // Go over atoms and generate relative position
    for(int i = 0; i < mAtomCount; i++)
    {
        // Create as many samples as desired
        float atomExtRadius = pGPUProtein->getRadii()->at(i) + probeRadius;
        for(int j = 0; j < mSampleCount; j++)
        {
             // Generate samples (http://mathworld.wolfram.com/SpherePointPicking.html)
            float u = (float)((double)std::rand() / (double)RAND_MAX);
            float v = (float)((double)std::rand() / (double)RAND_MAX);
            float theta = 2.f * glm::pi<float>() * u;
            float phi = glm::acos(2.f * v - 1);

            // Generate sample position
            glm::vec3 samplePosition(
                atomExtRadius * glm::sin(phi) * glm::cos(theta),
                atomExtRadius * glm::cos(phi),
                atomExtRadius * glm::sin(phi) * glm::sin(theta));

            // Push back sample's relative position
            mSamplesRelativePosition.push_back(samplePosition);
        }
    }

    // Copy information about relative sample position to GPU
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSamplesRelativePositionSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec3) * mSamplesRelativePosition.size(), mSamplesRelativePosition.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // ### SURFACE CLASSIFICATION ###

    // Decide how many unsigned integers are necessary to hold surface information on all frames for one sample of one atom
    int integerCountPerSample = glm::ceil(mLocalFrameCount / 32); // each unsigned int holds 32 bits
    int globalIntergerCount = integerCountPerSample * mAtomCount * mSampleCount; // frames are in integerCountPerSample

    // Initialize vector with zeros
    mSamplesSurfaceClassification = std::vector<GLuint>(globalIntergerCount, 0);

    // Create SSBO with created vector as input (everything zero and therefore marked as internal)
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSamplesSurfaceClassificationSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * mSamplesSurfaceClassification.size(), mSamplesSurfaceClassification.data(), GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // For each GPUSurface take surface atoms and calculate for their samples whether they are at surface or not
    mupComputeProgram->use();
    mupComputeProgram->update("atomCount", mAtomCount);
    mupComputeProgram->update("localFrameCount", mLocalFrameCount);
    mupComputeProgram->update("sampleCount", mSampleCount);
    mupComputeProgram->update("integerCountPerSample", integerCountPerSample);
    mupComputeProgram->update("probeRadius", probeRadius);
    pGPUProtein->bind(1, 2); // bind radii and trajectory buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, mSamplesRelativePositionSSBO); // bind relative position of samples
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, mSamplesSurfaceClassificationSSBO); // bind classification of samples which is written to
    for(int i = 0; i < pGPUSurfaces->size(); i++)
    {
        // Update values
        mupComputeProgram->update("frame", i + mStartFrame); // frame in global terms
        mupComputeProgram->update("inputAtomCount", i + pGPUSurfaces->at(i)->getCountOfSurfaceAtoms(0)); // count of input atoms

        // Bind surface indices buffer of that frame
        pGPUSurfaces->at(i)->bindSurfaceIndices(0, 0); // bind indices of surface atoms at that frame

        // Dispatch
        glDispatchCompute(
            (pGPUSurfaces->at(i)->getCountOfSurfaceAtoms(0) / 16) + 1,
            (mSampleCount / 16) + 1,
            1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
    }

    // Read SSBO with classification back to RAM
    // TODO
}

void GPUHullSamples::drawSamples(
    int frame,
    float pointSize,
    glm::vec3 internalSampleColor,
    glm::vec3 surfaceSampleColor,
    const glm::mat4& rViewMatrix,
    const glm::mat4& rProjectionMatrix,
    float clippingPlane) const
{
    // TODO

}
