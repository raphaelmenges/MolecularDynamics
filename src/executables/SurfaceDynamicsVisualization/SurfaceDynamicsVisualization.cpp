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

// ### Class implementation ###

SurfaceDynamicsVisualization::SurfaceDynamicsVisualization()
{
    // TODO: Test text-csv

    // # Setup members
    mCameraDeltaRotation = glm::vec2(0,0);
    mCameraRotationSmoothTime = 1.f;
    mLightDirection = glm::normalize(glm::vec3(-0.5, -0.75, -0.3));
    mWindowWidth = mInitialWindowWidth;
    mWindowHeight = mInitialWindowHeight;

    // Create window (which initializes OpenGL)
    mpWindow = generateWindow(mWindowTitle, mWindowWidth, mWindowHeight);

    // Init ImGui and load font
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
        0x0025, 0x0025, // percent
        0x2023, 0x2023, // triangle bullet
        0
    }; // has to be static to be available during run
    io.Fonts->AddFontFromFileTTF(fontpath.c_str(), 16, &config, ranges);

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

    // # Load protein (outcommented must be tested again, may not work)

    /*
    // Path to protein molecule
    std::vector<std::string> paths;
    // paths.push_back(std::string(RESOURCES_PATH) + "/molecules/PDB/1crn.pdb");
    paths.push_back(std::string(RESOURCES_PATH) + "/molecules/PDB/1a19.pdb");
    // paths.push_back(std::string(RESOURCES_PATH) + "/molecules/PDB/1vis.pdb");
    // paths.push_back(std::string(RESOURCES_PATH) + "/molecules/PDB/2mp3.pdb");
    // paths.push_back(std::string(RESOURCES_PATH) + "/molecules/PDB/4d2i.pdb");
    // paths.push_back(std::string(RESOURCES_PATH) + "/molecules/PDB/2AtomsIntersection.pdb");
    // paths.push_back(std::string(RESOURCES_PATH) + "/molecules/PDB/3AtomsIntersection.pdb");
    // paths.push_back(std::string(RESOURCES_PATH) + "/molecules/PDB/7AtomsIntersection.pdb");
    // paths.push_back(std::string(RESOURCES_PATH) + "/molecules/PDB/8AtomsIntersection.pdb");

    // Load protein
    MdTrajWrapper mdwrap;
    std::unique_ptr<Protein> upProtein = std::move(mdwrap.load(paths));

    // Get min/max extent of protein
    upProtein->minMax(); // first, one has to calculate min and max value of protein
    mProteinMinExtent = upProtein->getMin();
    mProteinMaxExtent = upProtein->getMax();

    */

    /*
    // Simple PDB loader (TODO: GPUProtein adaption)
    // mAtomStructs = parseSimplePDB(std::string(RESOURCES_PATH) + "/molecules/SimplePDB/PDB_Polymerase-of-E-coli-DNA.txt", mProteinMinExtent, mProteinMaxExtent);
    mAtomStructs = parseSimplePDB(std::string(RESOURCES_PATH) + "/molecules/SimplePDB/PDB_Myoglobin.txt", mProteinMinExtent, mProteinMaxExtent);
    // mAtomStructs = parseSimplePDB(std::string(RESOURCES_PATH) + "/molecules/SimplePDB/PDB_Nitrogen-Paracoccus-Cytochrome-C550.txt", mProteinMinExtent, mProteinMaxExtent);
    // mAtomStructs = parseSimplePDB(std::string(RESOURCES_PATH) + "/molecules/SimplePDB/8AtomsIntersection.txt", mProteinMinExtent, mProteinMaxExtent);
    */

    /*
    // Load series of proteins
    MdTrajWrapper mdwrap;
    std::vector<std::string> paths;
    paths.push_back("/home/raphael/Temp/XTC/Output_0.pdb");
    //paths.push_back(std::string(RESOURCES_PATH) + "/molecules/PDB/1a19.pdb");
    std::unique_ptr<Protein> upProtein = std::move(mdwrap.load(paths));

    // Get min/max extent of protein
    upProtein->minMax(); // first, one has to calculate min and max value of protein
    mProteinMinExtent = upProtein->getMin();
    mProteinMaxExtent = upProtein->getMax();

    // Fill GPUProtein
    mGPUProteins.push_back(std::move(std::unique_ptr<GPUProtein>(new GPUProtein(upProtein.get()))));

    // Load other animation frames
    paths.clear();
    paths.push_back("/home/raphael/Temp/XTC/Output_1.pdb");
    upProtein = std::move(mdwrap.load(paths));
    mGPUProteins.push_back(std::move(std::unique_ptr<GPUProtein>(new GPUProtein(upProtein.get()))));

    paths.clear();
    paths.push_back("/home/raphael/Temp/XTC/Output_2.pdb");
    upProtein = std::move(mdwrap.load(paths));
    mGPUProteins.push_back(std::move(std::unique_ptr<GPUProtein>(new GPUProtein(upProtein.get()))));

    paths.clear();
    paths.push_back("/home/raphael/Temp/XTC/Output_3.pdb");
    upProtein = std::move(mdwrap.load(paths));
    mGPUProteins.push_back(std::move(std::unique_ptr<GPUProtein>(new GPUProtein(upProtein.get()))));

    paths.clear();
    paths.push_back("/home/raphael/Temp/XTC/Output_4.pdb");
    upProtein = std::move(mdwrap.load(paths));
    mGPUProteins.push_back(std::move(std::unique_ptr<GPUProtein>(new GPUProtein(upProtein.get()))));

    paths.clear();
    paths.push_back("/home/raphael/Temp/XTC/Output_5.pdb");
    upProtein = std::move(mdwrap.load(paths));
    mGPUProteins.push_back(std::move(std::unique_ptr<GPUProtein>(new GPUProtein(upProtein.get()))));
    */

    // Loading XTC
    MdTrajWrapper mdwrap;
    std::vector<std::string> paths;
    paths.push_back("/home/raphael/Temp/XTC/MD_GIIIA_No_Water.pdb");
    paths.push_back("/home/raphael/Temp/XTC/MD_GIIIA_No_Water.xtc");
    std::unique_ptr<Protein> upProtein = std::move(mdwrap.load(paths));
    mupGPUProtein = std::unique_ptr<GPUProtein>(new GPUProtein(upProtein.get()));

    // Get min/max extent of protein
    upProtein->minMax(); // first, one has to calculate min and max value of protein
    mProteinMinExtent = upProtein->getMin();
    mProteinMaxExtent = upProtein->getMax();

    // # Prepare framebuffers for rendering
    mupMoleculeFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(mWindowWidth, mWindowHeight));
    mupMoleculeFramebuffer->bind();
    mupMoleculeFramebuffer->addAttachment(Framebuffer::ColorFormat::RGBA); // color
    mupMoleculeFramebuffer->addAttachment(Framebuffer::ColorFormat::RGB); // picking index
    mupMoleculeFramebuffer->unbind();
    mupOverlayFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(mWindowWidth, mWindowHeight));
    mupOverlayFramebuffer->bind();
    mupOverlayFramebuffer->addAttachment(Framebuffer::ColorFormat::RGBA);
    mupOverlayFramebuffer->unbind();

    // # Prepare background cubemaps
    mCVCubemapTexture = createCubemap(
        std::string(RESOURCES_PATH) + "/cubemaps/CV/posx.png",
        std::string(RESOURCES_PATH) + "/cubemaps/CV/negx.png",
        std::string(RESOURCES_PATH) + "/cubemaps/CV/posy.png",
        std::string(RESOURCES_PATH) + "/cubemaps/CV/negy.png",
        std::string(RESOURCES_PATH) + "/cubemaps/CV/posz.png",
        std::string(RESOURCES_PATH) + "/cubemaps/CV/negz.png");

    mBeachCubemapTexture = createCubemap(
        std::string(RESOURCES_PATH) + "/cubemaps/NissiBeach/posx.jpg",
        std::string(RESOURCES_PATH) + "/cubemaps/NissiBeach/negx.jpg",
        std::string(RESOURCES_PATH) + "/cubemaps/NissiBeach/posy.jpg",
        std::string(RESOURCES_PATH) + "/cubemaps/NissiBeach/negy.jpg",
        std::string(RESOURCES_PATH) + "/cubemaps/NissiBeach/posz.jpg",
        std::string(RESOURCES_PATH) + "/cubemaps/NissiBeach/negz.jpg");    

    // # Create camera
    glm::vec3 cameraCenter = (mProteinMinExtent + mProteinMaxExtent) / 2.f;
    float cameraRadius = glm::compMax(mProteinMaxExtent - cameraCenter);
    mupCamera = std::unique_ptr<OrbitCamera>(
        new OrbitCamera(
            cameraCenter,
            mCameraDefaultAlpha,
            mCameraDefaultBeta,
            cameraRadius,
            cameraRadius / 2.f,
            5.f * cameraRadius,
            45.f,
            0.1f));

    // # Other

    // Create path for analysis group
    mupPath = std::unique_ptr<Path>(new Path());

    // Create empty outline atoms indices after OpenGL initialization
    mupOutlineAtomIndices = std::unique_ptr<GPUTextureBuffer>(new GPUTextureBuffer(0)); // create empty outline atom indices buffer

    // Construct GPUSurfaceExtraction object after OpenGL has been initialized
    mupGPUSurfaceExtraction = std::unique_ptr<GPUSurfaceExtraction>(new GPUSurfaceExtraction);

    // Ascension
    mupAscension = std::unique_ptr<GPUBuffer<GLfloat> >(new GPUBuffer<GLfloat>);

    // Hull samples
    mupHullSamples = std::unique_ptr<GPUHullSamples>(new GPUHullSamples());

    // Prepare validation of the surface
    mupSurfaceValidation = std::unique_ptr<SurfaceValidation>(new SurfaceValidation());

    // Set endframe in GUI to maximum number
    mEndFrame = mupGPUProtein->getFrameCount() - 1;
}

