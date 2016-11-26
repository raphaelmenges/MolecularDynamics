//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

#include "SurfaceDynamicsVisualization.h"
#include "Utils/OrbitCamera.h"
#include "ShaderTools/Renderer.h"
#include "Molecule/MDtrajLoader/MdTraj/MdTrajWrapper.h"
#include "Molecule/MDtrajLoader/Data/Protein.h"
#include "SimpleLoader.h"
#include "imgui/imgui.h"
#include "imgui/examples/opengl3_example/imgui_impl_glfw_gl3.h"
#include "text-csv/include/text/csv/ostream.hpp"
#include <glm/gtx/component_wise.hpp>
#include <sstream>
#include <iomanip>

// stb_image wants those defines
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

// Namespace for text-csv
namespace csv = ::text::csv;

// ### Class implementation ###

SurfaceDynamicsVisualization::SurfaceDynamicsVisualization(std::string filepathPDB, std::string filepathXTC)
{
    Logger::instance().print("Welcome to Surface Dynamics Visualization!");

    // # Setup members
    mCameraDeltaRotation = glm::vec2(0,0);
    mCameraRotationSmoothTime = 1.f;
    mLightDirection = glm::normalize(glm::vec3(-0.5, -0.75, -0.3));
    mWindowWidth = mInitialWindowWidth;
    mWindowHeight = mInitialWindowHeight;
    mPDBFilepath = filepathPDB;
    mXTCFilepath = filepathXTC;

    // # Setup paths
    resetPath(mGlobalAnalysisFilePath, "/GlobalAnalysis.csv");
    resetPath(mGroupAnalysisFilePath, "/GroupAnalysis.csv");
    resetPath(mSurfaceIndicesFilePath, "/SurfaceIndices.csv");
    resetPath(mAminoAcidAnalysisAccumulatedFilePath, "/AminoAcidAnalysis-accumulated.csv");
    resetPath(mAminoAcidAnalysisAvgLayersFilePath, "/AminoAcidAnalysis-avgLayers.csv");
    resetPath(mAminoAcidAnalysisInverseAvgLayersFilePath, "/AminoAcidAnalysis-inverseAvgLayer.csv");
    resetPath(mAminoAcidAnalysisAvgLayersDeltaFilePath, "/AminoAcidAnalysis-avgLayersDelta.csv");
    resetPath(mAminoAcidAnalysisInverseAvgLayersDeltaFilePath, "/AminoAcidAnalysis-inverseAverageLayersDelta.csv");

    // Create window (which initializes OpenGL)
    Logger::instance().print("Create window..");
    mpWindow = generateWindow(mWindowTitle, mWindowWidth, mWindowHeight);
    Logger::instance().print("..done");

    // Init ImGui and load font
    Logger::instance().print("Load GUI..");
    ImGui_ImplGlfwGL3_Init(mpWindow, true);
    ImGuiIO& io = ImGui::GetIO();
    std::string fontpath = std::string(RESOURCES_PATH) + "/fonts/dejavu-fonts-ttf-2.35/ttf/DejaVuSans.ttf";
    io.Fonts->AddFontFromFileTTF(fontpath.c_str(), 14);

    // Add icons to font atlas
    ImFontConfig config;
    config.MergeMode = true;
    static const ImWchar ranges[] =
    {
        0x25B8, 0x25B8, // play
        0x5f0, 0x5f0, // pause
        0x2794, 0x2794, // right arrow
        0x25cb, 0x25cb, // circle
        0x212b, 0x212b, // angstrom
        0x2023, 0x2023, // triangle bullet
        0
    }; // has to be static to be available during run
    io.Fonts->AddFontFromFileTTF(fontpath.c_str(), 16, &config, ranges);
    Logger::instance().print("..done");

    // Clear color (has to be zero for sake of framebuffers)
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // # Callbacks after ImGui

    // Register keyboard callback
    std::function<void(int, int, int, int)> kC = [&](int k, int s, int a, int m)
    {
        // Check whether ImGui is handling this
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureKeyboard) { return; }
        this->keyCallback(k, s, a, m);
    };
    setKeyCallback(mpWindow, kC);

    // Register mouse button callback
    std::function<void(int, int, int)> kB = [&](int b, int a, int m)
    {
        // Check whether ImGui is handling this
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureMouse) { return; }
        this->mouseButtonCallback(b, a, m);
    };
    setMouseButtonCallback(mpWindow, kB);

    // Register scroll callback
    std::function<void(double, double)> kS = [&](double x, double y)
    {
        // Check whether ImGui is handling this
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureMouse) { return; }
        this->scrollCallback(x,y);
    };
    setScrollCallback(mpWindow, kS);

    // # Load molecule

    // Loading molecule
    Logger::instance().print("Import molecule..");
    MdTrajWrapper mdwrap;
    std::vector<std::string> paths;
    paths.push_back(mPDBFilepath);
    if(!mXTCFilepath.empty()) { paths.push_back(mXTCFilepath); }
    std::unique_ptr<Protein> upProtein = std::move(mdwrap.load(paths));
    mupGPUProtein = std::unique_ptr<GPUProtein>(new GPUProtein(upProtein.get()));
    Logger::instance().print("..done");

    // # Prepare framebuffers for rendering
    Logger::instance().print("Create framebuffer..");
    mupMoleculeFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(mWindowWidth, mWindowHeight, mSuperSampling));
    mupMoleculeFramebuffer->bind();
    mupMoleculeFramebuffer->addAttachment(Framebuffer::ColorFormat::RGBA); // color
    mupMoleculeFramebuffer->addAttachment(Framebuffer::ColorFormat::RGB); // picking index
    mupMoleculeFramebuffer->unbind();
    mupSelectedAtomFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(mWindowWidth, mWindowHeight, mSuperSampling));
    mupSelectedAtomFramebuffer->bind();
    mupSelectedAtomFramebuffer->addAttachment(Framebuffer::ColorFormat::RGBA); // color
    mupSelectedAtomFramebuffer->unbind();
    mupOverlayFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(mWindowWidth, mWindowHeight));
    mupOverlayFramebuffer->bind();
    mupOverlayFramebuffer->addAttachment(Framebuffer::ColorFormat::RGBA);
    mupOverlayFramebuffer->unbind();
    Logger::instance().print("..done");

    // # Prepare background cubemaps
    Logger::instance().print("Load cubemaps..");
    mScientificCubemapTexture = createCubemapTexture(
        std::string(RESOURCES_PATH) + "/cubemaps/Scientific/posx.png",
        std::string(RESOURCES_PATH) + "/cubemaps/Scientific/negx.png",
        std::string(RESOURCES_PATH) + "/cubemaps/Scientific/posy.png",
        std::string(RESOURCES_PATH) + "/cubemaps/Scientific/negy.png",
        std::string(RESOURCES_PATH) + "/cubemaps/Scientific/posz.png",
        std::string(RESOURCES_PATH) + "/cubemaps/Scientific/negz.png");

    mCVCubemapTexture = createCubemapTexture(
        std::string(RESOURCES_PATH) + "/cubemaps/CV/posx.png",
        std::string(RESOURCES_PATH) + "/cubemaps/CV/negx.png",
        std::string(RESOURCES_PATH) + "/cubemaps/CV/posy.png",
        std::string(RESOURCES_PATH) + "/cubemaps/CV/negy.png",
        std::string(RESOURCES_PATH) + "/cubemaps/CV/posz.png",
        std::string(RESOURCES_PATH) + "/cubemaps/CV/negz.png");

    mBeachCubemapTexture = createCubemapTexture(
        std::string(RESOURCES_PATH) + "/cubemaps/NissiBeach/posx.jpg",
        std::string(RESOURCES_PATH) + "/cubemaps/NissiBeach/negx.jpg",
        std::string(RESOURCES_PATH) + "/cubemaps/NissiBeach/posy.jpg",
        std::string(RESOURCES_PATH) + "/cubemaps/NissiBeach/negy.jpg",
        std::string(RESOURCES_PATH) + "/cubemaps/NissiBeach/posz.jpg",
        std::string(RESOURCES_PATH) + "/cubemaps/NissiBeach/negz.jpg");

    mWhiteCubemapTexture = createCubemapTexture(
        std::string(RESOURCES_PATH) + "/textures/white.png",
        std::string(RESOURCES_PATH) + "/textures/white.png",
        std::string(RESOURCES_PATH) + "/textures/white.png",
        std::string(RESOURCES_PATH) + "/textures/white.png",
        std::string(RESOURCES_PATH) + "/textures/white.png",
        std::string(RESOURCES_PATH) + "/textures/white.png");

    Logger::instance().print("..done");

    // # Create camera
    Logger::instance().print("Create camera..");
    glm::vec3 cameraCenter = (mupGPUProtein->getMinCoordinates() + mupGPUProtein->getMaxCoordinates()) / 2.f;
    glm::vec3 maxAbsCoordinates(
        glm::max(glm::abs(mupGPUProtein->getMinCoordinates().x), glm::abs(mupGPUProtein->getMaxCoordinates().x)),
        glm::max(glm::abs(mupGPUProtein->getMinCoordinates().y), glm::abs(mupGPUProtein->getMaxCoordinates().y)),
        glm::max(glm::abs(mupGPUProtein->getMinCoordinates().z), glm::abs(mupGPUProtein->getMaxCoordinates().z))        );
    float cameraRadius = glm::compMax(maxAbsCoordinates - cameraCenter);
    mupCamera = std::unique_ptr<OrbitCamera>(
        new OrbitCamera(
            cameraCenter,
            mCameraDefaultAlpha,
            mCameraDefaultBeta,
            2.f * cameraRadius,
            cameraRadius / 2.f,
            8.f * cameraRadius,
            45.f,
            0.1f));
    Logger::instance().print("..done");

    // # Surface extraction
    Logger::instance().print("Prepare surface extraction..");

    // Construct GPUSurfaceExtraction object after OpenGL has been initialized
    mupGPUSurfaceExtraction = std::unique_ptr<GPUSurfaceExtraction>(new GPUSurfaceExtraction);
    Logger::instance().print("..done");

    // # Analysis
    Logger::instance().print("Prepare analysis..");

    // Create path for analysis group
    mupPath = std::unique_ptr<Path>(new Path());

    // Group for highlighting
    mupGroupIndicators = std::unique_ptr<GPUBuffer<GLfloat> >(new GPUBuffer<GLfloat>);
    mupGroupIndicators->fill(std::vector<GLfloat>(mupGPUProtein->getAtomCount(), 0.f), GL_DYNAMIC_DRAW); // create buffer with zeros for startup since nothing is highlighted

    // Hull samples
    mupHullSamples = std::unique_ptr<GPUHullSamples>(new GPUHullSamples());

    // Create empty outline atoms indices after OpenGL initialization
    mupOutlineAtomIndices = std::unique_ptr<GPUTextureBuffer>(new GPUTextureBuffer(0)); // create empty outline atom indices buffer
    Logger::instance().print("..done");

    // # Ascension
    Logger::instance().print("Prepare ascension..");

    // Ascension
    mupAscension = std::unique_ptr<GPUBuffer<GLfloat> >(new GPUBuffer<GLfloat>);
    Logger::instance().print("..done");

    // # Validation
    Logger::instance().print("Prepare validation..");

    // Prepare validation of the surface
    mupSurfaceValidation = std::unique_ptr<SurfaceValidation>(new SurfaceValidation());
    Logger::instance().print("..done");

    // # Group rendering texture and semaphore
    glGenTextures(1, &mGroupRenderingTexture);
    glBindTexture(GL_TEXTURE_2D, mGroupRenderingTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    std::vector<GLfloat> emptyData(mupMoleculeFramebuffer->getWidth() * mupMoleculeFramebuffer->getHeight() * 4, 0.f);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mupMoleculeFramebuffer->getWidth(), mupMoleculeFramebuffer->getHeight(), 0, GL_RGBA, GL_FLOAT, &emptyData[0]);
    glBindTexture(GL_TEXTURE_2D, 0);
    std::vector<GLuint> emptyDataSemaphore(mupMoleculeFramebuffer->getWidth() * mupMoleculeFramebuffer->getHeight(), 0.f);
    mupGroupRenderingSemaphore = std::unique_ptr<GPUTextureBuffer>(new GPUTextureBuffer(emptyDataSemaphore));

    // # Ascension helper texture
    glGenTextures(1, &mAscensionHelperTexture);
    glBindTexture(GL_TEXTURE_2D, mAscensionHelperTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Load image from disk
    int channelCount;
    unsigned char* pData = stbi_load(std::string(std::string(RESOURCES_PATH) + "/surfaceDynamics/AscensionHelper.png").c_str(), &mAscensionHelperWidth, &mAscensionHelperHeight, &channelCount, 0);

    // Set texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mAscensionHelperWidth, mAscensionHelperHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pData);

    // Delete raw image data
    stbi_image_free(pData);
    glBindTexture(GL_TEXTURE_2D, 0);

    // # Other

    // Set endframe in GUI to maximum number
    mEndFrame = mupGPUProtein->getFrameCount() - 1;
}

SurfaceDynamicsVisualization::~SurfaceDynamicsVisualization()
{
    Logger::instance().print("Clean up..");

    // Delete cubemaps
    glDeleteTextures(1, &mScientificCubemapTexture);
    glDeleteTextures(1, &mCVCubemapTexture);
    glDeleteTextures(1, &mBeachCubemapTexture);
    glDeleteTextures(1, &mWhiteCubemapTexture);

    // Delete group rendering texture
    glDeleteTextures(1, &mGroupRenderingTexture);

    // Delete ascension helper texture
    glDeleteTextures(1, &mAscensionHelperTexture);

    Logger::instance().print("..done");

    Logger::instance().print("Goodbye!");
}

