#include "PerfectSurfaceDetection.h"

#include "OrbitCamera.h"
#include "ShaderTools/Renderer.h"
#include "ShaderTools/ShaderProgram.h"
#include "Molecule/MDtrajLoader/MdTraj/MdTrajWrapper.h"
#include "Molecule/MDtrajLoader/Data/Protein.h"
#include "Molecule/MDtrajLoader/Data/AtomLUT.h"

#include <glm/gtx/component_wise.hpp>

#include <iostream>

// ### Class implementation ###

PerfectSurfaceDetection::PerfectSurfaceDetection()
{
    // Create window
    mpWindow = generateWindow();

    // Register keyboard callback
    std::function<void(int, int, int, int)> kC = [&](int k, int s, int a, int m)
    {
        this->keyCallback(k, s, a, m);
    };
    setKeyCallback(mpWindow, kC);

    // Register scroll callback
    std::function<void(double, double)> kS = [&](double x, double y)
    {
        this->scrollCallback(x,y);
    };
    setScrollCallback(mpWindow, kS);

    // Clear color
    glClearColor(0.0f, 0.0f, 0.0f, 1);

    // Path to protein molecule
    std::vector<std::string> paths;
    paths.push_back(std::string(RESOURCES_PATH) + "/molecules/PDB/1crn.pdb");

    // Load protein
    MdTrajWrapper mdwrap;
    mupProtein = mdwrap.load(paths);

    // Load LUT
    AtomLUT atomLUT;

    // Atom count
    int atomCount = (int) mupProtein->getAtoms()->size();

    // Output atom count
    std::cout << "Atom count: " << atomCount << std::endl;

    // Get min/max extent of protein
    mupProtein->minMax(); // first, one has to calculate min and max value of protein
    glm::vec3 proteinMinExtent = mupProtein->getMin();
    glm::vec3 proteinMaxExtent = mupProtein->getMax();

    /*
    // Test protein extent
    std::cout
        << "Min extent of protein: "
        << proteinMinExtent.x << ", "
        << proteinMinExtent.y << ", "
        << proteinMinExtent.z << std::endl;
    std::cout
        << "Max extent of protein: "
        << proteinMaxExtent.x << ", "
        << proteinMaxExtent.y << ", "
        << proteinMaxExtent.z << std::endl;

    // Test atom radii
    std::vector<Atom*>* pAtoms = mupProtein->getAtoms();
    for(int i = 0; i < (int)pAtoms->size(); i++)
    {
        std::string element = pAtoms->at(i)->getElement();
        std::cout << "Atom: " <<  element << " Radius: " << atomLUT.vdW_radii_picometer.at(element) << std::endl;
    }
    */

    // Create camera
    float maxAtomExtent = glm::compMax(mupProtein->getMax());
    mupCamera = std::unique_ptr<OrbitCamera>(new OrbitCamera(glm::vec3(0, 0, 0), 90.f, 90.f, maxAtomExtent, maxAtomExtent / 2, 2 * maxAtomExtent));

    // ### Set up compute shader ###

    // Compile shader
    ShaderProgram surfaceDetectionProgram(GL_COMPUTE_SHADER, "/PerfectSurfaceDetection/surface.comp");

    // # Prepare atoms input (position + radius)

    // Struct which holds necessary information about singe atom
    struct AtomStruct
    {
        // Constructor
        AtomStruct(
            glm::vec3 position,
            float radius)
        {
            this->position = position;
            this->radius = radius;
        }

        // Fields
        glm::vec3 position;
        float radius;
    };

    // Vector which is used as data for SSBO
    std::vector<AtomStruct> atomStructs;
    for(Atom const * pAtom : *(mupProtein->getAtoms()))
    {
        // Push back all atoms
        atomStructs.push_back(
            AtomStruct(
                pAtom->getPosition(),
                atomLUT.vdW_radii_picometer.at(
                    pAtom->getElement())));
    }

    // Fill into ssbo
    glGenBuffers(1, &mAtomsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mAtomsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(AtomStruct) * atomStructs.size(), atomStructs.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // # Prepare atomic counter for writing results to unique position in image

    GLuint atomicCounter;
    glGenBuffers(1, &atomicCounter);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicCounter);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_STATIC_DRAW);
    resetAtomicCounter(atomicCounter);

    // # Prepare uint image to write indices of surface atoms. Lets call it list for easier understanding

    // Buffer
    GLuint surfaceAtomBuffer;
    glGenBuffers(1, &surfaceAtomBuffer);
    glBindBuffer(GL_TEXTURE_BUFFER, surfaceAtomBuffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(GLuint) * atomCount, 0, GL_STATIC_DRAW);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

    // Texture (which will be bound as image)
    glGenTextures(1, &mSurfaceAtomTexture);
    glBindTexture(GL_TEXTURE_BUFFER, mSurfaceAtomTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, surfaceAtomBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    // # Execute compute shader to determine surface atoms

    // Use compute shader program
    surfaceDetectionProgram.use();

    // Bind SSBO with atoms
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mAtomsSSBO);

    // Tell shader about count of atoms
    surfaceDetectionProgram.update("atomCount", atomCount);

    // Bind atomic counter
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, atomicCounter);

    // Bind image where indices of surface atoms are written to
    glBindImageTexture(2,
                       mSurfaceAtomTexture,
                       0,
                       GL_TRUE,
                       0,
                       GL_WRITE_ONLY,
                       GL_R32UI);

    // Dispatch
    glDispatchCompute((atomCount / 8) + 1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    // Fetch count
    mSurfaceAtomCount = readAtomicCounter(atomicCounter);

    // Output count
    std::cout << "Surface atom count: " << mSurfaceAtomCount << std::endl;

    // TODO: Delete atomic counter etc
}

