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

private:

    // Keyboard callback for GLFW
    void keyCallback(int key, int scancode, int action, int mods);

    // Mouse button callback for GLFW
    void mouseButtonCallback(int button, int action, int mods);

    // Scroll callback for GLFW
    void scrollCallback(double xoffset, double yoffset);

    // Atomic counter functions
    GLuint readAtomicCounter(GLuint atomicCounter) const;
    void resetAtomicCounter(GLuint atomicCounter) const;

    // Test for correctness of algorithm
    void testSurface();

    // Setup
    const bool mInitiallyUseGLSLImplementation = true;
    const float mCameraSmoothDuration = 1.5f;
    const float mAtomPointSize = 15.f;
    const float mSamplePointSize = 2.f;

    // Controllable parameters
    bool mRotateCamera = false;
    bool mRotateLight = false;
    int mSelectedAtom = 0;
    bool mRenderImpostor = true;
    bool mRenderWithProbeRadius = false;
    bool mUsePerspectiveCamera = false;
    bool mShowInternal = true;
    bool mShowSurface = true;
    bool mShowComputationWindow = true;
    bool mShowDebuggingWindow = true;
    float mMinXDraw = 0;
    float mMinYDraw = 0;
    float mMinZDraw = 0;
    float mMaxXDraw = 0;
    float mMaxYDraw = 0;
    float mMaxZDraw = 0;
    float mProbeRadius = 1.2f;
    int mCPPThreads = 8;
    int mSurfaceTestAtomSampleCount = 20;
    bool mShowSamplePoint = true;

    // Debugging output
    std::string mComputeInformation = "";

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

    // Samples for testing the surface
    GLuint mSurfaceTestVBO;
    GLuint mSurfaceTestVAO;
    std::unique_ptr<ShaderProgram> mupSurfaceTestProgram;
    int mSurfaceTestSampleCount = 0;

    // ### CPP implementation of surface atoms detection ###
    void runCPPImplementation(bool threaded);

    // ### GLSL implementation of surface atoms detection ###
    void runGLSLImplementation();

    // Update GUI
    void updateGUI();

    // Update computation information
    void updateComputationInformation(std::string device, float computationTime);

};

#endif // PERFECT_SURFACE_DETECTION_H