void SurfaceDynamicsVisualization::renderLoop()
{
    Logger::instance().print("Prepare rendering loop..");

    // Setup OpenGL
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    // Cursor is saved for camera rotation smoothing
    float prevCursorX, prevCursorY = 0;

    // # Create gizmo to display coordinate system axes
    GLuint axisGizmoVBO = 0;
    GLuint axisGizmoVAO = 0;
    ShaderProgram axisGizmoProgram("/SurfaceDynamicsVisualization/axis.vert", "/SurfaceDynamicsVisualization/axis.frag");

    // Generate and bind vertex array object
    glGenVertexArrays(1, &axisGizmoVAO);
    glBindVertexArray(axisGizmoVAO);

    // Fill vertex buffer with vertices
    std::vector<glm::vec3> axisVertices;
    axisVertices.push_back(glm::vec3(0,0,0));
    axisVertices.push_back(glm::vec3(1,0,0));
    glGenBuffers(1, &axisGizmoVBO);
    glBindBuffer(GL_ARRAY_BUFFER, axisGizmoVBO);
    glBufferData(GL_ARRAY_BUFFER, axisVertices.size() * sizeof(glm::vec3), axisVertices.data(), GL_STATIC_DRAW);

    // Bind it to shader program
    GLint posAttrib = glGetAttribLocation(axisGizmoProgram.getProgramHandle(), "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Unbind everything
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // # Prepare shader programs

    // Shader programs for rendering the molecule
    ShaderProgram hullProgram("/SurfaceDynamicsVisualization/hull.vert", "/SurfaceDynamicsVisualization/impostor.geom", "/SurfaceDynamicsVisualization/impostor.frag");
    ShaderProgram ascensionProgram("/SurfaceDynamicsVisualization/ascension.vert", "/SurfaceDynamicsVisualization/impostor.geom", "/SurfaceDynamicsVisualization/impostor.frag");
    ShaderProgram coloringProgram("/SurfaceDynamicsVisualization/coloring.vert", "/SurfaceDynamicsVisualization/impostor.geom", "/SurfaceDynamicsVisualization/impostor.frag");
    ShaderProgram analysisProgram("/SurfaceDynamicsVisualization/analysis.vert", "/SurfaceDynamicsVisualization/impostor.geom", "/SurfaceDynamicsVisualization/impostor.frag");
    ShaderProgram fallbackProgram("/SurfaceDynamicsVisualization/fallback.vert", "/SurfaceDynamicsVisualization/impostor.geom", "/SurfaceDynamicsVisualization/impostor.frag");
    ShaderProgram selectionProgram("/SurfaceDynamicsVisualization/selection.vert", "/SurfaceDynamicsVisualization/impostor.geom", "/SurfaceDynamicsVisualization/impostor.frag");

    // Shader program to mark surface atoms
    ShaderProgram surfaceMarksProgram("/SurfaceDynamicsVisualization/point.vert", "/SurfaceDynamicsVisualization/point.geom", "/SurfaceDynamicsVisualization/point.frag");

    // Shader program for screenfilling quad rendering
    ShaderProgram screenFillingProgram("/SurfaceDynamicsVisualization/screenfilling.vert", "/SurfaceDynamicsVisualization/screenfilling.geom", "/SurfaceDynamicsVisualization/screenfilling.frag");

    // Shader program for cubemap
    ShaderProgram cubemapProgram("/SurfaceDynamicsVisualization/cubemap.vert", "/SurfaceDynamicsVisualization/cubemap.geom", "/SurfaceDynamicsVisualization/cubemap.frag");

    // Shader program for outline rendering
    ShaderProgram outlineProgram("/SurfaceDynamicsVisualization/hull.vert", "/SurfaceDynamicsVisualization/impostor.geom", "/SurfaceDynamicsVisualization/outline.frag");

    Logger::instance().print("..done");

    Logger::instance().print("Enter render loop..");

    // Call render function of Rendering.h with lambda function
    render(mpWindow, [&] (float deltaTime)
    {
        if(mFrameLogging) { Logger::instance().print("### New Frame ###"); }

        // Time accumulation
        mAccTime += deltaTime;
        mAccTime = glm::mod(mAccTime, 60.f * 30.f); // reset time after 30 minutes. Otherwise float not precise enough

        // Viewport size
        glm::vec2 resolution = getResolution(mpWindow);
        mWindowWidth = resolution.x;
        mWindowHeight = resolution.y;

        // # Update everything before drawing

        // ### MOLECULE ANIMATION ##################################################################################
        if(mFrameLogging) { Logger::instance().print("Update animations.."); }

        // If playing, decide whether to switch to next frame of animation
        if(mPlayAnimation)
        {
            // Time each frame of animation should be displayed
            float duration = 1.f / (float)mPlayAnimationRate;

            // Go as many frames in animation forward as necessary to catch the time
            float dT = deltaTime;
            if(dT > 0)
            {
                mFramePlayTime += deltaTime;
                if(mFramePlayTime >= duration)
                {
                    // Decrement dT by time which was to much for that animation frame
                    dT -= mFramePlayTime - duration;

                    // Reset frame play time
                    mFramePlayTime = 0;

                    // Next frame
                    int nextFrame = mFrame + 1;

                    // Cylce if enabled
                    if(mRepeatOnlyComputed && mComputedStartFrame >= 0 && mComputedEndFrame >= 0)
                    {
                        if(mRepeatAnimation && nextFrame > mComputedEndFrame)
                        {
                            nextFrame = mComputedStartFrame;
                        }
                    }
                    else
                    {
                        if(mRepeatAnimation && nextFrame > mEndFrame)
                        {
                            nextFrame = mStartFrame;
                        }
                    }

                    // Increment time (checks are done by method)
                    if(!setFrame(nextFrame))
                    {
                        mPlayAnimation = false;
                    }
                }
            }
        }
        if(mFrameLogging) { Logger::instance().print("..done"); }

        // ### CAMERA UPDATE #######################################################################################
        if(mFrameLogging) { Logger::instance().print("Update camera.."); }

        // Calculate cursor movement
        double cursorX, cursorY;
        glfwGetCursorPos(mpWindow, &cursorX, &cursorY);
        GLfloat cursorDeltaX = (float)cursorX - prevCursorX;
        GLfloat cursorDeltaY = (float)cursorY - prevCursorY;
        bool lockCursorPosition = false;

        // Rotate camera
        if(mRotateCamera)
        {
            mCameraDeltaRotation = 0.25f * glm::vec2(cursorDeltaX, cursorDeltaY); // just weighted down a little bit
            mCameraRotationSmoothTime = 1.f;
            lockCursorPosition = true;
        }
        else
        {
            mCameraRotationSmoothTime -= deltaTime / mCameraSmoothDuration;
            mCameraRotationSmoothTime = glm::max(mCameraRotationSmoothTime, 0.f);
        }
        glm::vec2 cameraMovement = glm::lerp(glm::vec2(0), mCameraDeltaRotation, mCameraRotationSmoothTime);
        mupCamera->setAlpha(mupCamera->getAlpha() + 0.25f * cameraMovement.x);
        mupCamera->setBeta(mupCamera->getBeta() - 0.25f * cameraMovement.y);

        // Move camera
        if(mMoveCamera)
        {
            glm::vec3 a = glm::normalize(
                glm::cross(
                    glm::vec3(0,1,0),
                    mupCamera->getDirection())); // use up vector for cross product
            glm::vec3 b = glm::normalize(
                glm::cross(
                    mupCamera->getDirection(),
                    a));

            mupCamera->setCenter(
                mupCamera->getCenter()
                + (deltaTime * mupCamera->getRadius() * 0.3f
                    * (((float)cursorDeltaX * a)
                        + ((float)cursorDeltaY * b))));

            lockCursorPosition = true;
        }

        // Update camera
        mupCamera->update(mWindowWidth, mWindowHeight, mUsePerspectiveCamera);

        // Remember about cursor position
        if(lockCursorPosition)
        {
            glfwSetCursorPos(mpWindow, prevCursorX, prevCursorY);
        }
        else
        {
            prevCursorX = cursorX;
            prevCursorY = cursorY;
        }
        if(mFrameLogging) { Logger::instance().print("..done"); }

        // ### OVERLAY RENDERING ###################################################################################
        if(mFrameLogging) { Logger::instance().print("Render overlay.."); }

        // # Fill overlay framebuffer
        mupOverlayFramebuffer->bind();
        mupOverlayFramebuffer->resize(mWindowWidth, mWindowHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Only show analysis visualization when frame is computed
        if(frameComputed())
        {
            // Get count of atoms which will get a outline (size of buffer can be used here because all elements are valid)
            int outlineAtomCount = mupOutlineAtomIndices->getSize();
            if(mRenderOutline && (outlineAtomCount > 0))
            {
                // Bind buffers of radii and trajectory for rendering
                mupGPUProtein->bind(0, 1);

                // Slot 2 and 3 are not filled since GroupIndicatorBuffer and semaphore is not necessary

                // Bind ascension
                mupAscension->bind(5);

                // Bind texture buffer with input atoms
                mupOutlineAtomIndices->bindAsImage(6, GPUAccess::READ_ONLY);

                // Probe radius
                float probeRadius = mRenderWithProbeRadius ? mComputedProbeRadius : 0.f;

                // Enable stencil test
                glEnable(GL_STENCIL_TEST);

                // Set up shader for outline drawing
                outlineProgram.use();
                outlineProgram.update("view", mupCamera->getViewMatrix());
                outlineProgram.update("projection", mupCamera->getProjectionMatrix());
                outlineProgram.update("frame", mFrame);
                outlineProgram.update("atomCount", mupGPUProtein->getAtomCount());
                outlineProgram.update("smoothAnimationRadius", mSmoothAnimationRadius);
                outlineProgram.update("smoothAnimationMaxDeviation", mSmoothAnimationMaxDeviation);
                outlineProgram.update("frameCount", mupGPUProtein->getFrameCount());
                outlineProgram.update("outlineColor", mOutlineColor);
                outlineProgram.update("ascensionFrame", mFrame - mComputedStartFrame);
                outlineProgram.update("ascensionChangeRadiusMultiplier", mAscensionChangeRadiusMultiplier);

                // Create stencil
                glStencilFunc(GL_ALWAYS, 1, 0xFF); // set reference value for new stencil values
                glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); // use reference when stencil and depth test successful
                glStencilMask(0xFF); // write to stencil buffer
                glClear(GL_STENCIL_BUFFER_BIT); // clear stencil buffer (0 by default)
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // disable to write color
                glDepthMask(GL_FALSE); // disable to write depth
                outlineProgram.update("probeRadius", probeRadius);
                glDrawArrays(GL_POINTS, 0, outlineAtomCount);
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // enable to write color
                glDepthMask(GL_TRUE); // enable to write depth

                // Draw outline
                glStencilFunc(GL_NOTEQUAL, 1, 0xFF); // pass test if stencil at that pixel is not eqaul one
                glStencilMask(0x00); // do not write to stencil buffer
                outlineProgram.update("probeRadius", probeRadius + mOutlineWidth); // just add outline radius to probe radius
                glDrawArrays(GL_POINTS, 0, outlineAtomCount);

                // Disable stencil test
                glDisable(GL_STENCIL_TEST);
            }

            // Render point inside of surface atoms for marking
            if(mMarkSurfaceAtoms)
            {
                // Bind protein
                mupGPUProtein->bind(0, 1);

                // Bind surface indices
                mGPUSurfaces.at(mFrame - mComputedStartFrame)->bindSurfaceIndices(mLayer, 2);

                // Setup shader
                glPointSize(mSurfaceMarkPointSize);
                surfaceMarksProgram.use();
                surfaceMarksProgram.update("view", mupCamera->getViewMatrix());
                surfaceMarksProgram.update("projection", mupCamera->getProjectionMatrix());
                surfaceMarksProgram.update("clippingPlane", mClippingPlane);
                surfaceMarksProgram.update("frame", mFrame);
                surfaceMarksProgram.update("atomCount", mupGPUProtein->getAtomCount());
                surfaceMarksProgram.update("smoothAnimationRadius", mSmoothAnimationRadius);
                surfaceMarksProgram.update("smoothAnimationMaxDeviation", mSmoothAnimationMaxDeviation);
                surfaceMarksProgram.update("frameCount", mupGPUProtein->getFrameCount());
                surfaceMarksProgram.update("color", glm::vec4(mSurfaceAtomColor, 1.f));
                glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfSurfaceAtoms(mLayer));
            }

            // Hull samples
            if(mRenderHullSamples)
            {
                mupHullSamples->drawSamples(
                    mFrame,
                    mSamplePointSize,
                    mInternalHullSampleColor,
                    mSurfaceHullSampleColor,
                    mupCamera->getViewMatrix(),
                    mupCamera->getProjectionMatrix(),
                    mClippingPlane);
            }

            // Drawing of path (does not care for depth)
            if(mShowPath)
            {
                mupPath->draw(
                    mFrame,
                    mPathFrameRadius,
                    mupCamera->getViewMatrix(),
                    mupCamera->getProjectionMatrix(),
                    mPastPathColor,
                    mFuturePathColor,
                    mPathPointSize);
            }
        }

        // Drawing of surface validation before molecules, so sample points at same z coordinate as impostor are in front
        if(mShowValidationSamples)
        {
            mupSurfaceValidation->drawSamples(
                mSamplePointSize,
                mInternalValidationSampleColor,
                mSurfaceValidationSampleColor,
                mupCamera->getViewMatrix(),
                mupCamera->getProjectionMatrix(),
                mClippingPlane,
                mShowInternalSamples,
                mShowSurfaceSamples);
        }

        // Drawing of coordinates system axes
        if(mShowAxesGizmo)
        {
            glDisable(GL_DEPTH_TEST);
            glBindVertexArray(axisGizmoVAO);

            // General shader setup
            axisGizmoProgram.use();
            axisGizmoProgram.update("view", mupCamera->getViewMatrix());
            axisGizmoProgram.update("projection", mupCamera->getProjectionMatrix());

            // X axis
            glm::mat4 axisModelMatrix = glm::translate(glm::mat4(1.f), glm::vec3(0,0,0));
            axisGizmoProgram.update("model", axisModelMatrix);
            axisGizmoProgram.update("color", glm::vec3(1.f, 0.f, 0.f));
            glDrawArrays(GL_LINES, 0, axisVertices.size());

            // Y axis
            axisModelMatrix = glm::translate(glm::mat4(1.f), glm::vec3(0,0,0));
            axisModelMatrix = glm::rotate(axisModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            axisGizmoProgram.update("model", axisModelMatrix);
            axisGizmoProgram.update("color", glm::vec3(0.f, 1.f, 0.f));
            glDrawArrays(GL_LINES, 0, axisVertices.size());

            // Z axis
            axisModelMatrix = glm::translate(glm::mat4(1.f), glm::vec3(0,0,0));
            axisModelMatrix = glm::rotate(axisModelMatrix, glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            axisGizmoProgram.update("model", axisModelMatrix);
            axisGizmoProgram.update("color", glm::vec3(0.f, 0.f, 1.f));
            glDrawArrays(GL_LINES, 0, axisVertices.size());

            glBindVertexArray(0);
            glEnable(GL_DEPTH_TEST);
        }

        // Unbind framebuffer for overlay
        mupOverlayFramebuffer->unbind();
        if(mFrameLogging) { Logger::instance().print("..done"); }

        // ### MOLECULE RENDERING ##################################################################################
        if(mFrameLogging) { Logger::instance().print("Render molecule.."); }

        // # Fill molecule framebuffer
        mupMoleculeFramebuffer->bind();
        bool wasResized = mupMoleculeFramebuffer->resize(mWindowWidth, mWindowHeight, mSuperSampling);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // When molecule framebuffer has been resized, do so for group rendering texture, too
        if(wasResized)
        {
            glBindTexture(GL_TEXTURE_2D, mGroupRenderingTexture);
            std::vector<GLfloat> emptyData(mupMoleculeFramebuffer->getWidth() * mupMoleculeFramebuffer->getHeight() * 4, 0);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mupMoleculeFramebuffer->getWidth(), mupMoleculeFramebuffer->getHeight(), 0, GL_RGBA, GL_FLOAT, &emptyData[0]);
            glBindTexture(GL_TEXTURE_2D, 0);
            std::vector<GLuint> emptyDataSemaphore(mupMoleculeFramebuffer->getWidth() * mupMoleculeFramebuffer->getHeight(), 0.f);
            mupGroupRenderingSemaphore = std::unique_ptr<GPUTextureBuffer>(new GPUTextureBuffer(emptyDataSemaphore));
        }
        else
        {
            // Clear group rendering texture
            glClearTexImage(mGroupRenderingTexture, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        }

        // Bind buffers of radii and trajectory for rendering molecule
        mupGPUProtein->bind(0, 1);

        // Bind group indicator
        mupGroupIndicators->bind(2);

        // Bind image for group rendering
        glBindImageTexture(
            3,
            mGroupRenderingTexture,
            0,
            GL_TRUE,
            0,
            GL_READ_WRITE,
            GL_RGBA32F);

        // Bind semaphore image for group rendering
        mupGroupRenderingSemaphore->bindAsImage(4, GPUAccess::READ_WRITE);

        // Selected atom (is -1 if selection shall be hidden)
        int selectedAtom = mRenderSelection ? mSelectedAtom : -1;

        // Decide about surface rendering
        if(frameComputed())
        {
            // Bind ascension
            mupAscension->bind(5);

            // Frame is computed, decide how to render it
            switch(mRendering)
            {
            case Rendering::HULL:

                // Prepare shader program
                hullProgram.use();
                hullProgram.update("time", mAccTime);
                hullProgram.update("view", mupCamera->getViewMatrix());
                hullProgram.update("projection", mupCamera->getProjectionMatrix());
                hullProgram.update("cameraWorldPos", mupCamera->getPosition());
                hullProgram.update("probeRadius", mRenderWithProbeRadius ? mComputedProbeRadius : 0.f);
                hullProgram.update("lightDir", mLightDirection);
                hullProgram.update("selectedIndex", selectedAtom);
                hullProgram.update("clippingPlane", mClippingPlane);
                hullProgram.update("frame", mFrame);
                hullProgram.update("atomCount", mupGPUProtein->getAtomCount());
                hullProgram.update("smoothAnimationRadius", mSmoothAnimationRadius);
                hullProgram.update("smoothAnimationMaxDeviation", mSmoothAnimationMaxDeviation);
                hullProgram.update("frameCount", mupGPUProtein->getFrameCount());
                hullProgram.update("depthDarkeningStart", mDepthDarkeningStart);
                hullProgram.update("depthDarkeningEnd", mDepthDarkeningEnd);
                hullProgram.update("selectionColor", mSelectionColor);
                hullProgram.update("ascensionFrame", mFrame - mComputedStartFrame);
                hullProgram.update("ascensionChangeRadiusMultiplier", mAscensionChangeRadiusMultiplier);
                hullProgram.update("highlightMultiplier", mRenderOutline ? 1.f : 0.f);
                hullProgram.update("highlightColor", mOutlineColor);
                hullProgram.update("framebufferWidth", mupMoleculeFramebuffer->getWidth());

                // Draw internal (first, because at clipping plane are all set to same
                // viewport depth which means internal are always in front of surface)
                if(mShowInternal)
                {
                    mGPUSurfaces.at(mFrame - mComputedStartFrame)->bindInternalIndices(mLayer, 6);
                    hullProgram.update("color", mInternalAtomColor);
                    glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfInternalAtoms(mLayer));
                }

                // Draw surface
                if(mShowSurface)
                {
                    mGPUSurfaces.at(mFrame - mComputedStartFrame)->bindSurfaceIndices(mLayer, 6);
                    hullProgram.update("color", mSurfaceAtomColor);
                    glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfSurfaceAtoms(mLayer));
                }

                break;

            case Rendering::ASCENSION:

                // Prepare shader program
                ascensionProgram.use();
                ascensionProgram.update("time", mAccTime);
                ascensionProgram.update("view", mupCamera->getViewMatrix());
                ascensionProgram.update("projection", mupCamera->getProjectionMatrix());
                ascensionProgram.update("cameraWorldPos", mupCamera->getPosition());
                ascensionProgram.update("probeRadius", mRenderWithProbeRadius ? mComputedProbeRadius : 0.f);
                ascensionProgram.update("lightDir", mLightDirection);
                ascensionProgram.update("selectedIndex", selectedAtom);
                ascensionProgram.update("clippingPlane", mClippingPlane);
                ascensionProgram.update("frame", mFrame);
                ascensionProgram.update("atomCount", mupGPUProtein->getAtomCount());
                ascensionProgram.update("smoothAnimationRadius", mSmoothAnimationRadius);
                ascensionProgram.update("smoothAnimationMaxDeviation", mSmoothAnimationMaxDeviation);
                ascensionProgram.update("frameCount", mupGPUProtein->getFrameCount());
                ascensionProgram.update("depthDarkeningStart", mDepthDarkeningStart);
                ascensionProgram.update("depthDarkeningEnd", mDepthDarkeningEnd);
                ascensionProgram.update("ascensionFrame", mFrame - mComputedStartFrame);
                ascensionProgram.update("ascensionColorOffsetAngle", mAscensionColorOffsetAngle);
                ascensionProgram.update("selectionColor", mSelectionColor);
                ascensionProgram.update("ascensionChangeRadiusMultiplier", mAscensionChangeRadiusMultiplier);
                ascensionProgram.update("highlightMultiplier", mRenderOutline ? 1.f : 0.f);
                ascensionProgram.update("highlightColor", mOutlineColor);
                ascensionProgram.update("framebufferWidth", mupMoleculeFramebuffer->getWidth());
                glDrawArrays(GL_POINTS, 0, mupGPUProtein->getAtomCount());

                break;

            case Rendering::ELEMENTS:

                // Bind coloring
                mupGPUProtein->bindColorsElement(6);

                // Prepare shader program
                coloringProgram.use();
                coloringProgram.update("time", mAccTime);
                coloringProgram.update("view", mupCamera->getViewMatrix());
                coloringProgram.update("projection", mupCamera->getProjectionMatrix());
                coloringProgram.update("cameraWorldPos", mupCamera->getPosition());
                coloringProgram.update("probeRadius", mRenderWithProbeRadius ? mComputedProbeRadius : 0.f);
                coloringProgram.update("lightDir", mLightDirection);
                coloringProgram.update("selectedIndex", selectedAtom);
                coloringProgram.update("clippingPlane", mClippingPlane);
                coloringProgram.update("frame", mFrame);
                coloringProgram.update("atomCount", mupGPUProtein->getAtomCount());
                coloringProgram.update("smoothAnimationRadius", mSmoothAnimationRadius);
                coloringProgram.update("smoothAnimationMaxDeviation", mSmoothAnimationMaxDeviation);
                coloringProgram.update("frameCount", mupGPUProtein->getFrameCount());
                coloringProgram.update("depthDarkeningStart", mDepthDarkeningStart);
                coloringProgram.update("depthDarkeningEnd", mDepthDarkeningEnd);
                coloringProgram.update("selectionColor", mSelectionColor);
                coloringProgram.update("ascensionFrame", mFrame - mComputedStartFrame);
                coloringProgram.update("ascensionChangeRadiusMultiplier", mAscensionChangeRadiusMultiplier);
                coloringProgram.update("highlightMultiplier", mRenderOutline ? 1.f : 0.f);
                coloringProgram.update("highlightColor", mOutlineColor);
                coloringProgram.update("framebufferWidth", mupMoleculeFramebuffer->getWidth());

                // Draw internal (first, because at clipping plane are all set to same
                // viewport depth which means internal are always in front of surface)
                if(mShowInternal)
                {
                    mGPUSurfaces.at(mFrame - mComputedStartFrame)->bindInternalIndices(mLayer, 7);
                    glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfInternalAtoms(mLayer));
                }

                // Draw surface
                if(mShowSurface)
                {
                    mGPUSurfaces.at(mFrame - mComputedStartFrame)->bindSurfaceIndices(mLayer, 7);
                    glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfSurfaceAtoms(mLayer));
                }

                break;

            case Rendering::AMINOACIDS:

                // Bind coloring
                mupGPUProtein->bindColorsAminoacid(6);

                // Prepare shader program
                coloringProgram.use();
                coloringProgram.update("time", mAccTime);
                coloringProgram.update("view", mupCamera->getViewMatrix());
                coloringProgram.update("projection", mupCamera->getProjectionMatrix());
                coloringProgram.update("cameraWorldPos", mupCamera->getPosition());
                coloringProgram.update("probeRadius", mRenderWithProbeRadius ? mComputedProbeRadius : 0.f);
                coloringProgram.update("lightDir", mLightDirection);
                coloringProgram.update("selectedIndex", selectedAtom);
                coloringProgram.update("clippingPlane", mClippingPlane);
                coloringProgram.update("frame", mFrame);
                coloringProgram.update("atomCount", mupGPUProtein->getAtomCount());
                coloringProgram.update("smoothAnimationRadius", mSmoothAnimationRadius);
                coloringProgram.update("smoothAnimationMaxDeviation", mSmoothAnimationMaxDeviation);
                coloringProgram.update("frameCount", mupGPUProtein->getFrameCount());
                coloringProgram.update("depthDarkeningStart", mDepthDarkeningStart);
                coloringProgram.update("depthDarkeningEnd", mDepthDarkeningEnd);
                coloringProgram.update("selectionColor", mSelectionColor);
                coloringProgram.update("ascensionFrame", mFrame - mComputedStartFrame);
                coloringProgram.update("ascensionChangeRadiusMultiplier", mAscensionChangeRadiusMultiplier);
                coloringProgram.update("highlightMultiplier", mRenderOutline ? 1.f : 0.f);
                coloringProgram.update("highlightColor", mOutlineColor);
                coloringProgram.update("framebufferWidth", mupMoleculeFramebuffer->getWidth());

                // Draw internal (first, because at clipping plane are all set to same
                // viewport depth which means internal are always in front of surface)
                if(mShowInternal)
                {
                    mGPUSurfaces.at(mFrame - mComputedStartFrame)->bindInternalIndices(mLayer, 7);
                    glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfInternalAtoms(mLayer));
                }

                // Draw surface
                if(mShowSurface)
                {
                    mGPUSurfaces.at(mFrame - mComputedStartFrame)->bindSurfaceIndices(mLayer, 7);
                    glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfSurfaceAtoms(mLayer));
                }

                break;

            case Rendering::ANALYSIS:

                // Bind indices of analysis atoms
                mupOutlineAtomIndices->bindAsImage(6, GPUAccess::READ_ONLY);

                // Prepare shader program
                analysisProgram.use();
                analysisProgram.update("time", mAccTime);
                analysisProgram.update("view", mupCamera->getViewMatrix());
                analysisProgram.update("projection", mupCamera->getProjectionMatrix());
                analysisProgram.update("cameraWorldPos", mupCamera->getPosition());
                analysisProgram.update("probeRadius", mRenderWithProbeRadius ? mComputedProbeRadius : 0.f);
                analysisProgram.update("lightDir", mLightDirection);
                analysisProgram.update("selectedIndex", selectedAtom);
                analysisProgram.update("clippingPlane", mClippingPlane);
                analysisProgram.update("frame", mFrame);
                analysisProgram.update("atomCount", mupGPUProtein->getAtomCount());
                analysisProgram.update("smoothAnimationRadius", mSmoothAnimationRadius);
                analysisProgram.update("smoothAnimationMaxDeviation", mSmoothAnimationMaxDeviation);
                analysisProgram.update("frameCount", mupGPUProtein->getFrameCount());
                analysisProgram.update("depthDarkeningStart", mDepthDarkeningStart);
                analysisProgram.update("depthDarkeningEnd", mDepthDarkeningEnd);
                analysisProgram.update("groupAtomCount", (int)mupOutlineAtomIndices->getSize());
                analysisProgram.update("selectionColor", mSelectionColor);
                analysisProgram.update("ascensionFrame", mFrame - mComputedStartFrame);
                analysisProgram.update("ascensionChangeRadiusMultiplier", mAscensionChangeRadiusMultiplier);
                analysisProgram.update("highlightMultiplier", mRenderOutline ? 1.f : 0.f);
                analysisProgram.update("highlightColor", mOutlineColor);
                analysisProgram.update("framebufferWidth", mupMoleculeFramebuffer->getWidth());
                glDrawArrays(GL_POINTS, 0, mupGPUProtein->getAtomCount());

                break;

            case Rendering::LAYERS:

                // Only proceed when layers were extracted
                if(mGPUSurfaces.at(mFrame - mComputedStartFrame)->layersExtracted())
                {
                    // Reuse hull proram for that purpose
                    hullProgram.use();
                    hullProgram.update("time", mAccTime);
                    hullProgram.update("view", mupCamera->getViewMatrix());
                    hullProgram.update("projection", mupCamera->getProjectionMatrix());
                    hullProgram.update("cameraWorldPos", mupCamera->getPosition());
                    hullProgram.update("probeRadius", mRenderWithProbeRadius ? mComputedProbeRadius : 0.f);
                    hullProgram.update("lightDir", mLightDirection);
                    hullProgram.update("selectedIndex", selectedAtom);
                    hullProgram.update("clippingPlane", mClippingPlane);
                    hullProgram.update("frame", mFrame);
                    hullProgram.update("atomCount", mupGPUProtein->getAtomCount());
                    hullProgram.update("smoothAnimationRadius", mSmoothAnimationRadius);
                    hullProgram.update("smoothAnimationMaxDeviation", mSmoothAnimationMaxDeviation);
                    hullProgram.update("frameCount", mupGPUProtein->getFrameCount());
                    hullProgram.update("depthDarkeningStart", mDepthDarkeningStart);
                    hullProgram.update("depthDarkeningEnd", mDepthDarkeningEnd);
                    hullProgram.update("selectionColor", mSelectionColor);
                    hullProgram.update("ascensionFrame", mFrame - mComputedStartFrame);
                    hullProgram.update("ascensionChangeRadiusMultiplier", mAscensionChangeRadiusMultiplier);
                    hullProgram.update("highlightMultiplier", mRenderOutline ? 1.f : 0.f);
                    hullProgram.update("highlightColor", mOutlineColor);
                    hullProgram.update("framebufferWidth", mupMoleculeFramebuffer->getWidth());

                    // Draw inner to outer layers in given colors
                    int layerCount = mGPUSurfaces.at(mFrame - mComputedStartFrame)->getLayerCount();
                    for(int i = layerCount - 1; i >= 0; i--)
                    {
                        mGPUSurfaces.at(mFrame - mComputedStartFrame)->bindSurfaceIndices(i, 6);

                        glm::vec3 layerColor = glm::vec3(glm::pow3(float(i)/(layerCount - 1)), float(i)/(layerCount - 1), 160.0f/255.0f);
                        hullProgram.update("color", layerColor);

                        // hullProgram.update("color", mLayerColors.at(i % (int)mLayerColors.size()));

                        glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfSurfaceAtoms(i));
                    }
                }
                break;

            case Rendering::RESIDUE_SURFACE_PROXIMITY:

                // Only proceed when layers were extracted
                if(mGPUSurfaces.at(mFrame - mComputedStartFrame)->layersExtracted())
                {
                    // Render into k-Buffer

                    // Render into molecule framebuffer
                }
                break;
            }
        }
        else
        {
            // Render it with fallback shader
            fallbackProgram.use();
            fallbackProgram.update("time", mAccTime);
            fallbackProgram.update("view", mupCamera->getViewMatrix());
            fallbackProgram.update("projection", mupCamera->getProjectionMatrix());
            fallbackProgram.update("cameraWorldPos", mupCamera->getPosition());
            fallbackProgram.update("probeRadius", mRenderWithProbeRadius ? mComputedProbeRadius : 0.f);
            fallbackProgram.update("lightDir", mLightDirection);
            fallbackProgram.update("selectedIndex", selectedAtom);
            fallbackProgram.update("clippingPlane", mClippingPlane);
            fallbackProgram.update("frame", mFrame);
            fallbackProgram.update("atomCount", mupGPUProtein->getAtomCount());
            fallbackProgram.update("smoothAnimationRadius", mSmoothAnimationRadius);
            fallbackProgram.update("smoothAnimationMaxDeviation", mSmoothAnimationMaxDeviation);
            fallbackProgram.update("frameCount", mupGPUProtein->getFrameCount());
            fallbackProgram.update("depthDarkeningStart", mDepthDarkeningStart);
            fallbackProgram.update("depthDarkeningEnd", mDepthDarkeningEnd);
            fallbackProgram.update("color", mFallbackAtomColor);
            fallbackProgram.update("selectionColor", mSelectionColor);
            fallbackProgram.update("highlightMultiplier", mRenderOutline ? 1.f : 0.f);
            fallbackProgram.update("highlightColor", mOutlineColor);
            fallbackProgram.update("framebufferWidth", mupMoleculeFramebuffer->getWidth());
            glDrawArrays(GL_POINTS, 0, mupGPUProtein->getAtomCount());
        }

        // Unbind molecule framebuffer
        mupMoleculeFramebuffer->unbind();
        if(mFrameLogging) { Logger::instance().print("..done"); }

        // ### SELCTED ATOM RENDERING ##############################################################################
        if(mFrameLogging) { Logger::instance().print("Render selected atom.."); }

        // # Fill selected atom framebuffer
        mupSelectedAtomFramebuffer->bind();
        mupSelectedAtomFramebuffer->resize(mWindowWidth, mWindowHeight, mSuperSampling);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Only render it when enabled
        if(mRenderSelection)
        {
            // Bind buffers of radii and trajectory for rendering molecule
            mupGPUProtein->bind(0, 1);

            // Bind group indicator
            mupGroupIndicators->bind(2);

            // Bind image for group rendering
            glBindImageTexture(
                3,
                mGroupRenderingTexture,
                0,
                GL_TRUE,
                0,
                GL_READ_WRITE,
                GL_RGBA32F);

            // Bind semaphore image for group rendering
            mupGroupRenderingSemaphore->bindAsImage(4, GPUAccess::READ_WRITE);

            // Prepare shader program
            selectionProgram.use();
            selectionProgram.update("time", mAccTime);
            selectionProgram.update("view", mupCamera->getViewMatrix());
            selectionProgram.update("projection", mupCamera->getProjectionMatrix());
            selectionProgram.update("cameraWorldPos", mupCamera->getPosition());
            selectionProgram.update("probeRadius", mRenderWithProbeRadius ? mComputedProbeRadius : 0.f);
            selectionProgram.update("lightDir", mLightDirection);
            selectionProgram.update("clippingPlane", mClippingPlane);
            selectionProgram.update("frame", mFrame);
            selectionProgram.update("atomCount", mupGPUProtein->getAtomCount());
            selectionProgram.update("smoothAnimationRadius", mSmoothAnimationRadius);
            selectionProgram.update("smoothAnimationMaxDeviation", mSmoothAnimationMaxDeviation);
            selectionProgram.update("frameCount", mupGPUProtein->getFrameCount());
            selectionProgram.update("depthDarkeningStart", mDepthDarkeningStart);
            selectionProgram.update("depthDarkeningEnd", mDepthDarkeningEnd);
            selectionProgram.update("color", mSelectionColor);
            selectionProgram.update("atomIndex", mSelectedAtom);
            selectionProgram.update("highlightMultiplier", 0.f);
            selectionProgram.update("highlightColor", mOutlineColor);
            selectionProgram.update("framebufferWidth", mupMoleculeFramebuffer->getWidth());
            glDrawArrays(GL_POINTS, 0, 1);
        }

        // Unbind selected atom framebuffer
        mupSelectedAtomFramebuffer->unbind();
        if(mFrameLogging) { Logger::instance().print("..done"); }

        // ### COMPOSITING #########################################################################################
        if(mFrameLogging) { Logger::instance().print("Do compositing.."); }

        // Prepare viewport
        glViewport(0, 0, mWindowWidth, mWindowHeight);

        // # Fill standard framebuffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Disable depth test
        glDisable(GL_DEPTH_TEST);

        // Render cubemap
        glDisable(GL_DEPTH_TEST);
        glActiveTexture(GL_TEXTURE0);

        switch(mBackground)
        {
        case Background::NONE:
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
            break;
        case Background::SCIENTIFIC:
            glBindTexture(GL_TEXTURE_CUBE_MAP, mScientificCubemapTexture);
            break;
        case Background::COMPUTERVISUALISTIK:
            glBindTexture(GL_TEXTURE_CUBE_MAP, mCVCubemapTexture);
            break;
        case Background::BEACH:
            glBindTexture(GL_TEXTURE_CUBE_MAP, mBeachCubemapTexture);
            break;
        case Background::WHITE:
            glBindTexture(GL_TEXTURE_CUBE_MAP, mWhiteCubemapTexture);
            break;
        }

        cubemapProgram.use();
        cubemapProgram.update("cubemap", 0); // tell shader which slot to use
        cubemapProgram.update("view", mupCamera->getViewMatrix());
        cubemapProgram.update("projection", mupCamera->getProjectionMatrix());
        glDrawArrays(GL_POINTS, 0, 1);
        glEnable(GL_DEPTH_TEST);

        // Transparent rendering to render framebuffer on top of standard buffer
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Bind molecule framebuffer texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mupMoleculeFramebuffer->getAttachment(0));

        // Bind molecule framebuffer texture
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mupSelectedAtomFramebuffer->getAttachment(0));

        // Bind overlay framebuffer texture
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, mupOverlayFramebuffer->getAttachment(0));

        // Bind group rendering texture
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, mGroupRenderingTexture);

        // Draw screenfilling quad
        screenFillingProgram.use();
        screenFillingProgram.update("molecule", 0); // tell shader which slot to use
        screenFillingProgram.update("selectedAtom", 1); // tell shader which slot to use
        screenFillingProgram.update("overlay", 2); // tell shader which slot to use
        screenFillingProgram.update("groupRendering", 3); // tell shader which slot to use
        screenFillingProgram.update("moleculeAlpha", mNoneGroupOpacity); // alpha value of completely rendered molecule
        screenFillingProgram.update("groupAlpha", mRenderGroupOnTop ? 1.f : 0.f); // alpha value for group which rendered on top
        glDrawArrays(GL_POINTS, 0, 1);
        if(mFrameLogging) { Logger::instance().print("..done"); }

        // Back to opaque rendering
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);

        // Enable depth test
        glEnable(GL_DEPTH_TEST);

        // Render GUI in standard frame buffer on top of everything
        if(mFrameLogging) { Logger::instance().print("Render user interface.."); }
        ImGui_ImplGlfwGL3_NewFrame();
        renderGUI();
        if(mFrameLogging) { Logger::instance().print("..done"); }
    });

    Logger::instance().print("..exit");

    // Delete OpenGL structures
    glDeleteVertexArrays(1, &axisGizmoVAO);
    glDeleteBuffers(1, &axisGizmoVBO);
}

