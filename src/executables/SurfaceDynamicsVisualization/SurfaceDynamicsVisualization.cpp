#include "SurfaceDynamicsVisualization.h"
#include "Utils/OrbitCamera.h"
#include "ShaderTools/Renderer.h"
#include "Molecule/MDtrajLoader/MdTraj/MdTrajWrapper.h"
#include "Molecule/MDtrajLoader/Data/Protein.h"
#include "SimpleLoader.h"
#include "imgui/imgui.h"
#include "imgui/examples/opengl3_example/imgui_impl_glfw_gl3.h"
#include <glm/gtx/component_wise.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>

// ### Class implementation ###

SurfaceDynamicsVisualization::SurfaceDynamicsVisualization()
{
    // # Setup members
    mCameraDeltaRotation = glm::vec2(0,0);
    mCameraRotationSmoothTime = 1.f;
    mLightDirection = glm::normalize(glm::vec3(-0.5, -0.75, -0.3));

    // Create window (which initializes OpenGL)
    mpWindow = generateWindow("Surface Dynamics Visualization");

    // Construct GPUSurfaceExtraction object after OpenGL has been initialized
    mupGPUSurfaceExtraction = std::unique_ptr<GPUSurfaceExtraction>(new GPUSurfaceExtraction);

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
        0
    }; // has to be static to be available during run
    io.Fonts->AddFontFromFileTTF(fontpath.c_str(), 16, &config, ranges);

    // Clear color
    glClearColor(0.1f, 0.1f, 0.3f, 1.f);

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

    // # Load protein

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

    // TODO: testing XTC loading
    MdTrajWrapper mdwrap;
    std::vector<std::string> paths;
    paths.push_back("/home/raphael/Temp/XTC/MD_GIIIA_No_Water.pdb");
    paths.push_back("/home/raphael/Temp/XTC/MD_GIIIA_No_Water.xtc");
    std::unique_ptr<Protein> upProtein = std::move(mdwrap.load(paths));
    for(int i = 0; i < upProtein->getAtomAt(0)->getCountOfFrames(); i++)
    {
        mGPUProteins.push_back(std::move(std::unique_ptr<GPUProtein>(new GPUProtein(upProtein.get(), i))));
    }

    // Get min/max extent of protein
    upProtein->minMax(); // first, one has to calculate min and max value of protein
    mProteinMinExtent = upProtein->getMin();
    mProteinMaxExtent = upProtein->getMax();

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
            0.05f));

    // # Run implementation to extract surface atoms
    computeLayers(0, 0, mInitiallyUseGLSLImplementation);

    // Prepare validation of the surface
    mupSurfaceValidation = std::unique_ptr<SurfaceValidation>(new SurfaceValidation());

    // Set endframe in GUI to maximum number
    mComputationEndFrame = mGPUProteins.size() - 1;
}

