#include "GPUHullSamples.h"
#include "GPUProtein.h"
#include "GPUSurface.h"

GPUHullSamples::GPUHullSamples()
{
    // Generate SSBO
    glGenBuffers(1, &mSamplesRelativePositionSSBO);

    // Create compute shader which is used to classify samples
    mupComputeProgram = std::unique_ptr<ShaderProgram>(new ShaderProgram(GL_COMPUTE_SHADER, "/SurfaceExtraction/surfacesamples.comp"));

    // Generate empty vertex buffer array for drawing
    glGenVertexArrays(1, &mVAO);

    // Load shader for drawing
    mupShaderProgram = std::unique_ptr<ShaderProgram>(new ShaderProgram("/SurfaceExtraction/sample.vert", "/SurfaceExtraction/sample.geom", "/SurfaceExtraction/sample.frag"));
}

GPUHullSamples::~GPUHullSamples()
{
    // Delete SSBO
    glDeleteBuffers(1, &mSamplesRelativePositionSSBO);

    // Delete VAO
    glDeleteVertexArrays(1, &mVAO);
}

void GPUHullSamples::compute(
    GPUProtein const * pGPUProtein,
    std::vector<std::unique_ptr<GPUSurface> > const * pGPUSurfaces,
    int startFrame,
    float probeRadius,
    int sampleCountPerAtom,
    unsigned int sampleSeed)
{
    // Fill members
    mStartFrame = startFrame;
    mAtomCount = pGPUProtein->getAtomCount();
    mLocalFrameCount = pGPUSurfaces->size(); // not over complete animation but calculated surfaces!
    mSampleCount = sampleCountPerAtom;
    mIntegerCountPerSample = (int)glm::ceil((float)mLocalFrameCount / 32.f); // each unsigned int holds 32 bits

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

            // Generate sample point
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
    int globalIntergerCount = mIntegerCountPerSample * mAtomCount * mSampleCount; // frames are in integerCountPerSample

    // Initialize classification with zeros
    mupClassification = std::unique_ptr<GPUTextureBuffer>(new GPUTextureBuffer(std::vector<GLuint>(globalIntergerCount, 0)));

    // For each GPUSurface take surface atoms and calculate for their samples whether they are at surface or not
    mupComputeProgram->use();
    mupComputeProgram->update("atomCount", mAtomCount);
    mupComputeProgram->update("sampleCount", mSampleCount);
    mupComputeProgram->update("integerCountPerSample", mIntegerCountPerSample);
    mupComputeProgram->update("probeRadius", probeRadius);
    pGPUProtein->bind(1, 2); // bind radii and trajectory buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, mSamplesRelativePositionSSBO); // bind relative position of samples
    mupClassification->bindAsImage(4, GPUTextureBuffer::GPUAccess::READ_WRITE);
    for(int i = 0; i < pGPUSurfaces->size(); i++)
    {
        // Update values
        mupComputeProgram->update("frame", i + mStartFrame); // frame in global terms
        mupComputeProgram->update("localFrame", i);
        mupComputeProgram->update("inputAtomCount", pGPUSurfaces->at(i)->getCountOfSurfaceAtoms(0)); // count of input atoms

        // Bind surface indices buffer of that frame
        pGPUSurfaces->at(i)->bindSurfaceIndices(0, 0); // bind indices of surface atoms at that frame

        // Dispatch
        glDispatchCompute(
            ((pGPUSurfaces->at(i)->getCountOfSurfaceAtoms(0) * mSampleCount) / 64) + 1,
            1,
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
    // TODO: Problem: Using AtomCount from members but buffer from calling program

    // Setup drawing
    glPointSize(pointSize);

    // Use shader program
    mupShaderProgram->use();

    // Radii are expected to be bound at slot 0 and trajectory at 1
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mSamplesRelativePositionSSBO);
    mupClassification->bindAsImage(3, GPUTextureBuffer::GPUAccess::READ_ONLY);

    // Update uniform values
    mupShaderProgram->update("view", rViewMatrix);
    mupShaderProgram->update("projection", rProjectionMatrix);
    mupShaderProgram->update("clippingPlane", clippingPlane);
    mupShaderProgram->update("sampleCount", mSampleCount);
    mupShaderProgram->update("frame", frame),
    mupShaderProgram->update("atomCount", mAtomCount);
    mupShaderProgram->update("integerCountPerSample", mIntegerCountPerSample);
    mupShaderProgram->update("localFrame", frame - mStartFrame);

    // Bind vertex array object and draw samples
    glBindVertexArray(mVAO);
    glDrawArrays(GL_POINTS, 0, mAtomCount * mSampleCount);

    // Unbind vertex array object
    glBindVertexArray(0);
}