void SurfaceDynamicsVisualization::setProgressDisplay(std::string task, float progress)
{
    if(progress != 1.f)
    {
        int intProgress = (int)(progress * 100);
        std::string stringProgress = intProgress < 10 ? "0" + std::to_string(intProgress) : std::to_string(intProgress);
        glfwSetWindowTitle(mpWindow, std::string(mWindowTitle + ": " + task + " [" + stringProgress + "%]").c_str());
    }
    else
    {
        glfwSetWindowTitle(mpWindow, mWindowTitle.c_str());
    }
}

void SurfaceDynamicsVisualization::keyCallback(int key, int scancode, int action, int mods)
{
    if(action == GLFW_PRESS)
    {
        switch(key)
        {
            case GLFW_KEY_ESCAPE:
            {
                // Try to escape from camera movement or rotation
                if(mRotateCamera)
                {
                    glfwSetInputMode(mpWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    mRotateCamera = false;
                }
                else if(mMoveCamera)
                {
                    glfwSetInputMode(mpWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    mMoveCamera = false;
                }
                break;
            }
            case GLFW_KEY_1: { mRendering = Rendering::HULL; break; }
            case GLFW_KEY_2: { mRendering = Rendering::ASCENSION; break; }
            case GLFW_KEY_3: { mRendering = Rendering::ELEMENTS; break; }
            case GLFW_KEY_4: { mRendering = Rendering::AMINOACIDS; break; }
            case GLFW_KEY_5: { mRendering = Rendering::ANALYSIS; break; }
            case GLFW_KEY_6: { mRendering = Rendering::LAYERS; break; }
            case GLFW_KEY_7: { mRendering = Rendering::RESIDUE_SURFACE_PROXIMITY; break; }
        }
    }
}

void SurfaceDynamicsVisualization::mouseButtonCallback(int button, int action, int mods)
{
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        glfwSetInputMode(mpWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        mRotateCamera = true;
    }
    else if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        glfwSetInputMode(mpWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        mRotateCamera = false;
    }
    if(button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        glfwSetInputMode(mpWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        mMoveCamera = true;
    }
    else if(button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        glfwSetInputMode(mpWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        mMoveCamera = false;
    }
    else if(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        int atomIndex = getAtomBeneathCursor();
        mSelectedAtom = atomIndex >= 0 ? atomIndex : mSelectedAtom;
        mNextAnalyseAtomIndex = mSelectedAtom;
    }
}

void SurfaceDynamicsVisualization::scrollCallback(double xoffset, double yoffset)
{
    mupCamera->setRadius(mupCamera->getRadius() - (float)yoffset);
}

void SurfaceDynamicsVisualization::renderGUI()
{
    // ###################################################################################################
    // ### MAIN MENU BAR #######################################################################################
    // ###################################################################################################
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f)); // window background
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.2f, 0.2f, 0.2f, 0.25f)); // menu bar background

    // Main menu bar
    if(mFrameLogging) { Logger::instance().print("Main menu bar.."); }
    if (ImGui::BeginMainMenuBar())
    {
        // General menu
        if (ImGui::BeginMenu("Menu"))
        {
            if(ImGui::MenuItem("Quit", "", false, true)) { glfwSetWindowShouldClose(mpWindow, GL_TRUE); }
            ImGui::EndMenu();
        }

        // Window menu
        if (ImGui::BeginMenu("Window"))
        {
            // Computation window
            if(mShowComputationWindow)
            {
                if(ImGui::MenuItem("Hide Computation", "", false, true)) { mShowComputationWindow = false; }
            }
            else
            {
                if(ImGui::MenuItem("Show Computation", "", false, true)) { mShowComputationWindow = true; }
            }

            // Camera window
            if(mShowCameraWindow)
            {
                if(ImGui::MenuItem("Hide Camera", "", false, true)) { mShowCameraWindow = false; }
            }
            else
            {
                if(ImGui::MenuItem("Show Camera", "", false, true)) { mShowCameraWindow = true; }
            }

            // Visualization window
            if(mShowVisualizationWindow)
            {
                if(ImGui::MenuItem("Hide Visualization", "", false, true)) { mShowVisualizationWindow = false; }
            }
            else
            {
                if(ImGui::MenuItem("Show Visualization", "", false, true)) { mShowVisualizationWindow = true; }
            }

            // Information window
            if(mShowInformationWindow)
            {
                if(ImGui::MenuItem("Hide Information", "", false, true)) { mShowInformationWindow = false; }
            }
            else
            {
                if(ImGui::MenuItem("Show Information", "", false, true)) { mShowInformationWindow = true; }
            }

            // Validation window
            if(mShowValidationWindow)
            {
                if(ImGui::MenuItem("Hide Validation", "", false, true)) { mShowValidationWindow = false; }
            }
            else
            {
                if(ImGui::MenuItem("Show Validation", "", false, true)) { mShowValidationWindow = true; }
            }

            // Analysis window
            if(mShowAnalysisWindow)
            {
                if(ImGui::MenuItem("Hide Analysis", "", false, true)) { mShowAnalysisWindow = false; }
            }
            else
            {
                if(ImGui::MenuItem("Show Analysis", "", false, true)) { mShowAnalysisWindow = true; }
            }

            // Rendering window
            if(mShowRenderingWindow)
            {
                if(ImGui::MenuItem("Hide Rendering", "", false, true)) { mShowRenderingWindow = false; }
            }
            else
            {
                if(ImGui::MenuItem("Show Rendering", "", false, true)) { mShowRenderingWindow = true; }
            }

            ImGui::EndMenu();
        }

        // Help menu
        if (ImGui::BeginMenu("Help"))
        {
            ImGui::BeginPopup("Help");
            ImGui::Text("LMB: Rotate camera");
            ImGui::Text("MMB: Move camera");
            ImGui::Text("RMB: Select atom");
            if(mShowTooltips)
            {
                if(ImGui::MenuItem("Hide Tooltips")) { mShowTooltips = false; }
            }
            else
            {
                if(ImGui::MenuItem("Show Tooltips")) { mShowTooltips = true; }
            }
            ImGui::EndPopup();
        }

        // About menu
        if (ImGui::BeginMenu("About"))
        {
            ImGui::BeginPopup("About");
            ImGui::Text("Developer: Raphael Menges");
            ImGui::EndPopup();
        }

        // Frametime
        float framerate = ImGui::GetIO().Framerate;
        std::stringstream stream;
        stream << std::fixed << std::setprecision(0) << framerate;
        std::string fps = "FPS: " + stream.str();
        ImGui::MenuItem(fps.c_str(), "", false, false);

        // End main menu bar
        ImGui::EndMainMenuBar();
    }
    if(mFrameLogging) { Logger::instance().print("..done"); }

    // ###################################################################################################
    // ### WINDOWS #######################################################################################
    // ###################################################################################################
    ImGui::PopStyleColor(); // menu bar background
    ImGui::PopStyleColor(); // window background
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.5f, 0.5f, 0.5f, 0.5f)); // window title
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.5f, 0.5f, 0.5f, 0.5f)); // window title collapsed
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.1f, 0.1f, 0.1f, 0.75f)); // window title active
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.0f, 0.0f, 0.0f, 0.5f)); // scrollbar grab
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // scrollbar background
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.5f)); // button
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.75f)); // button hovered
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.2f, 0.2f, 0.75f)); // header
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.75f)); // header hovered
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.1f, 0.1f, 0.1f, 0.75f)); // header active
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.1f, 0.1f, 0.1f, 0.75f)); // slider grab active

    // ### COMPUTATION ###################################################################################
    if(mFrameLogging) { Logger::instance().print("Computation window.."); }
    if(mShowComputationWindow)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.5f, 0.0f, 0.0f, 0.75f)); // window background
        ImGui::Begin("Computation", NULL, 0);

        // ### Surface ###
        if (ImGui::CollapsingHeader("Surface", "Surface##Computation", true, true))
        {
            // Surface extraction
            ImGui::SliderFloat("Probe Radius", &mComputationProbeRadius, 0.f, 3.f, "%.1f");
            if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Radius of probe used for surface extraction."); }
            ImGui::Checkbox("Extract Layers", &mExtractLayers);
            if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Incremental usage of surface extraction."); }

            if(ImGui::Button("-1##startframe")) { mComputationStartFrame--; }
            ImGui::SameLine();
            if(ImGui::Button("+1##startframe")) { mComputationStartFrame++; }
            mComputationStartFrame = glm::clamp(mComputationStartFrame, mStartFrame, mComputationEndFrame);
            ImGui::SameLine();
            ImGui::SliderInt("Start Frame", &mComputationStartFrame, mStartFrame, mComputationEndFrame);
            if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Start frame of computation."); }

            if(ImGui::Button("-1##endframe")) { mComputationEndFrame--; }
            ImGui::SameLine();
            if(ImGui::Button("+1##endframe")) { mComputationEndFrame++; }
            mComputationEndFrame = glm::clamp(mComputationEndFrame, mComputationStartFrame, mEndFrame);
            ImGui::SameLine();
            ImGui::SliderInt("End Frame", &mComputationEndFrame, mComputationStartFrame, mEndFrame);
            if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("End frame of computation."); }

            ImGui::SliderInt("CPU Threads", &mCPUThreads, 1, 24);
            if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Count of threads utilized by CPU implementation."); }
            if(ImGui::Button("\u2794 GPGPU##surface")) { computeLayers(true); }
            if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Compute surface with OpenGL implementation, plus hull samples and ascension."); }
            ImGui::SameLine();
            if(ImGui::Button("\u2794 CPU##surface")) { computeLayers(false); }
            if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Compute surface with C++ implementation, plus hull samples and ascension."); }

            // Report
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.5f, 0.5f, 0.5f, 0.5f)); // header
            if (ImGui::CollapsingHeader("Report", "Report##Computation", true, false))
            {
                ImGui::Text(mComputeInformation.c_str());
            }
            ImGui::PopStyleColor(); // header
        }

        // ### Hull Samples ###
        if (ImGui::CollapsingHeader("Hull Samples", "Hull Samples##Computation", true, true))
        {
            ImGui::SliderInt("Atom Sample Count", &mHullSampleCount, 0, 1000);
            if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Count of samples per atom used for analysis purposes, not surface extraction."); }

            // Recomputation of hull samples only when there are frames with surface extracted
            if(mComputedStartFrame >= 0)
            {
                if(ImGui::Button("\u2794 GPGPU##hullsamples")) { computeHullSamples(); }
                if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Compute hull samples."); }
            }
        }

        // ### Ascension ###
        if (ImGui::CollapsingHeader("Ascension", "Ascension##Computation", true, true))
        {
            ImGui::SliderFloat("Hot Up", &mAscensionUpToHotFrameCount, 1.f, 1000.f, "%.0f");
            if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Frame count until surface atom gets up from cold to hot."); }
            ImGui::SliderFloat("Hot Down", &mAscensionDownToHotFrameCount, 1.f, 1000.f, "%.0f");
            if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Frame count until surface atom falls down to hot while dropping."); }
            ImGui::SliderFloat("Cool Up", &mAscensionUpToColdFrameCount, 1.f, 1000.f, "%.0f");
            if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Frame count until internal atom gets up from hot to cold."); }
            ImGui::SliderFloat("Cool Down", &mAscensionDownToColdFrameCount, 1.f, 1000.f, "%.0f");
            if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Frame count until internal atom falls down to cold while rising."); }

            // Recomputation of ascension only when there are frames with surface extracted
            if(mComputedStartFrame >= 0)
            {
                if(ImGui::Button("\u2794 CPU##ascension")) { computeAscension(); }
                if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Compute ascension."); }
            }
        }

        ImGui::End();
        ImGui::PopStyleColor(); // window background
    }
    if(mFrameLogging) { Logger::instance().print("..done"); }

    // ### CAMERA ########################################################################################
    if(mFrameLogging) { Logger::instance().print("Camera window.."); }
    if(mShowCameraWindow)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.5f, 0.75f)); // window background
        ImGui::Begin("Camera", NULL, 0);

        // ### Camera ###
        if (ImGui::CollapsingHeader("Orthographic Camera", "Orthographic Camera##Camera", true, true))
        {
            // Set to fixed positions
            if(ImGui::Button("Align to X"))
            {
                mupCamera->setAlpha(0);
                mupCamera->setBeta(90);
            }
            ImGui::SameLine();
            if(ImGui::Button("Align to Y"))
            {
                // Internal, this is not possible. There is some epsilon on beta inside the camera object
                mupCamera->setAlpha(0);
                mupCamera->setBeta(0);
            }
            ImGui::SameLine();
            if(ImGui::Button("Align to Z"))
            {
                mupCamera->setAlpha(90.f);
                mupCamera->setBeta(90.f);
            }

            // Rotation
            float alpha = mupCamera->getAlpha();
            ImGui::DragFloat("Horizontal", &alpha);
            mupCamera->setAlpha(alpha);

            float beta = mupCamera->getBeta();
            ImGui::DragFloat("Vertical", &beta);
            mupCamera->setBeta(beta);

            // Movement
            glm::vec3 center = mupCamera->getCenter();
            ImGui::DragFloat3("Position", glm::value_ptr(center));
            mupCamera->setCenter(center);

            // Put center of camera in protein's center
            if(ImGui::Button("Center", ImVec2(100, 22)))
            {
                // Applied for next frame
                mupCamera->setCenter(mupGPUProtein->getCenterOfMass(mFrame));
            }
            if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Sets center of camera to center of molecule."); }
            ImGui::SameLine();

            // Reset
            if(ImGui::Button("Reset Camera", ImVec2(100, 22)))
            {
                mupCamera->setAlpha(mCameraDefaultAlpha);
                mupCamera->setBeta(mCameraDefaultBeta);
                mupCamera->setCenter(glm::vec3(0));
            }
            if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Reset angle and center of camera."); }

            // Put center of camera in center of analysis group
            if(frameComputed() && !mAnalyseGroup.empty())
            {
                if(ImGui::Button("Center Analysis Group", ImVec2(208, 22)))
                {
                    // Average centers of atoms in analysis group
                    glm::vec3 avgCenter(0,0,0);
                    for(GLuint atomIndex : mAnalyseGroup)
                    {
                        avgCenter += mupGPUProtein->getTrajectory()->at(mFrame).at(atomIndex);
                    }

                    // Applied for next frame
                    mupCamera->setCenter(avgCenter / (float)(mAnalyseGroup.size()));
                }
                if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Sets center of camera to center of current analysis group."); }
            }
        }

        // ### Clipping ###
        if (ImGui::CollapsingHeader("Clipping Plane", "Clipping Plane##Camera", true, true))
        {
            if(ImGui::Button("+0.1"))
            {
                mClippingPlane += 0.1f;
                mClippingPlane = glm::clamp(mClippingPlane, mClippingPlaneMin, mClippingPlaneMax);
            }
            ImGui::SameLine();
            if(ImGui::Button("-0.1"))
            {
                mClippingPlane -= 0.1f;
                mClippingPlane = glm::clamp(mClippingPlane, mClippingPlaneMin, mClippingPlaneMax);
            }
            ImGui::SameLine();
            ImGui::SliderFloat("", &mClippingPlane, mClippingPlaneMin, mClippingPlaneMax, "%.1f");
            if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Value of zero indicates no clipping plane usage."); }
        }

        ImGui::End();
        ImGui::PopStyleColor(); // window background
    }
    if(mFrameLogging) { Logger::instance().print("..done"); }

    // ### VISUALIZATION #################################################################################
    if(mFrameLogging) { Logger::instance().print("Visualization window.."); }
    if(mShowVisualizationWindow)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.5f, 0.0f, 0.5f, 0.75f)); // window background
        ImGui::Begin("Visualization", NULL, 0);

        // ### Animation ###
        if (ImGui::CollapsingHeader("Animation", "Animation##Visualization", true, true))
        {
            if(mPlayAnimation)
            {
                if(ImGui::Button("\u05f0 Pause", ImVec2(90, 22)))
                {
                    mPlayAnimation = false;
                }
            }
            else
            {
                if(ImGui::Button("\u25B8 Play", ImVec2(90, 22)))
                {
                    mPlayAnimation = true;
                }
            }
            ImGui::SameLine();

            // Animation settings
            ImGui::Checkbox("Repeat", &mRepeatAnimation);

            if(mRepeatAnimation)
            {
                ImGui::SameLine();
                ImGui::Checkbox("Computed", &mRepeatOnlyComputed);
            }

            ImGui::SliderInt("Rate", &mPlayAnimationRate, 0, 100);

            // Current frame controlled with slider
            int frame = mFrame;
            ImGui::SliderInt("Frame", &frame, mStartFrame, mEndFrame);

            // Current frame controlled with buttons
            if(ImGui::Button("-10##frame", ImVec2(40, 22))) { frame -= 10; } ImGui::SameLine();
            if(ImGui::Button("-1##frame", ImVec2(40, 22))) { frame -= 1; } ImGui::SameLine();
            if(ImGui::Button("+1##frame", ImVec2(40, 22))) { frame += 1; } ImGui::SameLine();
            if(ImGui::Button("+10##frame", ImVec2(40, 22))) { frame += 10; }

            // Set current frame
            if(frame != mFrame)
            {
                setFrame(frame);
            }
        }

        // Stuff that is only possible on computed frames
        if(frameComputed())
        {
            // ### Layer ###
            if(mRendering == Rendering::HULL
            || mRendering == Rendering::ELEMENTS
            || mRendering == Rendering::AMINOACIDS)
            {
                if (ImGui::CollapsingHeader("Layer", "Layer##Visualization", true, true))
                {
                    ImGui::SliderInt("Layer", &mLayer, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getLayerCount() - 1);
                }
            }

            // ### Rendering ###
            if (ImGui::CollapsingHeader("Rendering", "Rendering##Visualization", true, true))
            {
                // Surface rendering
                ImGui::Combo("##RenderingCombo", (int*)&mRendering, "[1] Hull\0[2] Ascension\0[3] Elements\0[4] Aminoacids\0[5] Analysis\0[6] Layers\0[7] Residue Surface Proximity");

                // Rendering of internal and surface atoms
                if(mRendering == Rendering::HULL
                || mRendering == Rendering::ELEMENTS
                || mRendering == Rendering::AMINOACIDS)
                {
                    // Show / hide internal atoms
                    if(mShowInternal)
                    {
                        if(ImGui::Button("Hide Internal", ImVec2(100, 22)))
                        {
                            mShowInternal = false;
                        }
                    }
                    else
                    {
                        if(ImGui::Button("Show Internal", ImVec2(100, 22)))
                        {
                            mShowInternal = true;
                        }
                    }
                    ImGui::SameLine();

                    // Show / hide surface atoms
                    if(mShowSurface)
                    {
                        if(ImGui::Button("Hide Surface", ImVec2(100, 22)))
                        {
                            mShowSurface = false;
                        }
                    }
                    else
                    {
                        if(ImGui::Button("Show Surface", ImVec2(100, 22)))
                        {
                            mShowSurface = true;
                        }
                    }
                }

                // Show / hide hull samples
                if(mRenderHullSamples)
                {
                    if(ImGui::Button("Hide Hull Samples", ImVec2(208, 22)))
                    {
                        mRenderHullSamples = false;
                    }
                }
                else
                {
                    if(ImGui::Button("Show Hull Samples", ImVec2(208, 22)))
                    {
                        mRenderHullSamples = true;
                    }
                }
                if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Samples which are used for analysis."); }

                // Show / hide surface atoms' mark
                if(mMarkSurfaceAtoms)
                {
                    if(ImGui::Button("Unmark Surface Atoms", ImVec2(208, 22)))
                    {
                        mMarkSurfaceAtoms = false;
                    }
                }
                else
                {
                    if(ImGui::Button("Mark Surface Atoms", ImVec2(208, 22)))
                    {
                        mMarkSurfaceAtoms = true;
                    }
                }

                // Render / not render with probe radius
                if(mRenderWithProbeRadius)
                {
                    if(ImGui::Button("No Probe Radius", ImVec2(208, 22)))
                    {
                        mRenderWithProbeRadius = false;
                    }
                }
                else
                {
                    if(ImGui::Button("Add Probe Radius", ImVec2(208, 22)))
                    {
                        mRenderWithProbeRadius = true;
                    }
                }
                if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Add probe radius used for calculation of surface to rendered radius."); }

                // Ascension change radius multiplier
                ImGui::SliderFloat("Ascension Radius Multiplier", &mAscensionChangeRadiusMultiplier, 0.f, 1.f);
                if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Change of as- and descension is multiplied with radius."); }

                // Opacity of none group atoms
                ImGui::SliderFloat("Molecule Opacity", &mNoneGroupOpacity, 0.f, 1.f);
                if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Opacity of molecule visualization."); }
            }
        }

        // ### Animation smoothing ###
        if (ImGui::CollapsingHeader("Animation Smoothing", "Animation Smoothing##Visualization", true, true))
        {
            ImGui::SliderInt("Smooth Radius", &mSmoothAnimationRadius, 0, 10);
            ImGui::SliderFloat("Smooth Max Deviation", &mSmoothAnimationMaxDeviation, 0, 100, "%.1f");
        }

        // ### Ascension Help Image ###
        if (ImGui::CollapsingHeader("Ascension Helper", "Ascension Helper##Visualization", true, true))
        {
            ImGui::Image((ImTextureID*)mAscensionHelperTexture, ImVec2(mAscensionHelperWidth, mAscensionHelperHeight));
        }

        ImGui::End();
        ImGui::PopStyleColor(); // window background
    }
    if(mFrameLogging) { Logger::instance().print("..done"); }

    // ### INFORMATION ###################################################################################
    if(mFrameLogging) { Logger::instance().print("Information window.."); }
    if(mShowInformationWindow)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.5f, 0.0f, 0.75f)); // window background
        ImGui::Begin("Information", NULL, 0);

        // ### General infos ###
        if (ImGui::CollapsingHeader("General", "General##Information", true, true))
        {
            // Files
            ImGui::Text(std::string("PDB: " + mPDBFilepath.substr(mPDBFilepath.find_last_of("\\/") + 1)).c_str());
            if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip(mPDBFilepath.c_str()); }
            ImGui::Text(std::string("XTC: " + mXTCFilepath.substr(mXTCFilepath.find_last_of("\\/") + 1)).c_str());
            if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip(mXTCFilepath.c_str()); }

            // Atom count
            ImGui::Text(std::string("Atom Count: " + std::to_string(mupGPUProtein->getAtomCount())).c_str());

            // Frame count
            ImGui::Text(std::string("Frame Count: " + std::to_string(mupGPUProtein->getFrameCount())).c_str());
        }

        // ### Computation infos ###
        if (ImGui::CollapsingHeader("Computation", "Computation##Information", true, true))
        {
            ImGui::Text(std::string("Computed Start Frame: " + std::to_string(mComputedStartFrame)).c_str());
            ImGui::Text(std::string("Computed End Frame: " + std::to_string(mComputedEndFrame)).c_str());

            // Display whether layers are extracted for this frame
            bool extractedLayers = frameComputed() && mGPUSurfaces.at(mFrame-mComputedStartFrame)->layersExtracted();
            ImGui::Text(std::string("Extracted Layers: " + std::string(extractedLayers ? "True" : "False")).c_str());
        }

        // ### Selection infos ###
        if (ImGui::CollapsingHeader("Selection", "Selection##Information", true, true))
        {
            ImGui::InputInt("Index", &mSelectedAtom, 1);
            if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Index of selected atom."); }
            mSelectedAtom = glm::clamp(mSelectedAtom, 0, mupGPUProtein->getAtomCount() - 1);
            ImGui::Text(std::string("Index: " + std::to_string(mSelectedAtom)).c_str());
            ImGui::Text(std::string("Element: " + mupGPUProtein->getElement(mSelectedAtom)).c_str());
            ImGui::Text(std::string("Aminoacid: " + mupGPUProtein->getAminoacid(mSelectedAtom)).c_str());
            if(frameComputed())
            {
                // Layer
                ImGui::Text(std::string("Layer: " + std::to_string(mGPUSurfaces.at(mFrame - mComputedStartFrame)->getLayerOfAtom(mSelectedAtom))).c_str());

                // Surface area
                ImGui::Text(std::string("Surface Area: " + std::to_string(approximateSurfaceArea({ mSelectedAtom }, mFrame))).c_str());
            }

            // Show / hide selection rendering
            if(mRenderSelection)
            {
                if(ImGui::Button("Hide Selection", ImVec2(208, 22)))
                {
                    mRenderSelection = false;
                }
            }
            else
            {
                if(ImGui::Button("Show Selection", ImVec2(208, 22)))
                {
                    mRenderSelection = true;
                }
            }
        }

        // ### Hardware infos ###
        if (ImGui::CollapsingHeader("Hardware", "Hardware##Information", true, true))
        {
            // Show available GPU memory
            int availableMemory;
            glGetIntegerv(0x9049, &availableMemory); // Nvidia only
            availableMemory = availableMemory / 1000;
            ImGui::Text(std::string("Available VRAM: " + std::to_string(availableMemory) + "MB").c_str());
            // TODO: one may want to clean up glGetError if query failed and show failure message on GUI (for example on Intel or AMD)
        }

        ImGui::End();
        ImGui::PopStyleColor(); // window background
    }
    if(mFrameLogging) { Logger::instance().print("..done"); }

    // ### VALIDATION ####################################################################################
    if(mFrameLogging) { Logger::instance().print("Validation window.."); }
    if(mShowValidationWindow)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.5f, 0.5f, 0.75f)); // window background
        ImGui::Begin("Validation", NULL, 0);

        // Do validation
        ImGui::SliderInt("Samples", &mSurfaceValidationAtomSampleCount, 1, 10000);
        ImGui::SliderInt("Seed", &mSurfaceValidationSeed, 0, 1337);
        if(frameComputed())
        {
            if(ImGui::Button("Validate Surface"))
            {
                mupSurfaceValidation->validate(
                    mupGPUProtein.get(),
                    mGPUSurfaces.at(mFrame - mComputedStartFrame).get(),
                    mFrame,
                    mLayer,
                    mComputedProbeRadius,
                    mSurfaceValidationSeed,
                    mSurfaceValidationAtomSampleCount,
                    mValidationInformation,
                    std::vector<GLuint>());
            }
        }
        else
        {
            ImGui::Text(mNoComputedFrameMessage.c_str());
        }

        // Show / hide internal samples
        if(mShowInternalSamples)
        {
            if(ImGui::Button("Hide Internal", ImVec2(100, 22)))
            {
                mShowInternalSamples = false;
            }
        }
        else
        {
            if(ImGui::Button("Show Internal", ImVec2(100, 22)))
            {
                mShowInternalSamples = true;
            }
        }
        ImGui::SameLine();

        // Show / hide surface samples
        if(mShowSurfaceSamples)
        {
            if(ImGui::Button("Hide Surface", ImVec2(100, 22)))
            {
                mShowSurfaceSamples = false;
            }
        }
        else
        {
            if(ImGui::Button("Show Surface", ImVec2(100, 22)))
            {
                mShowSurfaceSamples = true;
            }
        }
        ImGui::Separator();

        // Report      
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.5f, 0.5f, 0.5f, 0.5f)); // header
        if (ImGui::CollapsingHeader("Report", "Report##Validation", true, false))
        {
            ImGui::Text(mValidationInformation.c_str());
        }
        ImGui::PopStyleColor(); // header

        ImGui::End();
        ImGui::PopStyleColor(); // window background
    }
    if(mFrameLogging) { Logger::instance().print("..done"); }

    // ### ANALYSIS ######################################################################################
    if(mFrameLogging) { Logger::instance().print("Analysis window.."); }
    if(mShowAnalysisWindow)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.5f, 0.5f, 0.0f, 0.75f)); // window background
        ImGui::Begin("Analysis", NULL, 0);

        // Only display when frame is computed
        if(frameComputed())
        {
            // Some variables for this window
            bool doUpdatePath = false;

            // ### Analysis of global ###
            if (ImGui::CollapsingHeader("Global", "Global##Analysis", true, true))
            {
                // ### Count of internal and surface atoms ###
                ImGui::Text(std::string("Internal Atoms In Frame: " + std::to_string(mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfInternalAtoms(mLayer))).c_str());
                ImGui::Text(std::string("Surface Atoms In Frame: " + std::to_string(mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfSurfaceAtoms(mLayer))).c_str());
                ImGui::Separator();

                // ### Save surface indices to file ###
                if(ImGui::Button("Save##surfaceindices"))
                {
                    // Open file
                    std::ofstream fs(mSurfaceIndicesFilePath, std::ios_base::out); // overwrite existing
                    csv::csv_ostream csvs(fs);

                    // Fill data
                    auto indices = mGPUSurfaces.at(mFrame - mComputedStartFrame)->getSurfaceIndices(0);
                    for(const int index : indices)
                    {
                        csvs << std::to_string(index);
                    }

                    // Tell user
                    Logger::instance().print("Saved file: " + mSurfaceIndicesFilePath);
                }
                if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Save surface indices of this frame to file."); }
                ImGui::SameLine();
                ImGui::Text(mSurfaceIndicesFilePath.c_str());
                ImGui::Separator();

                // ### Relation of internal and surface samples ###
                ImGui::PlotLines("Surface Amount", mAnalysisSurfaceAmount.data(), mAnalysisSurfaceAmount.size());
                ImGui::Text(std::string("Surface Amount In Frame: " + std::to_string(mAnalysisSurfaceAmount.at(mFrame - mComputedStartFrame) * 100) + " %%").c_str());
                if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Relation between hull samples on surface and internal ones over all atoms."); }
                ImGui::Separator();

                // ### Approximated surface of molecule ###
                ImGui::PlotLines("Surface Area", mAnalysisSurfaceArea.data(), mAnalysisSurfaceArea.size());
                ImGui::Text(std::string("Surface Area In Frame: " + std::to_string(mAnalysisSurfaceArea.at(mFrame - mComputedStartFrame)) + " \u212b²").c_str());
                ImGui::Separator();

                // ### Save global analysis to file ###
                if(ImGui::Button("Save##globalanalysis"))
                {
                    // Open file
                    std::ofstream fs(mGlobalAnalysisFilePath, std::ios_base::out); // overwrite existing
                    csv::csv_ostream csvs(fs);

                    // Create header
                    csvs << "Frame" << "SurfaceAmount" << "SurfaceArea";
                    csvs << csv::endl;

                    // Fill data
                    for(int frame = mComputedStartFrame; frame <= mComputedEndFrame; frame++)
                    {
                        // Relative frame
                        int relativeFrame = frame - mComputedStartFrame;

                        // Frame
                        csvs << std::to_string(frame);

                        // Surface Amount
                        csvs << std::to_string(mAnalysisSurfaceAmount.at(relativeFrame));

                        // SurfaceArea
                        csvs << std::to_string(mAnalysisSurfaceArea.at(relativeFrame));

                        // End line
                        csvs << csv::endl;
                    }

                    // Tell user
                    Logger::instance().print("Saved file: " + mGlobalAnalysisFilePath);
                }
                if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Save global analysis to file."); }
                ImGui::SameLine();
                ImGui::Text(mGlobalAnalysisFilePath.c_str());
                ImGui::Separator();

                // ### Save amino acid analysis to file ###
                if(ImGui::Button("Save Amino Acids Analysis##aminoacidsanalysis"))
                {
                    // Fetch values for all amino acids
                    struct AminoAcidAnalysis
                    {
                        std::string name;
                        int startIndex;
                        int endIndex;
                        float averageLayersDeltaAccumulation;
                        float inverseAverageLayersDeltaAccumulation;
                        std::vector<float> averageLayers;
                        std::vector<float> inverseAverageLayers;
                        std::vector<float> averageLayersDelta;
                        std::vector<float> inverseAverageLayersDelta;
                    };

                    // Go over all amino acids
                    std::vector<GPUProtein::AminoAcid> aminoAcids = mupGPUProtein->getAminoAcids();
                    std::vector<AminoAcidAnalysis> aminoAcidAnalysis; // vector to fill
                    for(const GPUProtein::AminoAcid& rAminoAcid : aminoAcids)
                    {
                        // Do calculations
                        AminoAcidAnalysis current;
                        current.name = rAminoAcid.name;
                        current.startIndex = rAminoAcid.startIndex;
                        current.endIndex = rAminoAcid.endIndex;
                        if(atomsInRangeCalculations(
                            current.startIndex,
                            current.endIndex,
                            current.averageLayersDeltaAccumulation,
                            current.inverseAverageLayersDeltaAccumulation,
                            current.averageLayers,
                            current.inverseAverageLayers,
                            current.averageLayersDelta,
                            current.inverseAverageLayersDelta))
                        {
                            // Only push back when successful
                            aminoAcidAnalysis.push_back(current);
                        }
                    }

                    // ### ACCUMULATED VALUES ###
                    if(!aminoAcidAnalysis.empty())
                    {
                        // Open file
                        std::ofstream fs(mAminoAcidAnalysisAccumulatedFilePath, std::ios_base::out); // overwrite existing
                        csv::csv_ostream csvs(fs);

                        // Create header
                        csvs << "AminoAcid" << "StartIndex" << "EndIndex" << "Average Layers Delta Accumulation" << "Inverse Average Layers Delta Accumulation";
                        csvs << csv::endl;

                        // Fill data
                        for(const AminoAcidAnalysis& rAnalysis : aminoAcidAnalysis)
                        {
                            // Name
                            csvs << rAnalysis.name;

                            // Start index
                            csvs << std::to_string(rAnalysis.startIndex);

                            // End index
                            csvs << std::to_string(rAnalysis.endIndex);

                            // Average layers delta accumulation
                            csvs << std::to_string(rAnalysis.averageLayersDeltaAccumulation);

                            // Inverse average layers delta accumulation
                            csvs << std::to_string(rAnalysis.inverseAverageLayersDeltaAccumulation);

                            // End line
                            csvs << csv::endl;
                        }

                        // Tell user
                        Logger::instance().print("Saved file: " + mAminoAcidAnalysisAccumulatedFilePath);
                    }

                    // ### AVERAGE LAYERS ###
                    if(!aminoAcidAnalysis.empty())
                    {
                        // Open file
                        std::ofstream fs(mAminoAcidAnalysisAvgLayersFilePath, std::ios_base::out); // overwrite existing
                        csv::csv_ostream csvs(fs);

                        // Create header
                        csvs << "Computed Frame";
                        for(const AminoAcidAnalysis& rAnalysis : aminoAcidAnalysis)
                        {
                            csvs << rAnalysis.name;
                        }
                        csvs << csv::endl;

                        // Go over computed frames
                        for(int computedFrame = 0; computedFrame < aminoAcidAnalysis.back().averageLayers.size(); computedFrame++) // assume that all average layers vectors have same size
                        {
                            // Computed frame
                            csvs << computedFrame;

                            // Average layer of amino acids in computed frame
                            for(const AminoAcidAnalysis& rAnalysis : aminoAcidAnalysis)
                            {
                                csvs << rAnalysis.averageLayers.at(computedFrame);
                            }
                            csvs << csv::endl;
                        }

                        // Tell user
                        Logger::instance().print("Saved file: " + mAminoAcidAnalysisAvgLayersFilePath);
                    }

                    // ### INVERSE AVERAGE LAYERS ###
                    if(!aminoAcidAnalysis.empty())
                    {
                        // Open file
                        std::ofstream fs(mAminoAcidAnalysisInverseAvgLayersFilePath, std::ios_base::out); // overwrite existing
                        csv::csv_ostream csvs(fs);

                        // Create header
                        csvs << "Computed Frame";
                        for(const AminoAcidAnalysis& rAnalysis : aminoAcidAnalysis)
                        {
                            csvs << rAnalysis.name;
                        }
                        csvs << csv::endl;

                        // Go over computed frames
                        for(int computedFrame = 0; computedFrame < aminoAcidAnalysis.back().inverseAverageLayers.size(); computedFrame++) // assume that all inverse average layers vectors have same size
                        {
                            // Computed frame
                            csvs << computedFrame;

                            // Inverse average layer of amino acids in computed frame
                            for(const AminoAcidAnalysis& rAnalysis : aminoAcidAnalysis)
                            {
                                csvs << rAnalysis.inverseAverageLayers.at(computedFrame);
                            }
                            csvs << csv::endl;
                        }

                        // Tell user
                        Logger::instance().print("Saved file: "+ mAminoAcidAnalysisInverseAvgLayersFilePath);
                    }

                    // ### AVERAGE LAYERS DELTA ###
                    if(!aminoAcidAnalysis.empty())
                    {
                        // Open file
                        std::ofstream fs(mAminoAcidAnalysisAvgLayersDeltaFilePath, std::ios_base::out); // overwrite existing
                        csv::csv_ostream csvs(fs);

                        // Create header
                        csvs << "Computed Frame";
                        for(const AminoAcidAnalysis& rAnalysis : aminoAcidAnalysis)
                        {
                            csvs << rAnalysis.name;
                        }
                        csvs << csv::endl;

                        // Go over computed frames
                        for(int computedFrame = 0; computedFrame < aminoAcidAnalysis.back().averageLayersDelta.size(); computedFrame++) // assume that all average layers delta vectors have same size
                        {
                            // Computed frame
                            csvs << computedFrame;

                            // Average layer of amino acids in computed frame
                            for(const AminoAcidAnalysis& rAnalysis : aminoAcidAnalysis)
                            {
                                csvs << rAnalysis.averageLayersDelta.at(computedFrame);
                            }
                            csvs << csv::endl;
                        }

                        // Tell user
                        Logger::instance().print("Saved file: " + mAminoAcidAnalysisAvgLayersDeltaFilePath);
                    }

                    // ### INVERSE AVERAGE LAYERS DELTA ###
                    if(!aminoAcidAnalysis.empty())
                    {
                        // Open file
                        std::ofstream fs(mAminoAcidAnalysisInverseAvgLayersDeltaFilePath, std::ios_base::out); // overwrite existing
                        csv::csv_ostream csvs(fs);

                        // Create header
                        csvs << "Computed Frame";
                        for(const AminoAcidAnalysis& rAnalysis : aminoAcidAnalysis)
                        {
                            csvs << rAnalysis.name;
                        }
                        csvs << csv::endl;

                        // Go over computed frames
                        for(int computedFrame = 0; computedFrame < aminoAcidAnalysis.back().inverseAverageLayersDelta.size(); computedFrame++) // assume that all inverse average layers delta vectors have same size
                        {
                            // Computed frame
                            csvs << computedFrame;

                            // Average layer of amino acids in computed frame
                            for(const AminoAcidAnalysis& rAnalysis : aminoAcidAnalysis)
                            {
                                csvs << rAnalysis.inverseAverageLayersDelta.at(computedFrame);
                            }
                            csvs << csv::endl;
                        }

                        // Tell user
                        Logger::instance().print("Saved file: " + mAminoAcidAnalysisInverseAvgLayersDeltaFilePath);
                    }

                }
                if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Save amino acids analysis to multiple files."); }
            }

            // ### Analysis of group ###
            if (ImGui::CollapsingHeader("Group", "Group##Analysis", true, true))
            {
                // ### Managment of group members ###

                // Scrollable part for members
                ImGui::BeginChild(
                    "AnalyseAtoms",
                    ImVec2(ImGui::GetWindowContentRegionWidth() * 1.0f, 100),
                    false,
                    ImGuiWindowFlags_HorizontalScrollbar);

                // Useful variables
                std::vector<int> toBeRemoved;
                bool analysisAtomsChanged = false;

                // Go over analyse atoms and list them
                for (int atomIndex : mAnalyseGroup)
                {
                    // Mark if currently selected
                    if(atomIndex == mSelectedAtom)
                    {
                        ImGui::TextColored(ImVec4(mSelectionColor.r, mSelectionColor.g, mSelectionColor.b ,1), "\u2023");
                        ImGui::SameLine();
                    }

                    // Index of atom
                    ImGui::Text("%05d", atomIndex);
                    ImGui::SameLine();

                    // Element of atom
                    ImGui::Text(mupGPUProtein->getElement(atomIndex).c_str());
                    ImGui::SameLine();

                    // Aminoacid of atom
                    ImGui::Text(mupGPUProtein->getAminoacid(atomIndex).c_str());
                    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 60);

                    // Select that atom (use ## to add number for an unique button id)
                    if(ImGui::Button(std::string("\u25cb##" + std::to_string(atomIndex)).c_str()))
                    {
                        mSelectedAtom = atomIndex;
                    }
                    if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Select atom."); }
                    ImGui::SameLine();

                    // Remove that atom from analysis atoms (use ## to add number for an unique button id)
                    if(ImGui::Button(std::string("\u00D7##" + std::to_string(atomIndex)).c_str()))
                    {
                        toBeRemoved.push_back(atomIndex);
                        analysisAtomsChanged = true;
                    }
                    if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Remove atom."); }
                }

                // Remove atoms from analysis
                for(int atomIndex : toBeRemoved) { mAnalyseGroup.erase(atomIndex); }
                ImGui::EndChild();

                // Add new atoms to analyse
                ImGui::InputInt("##nextanalysisatom", &mNextAnalyseAtomIndex);
                if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Index of atom."); }
                mNextAnalyseAtomIndex = glm::clamp(mNextAnalyseAtomIndex, 0, mupGPUProtein->getAtomCount());
                ImGui::SameLine();
                if(ImGui::Button("Add Atom"))
                {
                    // Add atom to list of analyse atoms
                    mAnalyseGroup.insert((GLuint)mNextAnalyseAtomIndex);
                    analysisAtomsChanged = true;
                }

                // Range to add atoms
                ImGui::InputInt("Start Index", &mNewGroupAtomsStartIndex);
                mNewGroupAtomsStartIndex = glm::clamp(mNewGroupAtomsStartIndex, 0, mupGPUProtein->getAtomCount() - 1);
                if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Set start index of atoms which shall be added to group."); }
                ImGui::InputInt("End Index", &mNewGroupAtomsEndIndex);
                mNewGroupAtomsEndIndex = glm::clamp(mNewGroupAtomsEndIndex, 0, mupGPUProtein->getAtomCount() - 1);
                if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Set end index of atoms which shall be added to group."); }
                if(ImGui::Button("Add Atom Range"))
                {
                    // Add atoms to list of analyse atoms
                    for(int i = mNewGroupAtomsStartIndex; i <= mNewGroupAtomsEndIndex; i++)
                    {
                        mAnalyseGroup.insert((GLuint)i);
                    }
                    analysisAtomsChanged = true;

                }
                if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Add all atoms from start to end index defined above."); }
                ImGui::SameLine();

                // Remove all atoms from group
                if(ImGui::Button("Clear Group"))
                {
                    mAnalyseGroup.clear();
                    analysisAtomsChanged = true;
                }
                if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Remove all atoms from analysis group."); }

                // Recreate outline atom indices, path visualization and group analysis
                if(analysisAtomsChanged)
                {
                    // Outline
                    std::vector<GLuint> analyseAtomVector;
                    std::copy(mAnalyseGroup.begin(), mAnalyseGroup.end(), std::back_inserter(analyseAtomVector));
                    mupOutlineAtomIndices = std::unique_ptr<GPUTextureBuffer>(new GPUTextureBuffer(analyseAtomVector));

                    // Path
                    doUpdatePath = true;

                    // Group analysis update
                    updateGroupAnalysis();

                    // Indicators for highlighting
                    std::vector<float> groupIndicators(mupGPUProtein->getAtomCount(), 0.f);
                    for(GLuint index : analyseAtomVector)
                    {
                        groupIndicators.at(index) = 1.f;
                    }
                    mupGroupIndicators->fill(groupIndicators, GL_DYNAMIC_DRAW);
                }
                ImGui::Separator();

                // ### Rendering ###

                // Show / hide outline
                if(mRenderOutline)
                {
                    if(ImGui::Button("Hide Outline", ImVec2(90, 22)))
                    {
                        mRenderOutline = false;
                    }
                }
                else
                {
                    if(ImGui::Button("Show Outline", ImVec2(90, 22)))
                    {
                        mRenderOutline = true;
                    }
                }
                ImGui::SameLine();

                // Show / hide path
                if(mShowPath)
                {
                    if(ImGui::Button("Hide Path", ImVec2(75, 22)))
                    {
                        mShowPath = false;
                    }
                }
                else
                {
                    if(ImGui::Button("Show Path", ImVec2(75, 22)))
                    {
                        mShowPath = true;
                    }
                }

                // Show / hide group on top
                if(mRenderGroupOnTop)
                {
                    if(ImGui::Button("Hide Group On Top", ImVec2(173, 22)))
                    {
                        mRenderGroupOnTop = false;
                    }
                }
                else
                {
                    if(ImGui::Button("Show Group On Top", ImVec2(173, 22)))
                    {
                        mRenderGroupOnTop = true;
                    }
                }
                ImGui::Separator();

                // Analysis of group
                if(mAnalyseGroup.size() > 0)
                {
                    // ### Path ###
                    std::ostringstream stringPathLength;
                    int maxFrameIndex = mupGPUProtein->getFrameCount() - 1;

                    // Manual global path length determination
                    ImGui::InputInt("Path Start Frame", &mPathLengthStartFrame);
                    if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Start frame used for path length calculation below."); }
                    ImGui::InputInt("Path End Frame", &mPathLengthEndFrame);
                    if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("End frame used for path length calculation below."); }
                    mPathLengthStartFrame = glm::clamp(mPathLengthStartFrame, 0, maxFrameIndex);
                    mPathLengthEndFrame = glm::clamp(mPathLengthEndFrame, 0, maxFrameIndex);
                    mPathLengthStartFrame = glm::min(mPathLengthEndFrame, mPathLengthStartFrame);
                    mPathLengthEndFrame = glm::max(mPathLengthStartFrame, mPathLengthEndFrame);

                    // Global path length
                    stringPathLength = std::ostringstream();
                    stringPathLength << std::fixed << std::setprecision(2) << mupPath->getLength(mPathLengthStartFrame, mPathLengthEndFrame);
                    ImGui::Text(std::string("Global Path Length: " + stringPathLength.str() + " \u212b").c_str());
                    if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Global path length between set start and end frame."); }

                    // Local path length
                    stringPathLength = std::ostringstream();
                    stringPathLength << std::fixed << std::setprecision(2) << mupPath->getLocalLength(mPathLengthStartFrame, mPathLengthEndFrame);
                    ImGui::Text(std::string("Local Path Length: " + stringPathLength.str() + " \u212b").c_str());
                    if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Local path length, for each position the molecule's center is subtracted."); }
                    ImGui::Separator();

                    // Radius of frames which are taken into account for path smoothing
                    int radius = mPathSmoothRadius;
                    ImGui::SliderInt("Path Smooth Radius", &radius, 0, 10);
                    if(radius != mPathSmoothRadius)
                    {
                        mPathSmoothRadius = radius; // save new radius
                        doUpdatePath = true; // remember to update display path
                    }

                    // Radius of frames in path visualization
                    ImGui::SliderInt("Path Display Radius", &mPathFrameRadius, 1, 1000);

                    // Length of displayed path
                    int startFrame = glm::max(0, mFrame - mPathFrameRadius);
                    int endFrame = glm::min(mupPath->getVertexCount()-1, mFrame + mPathFrameRadius);
                    stringPathLength = std::ostringstream();
                    stringPathLength << std::fixed << std::setprecision(2) << mupPath->getLength(startFrame, endFrame);
                    ImGui::Text(std::string("Path Displayed Length: " + stringPathLength.str() + " \u212b").c_str());

                    // Length of complete path
                    stringPathLength = std::ostringstream();
                    stringPathLength << std::fixed << std::setprecision(2) << mupPath->getCompleteLength();
                    ImGui::Text(std::string("Path Complete Length: " + stringPathLength.str() + " \u212b").c_str());
                    ImGui::Separator();

                    // ### Layer of group ###
                    ImGui::PlotLines("Group Min Layer", mAnalysisGroupMinLayers.data(), mAnalysisGroupMinLayers.size());
                    ImGui::Text(std::string("Group Min Layer In Frame: " + std::to_string(mAnalysisGroupMinLayers.at(mFrame - mComputedStartFrame))).c_str());
                    ImGui::Separator();

                    ImGui::PlotLines("Group Avg Layer", mAnalysisGroupAvgLayers.data(), mAnalysisGroupAvgLayers.size());
                    ImGui::Text(std::string("Group Avg Layer In Frame: " + std::to_string(mAnalysisGroupAvgLayers.at(mFrame - mComputedStartFrame))).c_str());
                    ImGui::Separator();

                    // ### Surface amount of group ###
                    ImGui::PlotLines("Group Surface Amount", mAnalysisGroupSurfaceAmount.data(), mAnalysisGroupSurfaceAmount.size());
                    ImGui::Text(std::string("Group Surface Amount In Frame: " + std::to_string(mAnalysisGroupSurfaceAmount.at(mFrame - mComputedStartFrame) * 100) + " %%").c_str());
                    if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Relation between hull samples on surface and internal ones over group's atoms."); }
                    ImGui::Separator();

                    // ### Surface area of group ###
                    ImGui::PlotLines("Group Surface Area", mAnalysisGroupSurfaceArea.data(), mAnalysisGroupSurfaceArea.size());
                    ImGui::Text(std::string("Group Surface Area In Frame: " + std::to_string(mAnalysisGroupSurfaceArea.at(mFrame - mComputedStartFrame)) + " \u212b²").c_str());
                    ImGui::Separator();

                    // ### Average Layers Delta Accumulation ###
                    ImGui::Text(std::string("Average Layers Delta Accumulation: " + std::to_string(mAvgLayersDeltaAcc)).c_str());
                    if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Accumulation of absolute delta of average layers over time."); }
                    ImGui::Separator();

                    // ### Save group analysis to file ###
                    if(ImGui::Button("Save##groupanalysis"))
                    {
                        // Open file
                        std::ofstream fs(mGroupAnalysisFilePath, std::ios_base::out); // overwrite existing
                        csv::csv_ostream csvs(fs);

                        // Create header
                        csvs << "Frame" << "MinLayer" << "AvgLayer" << "SurfaceAmount" << "SurfaceArea" << "AccPath" << "LocalAccPath";
                        csvs << csv::endl;

                        // Fill data
                        for(int frame = mComputedStartFrame; frame <= mComputedEndFrame; frame++)
                        {
                            // Relative frame
                            int relativeFrame = frame - mComputedStartFrame;

                            // Frame
                            csvs << std::to_string(frame);

                            // MinLayer
                            csvs << std::to_string(mAnalysisGroupMinLayers.at(relativeFrame));

                            // AvgLayer
                            csvs << std::to_string(mAnalysisGroupAvgLayers.at(relativeFrame));

                            // Surface Amount
                            csvs << std::to_string(mAnalysisGroupSurfaceAmount.at(relativeFrame));

                            // SurfaceArea
                            csvs << std::to_string(mAnalysisGroupSurfaceArea.at(relativeFrame));

                            // Accumulated path length
                            csvs << std::to_string(mupPath->getLength(0, frame));

                            // Accumulated local path length
                            csvs << std::to_string(mupPath->getLocalLength(0, frame));

                            // End line
                            csvs << csv::endl;
                        }

                        // Tell user
                        Logger::instance().print("Saved file: " + mGroupAnalysisFilePath);
                    }
                    if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Save group analysis to file."); }
                    ImGui::SameLine();
                    ImGui::Text(mGroupAnalysisFilePath.c_str());
                }
            }

            // Update path if necessary
            if(doUpdatePath)
            {
                mupPath->update(mupGPUProtein.get(), mAnalyseGroup, mPathSmoothRadius);
            }
        }
        else
        {
            ImGui::Text(mNoComputedFrameMessage.c_str());
        }

        ImGui::End();
        ImGui::PopStyleColor(); // window background
    }
    if(mFrameLogging) { Logger::instance().print("..done"); }

    // ### RENDERING #####################################################################################
    if(mFrameLogging) { Logger::instance().print("Rendering window.."); }
    if(mShowRenderingWindow)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.25f, 0.25f, 0.25f, 0.75f)); // window background
        ImGui::Begin("Rendering", NULL, 0);

        // Background
        ImGui::Combo("Background", (int*)&mBackground, "None\0Scientific\0Computervisualistik\0Beach\0White\0");
        if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Choose cubemap used for background."); }

        // Lighting
        if(ImGui::Button("Spot Light"))
        {
            mLightDirection = -glm::normalize(mupCamera->getPosition() - mupCamera->getCenter());
        }
        if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Set light direction to current camera direction."); }
        ImGui::SameLine();

        // Super sampling
        ImGui::Checkbox("Use Super Sampling", &mSuperSampling);
        if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Switch 2x super sampling on or off."); }

        // Depth darkening
        ImGui::SliderFloat("Depth Darkening Start", &mDepthDarkeningStart, 0, mDepthDarkeningEnd, "%.1f");
        ImGui::SliderFloat("Depth Darkening End", &mDepthDarkeningEnd, mDepthDarkeningStart, mDepthDarkeningMaxEnd, "%.1f");

        // Show / hide axes gizmo
        if(mShowAxesGizmo)
        {
            if(ImGui::Button("Hide Axes Gizmo", ImVec2(208, 22)))
            {
                mShowAxesGizmo = false;
            }
        }
        else
        {
            if(ImGui::Button("Show Axes Gizmo", ImVec2(208, 22)))
            {
                mShowAxesGizmo = true;
            }
        }
        if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Gizmo visualizing coordinate axes."); }

        ImGui::End();
        ImGui::PopStyleColor(); // window background
    }
    if(mFrameLogging) { Logger::instance().print("..done"); }

    ImGui::PopStyleColor(); // slider grab active
    ImGui::PopStyleColor(); // header active
    ImGui::PopStyleColor(); // header hovered
    ImGui::PopStyleColor(); // header
    ImGui::PopStyleColor(); // button hovered
    ImGui::PopStyleColor(); // button
    ImGui::PopStyleColor(); // scrollbar background
    ImGui::PopStyleColor(); // scrollbar grab
    ImGui::PopStyleColor(); // window title active
    ImGui::PopStyleColor(); // window title collapsed
    ImGui::PopStyleColor(); // window title

    ImGui::Render();
}