void PerfectSurfaceDetection::renderLoop()
{
    // Point size for rendering
    glPointSize(15.f);

    // Cursor
    float prevCursorX, prevCursorY = 0;

    // Prepare shader for rendering
    ShaderProgram pointProgram = ShaderProgram("/PerfectSurfaceDetection/point.vert", "/PerfectSurfaceDetection/point.frag");

    // Initial setup of shader
    pointProgram.use();

    // Bind SSBO with atoms
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mAtomsSSBO);

    // Bind image where indices of surface atoms are read from
    glBindImageTexture(1,
       mSurfaceAtomTexture,
       0,
       GL_TRUE,
       0,
       GL_READ_ONLY,
       GL_R32UI);

    // Projection matrix (hardcoded viewport size)
    pointProgram.update("projection", glm::perspective(glm::radians(45.f), (GLfloat)1280 / (GLfloat)720, 0.1f, 1000.f));

    // Call render function of Rendering.h with lambda function
    render(mpWindow, [&] (float deltaTime)
    {
        // Clear buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Calculate cursor movement
        double cursorX, cursorY;
        glfwGetCursorPos(mpWindow, &cursorX, &cursorY);
        GLfloat cursorDeltaX = (float)cursorX - prevCursorX;
        GLfloat cursorDeltaY = (float)cursorY - prevCursorY;
        prevCursorX = cursorX;
        prevCursorY = cursorY;

        // Orbit camera
        mupCamera->setAlpha(mupCamera->getAlpha() + 0.25f * cursorDeltaX);
        mupCamera->setBeta(mupCamera->getBeta() - 0.25f * cursorDeltaY);
        mupCamera->update();

        // Update view matrix
        pointProgram.update("view", mupCamera->getViewMatrix());

        // Draw points
        glDrawArrays(GL_POINTS, 0, mSurfaceAtomCount);
    });
}

void PerfectSurfaceDetection::keyCallback(int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        case GLFW_KEY_ESCAPE: { glfwSetWindowShouldClose(mpWindow, GL_TRUE); break; } // Does not work, but method gets called?!
        }
    }
}

void PerfectSurfaceDetection::scrollCallback(double xoffset, double yoffset)
{
    mupCamera->setRadius(mupCamera->getRadius() - 0.1f * (float)yoffset);
}


GLuint PerfectSurfaceDetection::readAtomicCounter(GLuint atomicCounter) const
{
    // Read atomic counter
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicCounter);

    GLuint *mapping = (GLuint*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER,
                                                0,
                                                sizeof(GLuint),
                                                GL_MAP_READ_BIT);

    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    return mapping[0];
}

void PerfectSurfaceDetection::resetAtomicCounter(GLuint atomicCounter) const
{
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicCounter);

    // Map the buffer
    GLuint* mapping = (GLuint*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER,
                                                0 ,
                                                sizeof(GLuint),
                                                GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
    // Set memory to new value
    memset(mapping, 0, sizeof(GLuint));

    // Unmap the buffer
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}


// ### Main function ###

int main()
{
    PerfectSurfaceDetection detection;
    detection.renderLoop();
    return 0;
}

// ### Snippets ###

/*
    // Read back SSBO
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, atomsSSBO);
    AtomStruct *ptr;
    ptr = (AtomStruct *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for(int i = 0; i < atomCount; i++)
    {
        std::cout << i << ". " << ptr[i].radius << std::endl;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

*/