SurfaceDynamicsVisualization::~SurfaceDynamicsVisualization()
{
    // Nothing to do here
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

    // # Prepare shader programs for rendering the protein
    ShaderProgram pointProgram("/SurfaceDynamicsVisualization/point.vert", "/SurfaceDynamicsVisualization/point.geom", "/SurfaceDynamicsVisualization/point.frag");
    ShaderProgram impostorProgram("/SurfaceDynamicsVisualization/impostor.vert", "/SurfaceDynamicsVisualization/impostor.geom", "/SurfaceDynamicsVisualization/impostor.frag");

    // Call render function of Rendering.h with lambda function
    render(mpWindow, [&] (float deltaTime)
    {
        // Clear buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ImGui new frame
        ImGui_ImplGlfwGL3_NewFrame();

        // Viewport size
        glm::vec2 resolution = getResolution(mpWindow);
        glViewport(0, 0, resolution.x, resolution.y);

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
                    if(mRepeatAnimation && nextFrame > mComputedEndFrame)
                    {
                        nextFrame = mComputedStartFrame;
                    }

                    // Increment time (checks are done by method)
                    if(!setFrame(nextFrame))
                    {
                        mPlayAnimation = false;
                    }
                }
            }
        }

        // Bind SSBO with atoms (TODO: only at frame change)
        mGPUProteins.at(mFrame)->bind(0);

        // Calculate cursor movement
        double cursorX, cursorY;
        glfwGetCursorPos(mpWindow, &cursorX, &cursorY);
        GLfloat cursorDeltaX = (float)cursorX - prevCursorX;
        GLfloat cursorDeltaY = (float)cursorY - prevCursorY;
        prevCursorX = cursorX;
        prevCursorY = cursorY;

        // Rotate camera
        if(mRotateCamera)
        {
            mCameraDeltaRotation = glm::vec2(cursorDeltaX, cursorDeltaY);
            mCameraRotationSmoothTime = 1.f;
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
            glm::vec3 a = glm::cross(glm::vec3(0,1,0), mupCamera->getDirection()); // use up vector for cross product
            glm::vec3 b = glm::cross(mupCamera->getDirection(), a);

            mupCamera->setCenter(
                mupCamera->getCenter()
                + (deltaTime * mupCamera->getRadius()
                    * (((float)cursorDeltaX * a)
                        + ((float)cursorDeltaY * b))));
        }

        // Update camera
        mupCamera->update(resolution.x, resolution.y, mUsePerspectiveCamera);

        // Drawing of surface validation before everything else, so sample points at same z coordinate as impostor are in front
        if(mShowValidationSamples)
        {
            mupSurfaceValidation->drawSamples(
                mSamplePointSize,
                mInternalSamplePointColor,
                mSurfaceSamplePointColor,
                mupCamera->getViewMatrix(),
                mupCamera->getProjectionMatrix(),
                mClippingPlane,
                mShowInternalSamples,
                mShowSurfaceSamples);
        }

        // Drawing of molecule
        if(mRenderImpostor)
        {
            // Prepare impostor drawing
            impostorProgram.use();
            impostorProgram.update("view", mupCamera->getViewMatrix());
            impostorProgram.update("projection", mupCamera->getProjectionMatrix());
            impostorProgram.update("cameraWorldPos", mupCamera->getPosition());
            impostorProgram.update("probeRadius", mRenderWithProbeRadius ? mProbeRadius : 0.f);
            impostorProgram.update("lightDir", mLightDirection);
            impostorProgram.update("selectedIndex", mSelectedAtom);
            impostorProgram.update("clippingPlane", mClippingPlane);

            // Draw internal (first, because at clipping plane are all set to same
            // viewport depth which means internal are always in front of surface)
            if(mShowInternal)
            {
                mGPUSurfaces.at(mFrame - mComputedStartFrame)->bindInternalIndicesForDrawing(mLayer, 1);
                impostorProgram.update("color", mInternalAtomColor);
                glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfInternalAtoms(mLayer));
            }

            // Draw surface
            if(mShowSurface)
            {
                mGPUSurfaces.at(mFrame - mComputedStartFrame)->bindSurfaceIndicesForDrawing(mLayer, 1);
                impostorProgram.update("color", mSurfaceAtomColor);
                glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfSurfaceAtoms(mLayer));
            }
        }
        else
        {
            // Point size for rendering
            glPointSize(mAtomPointSize);

            // Prepare point drawing
            pointProgram.use();
            pointProgram.update("view", mupCamera->getViewMatrix());
            pointProgram.update("projection", mupCamera->getProjectionMatrix());
            pointProgram.update("selectedIndex", mSelectedAtom);
            pointProgram.update("clippingPlane", mClippingPlane);

            // Draw internal
            if(mShowInternal)
            {
                mGPUSurfaces.at(mFrame - mComputedStartFrame)->bindInternalIndicesForDrawing(mLayer, 1);
                pointProgram.update("color", mInternalAtomColor);
                glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfInternalAtoms(mLayer));
            }

            // Draw surface
            if(mShowSurface)
            {
                mGPUSurfaces.at(mFrame - mComputedStartFrame)->bindSurfaceIndicesForDrawing(mLayer, 1);
                pointProgram.update("color", mSurfaceAtomColor);
                glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfSurfaceAtoms(mLayer));
            }
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

        // GUI updating
        updateGUI();
    });

    // Delete OpenGL structures
    glDeleteVertexArrays(1, &axisGizmoVAO);
    glDeleteBuffers(1, &axisGizmoVBO);
}