void SurfaceDynamicsVisualization::updateComputationInformation(std::string device, float computationTime)
{
    std::stringstream stream;
    stream <<
        device << " used" << "\n"
        << "Probe radius: " + std::to_string(mComputationProbeRadius) << "\n"
        << "Extracted layers: " << (mExtractLayers ? "yes" : "no") << "\n"
        << "Start frame: " << mComputationStartFrame << " End frame: " << mComputationEndFrame << "\n"
        << "Count of frames: " << (mComputationEndFrame - mComputationStartFrame + 1) << "\n"
        << "Extraction time: " << computationTime << "ms";
    mComputeInformation = stream.str();
}

bool SurfaceDynamicsVisualization::setFrame(int frame)
{
    // Clamp frame
    frame = glm::clamp(frame, mStartFrame, mEndFrame);

    // Write it to variable
    bool success = false;
    if(mFrame != frame)
    {
        mFrame = frame;
        success = true;
    }

    // Check whether there are enough layers to display
    if(frameComputed()) // set mFrame before calling this
    {
        int layerCount = mGPUSurfaces.at(frame - mComputedStartFrame)->getLayerCount();
        if(mLayer >= layerCount)
        {
            mLayer = layerCount -1;
        }
    }

    return success;
}

void SurfaceDynamicsVisualization::computeLayers(bool useGPU)
{
    // # Surface calculation

    // Reset surfaces
    mGPUSurfaces.clear();

    // Do it for all animation frames
    float computationTime = 0;
    for(int i = mComputationStartFrame; i <= mComputationEndFrame; i++)
    {
        // Process one frame
        if(useGPU)
        {
            mGPUSurfaces.push_back(std::move(mupGPUSurfaceExtraction->calculateSurface(mupGPUProtein.get(), i, mComputationProbeRadius, mExtractLayers)));
        }
        else
        {
            mGPUSurfaces.push_back(std::move(mupGPUSurfaceExtraction->calculateSurface(mupGPUProtein.get(), i, mComputationProbeRadius, mExtractLayers, true, mCPUThreads)));
        }
        computationTime += mGPUSurfaces.back()->getComputationTime();

        // Show progress
        float progress = (float)(i- mComputationStartFrame + 1) / (float)(mComputationEndFrame - mComputationStartFrame + 1);
        setProgressDisplay("Surface", progress);
    }

    // Update compute information
    updateComputationInformation(
        (useGPU ? "GPU" : "CPU with " + std::to_string(mCPUThreads) + " threads"), computationTime);

    // Remember which frames were computed
    mComputedStartFrame = mComputationStartFrame;
    mComputedEndFrame = mComputationEndFrame;

    // Remember which probe radius was used
    mComputedProbeRadius = mComputationProbeRadius;

    // Hull sample computation
    computeHullSamples();

    // Ascension computation
    computeAscension();

    // Set to first computed frame
    setFrame(mComputedStartFrame);
}

