#include "GPUHullSamples.h"
#include "GPUProtein.h"
#include "GPUSurface.h"
#include "Utils/AtomicCounter.h"

GPUHullSamples::GPUHullSamples()
{
    // Create compute shader which is used to classify samples
    mupComputeProgram = std::unique_ptr<ShaderProgram>(new ShaderProgram(GL_COMPUTE_SHADER, "/SurfaceExtraction/surfacesamples.comp"));

    // Generate empty vertex buffer array for drawing
    glGenVertexArrays(1, &mVAO);

    // Load shader for drawing
    mupShaderProgram = std::unique_ptr<ShaderProgram>(new ShaderProgram("/SurfaceExtraction/sample.vert", "/SurfaceExtraction/sample.geom", "/SurfaceExtraction/sample.frag"));
}

GPUHullSamples::~GPUHullSamples()
{
    // Delete VAO
    glDeleteVertexArrays(1, &mVAO);
}

void GPUHullSamples::compute(
    GPUProtein const * pGPUProtein,
    std::vector<std::unique_ptr<GPUSurface> > const * pGPUSurfaces,
    int startFrame,
    float probeRadius,
    int sampleCountPerAtom,
    unsigned int sampleSeed,
    std::function<void(float)> progressCallback)
{
    // Fill members
    mStartFrame = startFrame;
    mAtomCount = pGPUProtein->getAtomCount();
    mLocalFrameCount = pGPUSurfaces->size(); // not over complete animation but calculated surfaces!
    mSampleCount = sampleCountPerAtom;
    mIntegerCountPerSample = (int)glm::ceil((float)mLocalFrameCount / 32.f); // each unsigned int holds 32 bits

    // Initialize progress with zero
    if(progressCallback != NULL)
    {
        progressCallback(0);
    }

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
    mSamplesRelativePositionBuffer.fill(mSamplesRelativePosition, GL_DYNAMIC_READ);

    // ### SURFACE CLASSIFICATION ###

    // Decide how many unsigned integers are necessary to hold surface information on all frames for one sample of one atom
    int globalIntergerCount = mIntegerCountPerSample * mAtomCount * mSampleCount; // frames are in integerCountPerSample

    // Initialize classification with zeros
    mupClassification = std::unique_ptr<GPUTextureBuffer>(new GPUTextureBuffer(std::vector<GLuint>(globalIntergerCount, 0)));

    // Count surface samples
    mSurfaceSampleCount.clear();
    AtomicCounter surfaceSampleCounter;

    // For each GPUSurface take surface atoms and calculate for their samples whether they are at surface or not
    mupComputeProgram->use();
    mupComputeProgram->update("atomCount", mAtomCount);
    mupComputeProgram->update("sampleCount", mSampleCount);
    mupComputeProgram->update("integerCountPerSample", mIntegerCountPerSample);
    mupComputeProgram->update("probeRadius", probeRadius);
    pGPUProtein->bind(1, 2); // bind radii and trajectory buffers
    mSamplesRelativePositionBuffer.bind(3); // bind relative position of samples
    mupClassification->bindAsImage(4, GPUTextureBuffer::GPUAccess::READ_WRITE);
    surfaceSampleCounter.bind(5);
    for(int i = 0; i < pGPUSurfaces->size(); i++)
    {
        // Reset counter of surface samples
        surfaceSampleCounter.reset();

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
        glFinish(); // memory barrier does not do the job

        // Push back count of surface atoms
        mSurfaceSampleCount.push_back(surfaceSampleCounter.read());

        // Update progress
        if(progressCallback != NULL)
        {
            progressCallback(((float)i) / ((float)pGPUSurfaces->size()));
        }
    }

    // Read image with classification back to member
    mClassification = mupClassification->read(mupClassification->getSize());

    // Finih progress
    if(progressCallback != NULL)
    {
        progressCallback(1.f);
    }
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

    if(mSampleCount > 0 && mAtomCount > 0)
    {
        // Setup drawing
        glPointSize(pointSize);

        // Use shader program
        mupShaderProgram->use();

        // Radii are expected to be bound at slot 0 and trajectory at 1
        mSamplesRelativePositionBuffer.bind(2);
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
        mupShaderProgram->update("internalColor", internalSampleColor);
        mupShaderProgram->update("surfaceColor", surfaceSampleColor);

        // Bind vertex array object and draw samples
        glBindVertexArray(mVAO);
        glDrawArrays(GL_POINTS, 0, mAtomCount * mSampleCount);

        // Unbind vertex array object
        glBindVertexArray(0);
    }
}

int GPUHullSamples::getCountOfSurfaceSamples(int frame, std::set<GLuint> atomIndices) const
{
    // Calculate local frame
    int localFrame = frame - mStartFrame;

    // Result
    int surfaceSampleCount = 0;

    // Go over atoms where count of surface samples should be got
    for(GLuint a : atomIndices)
    {
        // Go over samples of that atom
        for(int j = 0; j < mSampleCount; j++)
        {
            // Calculate indices to look up classification
            int uintIndex =
                (a * mSampleCount * mIntegerCountPerSample) // offset for current atom's samples
                + (j *  mIntegerCountPerSample) // offset for current sample's slot
                + (localFrame / 32); // offset for unsigned int which has to be read
            int bitIndex = localFrame - (32 * int(localFrame / 32)); // bit index within unsigned integer

            // Fetch classification
            if(((mClassification.at(uintIndex) >> bitIndex) & 1) > 0)
            {
                // Increment count of found surface samples
                surfaceSampleCount++;
            }
        }
    }

    return surfaceSampleCount;
}

int GPUHullSamples::getCountOfSurfaceSamples(int frame) const
{
    return mSurfaceSampleCount.at(frame - mStartFrame);
}

int GPUHullSamples::getCountOfSamples(int atomCount) const
{
    return mSampleCount * atomCount;
}