void SurfaceDynamicsVisualization::keyCallback(int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
            case GLFW_KEY_ESCAPE: { glfwSetWindowShouldClose(mpWindow, GL_TRUE); break; }
            case GLFW_KEY_P: { mRenderImpostor = !mRenderImpostor; break; }
            case GLFW_KEY_R: { mRenderWithProbeRadius = !mRenderWithProbeRadius; break; }
            case GLFW_KEY_RIGHT:
            {
                mSelectedAtom++;
                if(mSelectedAtom >= mGPUProteins.at(mFrame)->getAtomCount()) { mSelectedAtom = 0; }
                break;
            }
            case GLFW_KEY_LEFT:
            {
                mSelectedAtom--;
                if(mSelectedAtom < 0) { mSelectedAtom = mGPUProteins.at(mFrame)->getAtomCount() - 1; }
                break;
            }
            // case GLFW_KEY_C: { mUsePerspectiveCamera = !mUsePerspectiveCamera; break; }
            case GLFW_KEY_I: { mShowInternal = !mShowInternal; break; }
            case GLFW_KEY_S: { mShowSurface = !mShowSurface; break; }
            case GLFW_KEY_T: { mShowValidationSamples = !mShowValidationSamples; break; }
            case GLFW_KEY_A: { mShowAxesGizmo = !mShowAxesGizmo; break; }
        }
    }
}

void SurfaceDynamicsVisualization::mouseButtonCallback(int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        mRotateCamera = true;
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        mRotateCamera = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        mMoveCamera = true;
    }
    else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        mMoveCamera = false;
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        mSelectedAtom = getAtomBeneathCursor();
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
        << "Probe radius: " + std::to_string(mProbeRadius) << "\n"
        << "Extracted layers: " << (mExtractLayers ? "yes" : "no") << "\n"
        << "Start frame: " << mComputedStartFrame << " End frame: " << mComputedEndFrame << "\n"
        << "Count of frames: " << (mComputedEndFrame - mComputedStartFrame + 1) << "\n"
        << "Time: " << computationTime << "ms";
    mComputeInformation = stream.str();
}