void SurfaceDynamicsVisualization::computeHullSamples()
{
    // Compute hull samples
    mupHullSamples->compute(
        mupGPUProtein.get(),
        &mGPUSurfaces,
        mComputationStartFrame,
        mComputationProbeRadius,
        mHullSampleCount,
        0,
        [this](float progress) // [0,1]
        {
            this->setProgressDisplay("Hull Samples", progress);
        });

    // Update analysis which depends on hull samples
    updateGlobalAnalysis();
    updateGroupAnalysis();
}

void SurfaceDynamicsVisualization::computeAscension()
{
    // Calculate ascension for visualization
    int atomCount = mupGPUProtein->getAtomCount();
    std::vector<float> ascension; // linear accumulation of ascension for all computed frames and all atoms
    ascension.reserve(atomCount * (int)mGPUSurfaces.size());
    GLuint frame = 0; // ascension frame, incremented in outer for loop
    float pi = glm::pi<float>();
    float upToHot = pi / mAscensionUpToHotFrameCount;
    float downToHot = pi / mAscensionDownToHotFrameCount;
    float upToCold = pi / mAscensionUpToColdFrameCount;
    float downToCold = pi / mAscensionDownToColdFrameCount;

    // Go over frames for which surface exist
    for(const auto& rupGPUSurface : mGPUSurfaces)
    {
        // Get surface indices of layer zero for that frame
        std::vector<GLuint> surfaceIndices = rupGPUSurface->getSurfaceIndices(0);

        // Go over atoms
        for(int a = 0; a < atomCount; a++)
        {
            bool surface = false;

            // Go over surface indices
            for(GLuint s : surfaceIndices)
            {
                // Check whether current atom is on surface
                if(a == s)
                {
                    surface = true;
                    break;
                }
            }

            // Value which will be filled and pushed back
            float value = 0;

            // Check for first frame
            if(frame == 0)
            {
                // Push back value for first frame of ascension (either at surface or not)
                value = surface ? pi : 0.f;
            }
            else
            {
                // Fetch value of previous frame for this atom
                float previousValue = ascension.at(((frame - 1) * atomCount + a));

                // Decide what to happen with ascension value
                if(surface) // surface
                {
                    // Decide whether increase or decrease
                    if(previousValue == pi)
                    {
                        // Already hot, stay that way
                        value = pi;
                    }
                    else if(previousValue < pi)
                    {
                        // Getting hotter, coming from cold
                        value = glm::min(pi, previousValue + upToHot);
                    }
                    else
                    {
                        // Getting hotter again, was already on its way to becoming cold
                        value = glm::max(pi, previousValue - downToHot);
                    }
                }
                else // internal
                {
                    // Decide whether increase or decrease
                    if(previousValue == 0)
                    {
                        // Already cold, stay that way
                        value = 0.f;
                    }
                    else if(previousValue < pi)
                    {
                        // A throwback on the way of getting hot
                        value = glm::max(0.f, previousValue - downToCold);
                    }
                    else
                    {
                        // Was hot and getting cold again
                        value = glm::mod(
                                    glm::min(
                                        2.f * pi,
                                        previousValue + upToCold),
                                    2.f * pi);
                    }
                }
            }

            // Push back calculated value for that atom on that frame
            ascension.push_back(value);
        }

        // Increment counter of frames
        frame++;
    }

    // Fill ascension to texture buffer
    mupAscension->fill(ascension, GL_DYNAMIC_DRAW);
}

