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
// - Binding points ok? not sure whether atomic counter and image use the same
// - Instead of direclty use ALL atoms as input, us index list InputIndices

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
    const float mProbeRadius = 0.894f;
    const bool mUseGLSLImplementation = true;
    const float mCameraSmoothDuration = 1.5f;

    // Controllable parameters
    bool mRotateCamera = false;
    bool mRotateLight = false;
    int mSelectedAtom = 0;
    bool mRenderImpostor = false;
    bool mRenderWithProbeRadius = false;
    bool mUsePerspectiveCamera = true;

    // Members
    GLFWwindow* mpWindow;
    std::unique_ptr<OrbitCamera> mupCamera; // camera for visualization
    int mAtomCount;
    AtomLUT mAtomLUT;
    std::vector<AtomStruct> mAtomStructs;
    glm::vec2 mCameraDeltaMovement;
    float mCameraSmoothTime;
    glm::vec3 mLightDirection;
    glm::vec3 mProteinMinExtent;
    glm::vec3 mProteinMaxExtent;
    GLint mInternalCount; // general count of internal atoms
    GLint mSurfaceCount; // general count of surface atoms

    // SSBO
    GLuint mAtomsSSBO; // SSBO with struct of position and radius for each atom

    // Images
    GLuint mInternalIndicesTexture; // list of indices of internal output atoms encoded in uint32
    GLuint mInternalIndicesBuffer;
    GLuint mSurfaceIndicesTexture; // list of indices of surface output atoms encoded in uint32
    GLuint mSurfaceIndicesBuffer;

    // Queries
    GLuint mQuery;

    // ### CPP implementation of surface atoms detection ###
    void runCPPImplementation();

    // ### GLSL implementation of surface atoms detection ###
    void runGLSLImplementation();

};

#endif // PERFECT_SURFACE_DETECTION_H