SurfaceDynamicsVisualization::~SurfaceDynamicsVisualization()
{
    // Delete cubemaps
    glDeleteTextures(1, &mCVCubemapTexture);
    glDeleteTextures(1, &mBeachCubemapTexture);
}

void SurfaceDynamicsVisualization::renderLoop()
{
    // Setup OpenGL
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    // Cursor
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

    // Shader program for screenfilling quad rendering
    ShaderProgram screenFillingProgram("/SurfaceDynamicsVisualization/screenfilling.vert", "/SurfaceDynamicsVisualization/screenfilling.geom", "/SurfaceDynamicsVisualization/screenfilling.frag");

    // Shader program for cubemap
    ShaderProgram cubemapProgram("/SurfaceDynamicsVisualization/cubemap.vert", "/SurfaceDynamicsVisualization/cubemap.geom", "/SurfaceDynamicsVisualization/cubemap.frag");

    // Shader program for outline rendering
    ShaderProgram outlineProgram("/SurfaceDynamicsVisualization/hull.vert", "/SurfaceDynamicsVisualization/impostor.geom", "/SurfaceDynamicsVisualization/outline.frag");

    // Call render function of Rendering.h with lambda function
    render(mpWindow, [&] (float deltaTime)
    {
        // Viewport size
        glm::vec2 resolution = getResolution(mpWindow);
        mWindowWidth = resolution.x;
        mWindowHeight = resolution.y;

        // # Update everything before drawing

        // ### MOLECULE ANIMATION ##################################################################################

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
                    if(mRepeatAnimation && nextFrame > mEndFrame)
                    {
                        nextFrame = mStartFrame;
                    }

                    // Increment time (checks are done by method)
                    if(!setFrame(nextFrame))
                    {
                        mPlayAnimation = false;
                    }
                }
            }
        }

        // ### CAMERA UPDATE #######################################################################################

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
                + (deltaTime * mupCamera->getRadius()
                    * (((float)cursorDeltaX * a)
                        + ((float)cursorDeltaY * b))));

            lockCursorPosition = true;
        }

        // Automatic setting of camera center if wished
        if(mAutoCenterCamera)
        {
            // Smooth center by taking average of multiple
            int startFrame = glm::max(0, mFrame - mCameraSmoothFrameRadius);
            int endFrame = glm::min(mupGPUProtein->getFrameCount() - 1, mFrame + mCameraSmoothFrameRadius);
            glm::vec3 accCenter(0, 0, 0);
            for(int i = startFrame; i <= endFrame; i++)
            {
                accCenter += mupGPUProtein->getCenterOfMass(mFrame);
            }
            mupCamera->setCenter(accCenter / (float)((endFrame - startFrame) + 1.f));
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

        // ### OVERLAY RENDERING ###################################################################################

        // Prepare for rendering by setting viewport to full window resolution
        glViewport(0, 0, mWindowWidth, mWindowHeight);

        // # Fill overlay framebuffer
        mupOverlayFramebuffer->bind();
        mupOverlayFramebuffer->resize(mWindowWidth, mWindowHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Get count of atoms which will get a outline (size of buffer can be used here because all elements are valid)
        int outlineAtomCount = mupOutlineAtomIndices->getSize();
        if(frameComputed() && mRenderOutline && (outlineAtomCount > 0))
        {
            // Bind buffers of radii and trajectory for rendering
            mupGPUProtein->bind(0, 1);

            // Bind texture buffer with input atoms
            mupOutlineAtomIndices->bindAsImage(2, GPUAccess::READ_ONLY);

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

        // Hull samples
        if(frameComputed() && mRenderHullSamples)
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

        // Unbind framebuffer for overlay
        mupOverlayFramebuffer->unbind();

        // ### MOLECULE RENDERING ##################################################################################

        // # Fill molecule framebuffer
        mupMoleculeFramebuffer->bind();
        mupMoleculeFramebuffer->resize(mWindowWidth, mWindowHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Bind buffers of radii and trajectory for rendering
        mupGPUProtein->bind(0, 1);

        // Decide about surface rendering
        if(frameComputed())
        {
            // Frame is computed, decide how to render it
            switch(mSurfaceRendering)
            {
            case SurfaceRendering::HULL:

                // Prepare shader program
                hullProgram.use();
                hullProgram.update("view", mupCamera->getViewMatrix());
                hullProgram.update("projection", mupCamera->getProjectionMatrix());
                hullProgram.update("cameraWorldPos", mupCamera->getPosition());
                hullProgram.update("probeRadius", mRenderWithProbeRadius ? mComputedProbeRadius : 0.f);
                hullProgram.update("lightDir", mLightDirection);
                hullProgram.update("selectedIndex", mSelectedAtom);
                hullProgram.update("clippingPlane", mClippingPlane);
                hullProgram.update("frame", mFrame);
                hullProgram.update("atomCount", mupGPUProtein->getAtomCount());
                hullProgram.update("smoothAnimationRadius", mSmoothAnimationRadius);
                hullProgram.update("smoothAnimationMaxDeviation", mSmoothAnimationMaxDeviation);
                hullProgram.update("frameCount", mupGPUProtein->getFrameCount());
                hullProgram.update("depthDarkeningStart", mDepthDarkeningStart);
                hullProgram.update("depthDarkeningEnd", mDepthDarkeningEnd);

                // Draw internal (first, because at clipping plane are all set to same
                // viewport depth which means internal are always in front of surface)
                if(mShowInternal)
                {
                    mGPUSurfaces.at(mFrame - mComputedStartFrame)->bindInternalIndices(mLayer, 2);
                    hullProgram.update("color", mInternalAtomColor);
                    glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfInternalAtoms(mLayer));
                }

                // Draw surface
                if(mShowSurface)
                {
                    mGPUSurfaces.at(mFrame - mComputedStartFrame)->bindSurfaceIndices(mLayer, 2);
                    hullProgram.update("color", mSurfaceAtomColor);
                    glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfSurfaceAtoms(mLayer));
                }

                break;

            case SurfaceRendering::ASCENSION:

                // Bind ascension buffer
                mupAscension->bind(2);

                // Prepare shader program
                ascensionProgram.use();
                ascensionProgram.update("view", mupCamera->getViewMatrix());
                ascensionProgram.update("projection", mupCamera->getProjectionMatrix());
                ascensionProgram.update("cameraWorldPos", mupCamera->getPosition());
                ascensionProgram.update("probeRadius", mRenderWithProbeRadius ? mComputedProbeRadius : 0.f);
                ascensionProgram.update("lightDir", mLightDirection);
                ascensionProgram.update("selectedIndex", mSelectedAtom);
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
                glDrawArrays(GL_POINTS, 0, mupGPUProtein->getAtomCount());

                break;

            case SurfaceRendering::ELEMENTS:

                // Bind coloring
                mupGPUProtein->bindColorsElement(3);

                // Prepare shader program
                coloringProgram.use();
                coloringProgram.update("view", mupCamera->getViewMatrix());
                coloringProgram.update("projection", mupCamera->getProjectionMatrix());
                coloringProgram.update("cameraWorldPos", mupCamera->getPosition());
                coloringProgram.update("probeRadius", mRenderWithProbeRadius ? mComputedProbeRadius : 0.f);
                coloringProgram.update("lightDir", mLightDirection);
                coloringProgram.update("selectedIndex", mSelectedAtom);
                coloringProgram.update("clippingPlane", mClippingPlane);
                coloringProgram.update("frame", mFrame);
                coloringProgram.update("atomCount", mupGPUProtein->getAtomCount());
                coloringProgram.update("smoothAnimationRadius", mSmoothAnimationRadius);
                coloringProgram.update("smoothAnimationMaxDeviation", mSmoothAnimationMaxDeviation);
                coloringProgram.update("frameCount", mupGPUProtein->getFrameCount());
                coloringProgram.update("depthDarkeningStart", mDepthDarkeningStart);
                coloringProgram.update("depthDarkeningEnd", mDepthDarkeningEnd);

                // Draw internal (first, because at clipping plane are all set to same
                // viewport depth which means internal are always in front of surface)
                if(mShowInternal)
                {
                    mGPUSurfaces.at(mFrame - mComputedStartFrame)->bindInternalIndices(mLayer, 2);
                    glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfInternalAtoms(mLayer));
                }

                // Draw surface
                if(mShowSurface)
                {
                    mGPUSurfaces.at(mFrame - mComputedStartFrame)->bindSurfaceIndices(mLayer, 2);
                    glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfSurfaceAtoms(mLayer));
                }

                break;

            case SurfaceRendering::AMINOACIDS:

                // Bind coloring
                mupGPUProtein->bindColorsAminoacid(3);

                // Prepare shader program
                coloringProgram.use();
                coloringProgram.update("view", mupCamera->getViewMatrix());
                coloringProgram.update("projection", mupCamera->getProjectionMatrix());
                coloringProgram.update("cameraWorldPos", mupCamera->getPosition());
                coloringProgram.update("probeRadius", mRenderWithProbeRadius ? mComputedProbeRadius : 0.f);
                coloringProgram.update("lightDir", mLightDirection);
                coloringProgram.update("selectedIndex", mSelectedAtom);
                coloringProgram.update("clippingPlane", mClippingPlane);
                coloringProgram.update("frame", mFrame);
                coloringProgram.update("atomCount", mupGPUProtein->getAtomCount());
                coloringProgram.update("smoothAnimationRadius", mSmoothAnimationRadius);
                coloringProgram.update("smoothAnimationMaxDeviation", mSmoothAnimationMaxDeviation);
                coloringProgram.update("frameCount", mupGPUProtein->getFrameCount());
                coloringProgram.update("depthDarkeningStart", mDepthDarkeningStart);
                coloringProgram.update("depthDarkeningEnd", mDepthDarkeningEnd);

                // Draw internal (first, because at clipping plane are all set to same
                // viewport depth which means internal are always in front of surface)
                if(mShowInternal)
                {
                    mGPUSurfaces.at(mFrame - mComputedStartFrame)->bindInternalIndices(mLayer, 2);
                    glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfInternalAtoms(mLayer));
                }

                // Draw surface
                if(mShowSurface)
                {
                    mGPUSurfaces.at(mFrame - mComputedStartFrame)->bindSurfaceIndices(mLayer, 2);
                    glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfSurfaceAtoms(mLayer));
                }

                break;

            case SurfaceRendering::ANALYSIS:

                // Bind indices of analysis atoms
                mupOutlineAtomIndices->bindAsImage(2, GPUAccess::READ_ONLY);

                // Prepare shader program
                analysisProgram.use();
                analysisProgram.update("view", mupCamera->getViewMatrix());
                analysisProgram.update("projection", mupCamera->getProjectionMatrix());
                analysisProgram.update("cameraWorldPos", mupCamera->getPosition());
                analysisProgram.update("probeRadius", mRenderWithProbeRadius ? mComputedProbeRadius : 0.f);
                analysisProgram.update("lightDir", mLightDirection);
                analysisProgram.update("selectedIndex", mSelectedAtom);
                analysisProgram.update("clippingPlane", mClippingPlane);
                analysisProgram.update("frame", mFrame);
                analysisProgram.update("atomCount", mupGPUProtein->getAtomCount());
                analysisProgram.update("smoothAnimationRadius", mSmoothAnimationRadius);
                analysisProgram.update("smoothAnimationMaxDeviation", mSmoothAnimationMaxDeviation);
                analysisProgram.update("frameCount", mupGPUProtein->getFrameCount());
                analysisProgram.update("depthDarkeningStart", mDepthDarkeningStart);
                analysisProgram.update("depthDarkeningEnd", mDepthDarkeningEnd);
                analysisProgram.update("groupAtomCount", (int)mupOutlineAtomIndices->getSize());
                glDrawArrays(GL_POINTS, 0, mupGPUProtein->getAtomCount());

                break;
            }
        }
        else
        {
            // Render it with fallback shader
            fallbackProgram.use();
            fallbackProgram.update("view", mupCamera->getViewMatrix());
            fallbackProgram.update("projection", mupCamera->getProjectionMatrix());
            fallbackProgram.update("cameraWorldPos", mupCamera->getPosition());
            fallbackProgram.update("probeRadius", mRenderWithProbeRadius ? mComputedProbeRadius : 0.f);
            fallbackProgram.update("lightDir", mLightDirection);
            fallbackProgram.update("selectedIndex", mSelectedAtom);
            fallbackProgram.update("clippingPlane", mClippingPlane);
            fallbackProgram.update("frame", mFrame);
            fallbackProgram.update("atomCount", mupGPUProtein->getAtomCount());
            fallbackProgram.update("smoothAnimationRadius", mSmoothAnimationRadius);
            fallbackProgram.update("smoothAnimationMaxDeviation", mSmoothAnimationMaxDeviation);
            fallbackProgram.update("frameCount", mupGPUProtein->getFrameCount());
            fallbackProgram.update("depthDarkeningStart", mDepthDarkeningStart);
            fallbackProgram.update("depthDarkeningEnd", mDepthDarkeningEnd);
            fallbackProgram.update("color", mFallbackAtomColor);
            glDrawArrays(GL_POINTS, 0, mupGPUProtein->getAtomCount());
        }

        // Unbind molecule framebuffer
        mupMoleculeFramebuffer->unbind();

        // ### COMPOSITING #########################################################################################

        // # Fill standard framebuffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Disable depth test
        glDisable(GL_DEPTH_TEST);

        // Render cubemap
        glDisable(GL_DEPTH_TEST);
        glActiveTexture(GL_TEXTURE0);

        switch(mBackground)
        {
        case Background::COMPUTERVISUALISTIK:
            glBindTexture(GL_TEXTURE_CUBE_MAP, mCVCubemapTexture);
            break;
        case Background::BEACH:
            glBindTexture(GL_TEXTURE_CUBE_MAP, mBeachCubemapTexture);
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

        // Bind overlay framebuffer texture
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mupOverlayFramebuffer->getAttachment(0));

        // Draw screenfilling quad
        screenFillingProgram.use();
        screenFillingProgram.update("molecule", 0); // tell shader which slot to use
        screenFillingProgram.update("overlay", 1); // tell shader which slot to use
        glDrawArrays(GL_POINTS, 0, 1);

        // Back to opaque rendering
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);

        // Enable depth test
        glEnable(GL_DEPTH_TEST);

        // Render GUI in standard frame buffer on top of everything
        ImGui_ImplGlfwGL3_NewFrame();
        renderGUI();
    });

    // Delete OpenGL structures
    glDeleteVertexArrays(1, &axisGizmoVAO);
    glDeleteBuffers(1, &axisGizmoVBO);
}

