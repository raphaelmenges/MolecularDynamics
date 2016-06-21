#ifndef SURFACE_DYNAMICS_VISUALIZATION_H
#define SURFACE_DYNAMICS_VISUALIZATION_H

#include <memory>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "ShaderTools/ShaderProgram.h"
#include "SurfaceExtraction/GPUProtein.h"
#include "SurfaceExtraction/GPUSurfaceExtraction.h"

// Notes:
// - Calculations done in angstrom

// Forward declaration
class Protein;
class GPUProtein;
class OrbitCamera;

// Class
class SurfaceDynamicsVisualization
{
public:

    // Constructor
    SurfaceDynamicsVisualization();

    // Destructor
    virtual ~SurfaceDynamicsVisualization();

    // Render
    void renderLoop();

private:

    // Keyboard callback for GLFW
    void keyCallback(int key, int scancode, int action, int mods);

    // Mouse button callback for GLFW
    void mouseButtonCallback(int button, int action, int mods);

    // Scroll callback for GLFW
    void scrollCallback(double xoffset, double yoffset);

    // Update computation information
    void updateComputationInformation(std::string device, float computationTime);

    // Update GUI
    void updateGUI();

    // Test for correctness of algorithm
    void testSurface();

    // Setup
    const bool mInitiallyUseGLSLImplementation = true;
    const float mCameraSmoothDuration = 1.5f;
    const float mAtomPointSize = 15.f;
    const float mSamplePointSize = 2.f;
    const float mMinDrawingExtentOffset = -5.f;
    const float mMaxDrawingExtentOffset = 5.f;
    const float mCameraDefaultAlpha = 90.f;
    const float mCameraDefaultBeta = 45.f;
    const glm::vec3 mInternalAtomColor = glm::vec3(0.75f, 0.75f, 0.75f);
    const glm::vec3 mSurfaceAtomColor = glm::vec3(1.f, 0.25f, 0.f);
    const glm::vec3 mSamplePointColor = glm::vec3(0.f, 1.0f, 0.25f);
    const float mClippingPlaneMin = 0.f;
    const float mClippingPlaneMax = 200.f;

    // Controllable parameters
    bool mRotateCamera = false;
    bool mRotateLight = false;
    int mSelectedAtom = 0;
    bool mRenderImpostor = true;
    bool mRenderWithProbeRadius = false;
    bool mUsePerspectiveCamera = false; // removed from GUI since both spheres
                                        // and cut of spheres is only correct
                                        // for orthographic projection
    bool mShowInternal = true;
    bool mShowSurface = true;
    bool mShowSurfaceComputationWindow = true;
    bool mShowCameraWindow = true;
    bool mShowDebuggingWindow = true;
    float mProbeRadius = 1.4f;
    int mCPPThreads = 8;
    int mSurfaceTestAtomSampleCount = 20;
    bool mShowSamplePoint = true;
    float mClippingPlane = 0.f;
    int mSurfaceTestSeed = 0;
    bool mShowAxesGizmo = false;
    bool mShowVisualizationWindow = true;

    // Debugging output
    std::string mComputeInformation = "";
    std::string mTestOutput = "";

    // Members
    GLFWwindow* mpWindow;
    std::unique_ptr<OrbitCamera> mupCamera; // camera for visualization
    glm::vec2 mCameraDeltaMovement;
    float mCameraSmoothTime;
    glm::vec3 mLightDirection;
    glm::vec3 mProteinMinExtent;
    glm::vec3 mProteinMaxExtent;
    std::vector<std::unique_ptr<GPUProtein> > mGPUProteins; // proteins on GPU
    std::unique_ptr<GPUSurfaceExtraction> mupGPUSurfaceExtraction;  // factory for GPUSurfaces
                                                                    // (unique pointer because has to be constructed after OpenGL initialization)
    std::vector<std::unique_ptr<GPUSurface> > mGPUSurfaces; // vector with surfaces
    int mFrame = 0;
    int mLayer = 0;

    // Samples for testing the surface
    GLuint mSurfaceTestVBO;
    GLuint mSurfaceTestVAO;
    std::unique_ptr<ShaderProgram> mupSurfaceTestProgram;
    int mSurfaceTestSampleCount = 0;

    // ### CPP implementation of surface atoms detection ###
    void runCPPImplementation(bool threaded);

    // ### GLSL implementation of surface atoms detection ###
    void runGLSLImplementation();

};

#endif // SURFACE_DYNAMICS_VISUALIZATION_H
