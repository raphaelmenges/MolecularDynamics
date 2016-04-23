#ifndef PERFECT_SURFACE_DETECTION_H
#define PERFECT_SURFACE_DETECTION_H

#include <memory>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "ShaderTools/ShaderProgram.h"
#include "Molecule/MDtrajLoader/Data/AtomLUT.h"
#include "AtomStruct.h"

// Notes:
// - Calculations done in angstrom

// TODO
// - Delete OpenGL stuff after usage
// - Work group size?
// - Binding points ok? not sure wheather atomic counter and image use the same
// - Use distance point / plane instead of scalar product, for example in testEndpoint() function

// Forward declaration
class Protein;
class OrbitCamera;

// Class
class PerfectSurfaceDetection
{
public:

    // Constructor
    PerfectSurfaceDetection();

    // Destructor
    virtual ~PerfectSurfaceDetection();

    // Render
    void renderLoop();

    // Keyboard callback for GLFW
    void keyCallback(int key, int scancode, int action, int mods);

    // Mouse button callback for GLFW
    void mouseButtonCallback(int button, int action, int mods);

    // Scroll callback for GLFW
    void scrollCallback(double xoffset, double yoffset);

private:

    // Atomic counter functions
    GLuint readAtomicCounter(GLuint atomicCounter) const;
    void resetAtomicCounter(GLuint atomicCounter) const;

    // Setup
    const float mProbeRadius = 1.4f;
    const bool mUseGLSLImplementation = false;

    // Members
    GLFWwindow* mpWindow;
    bool mRotateCamera;
    std::unique_ptr<Protein> mupProtein; // protein raw data
    std::unique_ptr<OrbitCamera> mupCamera; // camera for visualization
    int mAtomCount;
    GLuint mAtomsSSBO; // SSBO with struct of position and radius for each atom
    AtomLUT mAtomLUT;
    GLuint mSurfaceAtomTexture; // list of indices of surface atoms encoded in uint32
    GLuint mSurfaceAtomBuffer;
    GLint mSurfaceAtomCount; // count of atoms in surface
    GLuint mQuery;
    std::vector<AtomStruct> mAtomStructs;

    // ### CPP implementation of surface atoms detection ###
    void runCPPImplementation();

    // ### GLSL implementation of surface atoms detection ###
    void runGLSLImplementation();

};

#endif // PERFECT_SURFACE_DETECTION_H