void SurfaceDynamicsVisualization::setProgressDispaly(std::string task, float progress)
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
            case GLFW_KEY_ESCAPE: { glfwSetWindowShouldClose(mpWindow, GL_TRUE); break; }
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
    mupCamera->setRadius(mupCamera->getRadius() - 0.5f * (float)yoffset);
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

void SurfaceDynamicsVisualization::renderGUI()
{
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f)); // window background
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.2f, 0.2f, 0.2f, 0.25f)); // menu bar background

    // Main menu bar
    if (ImGui::BeginMainMenuBar())
    {
        // General menu
        if (ImGui::BeginMenu("Menu"))
        {
            if(ImGui::MenuItem("Quit", "Esc", false, true)) { glfwSetWindowShouldClose(mpWindow, GL_TRUE); }
            ImGui::EndMenu();
        }

        // Window menu
        if (ImGui::BeginMenu("Window"))
        {
            // Computation window
            if(mShowSurfaceExtractionWindow)
            {
                if(ImGui::MenuItem("Hide Computation", "", false, true)) { mShowSurfaceExtractionWindow = false; }
            }
            else
            {
                if(ImGui::MenuItem("Show Computation", "", false, true)) { mShowSurfaceExtractionWindow = true; }
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

    ImGui::PopStyleColor(); // menu bar background
    ImGui::PopStyleColor(); // window background
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.5f, 0.5f, 0.5f, 0.5f)); // window title
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.5f, 0.5f, 0.5f, 0.5f)); // window title collapsed
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.1f, 0.1f, 0.1f, 0.75f)); // window title active
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.0f, 0.0f, 0.0f, 0.5f)); // scrollbar grab
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // scrollbar background
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 0.5f)); // button
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.75f)); // button hovered
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.5f, 0.5f, 0.5f, 0.5f)); // header
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.5f, 0.5f, 0.5f, 0.75f)); // header hovered
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.1f, 0.1f, 0.1f, 0.75f)); // header active
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.1f, 0.1f, 0.1f, 0.75f)); // slider grab active

    // Extraction window
    if(mShowSurfaceExtractionWindow)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.5f, 0.0f, 0.0f, 0.75f)); // window background
        ImGui::Begin("Computation", NULL, 0);

        // Computatiom
        ImGui::Text("[Computation]");
        ImGui::SliderFloat("Probe Radius", &mComputationProbeRadius, 0.f, 2.f, "%.1f");
        ImGui::Checkbox("Extract Layers", &mExtractLayers);
        ImGui::SliderInt("Start Frame", &mComputationStartFrame, mStartFrame, mComputationEndFrame);
        ImGui::SliderInt("End Frame", &mComputationEndFrame, mComputationStartFrame, mEndFrame);
        ImGui::SliderInt("Sample Count", &mHullSampleCount, 0, 1000);
        if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Count of samples used for analysis purposes, not surface extraction."); }
        ImGui::SliderInt("CPU Threads", &mCPUThreads, 1, 24);
        if(ImGui::Button("\u2794 GPGPU")) { computeLayers(true); }
        ImGui::SameLine();
        if(ImGui::Button("\u2794 CPU")) { computeLayers(false); }
        ImGui::Separator();

        // Report
        if (ImGui::CollapsingHeader("Report"))
        {
            ImGui::Text(mComputeInformation.c_str());
        }

        ImGui::End();
        ImGui::PopStyleColor(); // window background
    }

    // Camera window
    if(mShowCameraWindow)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.5f, 0.75f)); // window background
        ImGui::Begin("Camera", NULL, 0);

        // Camera
        ImGui::Text("[Orthographic Camera]");

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

        // Automatic put center of camera in protein's center
        if(mAutoCenterCamera)
        {
            if(ImGui::Button("Manual Center", ImVec2(100, 22)))
            {
                mAutoCenterCamera = false;
            }
        }
        else
        {
            if(ImGui::Button("Auto Center", ImVec2(100, 22)))
            {
                mAutoCenterCamera = true;
            }
        }
        ImGui::SameLine();

        // Reset
        if(ImGui::Button("Reset Camera", ImVec2(100, 22)))
        {
            mupCamera->setAlpha(mCameraDefaultAlpha);
            mupCamera->setBeta(mCameraDefaultBeta);
            mupCamera->setCenter(glm::vec3(0));
        }
        ImGui::Separator();

        // Clipping
        ImGui::Text("[Clipping Plane]");
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

        ImGui::End();
        ImGui::PopStyleColor(); // window background
    }

    // Visualization window
    if(mShowVisualizationWindow)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.5f, 0.5f, 0.0f, 0.75f)); // window background
        ImGui::Begin("Visualization", NULL, 0);

        // Animation
        ImGui::Text("[Animation]");
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

        ImGui::Checkbox("Repeat", &mRepeatAnimation);

        ImGui::SliderInt("Rate", &mPlayAnimationRate, 0, 100);
        int frame = mFrame;
        ImGui::SliderInt("Frame", &frame, mStartFrame, mEndFrame);
        if(frame != mFrame)
        {
            setFrame(frame);
        }
        ImGui::Separator();

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

        // Stuff that is only possible on computed frames
        if(frameComputed())
        {
            // Layer
            ImGui::Text("[Layer]");
            ImGui::SliderInt("Layer", &mLayer, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getLayerCount() - 1);
            ImGui::Separator();

            // Rendering
            ImGui::Text("[Rendering]");

            // Surface rendering
            ImGui::Combo("##SurfaceRenderingCombo", (int*)&mSurfaceRendering, "Hull\0Ascension\0Elements\0Aminoacids\0Analysis\0");

            // Rendering of internal and surface atoms
            if(mSurfaceRendering != SurfaceRendering::ASCENSION)
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
        }

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

        ImGui::Separator();

        // Animation smoothing
        ImGui::Text("[Animation Smoothing]");
        ImGui::SliderInt("Smooth Radius", &mSmoothAnimationRadius, 0, 10);
        ImGui::SliderFloat("Smooth Max Deviation", &mSmoothAnimationMaxDeviation, 0, 100, "%.1f");

        ImGui::End();
        ImGui::PopStyleColor(); // window background
    }

    // Information window
    if(mShowInformationWindow)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.5f, 0.0f, 0.75f)); // window background
        ImGui::Begin("Information", NULL, 0);

        // General infos
        ImGui::Text("[General]");
        ImGui::Text(std::string("Atom Count: " + std::to_string(mupGPUProtein->getAtomCount())).c_str());
        // TODO: move to analysis: ImGui::Text(std::string("Internal Atoms: " + std::to_string(mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfInternalAtoms(mLayer))).c_str());
        // TODO: move to analysis: ImGui::Text(std::string("Surface Atoms: " + std::to_string(mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfSurfaceAtoms(mLayer))).c_str());
        ImGui::Separator();

        // Selection infos
        ImGui::Text("[Selection]");
        ImGui::Text(std::string("Index: " + std::to_string(mSelectedAtom)).c_str());
        ImGui::Text(std::string("Element: " + mupGPUProtein->getElement(mSelectedAtom)).c_str());
        ImGui::Text(std::string("Aminoacid: " + mupGPUProtein->getAminoacid(mSelectedAtom)).c_str());
        ImGui::Separator();

        // Hardware infos
        ImGui::Text("[Hardware]");

        // Show available GPU memory
        int availableMemory;
        glGetIntegerv(0x9049, &availableMemory); // Nvidia only
        availableMemory = availableMemory / 1000;
        ImGui::Text(std::string("Available VRAM: " + std::to_string(availableMemory) + "MB").c_str());
        // TODO: one may want to clean up glGetError if query failed and show failure message on GUI (for example on Intel or AMD)

        ImGui::End();
        ImGui::PopStyleColor(); // window background
    }

    // Validation window
    if(mShowValidationWindow)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.5f, 0.5f, 0.75f)); // window background
        ImGui::Begin("Validation", NULL, 0);

        // Do validation
        ImGui::SliderInt("Samples", &mSurfaceValidationAtomSampleCount, 1, 10000);
        ImGui::SliderInt("Seed", &mSurfaceValidationSeed, 0, 1337);
        if(frameComputed() && ImGui::Button("Validate Surface"))
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
        else
        {
            ImGui::Text("Frame Not Computed");
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
        if (ImGui::CollapsingHeader("Report"))
        {
            ImGui::Text(mValidationInformation.c_str());
        }

        ImGui::End();
        ImGui::PopStyleColor(); // window background
    }

    // Analysis window
    if(mShowAnalysisWindow)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.5f, 0.0f, 0.5f, 0.75f)); // window background
        ImGui::Begin("Analysis", NULL, 0);

        // Only display when frame is computed
        if(frameComputed())
        {
            // Some variables for this window
            bool doUpdatePath = false;

            // Analysis of global
            ImGui::Text("[Global]");

            // Graph about relation of surface and internal samples (in relative frames)
            // TODO: stupid to calculate it every frame and bad texts
            float globalSampleCount = (float)mupHullSamples->getSampleCount();
            auto sampleCount = mupHullSamples->getSurfaceSampleCount();
            std::vector<float> floatSampleAmount;
            floatSampleAmount.reserve(sampleCount.size());
            for(int i = 0; i < sampleCount.size(); i++)
            {
                floatSampleAmount.push_back((float)sampleCount.at(i) / globalSampleCount);
            }
            ImGui::PlotLines("Surface Samples", floatSampleAmount.data(), floatSampleAmount.size());
            ImGui::Text(std::string("Surface Amount: " + std::to_string(floatSampleAmount.at(mFrame - mComputedStartFrame) * 100) + " percent").c_str());

            // Surface area of molecule
            ImGui::Text(std::string("Approximate Surface Area: " + std::to_string(approximateSurfaceArea()) + "\u212b²").c_str());

            ImGui::Separator();

            // Analysis of group
            ImGui::Text("[Group]");
            ImGui::BeginChild(
                "AnalyseAtoms",
                ImVec2(ImGui::GetWindowContentRegionWidth() * 1.0f, 100),
                false,
                ImGuiWindowFlags_HorizontalScrollbar);

            // Useful variables
            std::vector<int> toBeRemoved;
            bool analysisAtomsChanged = false;

            // Go over analyse atoms and list them
            for (int atomIndex : mAnalyseAtoms)
            {
                // Mark if currently selected
                if(atomIndex == mSelectedAtom)
                {
                    ImGui::TextColored(ImVec4(0,1,0,1), "\u2023");
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
                if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Select Atom"); } // tooltip
                ImGui::SameLine();

                // Remove that atom from analysis atoms (use ## to add number for an unique button id)
                if(ImGui::Button(std::string("\u00D7##" + std::to_string(atomIndex)).c_str()))
                {
                    toBeRemoved.push_back(atomIndex);
                    analysisAtomsChanged = true;
                }
                if(ImGui::IsItemHovered() && mShowTooltips) { ImGui::SetTooltip("Remove Atom"); } // tooltip
            }

            // Remove atoms from analysis
            for(int atomIndex : toBeRemoved) { mAnalyseAtoms.erase(atomIndex); }
            ImGui::EndChild();

            // Add new atoms to analyse
            ImGui::InputInt("", &mNextAnalyseAtomIndex);
            mNextAnalyseAtomIndex = glm::clamp(mNextAnalyseAtomIndex, 0, mupGPUProtein->getAtomCount());
            ImGui::SameLine();
            if(ImGui::Button("Add Atom"))
            {
                // Add atom to list of analyse atoms
                mAnalyseAtoms.insert((GLuint)mNextAnalyseAtomIndex);
                analysisAtomsChanged = true;
            }

            // Recreate outline atom indices and path visualization
            if(analysisAtomsChanged)
            {
                // Outline
                std::vector<GLuint> analyseAtomVector;
                std::copy(mAnalyseAtoms.begin(), mAnalyseAtoms.end(), std::back_inserter(analyseAtomVector));
                mupOutlineAtomIndices = std::unique_ptr<GPUTextureBuffer>(new GPUTextureBuffer(analyseAtomVector));

                // Path
                doUpdatePath = true;
            }

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

            // Length of complete path
            std::ostringstream stringPathLength;
            stringPathLength << std::fixed << std::setprecision(2) << mupPath->getCompleteLength();
            ImGui::Text(std::string("Path Complete Length: " + stringPathLength.str() + " \u212b").c_str());

            // Length of displayed path
            int startFrame = glm::max(0, mFrame - mPathFrameRadius);
            int endFrame = glm::min(mupPath->getVertexCount()-1, mFrame + mPathFrameRadius);
            stringPathLength = std::ostringstream();
            stringPathLength << std::fixed << std::setprecision(2) << mupPath->getLength(startFrame, endFrame);
            ImGui::Text(std::string("Path Displayed Length: " + stringPathLength.str() + " \u212b").c_str());

            // Radius of frames which are taken into account for path smoothing
            int radius = mPathSmoothRadius;
            ImGui::SliderInt("Path Smooth Radius", &radius, 0, 10);
            if(radius != mPathSmoothRadius)
            {
                mPathSmoothRadius = radius; // save new radius
                doUpdatePath = true; // remember to update display path
            }

            // Radius of frames in path visualization
            ImGui::SliderInt("Path Radius", &mPathFrameRadius, 1, 100);

            // Amount of surface covered by analysis group
            // TODO: very stupid to do every frame and bad texts
            if(mAnalyseAtoms.size() > 0)
            {
                // TODO does not work right now
                /*
                std::vector<float> floatGroupSurfaceSampleAmount;
                int computedFramesCount = mComputedEndFrame - mComputedStartFrame + 1;
                floatGroupSurfaceSampleAmount.reserve(computedFramesCount);
                for(int i = 0; i < computedFramesCount; i++)
                {

                    floatGroupSurfaceSampleAmount.push_back(
                        (float)mupHullSamples->getSurfaceSampleCount(i, mAnalyseAtoms) // sample count for group
                        / (float)mupHullSamples->getSurfaceSampleCount(i)); // sample count for all atoms

                }
                ImGui::PlotLines("Group Samples", floatGroupSurfaceSampleAmount.data(), floatGroupSurfaceSampleAmount.size());
                ImGui::Text(std::string("Surface Amount: " + std::to_string(floatGroupSurfaceSampleAmount.at(mFrame - mComputedStartFrame) * 100) + " percent").c_str());
                */
            }

            // Update path if necessary
            if(doUpdatePath)
            {
                mupPath->update(mupGPUProtein.get(), mAnalyseAtoms, mPathSmoothRadius);
            }
        }

        ImGui::End();
        ImGui::PopStyleColor(); // window background
    }

    // Rendering window
    if(mShowRenderingWindow)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.25f, 0.25f, 0.25f, 0.75f)); // window background
        ImGui::Begin("Rendering", NULL, 0);

        // Background
        ImGui::Combo("Background", (int*)&mBackground, "Computervisualistik\0Beach\0");

        // Lighting
        if(ImGui::Button("Spot Light"))
        {
            mLightDirection = -glm::normalize(mupCamera->getPosition() - mupCamera->getCenter());
        }

        // Depth darkening
        ImGui::SliderFloat("Depth Darkening Start", &mDepthDarkeningStart, 0, mDepthDarkeningEnd, "%.1f");
        ImGui::SliderFloat("Depth Darkening End", &mDepthDarkeningEnd, mDepthDarkeningStart, mDepthDarkeningMaxEnd, "%.1f");

        ImGui::End();
        ImGui::PopStyleColor(); // window background
    }

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
        setProgressDispaly("Surface Extraction", progress);
    }

    // Update compute information
    updateComputationInformation(
        (useGPU ? "GPU" : "CPU with " + std::to_string(mCPUThreads) + " threads"), computationTime);

    // # Ascension calculation

    // Calculate ascension for visualization
    int atomCount = mupGPUProtein->getAtomCount();
    std::vector<float> ascension; // linear accumulation of ascension for all computed frames and all atoms
    ascension.reserve(atomCount * (int)mGPUSurfaces.size());
    GLuint i = 0; // ascension frame count
    float pi = glm::pi<float>();
    float upToHot = (1.f / mAscensionUpToHotFrameCount) * pi;
    float backToHot = (1.f / mAscensionBackToHotFrameCount) * pi;
    float upToCold = (1.f / mAscensionUpToColdFrameCount) * pi;
    float backToCold = (1.f / mAscensionBackToColdFrameCount) * pi;

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

            if(i == 0)
            {
                // Push back value for first frame of ascension (either at surface or not)
                value = surface ? pi : 0.f;
            }
            else
            {
                // Fetch value of previous frame
                float previousValue = ascension.at(((i-1) * atomCount + a));

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
                        value = glm::max(pi, previousValue - backToHot);
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
                        // On the way of getting hot a throwback
                        value = glm::max(0.f, previousValue - backToCold);
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
        i++;
    }

    // Fill ascension to texture buffer
    mupAscension->fill(ascension, GL_DYNAMIC_DRAW);

    // # Hull samples calculation
    mupHullSamples->compute(
        mupGPUProtein.get(),
        &mGPUSurfaces,
        mComputationStartFrame,
        mComputationProbeRadius,
        mHullSampleCount,
        0,
        [this](float progress) // [0,1]
        {
            this->setProgressDispaly("Sample Creation", progress);
        });

    // # Frame setting

    // Remember which frames were computed
    mComputedStartFrame = mComputationStartFrame;
    mComputedEndFrame = mComputationEndFrame;

    // Remember which probe radius was used
    mComputedProbeRadius = mComputationProbeRadius;

    // Set to first computed frame
    setFrame(mComputedStartFrame);
}