void SurfaceDynamicsVisualization::updateGUI()
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

        // Rendering menu
        if (ImGui::BeginMenu("Rendering"))
        {
            // Impostors
            if(mRenderImpostor)
            {
                if(ImGui::MenuItem("Show Atom Points", "P", false, true)) { mRenderImpostor = false; }
            }
            else
            {
                if(ImGui::MenuItem("Show Atom Impostor", "P", false, true)) { mRenderImpostor = true; }
            }

            // Camera
            /*
            if(mUsePerspectiveCamera)
            {
                if(ImGui::MenuItem("Orthographic Projection", "C", false, true)) { mUsePerspectiveCamera = false; }
            }
            else
            {
                if(ImGui::MenuItem("Perspective Projection", "C", false, true)) { mUsePerspectiveCamera = true; }
            }
            */

            // Internal atoms
            if(mShowInternal)
            {
                if(ImGui::MenuItem("Hide Internal Atoms", "I", false, true)) { mShowInternal = false; }
            }
            else
            {
                if(ImGui::MenuItem("Show Internal Atoms", "I", false, true)) { mShowInternal = true; }
            }

            // Surface atoms
            if(mShowSurface)
            {
                if(ImGui::MenuItem("Hide Surface Atoms", "S", false, true)) { mShowSurface = false; }
            }
            else
            {
                if(ImGui::MenuItem("Show Surface Atoms", "S", false, true)) { mShowSurface = true; }
            }

            // Add probe radius
            if(mRenderWithProbeRadius)
            {
                if(ImGui::MenuItem("No Probe Radius", "R", false, true)) { mRenderWithProbeRadius = false; }
            }
            else
            {
                if(ImGui::MenuItem("Add Probe Radius", "R", false, true)) { mRenderWithProbeRadius = true; }
            }

            // Sample point of surface test
            if(mShowValidationSamples)
            {
                if(ImGui::MenuItem("Hide Validation Samples", "T", false, true)) { mShowValidationSamples = false; }
            }
            else
            {
                if(ImGui::MenuItem("Show Validation Samples", "T", false, true)) { mShowValidationSamples = true; }
            }

            // Axes gizmo
            if(mShowAxesGizmo)
            {
                if(ImGui::MenuItem("Hide Axes Gizmo", "A", false, true)) { mShowAxesGizmo = false; }
            }
            else
            {
                if(ImGui::MenuItem("Show Axes Gizmo", "A", false, true)) { mShowAxesGizmo = true; }
            }
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

            // Debugging window
            if(mShowDebuggingWindow)
            {
                if(ImGui::MenuItem("Hide Debugging", "", false, true)) { mShowDebuggingWindow = false; }
            }
            else
            {
                if(ImGui::MenuItem("Show Debugging", "", false, true)) { mShowDebuggingWindow = true; }
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
            ImGui::EndMenu();
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

    // Extraction window
    if(mShowSurfaceExtractionWindow)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.5f, 0.0f, 0.0f, 0.75f)); // window background
        ImGui::Begin("Computation", NULL, 0);

        // Computatiom
        ImGui::Text("Computation");
        ImGui::SliderFloat("Probe Radius", &mProbeRadius, 0.f, 2.f, "%.1f");
        ImGui::Checkbox("Extract Layers", &mExtractLayers);
        ImGui::SliderInt("Start Frame", &mComputationStartFrame, 0, mComputationEndFrame);
        ImGui::SliderInt("End Frame", &mComputationEndFrame, mComputationStartFrame, mGPUProteins.size() - 1);
        ImGui::SliderInt("CPU Cores", &mCPUThreads, 1, 24);
        if(ImGui::Button("\u2794 GPGPU")) { computeLayers(mComputationStartFrame, mComputationEndFrame, true); }
        ImGui::SameLine();
        if(ImGui::Button("\u2794 CPU")) { computeLayers(mComputationStartFrame, mComputationEndFrame, false); }
        ImGui::Separator();

        // Report
        ImGui::Text("Report");
        ImGui::Text(mComputeInformation.c_str());

        ImGui::End();
        ImGui::PopStyleColor(); // window background
    }

    // Camera window
    if(mShowCameraWindow)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.5f, 0.75f)); // window background
        ImGui::Begin("Camera", NULL, 0);

        // Camera
        ImGui::Text("Orthographic Camera");

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

        // Reset
        if(ImGui::Button("Reset Camera"))
        {
            mupCamera->setAlpha(mCameraDefaultAlpha);
            mupCamera->setBeta(mCameraDefaultBeta);
            mupCamera->setCenter(glm::vec3(0));
        }
        ImGui::Separator();

        // Clipping
        ImGui::Text("Clipping Plane");
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
        ImGui::Text("Animation");
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
        ImGui::SliderInt("Frame", &frame, mComputedStartFrame, mComputedEndFrame);
        if(frame != mFrame)
        {
            setFrame(frame);
        }
        ImGui::Separator();

        // Layer
        ImGui::Text("Layer");
        ImGui::SliderInt("Layer", &mLayer, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getLayerCount() - 1);
        ImGui::Separator();

        // Show / hide internal atoms
        ImGui::Text("Rendering");
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
        ImGui::Separator();

        // Light direction
        ImGui::Text("Lighting");
        if(ImGui::Button("Spot Light"))
        {
            mLightDirection = -glm::normalize(mupCamera->getPosition() - mupCamera->getCenter());
        }

        ImGui::End();
        ImGui::PopStyleColor(); // window background
    }

    // Debugging window
    if(mShowDebuggingWindow)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.5f, 0.0f, 0.75f)); // window background
        ImGui::Begin("Debugging", NULL, 0);

        // General infos
        ImGui::Text(std::string("Selected Atom: " + std::to_string(mSelectedAtom)).c_str());
        ImGui::Text(std::string("Atom Count: " + std::to_string(mGPUProteins.at(mFrame)->getAtomCount())).c_str());

        // Show available GPU memory
        int availableMemory;
        glGetIntegerv(0x9049, &availableMemory); // Nvidia only
        availableMemory = availableMemory / 1000;
        ImGui::Text(std::string("Available VRAM: " + std::to_string(availableMemory) + "MB").c_str());
        // TODO: one may want to clean up glGetError if query failed and show failure message on GUI (for example on Intel or AMD)

        // Show / hide axes gizmo
        if(mShowAxesGizmo)
        {
            if(ImGui::Button("Hide Axes Gizmo", ImVec2(140, 22)))
            {
                mShowAxesGizmo = false;
            }
        }
        else
        {
            if(ImGui::Button("Show Axes Gizmo", ImVec2(140, 22)))
            {
                mShowAxesGizmo = true;
            }
        }

        ImGui::End();
        ImGui::PopStyleColor(); // window background
    }

    // Validation window
    if(mShowValidationWindow)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.5f, 0.5f, 0.75f)); // window background
        ImGui::Begin("Validation", NULL, 0);

        // Do validation
        ImGui::Text("Validation");
        ImGui::SliderInt("Samples", &mSurfaceValidationAtomSampleCount, 1, 10000);
        ImGui::SliderInt("Seed", &mSurfaceValidationSeed, 0, 1337);
        if(ImGui::Button("Validate Surface"))
        {
            mupSurfaceValidation->validate(
                mGPUProteins.at(mFrame).get(),
                mGPUSurfaces.at(mFrame - mComputedStartFrame).get(),
                mLayer,
                mProbeRadius,
                mSurfaceValidationSeed,
                mSurfaceValidationAtomSampleCount,
                mValidationInformation,
                std::vector<GLuint>());
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
        ImGui::Text("Report");
        ImGui::Text(mValidationInformation.c_str());

        ImGui::End();
        ImGui::PopStyleColor(); // window background
    }

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
    // Bring frame into valid interval
    frame = glm::clamp(frame, mComputedStartFrame, mComputedEndFrame);

    // Check whether there are enough layers to display
    int layerCount = mGPUSurfaces.at(frame - mComputedStartFrame)->getLayerCount();
    if(mLayer >= layerCount)
    {
        mLayer = layerCount -1;
    }

    // Write it to variable
    if(mFrame == frame)
    {
        return false;
    }
    else
    {
        mFrame = frame;
        return true;
    }
}