int SurfaceDynamicsVisualization::getAtomBeneathCursor() const
{
    // Bind correct framebuffer and point to attachment with pick indices
    mupMoleculeFramebuffer->bind();
    glReadBuffer(GL_COLOR_ATTACHMENT1);

    // Get position of cursor
    double cursorX, cursorY;
    glfwGetCursorPos(mpWindow, &cursorX, &cursorY);
    int windowHeight = mWindowHeight;

    // When super sampling is used, multiply cursor positions
    if(mupMoleculeFramebuffer->superSampling())
    {
        cursorX *= mupMoleculeFramebuffer->getSuperSamplingMultiplier();
        cursorY *= mupMoleculeFramebuffer->getSuperSamplingMultiplier();
        windowHeight *= mupMoleculeFramebuffer->getSuperSamplingMultiplier();
    }

    // Get pick index
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    unsigned char data[3];
    glReadPixels((int)cursorX, windowHeight - (int)cursorY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
    int index =
        data[0] +
        data[1] * 256 +
        data[2] * 256*256;
    mupMoleculeFramebuffer->unbind();

    // Index of zero means, that nothing was there
    if(index <= 0)
    {
        return -1;
    }
    else
    {
        return (index - 1); // starts at one but should start at zero
    }
}

float SurfaceDynamicsVisualization::approximateSurfaceArea(std::vector<GLuint> indices, int frame) const
{
    // Go over all surface atoms' radii
    float surface = 0;
    for(int index : indices)
    {
        // Add surface
        const float radius = mupGPUProtein->getRadii()->at(index);
        const float extendedRadius = radius + mComputedProbeRadius;
        const float atomSurface = 4.f * glm::pi<float>() * extendedRadius * extendedRadius;
        surface += atomSurface * ((float)mupHullSamples->getSurfaceSampleCount(frame, index) / (float)mupHullSamples->getSampleCount());
    }

    return surface;
}

void SurfaceDynamicsVisualization::updateGlobalAnalysis()
{
    // Surface amount of molecule
    auto sampleCounts = mupHullSamples->getSurfaceSampleCount();
    mAnalysisSurfaceAmount = std::vector<float>(sampleCounts.size(), -1);
    for(int i = 0; i < sampleCounts.size(); i++)
    {
        mAnalysisSurfaceAmount.at(i) = (float)sampleCounts.at(i) / (float)mupHullSamples->getProteinSampleCount();
    }

    // Surface area of molecule
    mAnalysisSurfaceArea = std::vector<float>(mGPUSurfaces.size(), -1);
    for(int frame = mComputedStartFrame; frame <= mComputedEndFrame; frame++)
    {
        // Relative frame
        int relativeFrame = frame - mComputedStartFrame;

        // Approximate surface for that frame
        mAnalysisSurfaceArea.at(relativeFrame) = approximateSurfaceArea(mGPUSurfaces.at(relativeFrame)->getSurfaceIndices(0), frame);
    }
}

void SurfaceDynamicsVisualization::updateGroupAnalysis()
{
    // Go over frames and extract layer of group
    mAnalysisGroupMinLayers = std::vector<float>(mGPUSurfaces.size(), -1); // minus one means no data
    mAnalysisGroupAvgLayers = std::vector<float>(mGPUSurfaces.size(), -1); // minus one means no data
    for(int frame = mComputedStartFrame; frame <= mComputedEndFrame; frame++)
    {
        // Relative frame
        int relativeFrame = frame - mComputedStartFrame;

        // Do it only when layers were extracted for this frame
        if(mGPUSurfaces.at(relativeFrame)->layersExtracted())
        {
            // Calculate layer of group for that frame
            int minLayer = std::numeric_limits<int>::max();
            float avgLayer = 0;
            for(GLuint atomIndex : mAnalyseGroup)
            {
                // Get layer of that atom
                int layer = mGPUSurfaces.at(relativeFrame)->getLayerOfAtom(atomIndex);

                // Extract min layer (which mean the one closest or at surface)
                minLayer = minLayer > layer ? layer : minLayer;

                // Accumulate for average layer calculation
                avgLayer += (float)layer;
            }
            mAnalysisGroupMinLayers.at(relativeFrame) = (float)minLayer;
            mAnalysisGroupAvgLayers.at(relativeFrame) = avgLayer / (float)mAnalyseGroup.size();
        }
    }

    // Go over frames and calculate surface amount and area of group atoms
    mAnalysisGroupSurfaceAmount = std::vector<float>(mGPUSurfaces.size(), -1); // minus one means no data
    mAnalysisGroupSurfaceArea = std::vector<float>(mGPUSurfaces.size(), -1); // minus one means no data
    for(int frame = mComputedStartFrame; frame <= mComputedEndFrame; frame++)
    {
        // Relative frame
        int relativeFrame = frame - mComputedStartFrame;

        // Accumulate sample count
        float surfaceSampleCount = 0;
        for(GLuint atomIndex : mAnalyseGroup)
        {
            // Surface amount for group in that frame for that atom
            surfaceSampleCount += (float)mupHullSamples->getSurfaceSampleCount(frame, atomIndex);
        }

        // Save surface amount of group for that frame
        mAnalysisGroupSurfaceAmount.at(relativeFrame) = surfaceSampleCount / (float)mupHullSamples->getSampleCount(mAnalyseGroup.size());

        // Save surface area
        mAnalysisGroupSurfaceArea.at(relativeFrame) = approximateSurfaceArea(std::vector<GLuint>(mAnalyseGroup.begin(), mAnalyseGroup.end()), frame);
    }

    // Average layers delta accumulation
    mAvgLayersDeltaAcc = 0.f;
    for(int i = 0; i < mAnalysisGroupAvgLayers.size() - 1; i++)
    {
        float delta = mAnalysisGroupAvgLayers.at(i + 1) - mAnalysisGroupAvgLayers.at(i);
        delta = delta < 0 ? -delta : delta;
        mAvgLayersDeltaAcc += delta;
    }
}

GLuint SurfaceDynamicsVisualization::createCubemapTexture(
        std::string filepathPosX,
        std::string filepathNegX,
        std::string filepathPosY,
        std::string filepathNegY,
        std::string filepathPosZ,
        std::string filepathNegZ) const
{
    // Prepare stb_image loading
    // stbi_set_flip_vertically_on_load(true);
    int width, height, channelCount;

    // Create texture
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Push paths to vector
    std::vector<std::string> cubemapFullpaths;
    cubemapFullpaths.push_back(filepathPosX);
    cubemapFullpaths.push_back(filepathNegX);
    cubemapFullpaths.push_back(filepathPosY);
    cubemapFullpaths.push_back(filepathNegY);
    cubemapFullpaths.push_back(filepathPosZ);
    cubemapFullpaths.push_back(filepathNegZ);

    // Load all directions
    for(int i = 0; i < cubemapFullpaths.size(); i++)
    {
        // Try to load image
        unsigned char* pData = stbi_load(cubemapFullpaths.at(i).c_str(), &width, &height, &channelCount, 0);

        // Check whether file was found and parsed
        if (pData == NULL)
        {
            Logger::instance().print("Image file not found or error at parsing: " + cubemapFullpaths.at(i));
            continue;
        }

        // Set texture
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pData); // TODO: use channel count given by stb_image?

        // Delete raw image data
        stbi_image_free(pData);
    }

    // Unbind texture
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    // Return texture handle
    return texture;
}