int SurfaceDynamicsVisualization::getAtomBeneathCursor() const
{
    // Bind correct framebuffer and point to attachment with pick indices
    mupMoleculeFramebuffer->bind();
    glReadBuffer(GL_COLOR_ATTACHMENT1);

    // Get position of cursor
    double cursorX, cursorY;
    glfwGetCursorPos(mpWindow, &cursorX, &cursorY);

    // Get pick index
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    unsigned char data[3];
    glReadPixels((int)cursorX, mWindowHeight - (int)cursorY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
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

float SurfaceDynamicsVisualization::approximateSurfaceArea() const
{
    // Go over all surface atoms' radii
    float surface = 0;
    auto atomIndices = mGPUSurfaces.at(mFrame - mComputedStartFrame)->getSurfaceIndices(0);
    for(int i = 0; i < atomIndices.size(); i++)
    {
        // Extract atom index
        int index = atomIndices.at(i);

        // Add surface
        float radius = mupGPUProtein->getRadii()->at(index);
        float atomSurface = 4.f * glm::pi<float>() * radius * radius;
        surface += atomSurface * ((float)mupHullSamples->getSurfaceSampleCount(mFrame, index) / (float)mupHullSamples->getProteinSampleCount());
    }

    return surface;
}

GLuint SurfaceDynamicsVisualization::createCubemap(
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
            std::cout << "Image file not found or error at parsing: " << cubemapFullpaths.at(i) << std::endl;
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

// ### Main function ###

int main()
{
    SurfaceDynamicsVisualization detection;
    detection.renderLoop();
    return 0;
}
