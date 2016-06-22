#include "SurfaceValidation.h"
#include "GPUProtein.h"
#include "GPUSurfaceExtraction.h"
#include <stdlib.h>

SurfaceValidation::SurfaceValidation()
{
    // Generate VBOs and VAOs
    glGenBuffers(1, &mInternalVBO);
    glGenVertexArrays(1, &mInternalVAO);
    glGenBuffers(1, &mSurfaceVBO);
    glGenVertexArrays(1, &mSurfaceVAO);

    // Load shader for drawing
    mupShaderProgram = std::unique_ptr<ShaderProgram>(new ShaderProgram("/SurfaceExtraction/sample.vert", "/SurfaceExtraction/sample.frag"));

    // Bind buffers to attributes for internal samples
    GLint posAttrib = glGetAttribLocation(mupShaderProgram->getProgramHandle(), "position");
    glBindVertexArray(mInternalVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mInternalVBO);
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Bind buffers to attributes for surface samples
    glBindVertexArray(mSurfaceVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mSurfaceVBO);
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

}

SurfaceValidation::~SurfaceValidation()
{
    // Delete VBOs and VAOs
    glDeleteVertexArrays(1, &mSurfaceVAO);
    glDeleteBuffers(1, &mSurfaceVBO);
    glDeleteVertexArrays(1, &mInternalVAO);
    glDeleteBuffers(1, &mInternalVBO);
}

void SurfaceValidation::validate(
    GPUProtein const * pGPUProtein,
    GPUSurface const * pGPUSurface,
    int layer,
    float probeRadius,
    unsigned int sampleSeed,
    int samplesPerAtomCount,
    std::string& rInformation,
    std::vector<GLuint> rMaybeIncorrectSurfaceAtomIndices)
{
    // Clear references
    rInformation.clear();
    rMaybeIncorrectSurfaceAtomIndices.clear();

    // Read data back from OpenGL buffers
    std::vector<GLuint> inputIndices = pGPUSurface->getInputIndices(layer);
    std::vector<GLuint> internalIndices = pGPUSurface->getInternalIndices(layer);
    std::vector<GLuint> surfaceIndices = pGPUSurface->getSurfaceIndices(layer);

    // Seed
    std::srand(sampleSeed);

    // Vectors of samples
    std::vector<glm::vec3> internalSamples;
    std::vector<glm::vec3> surfaceSamples;

    // Count cases of failure
    int internalSampleFailures = 0;
    int surfaceAtomsFailures = 0;

    // Copy atoms of protein to stack
    auto atoms = pGPUProtein->getAtoms();

    // Go over atoms (using indices from input indices buffer)
    for(unsigned int i : inputIndices) // using results from algorithm here. Not so good for independend test but necessary for layers
    {
        // Just to see, that something is going on
        std::cout << "Validating Atom: " << i << std::endl;

        // Check, whether atom is internal or surface (would be faster to iterate over those structures, but this way the test is more testier)
        bool found = false;
        bool internalAtom = false;
        for(auto& rIndex : internalIndices)
        {
            if(rIndex == i)
            {
                found = true;
                internalAtom = true;
                break;
            }
        }
        if(!found)
        {
            for(auto& rIndex : surfaceIndices)
            {
                if(rIndex == i)
                {
                    found = true;
                    internalAtom = false;
                    break;
                }
            }
        }
        if(!found)
        {
            rInformation =
                "Atom "
                + std::to_string(i)
                + " neither classified as internal nor as surface.\nSurface extraction algorithm has failed.";
            return;
        }

        // Get position and radius
        glm::vec3 atomCenter = atoms[i].center;
        float atomExtRadius = atoms[i].radius + probeRadius;

        // Count samples which are classified as internal for that atom
        int atomInternalSampleCount = 0;

        // Do some samples per atom
        for(int j = 0; j < samplesPerAtomCount; j++)
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
            samplePosition += atomCenter;

            // Go over all atoms and test, whether sample is inside in at least one
            bool inside = false;
            for(unsigned int k : inputIndices)
            {
                // Test not against atom that generated sample
                if(k == i) { continue; };

                // Actual test
                if(glm::distance(samplePosition, atoms[k].center) < (atoms[k].radius + probeRadius))
                {
                    inside = true;
                    break;
                }
            }

            // Check result
            if(inside)
            {
                // Count to check whether atom was classified as surface and all samples are inside
                atomInternalSampleCount++;

                // Push back to vector
                internalSamples.push_back(samplePosition);
            }
            else
            {
                // If sample was created by internal atom and is not classified as internal in test, something went terribly wrong
                if(internalAtom)
                {
                    // Sample is not inside any other atom's extended hull but should be
                    internalSampleFailures++;
                }

                // Push back to vector
                surfaceSamples.push_back(samplePosition);
            }
        }

        // If all samples are classified internal and the atom was classified as surface by the algorithm something MAY have went wront
        if((atomInternalSampleCount == samplesPerAtomCount) && !internalAtom)
        {
            rMaybeIncorrectSurfaceAtomIndices.push_back(i);
            surfaceAtomsFailures++;
        }
    }

    // Fill vertex buffer with vertices
    glBindBuffer(GL_ARRAY_BUFFER, mInternalVBO);
    glBufferData(GL_ARRAY_BUFFER, internalSamples.size() * sizeof(glm::vec3), internalSamples.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ARRAY_BUFFER, mSurfaceVBO);
    glBufferData(GL_ARRAY_BUFFER, surfaceSamples.size() * sizeof(glm::vec3), surfaceSamples.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Remember about complete count of samples for drawing
    mInternalSampleCount = internalSamples.size();
    mSurfaceSampleCount = surfaceSamples.size();

    // Draw output of test to GUI
    rInformation = "Wrong classification as internal for " + std::to_string(internalSampleFailures) + " samples.\n";
    rInformation += "Maybe wrong classification as surface for " + std::to_string(surfaceAtomsFailures) + " atoms.";
}

void SurfaceValidation::drawSamples(
        float pointSize,
        glm::vec3 internalSampleColor,
        glm::vec3 surfaceSampleColor,
        const glm::mat4& rViewMatrix,
        const glm::mat4& rProjectionMatrix,
        bool drawInternalSamples,
        bool drawSurfaceSamples) const
{
    if(drawInternalSamples || drawSurfaceSamples)
    {
        glPointSize(pointSize);
        mupShaderProgram->use();
        mupShaderProgram->update("view", rViewMatrix);
        mupShaderProgram->update("projection", rProjectionMatrix);

        // Draw internal samples
        if(drawInternalSamples)
        {
            mupShaderProgram->update("color", internalSampleColor);
            glBindVertexArray(mInternalVAO);
            glDrawArrays(GL_POINTS, 0, mInternalSampleCount);
        }

        // Draw surface samples
        if(drawSurfaceSamples)
        {
            mupShaderProgram->update("color", surfaceSampleColor);
            glBindVertexArray(mSurfaceVAO);
            glDrawArrays(GL_POINTS, 0, mSurfaceSampleCount);
        }

        glBindVertexArray(0);
    }
}