void SurfaceDynamicsVisualization::resetPath(std::string& rPath, std::string appendage) const
{
    // Fetch directory for saving bookmarks etc.
    std::string path(getenv("HOME"));

    // Append stuff
    path.append(appendage);

    // Set reference
    rPath = path;
}

bool SurfaceDynamicsVisualization::atomsInRangeCalculations(
    int startIndex,
    int endIndex,
    float& rAcc,
    float& rInverseAcc,
    std::vector<float>& rAvgLayers,
    std::vector<float>& rInverseAvgLayers,
    std::vector<float>& rAvgLayersDelta,
    std::vector<float>& rInverseAvgLayersDelta) const
{
    // Go over frames and extract average layer of atoms
    std::vector<float> avgLayers = std::vector<float>(mGPUSurfaces.size(), -1.f); // minus one means no data
    std::vector<float> invAvgLayers = std::vector<float>(mGPUSurfaces.size(), -1.f); // minus one means no data

    for(int frame = mComputedStartFrame; frame <= mComputedEndFrame; frame++)
    {
        // Relative frame
        int relativeFrame = frame - mComputedStartFrame;

        // Do it only when layers were extracted for this frame
        if(mGPUSurfaces.at(relativeFrame)->layersExtracted())
        {
            // Calculate layer of atoms in rage for that frame
            float avgLayer = 0;
            float invAvgLayer = 0;
            for(int atomIndex = startIndex; atomIndex <= endIndex; atomIndex++)
            {
                // Get layer of that atom
                int layer = mGPUSurfaces.at(relativeFrame)->getLayerOfAtom(atomIndex);
                int invLayer = (mGPUSurfaces.at(relativeFrame)->getLayerCount() - 1) - layer;

                // Accumulate for average layer calculation
                avgLayer += (float)layer;
                invAvgLayer += (float)invLayer;
            }
            avgLayers.at(relativeFrame) = avgLayer / (float)((endIndex - startIndex) + 1); // divide through count of atoms
            invAvgLayers.at(relativeFrame) = invAvgLayer / (float)((endIndex - startIndex) + 1); // divide through count of atoms
        }
    }

    // Store deltas to vectors
    std::vector<float> avgLayersDelta = std::vector<float>(avgLayers.size() - 1, -1.f); // minus one means no data
    std::vector<float> invAvgLayersDelta = std::vector<float>(invAvgLayers.size() - 1, -1.f); // minus one means no data

    // Average layers delta accumulation
    float avgLayersDeltaAcc = 0.f;
    for(int i = 0; i < avgLayers.size() - 1; i++)
    {
        // Return false if any avg is not ok
        if(avgLayers.at(i) < 0 || avgLayers.at(i+1) < 0) { return false; }

        float delta = avgLayers.at(i + 1) - avgLayers.at(i);
        delta = delta < 0 ? -delta : delta;
        avgLayersDelta.at(i) = delta;
        avgLayersDeltaAcc += delta;
    }

    // Inverse average delta accumulation
    float invAvgLayersDeltaAcc = 0.f;
    for(int i = 0; i < invAvgLayers.size() - 1; i++)
    {
        // Return false if any invAvg is not ok
        if(invAvgLayers.at(i) < 0 || invAvgLayers.at(i+1) < 0) { return false; }

        float delta = invAvgLayers.at(i + 1) - invAvgLayers.at(i);
        delta = delta < 0 ? -delta : delta;
        invAvgLayersDelta.at(i) = delta;
        invAvgLayersDeltaAcc += delta;
    }

    // Return results
    rAcc = avgLayersDeltaAcc;
    rInverseAcc = invAvgLayersDeltaAcc;
    rAvgLayers = avgLayers;
    rInverseAvgLayers = invAvgLayers;
    rAvgLayersDelta = avgLayersDelta;
    rInverseAvgLayersDelta = invAvgLayersDelta;
    return true;
}

// ### Main function ###

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        Logger::instance().print("Please give PDB and optional XTC file as argument");
    }
    else
    {
        // Extract files to load
        std::string filepathPDB = argv[1];
        std::string filepathXTC;
        if(argc >= 3)
        {
            filepathXTC = argv[2];
        }

        // Create application and enter loop
        SurfaceDynamicsVisualization detection(filepathPDB, filepathXTC);
        detection.renderLoop();
    }

    // Exit
    return 0;
}
