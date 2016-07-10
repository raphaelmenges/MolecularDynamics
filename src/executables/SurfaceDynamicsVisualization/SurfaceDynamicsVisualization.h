#ifndef SURFACE_DYNAMICS_VISUALIZATION_H
#define SURFACE_DYNAMICS_VISUALIZATION_H

#include <memory>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "ShaderTools/ShaderProgram.h"
#include "SurfaceExtraction/GPUProtein.h"
#include "SurfaceExtraction/GPUSurfaceExtraction.h"
#include "SurfaceExtraction/SurfaceValidation.h"

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

    // Set window title (hides processing part when progress is one)
    void setWindowTitle(float progress = 1.0f);

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

    // Set frame. Returns whether frame has been changed
    bool setFrame(int frame);

    // Compute layers
    void computeLayers(int startFrame, int endFrame, bool useGPU);

    // Get atom beneath cursor. Returns -1 when fails
    int getAtomBeneathCursor() const;

    // Setup
    const bool mInitiallyUseGLSLImplementation = false;
    const float mCameraSmoothDuration = 1.5f;
    const float mAtomPointSize = 15.f;
    const float mSamplePointSize = 2.f;
    const float mMinDrawingExtentOffset = -5.f;
    const float mMaxDrawingExtentOffset = 5.f;
    const float mCameraDefaultAlpha = 90.f;
    const float mCameraDefaultBeta = 45.f;
    const glm::vec3 mInternalAtomColor = glm::vec3(0.75f, 0.75f, 0.75f);
    const glm::vec3 mSurfaceAtomColor = glm::vec3(1.f, 0.25f, 0.f);
    const glm::vec3 mInternalSamplePointColor = glm::vec3(1.f, 0.9f, 0.0f);
    const glm::vec3 mSurfaceSamplePointColor = glm::vec3(0.f, 1.0f, 0.2f);
    const float mClippingPlaneMin = 0.f;
    const float mClippingPlaneMax = 200.f;
    const std::string mWindowTitle = "Surface Dynamics Visualization";

    // Controllable parameters
    bool mRotateCamera = false;
    bool mMoveCamera = false;
    int mSelectedAtom = 0;
    bool mRenderImpostor = true;
    bool mRenderWithProbeRadius = false;
    bool mUsePerspectiveCamera = false; // removed from GUI since both spheres
                                        // and cut of spheres is only correct
                                        // for orthographic projection
    bool mShowInternal = true;
    bool mShowSurface = true;
    bool mShowSurfaceExtractionWindow = true;
    bool mShowCameraWindow = true;
    bool mShowDebuggingWindow = true;
    float mProbeRadius = 1.4f;
    int mCPUThreads = 8;
    int mSurfaceValidationAtomSampleCount = 20;
    bool mShowValidationSamples = true;
    float mClippingPlane = 0.f;
    int mSurfaceValidationSeed = 0;
    bool mShowAxesGizmo = false;
    bool mShowVisualizationWindow = true;
    bool mPlayAnimation = false;
    int mPlayAnimationRate = 15;
    bool mShowValidationWindow = true;
    bool mShowInternalSamples = true;
    bool mShowSurfaceSamples = true;
    int mComputationStartFrame = 0;
    int mComputationEndFrame = 0;
    bool mExtractLayers = false;
    bool mRepeatAnimation = false;
    int mSmoothAnimationRadius = 0;
    float mSmoothAnimationMaxDeviation = 5;

    // Debugging output
    std::string mComputeInformation = "No computation info available";
    std::string mValidationInformation = "No validation info available";

    // Members
    GLFWwindow* mpWindow;
    std::unique_ptr<OrbitCamera> mupCamera; // camera for visualization
    glm::vec2 mCameraDeltaRotation;
    float mCameraRotationSmoothTime;
    glm::vec3 mLightDirection;
    glm::vec3 mProteinMinExtent;
    glm::vec3 mProteinMaxExtent;
    std::unique_ptr<GPUProtein> mupGPUProtein; // protein on GPU
    std::unique_ptr<GPUSurfaceExtraction> mupGPUSurfaceExtraction;  // factory for GPUSurfaces
                                                                    // (unique pointer because has to be constructed after OpenGL initialization)
    std::vector<std::unique_ptr<GPUSurface> > mGPUSurfaces; // vector with surfaces
    int mFrame = 0; // do not set it directly, let it be done by setFrame() method!
    int mLayer = 0;
    float mFramePlayTime = 0; // time of displaying a molecule state at playing the animation
    int mComputedStartFrame = 0;
    int mComputedEndFrame = 0;

    // Surface validation
    std::unique_ptr<SurfaceValidation> mupSurfaceValidation;
    int mSurfaceValidationSampleCount = 0;
};

#endif // SURFACE_DYNAMICS_VISUALIZATION_H
