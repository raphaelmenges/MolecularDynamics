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
// - Camera and Rotation / Translation do not like each other
// - Orthographic Projection

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
    const float mProbeRadius = 1.2f;
    const bool mUseGLSLImplementation = false;
    const float mCameraSmoothDuration = 1.5f;

    // Members
    GLFWwindow* mpWindow;
    bool mRotateCamera;
    bool mRotateLight;
    std::unique_ptr<OrbitCamera> mupCamera; // camera for visualization
    int mAtomCount;
    GLuint mAtomsSSBO; // SSBO with struct of position and radius for each atom
    AtomLUT mAtomLUT;
    GLuint mSurfaceAtomTexture; // list of indices of surface atoms encoded in uint32
    GLuint mSurfaceAtomBuffer;
    GLint mSurfaceAtomCount; // count of atoms in surface
    GLuint mQuery;
    std::vector<AtomStruct> mAtomStructs;
    bool mRenderImpostor;
    bool mRenderWithProbeRadius;
    glm::vec2 mCameraDeltaMovement;
    float mCameraSmoothTime;
    glm::vec3 mLightDirection;
    glm::vec3 mProteinMinExtent;
    glm::vec3 mProteinMaxExtent;
    int mSelectedAtom;

    // ### CPP implementation of surface atoms detection ###
    void runCPPImplementation();

    // ### GLSL implementation of surface atoms detection ###
    void runGLSLImplementation();

};

#endif // PERFECT_SURFACE_DETECTION_H