void SurfaceDynamicsVisualization::computeLayers(int startFrame, int endFrame, bool useGPU)
{
    // Reset surfaces
    mGPUSurfaces.clear();

    // Do it for all animation frames
    float computationTime = 0;
    for(int i = startFrame; i <= endFrame; i++)
    {
        if(useGPU)
        {
            mGPUSurfaces.push_back(std::move(mupGPUSurfaceExtraction->calculateSurface(mGPUProteins.at(i).get(), mProbeRadius, mExtractLayers)));
        }
        else
        {
            mGPUSurfaces.push_back(std::move(mupGPUSurfaceExtraction->calculateSurface(mGPUProteins.at(i).get(), mProbeRadius, mExtractLayers, true, mCPUThreads)));
        }
        computationTime += mGPUSurfaces.back()->getComputationTime();
    }

    // Remember which frames were computed
    mComputedStartFrame = startFrame;
    mComputedEndFrame = endFrame;

    // Update compute information
    updateComputationInformation(
        (useGPU ? "GPU" : "CPU with " + std::to_string(mCPUThreads) + " threads"), computationTime);

    // Set to first animation frame
    setFrame(mComputedStartFrame);
}

int SurfaceDynamicsVisualization::getAtomBeneathCursor() const
{
    // Variables to collect results
    int foundIndex = 0;
    float foundDistance = std::numeric_limits<float>::max();

    // Preallocation
    glm::vec3 sphereCenter;
    float sphereRadius;

    // Generate ray
    double cursorX, cursorY;
    glfwGetCursorPos(mpWindow, &cursorX, &cursorY);
    glm::vec3 linePoint = mupCamera->getPositionAtPixel(cursorX, cursorY);
    glm::vec3 lineDir = mupCamera->getDirection();

    // TODO: TESTING
    // std::cout << "(" << linePoint.x << ", " << linePoint.y << ", " << linePoint.z << ")" << std::endl;

    // Go over display atoms
    auto indices = mGPUSurfaces.at(mFrame - mComputedStartFrame)->getInputIndices(mLayer);
    auto atoms = mGPUProteins.at(mFrame)->getAtoms();
    for(const auto& rIndex : indices)
    {
        // Get information about atom
        sphereCenter = atoms.at(rIndex).center;
        sphereRadius = atoms.at(rIndex).radius;
        if(mRenderWithProbeRadius) { sphereRadius += mProbeRadius; }

        // INTERSECT

        // Right part of equation beneath square root
        float underSQRT1 = glm::dot(lineDir, (linePoint - sphereCenter));
        underSQRT1 = underSQRT1 * underSQRT1;
        float underSQRT2 = glm::length(linePoint - sphereCenter);
        underSQRT2 = underSQRT2 * underSQRT2;
        float underSQRT = (underSQRT1 - underSQRT2 + (sphereRadius * sphereRadius));

        // Left part of equation
        float left = -(glm::dot(lineDir, (linePoint - sphereCenter)));

        if(underSQRT > 0)
        {
            // Right part of equation
            float right = glm::sqrt(underSQRT);

            // First point
            float d = left + right;
            float length = glm::length(linePoint + (d * lineDir));
            if(length < foundDistance)
            {
                foundIndex = rIndex;
                foundDistance = length;
            }

            // Second point
            d = left - right;
            length = glm::length(linePoint + (d * lineDir));
            if(length < foundDistance)
            {
                foundIndex = rIndex;
                foundDistance = length;
            }
        }
        else if(underSQRT == 0)
        {
            // Single point
            float d = left;
            float length = glm::length(linePoint + (d * lineDir));
            if(length < foundDistance)
            {
                foundIndex = rIndex;
                foundDistance = length;
            }
        }
    }

    return foundIndex;
}

// ### Main function ###

int main()
{
    SurfaceDynamicsVisualization detection;
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
