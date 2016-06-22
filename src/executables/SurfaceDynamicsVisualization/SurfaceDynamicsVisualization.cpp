#include "SurfaceDynamicsVisualization.h"
#include "CPPImplementation.h"
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
#include <thread>
#include <functional>

// ### Class implementation ###

SurfaceDynamicsVisualization::SurfaceDynamicsVisualization()
{
    // # Setup members
    mCameraDeltaMovement = glm::vec2(0,0);
    mCameraSmoothTime = 1.f;
    mLightDirection = glm::normalize(glm::vec3(-0.5, -0.75, -0.3));

    // Create window (which initializes OpenGL)
    mpWindow = generateWindow("Surface Dynamics Visualization");

    // Construct GPUSurfaceExtraction object after OpenGL has been initialized
    mupGPUSurfaceExtraction = std::unique_ptr<GPUSurfaceExtraction>(new GPUSurfaceExtraction);

    // Init ImGui
    ImGui_ImplGlfwGL3_Init(mpWindow, true);
    ImGuiIO& io = ImGui::GetIO();
    std::string fontpath = std::string(IMGUI_FONTS_PATH) + "/DroidSans.ttf";
    io.Fonts->AddFontFromFileTTF(fontpath.c_str(), 16);
    //io.Fonts->AddFontDefault();

    // Clear color
    glClearColor(0.f, 0.f, 0.f, 1.f);

    // # Callbacks after ImGui

    // Register keyboard callback
    std::function<void(int, int, int, int)> kC = [&](int k, int s, int a, int m)
    {
        // Check whether ImGui is handling this
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureKeyboard)
        {
            return;
        }
        this->keyCallback(k, s, a, m);
    };
    setKeyCallback(mpWindow, kC);

    // Register mouse button callback
    std::function<void(int, int, int)> kB = [&](int b, int a, int m)
    {
        // Check whether ImGui is handling this
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureMouse)
        {
            return;
        }
        this->mouseButtonCallback(b, a, m);
    };
    setMouseButtonCallback(mpWindow, kB);

    // Register scroll callback
    std::function<void(double, double)> kS = [&](double x, double y)
    {
        // Check whether ImGui is handling this
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureMouse)
        {
            return;
        }
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

    // Test protein extent
    std::cout
        << "Min extent of protein: "
        << mProteinMinExtent.x << ", "
        << mProteinMinExtent.y << ", "
        << mProteinMinExtent.z << std::endl;
    std::cout
        << "Max extent of protein: "
        << mProteinMaxExtent.x << ", "
        << mProteinMaxExtent.y << ", "
        << mProteinMaxExtent.z << std::endl;

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
    if(mInitiallyUseGLSLImplementation)
    {
        runGLSLImplementation();
    }
    else
    {
        runCPPImplementation(true);
    }

    // Prepare testing the surface
    glGenBuffers(1, &mSurfaceValidationVBO);
    glGenVertexArrays(1, &mSurfaceValidationVAO);
    mupSurfaceValidationProgram = std::unique_ptr<ShaderProgram>(new ShaderProgram("/SurfaceDynamicsVisualization/sample.vert", "/SurfaceDynamicsVisualization/sample.frag"));
}

SurfaceDynamicsVisualization::~SurfaceDynamicsVisualization()
{
    // Surface validation
    glDeleteVertexArrays(1, &mSurfaceValidationVAO);
    glDeleteBuffers(1, &mSurfaceValidationVBO);
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

    // Bind buffer to texture (may not be done before buffer filling? Probably not necessary for GLSL implementation since already bound there and filled on GPU)
    /*
    glBindTexture(GL_TEXTURE_BUFFER, mInternalIndicesTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, mInternalIndicesBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glBindTexture(GL_TEXTURE_BUFFER, mSurfaceIndicesTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, mSurfaceIndicesBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    */

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

        // Bind SSBO with atoms (TODO: only at frame change)
        mGPUProteins.at(mFrame)->bind(0);

        // Calculate cursor movement
        double cursorX, cursorY;
        glfwGetCursorPos(mpWindow, &cursorX, &cursorY);
        GLfloat cursorDeltaX = (float)cursorX - prevCursorX;
        GLfloat cursorDeltaY = (float)cursorY - prevCursorY;
        prevCursorX = cursorX;
        prevCursorY = cursorY;

        // Orbit camera
        if(mRotateCamera)
        {
            mCameraDeltaMovement = glm::vec2(cursorDeltaX, cursorDeltaY);
            mCameraSmoothTime = 1.f;
        }
        else
        {
            mCameraSmoothTime -= deltaTime / mCameraSmoothDuration;
            mCameraSmoothTime = glm::max(mCameraSmoothTime, 0.f);
        }
        glm::vec2 cameraMovement = glm::lerp(glm::vec2(0), mCameraDeltaMovement, mCameraSmoothTime);
        mupCamera->setAlpha(mupCamera->getAlpha() + 0.25f * cameraMovement.x);
        mupCamera->setBeta(mupCamera->getBeta() - 0.25f * cameraMovement.y);
        mupCamera->update(resolution.x, resolution.y, mUsePerspectiveCamera);

        // Light direction
        if(mRotateLight)
        {
            mLightDirection = - glm::normalize(mupCamera->getPosition() - mupCamera->getCenter());
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
                mGPUSurfaces.at(mFrame)->bindInternalIndicesForDrawing(mLayer, 1);
                impostorProgram.update("color", mInternalAtomColor);
                glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame)->getCountOfInternalAtoms(mLayer));
            }

            // Draw surface
            if(mShowSurface)
            {
                mGPUSurfaces.at(mFrame)->bindSurfaceIndicesForDrawing(mLayer, 1);
                impostorProgram.update("color", mSurfaceAtomColor);
                glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame)->getCountOfSurfaceAtoms(mLayer));
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
                mGPUSurfaces.at(mFrame)->bindInternalIndicesForDrawing(mLayer, 1);
                pointProgram.update("color", mInternalAtomColor);
                glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame)->getCountOfInternalAtoms(mLayer));
            }

            // Draw surface
            if(mShowSurface)
            {
                mGPUSurfaces.at(mFrame)->bindSurfaceIndicesForDrawing(mLayer, 1);
                pointProgram.update("color", mSurfaceAtomColor);
                glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame)->getCountOfSurfaceAtoms(mLayer));
            }
        }

        // Drawing of surface test
        if(mShowSamplePoint)
        {
            glPointSize(mSamplePointSize);
            mupSurfaceValidationProgram->use();
            mupSurfaceValidationProgram->update("view", mupCamera->getViewMatrix());
            mupSurfaceValidationProgram->update("projection", mupCamera->getProjectionMatrix());
            mupSurfaceValidationProgram->update("color", mSamplePointColor);
            glBindVertexArray(mSurfaceValidationVAO);
            glDrawArrays(GL_POINTS, 0, mSurfaceValidationSampleCount);
            glBindVertexArray(0);
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
            glm::mat4 axisModelMatrix = glm::translate(glm::mat4(1.f), mupCamera->getCenter());
            axisGizmoProgram.update("model", axisModelMatrix);
            axisGizmoProgram.update("color", glm::vec3(1.f, 0.f, 0.f));
            glDrawArrays(GL_LINES, 0, axisVertices.size());

            // Y axis
            axisModelMatrix = glm::translate(glm::mat4(1.f), mupCamera->getCenter());
            axisModelMatrix = glm::rotate(axisModelMatrix, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            axisGizmoProgram.update("model", axisModelMatrix);
            axisGizmoProgram.update("color", glm::vec3(0.f, 1.f, 0.f));
            glDrawArrays(GL_LINES, 0, axisVertices.size());

            // Z axis
            axisModelMatrix = glm::translate(glm::mat4(1.f), mupCamera->getCenter());
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
            case GLFW_KEY_T: { mShowSamplePoint = !mShowSamplePoint; break; }
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
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        mRotateLight = true;
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        mRotateLight = false;
    }
}

void SurfaceDynamicsVisualization::scrollCallback(double xoffset, double yoffset)
{
    mupCamera->setRadius(mupCamera->getRadius() - 0.5f * (float)yoffset);
}

void SurfaceDynamicsVisualization::updateComputationInformation(std::string device, float computationTime)
{
    /*
    std::stringstream stream;
    stream <<
        device << " used" << "\n"
        << "Probe radius: " + std::to_string(mProbeRadius) << "\n"
        << "Time: " << computationTime << "ms" << "\n"
        << "Atom count: " + std::to_string(mupGPUProtein->getAtomCount()) << "\n"
        << "Internal atoms: " + std::to_string(mInternalCount) << "\n"
        << "Surface atoms: " + std::to_string(mSurfaceCount);
    mComputeInformation = stream.str();
    */
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
            if(mShowSamplePoint)
            {
                if(ImGui::MenuItem("Hide Sample Points", "T", false, true)) { mShowSamplePoint = false; }
            }
            else
            {
                if(ImGui::MenuItem("Show Sample Points", "T", false, true)) { mShowSamplePoint = true; }
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
            if(mShowSurfaceComputationWindow)
            {
                if(ImGui::MenuItem("Hide Computation", "", false, true)) { mShowSurfaceComputationWindow = false; }
            }
            else
            {
                if(ImGui::MenuItem("Show Computation", "", false, true)) { mShowSurfaceComputationWindow = true; }
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
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 0.75f)); // window background
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.2f, 0.2f, 0.2f, 0.75f)); // window title
    ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.0f, 0.0f, 0.0f, 0.5f)); // scrollbar grab
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // scrollbar background
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.5f)); // button

    // Computation window
    if(mShowSurfaceComputationWindow)
    {
        ImGui::Begin("Surface Computation", NULL, 0);
        ImGui::SliderFloat("Probe radius", &mProbeRadius, 0.f, 2.f, "%.1f");
        ImGui::SliderInt("CPU Cores", &mCPPThreads, 1, 24);
        if(ImGui::Button("Run GPGPU")) { runGLSLImplementation(); }
        ImGui::SameLine();
        if(ImGui::Button("Run Single-Core CPU")) { runCPPImplementation(false); }
        ImGui::SameLine();
        if(ImGui::Button("Run Multi-Core CPU")) { runCPPImplementation(true); }
        ImGui::Text("Layer removal is GPU only!");
        ImGui::Text(mComputeInformation.c_str());
        ImGui::SliderInt("Samples", &mSurfaceTestAtomSampleCount, 1, 10000);
        ImGui::SliderInt("Seed", &mSurfaceTestSeed, 0, 1337);
        if(ImGui::Button("Test Surface")) { testSurface(); }
        ImGui::Text(mTestOutput.c_str());
        ImGui::End();
    }

    // Camera window
    if(mShowCameraWindow)
    {
        ImGui::Begin("Camera", NULL, 0);
        ImGui::SliderFloat("Clipping Plane Offset", &mClippingPlane, mClippingPlaneMin, mClippingPlaneMax, "%.1f");
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
        if(ImGui::Button("X-Axis"))
        {
            mupCamera->setAlpha(0);
            mupCamera->setBeta(90);
        }
        ImGui::SameLine();
        if(ImGui::Button("Y-Axis"))
        {
            // Internal, this is not possible. There is some epsilon on beta inside the camera object
            mupCamera->setAlpha(0);
            mupCamera->setBeta(0);
        }
        ImGui::SameLine();
        if(ImGui::Button("Z-Axis"))
        {
            mupCamera->setAlpha(90.f);
            mupCamera->setBeta(90.f);
        }
        if(ImGui::Button("Reset Camera"))
        {
            mupCamera->setAlpha(mCameraDefaultAlpha);
            mupCamera->setBeta(mCameraDefaultBeta);
        }
        ImGui::End();
    }

    // Visualization window
    if(mShowVisualizationWindow)
    {
        ImGui::Begin("Visualization", NULL, 0);
        ImGui::SliderInt("Frame", &mFrame, 0, mGPUSurfaces.size() - 1); // TODO: extra method to set frame: one has to clamp layers for example, reset selected atom...
        ImGui::SliderInt("Layer", &mLayer, 0, mGPUSurfaces.at(mFrame)->getLayerCount() - 1);

        // Show / hide internal atoms
        if(mShowInternal)
        {
            if(ImGui::Button("Hide Internal", ImVec2(90, 22)))
            {
                mShowInternal = false;
            }
        }
        else
        {
            if(ImGui::Button("Show Internal", ImVec2(90, 22)))
            {
                mShowInternal = true;
            }
        }
        ImGui::SameLine();

        // Show / hide surface atoms
        if(mShowSurface)
        {
            if(ImGui::Button("Hide Surface", ImVec2(90, 22)))
            {
                mShowSurface = false;
            }
        }
        else
        {
            if(ImGui::Button("Show Surface", ImVec2(90, 22)))
            {
                mShowSurface = true;
            }
        }

        ImGui::End();
    }

    // Debugging window
    if(mShowDebuggingWindow)
    {
        ImGui::Begin("Debugging", NULL, 0);
        ImGui::Text(std::string("Selected Atom: " + std::to_string(mSelectedAtom)).c_str());
        ImGui::End();
    }

    ImGui::PopStyleColor(); // button
    ImGui::PopStyleColor(); // scrollbar background
    ImGui::PopStyleColor(); // scrollbar grab
    ImGui::PopStyleColor(); // window title
    ImGui::PopStyleColor(); // window background

    ImGui::Render();
}

void SurfaceDynamicsVisualization::testSurface()
{
    // Read data back from OpenGL buffers
    std::vector<GLuint> inputIndices = mGPUSurfaces.at(mFrame)->getInputIndices(mLayer);
    std::vector<GLuint> internalIndices = mGPUSurfaces.at(mFrame)->getInternalIndices(mLayer);
    std::vector<GLuint> surfaceIndices = mGPUSurfaces.at(mFrame)->getSurfaceIndices(mLayer);

    // Seed
    std::srand(mSurfaceTestSeed);

    // Vector of samples
    std::vector<glm::vec3> samples;

    // Count cases of failure
    int internalSampleFailures = 0;
    int surfaceAtomsFailures = 0;

    // Copy atoms of protein to stack
    auto atoms = mGPUProteins.at(mFrame)->getAtoms();

    // Go over atoms (using indices from input indices buffer)
    for(unsigned int i : inputIndices) // using results from algorithm here. Not so good for independend test but necessary for layers
    {
        // Just to see, that something is going on
        std::cout << "Processing Atom: " << i << std::endl;

        // Check, whether atom is internal or surface (would be faster to iterate over those structures, but this way the test is more testier)
        bool found = false;
        bool internalAtom = false;
        for(auto& rIndex : internalIndices)
        {
            if(rIndex == i)
            {
                found = true;
                internalAtom = true;
                break;
            }
        }
        if(!found)
        {
            for(auto& rIndex : surfaceIndices)
            {
                if(rIndex == i)
                {
                    found = true;
                    internalAtom = false;
                    break;
                }
            }
        }
        if(!found)
        {
            std::cout << "ERROR: Atom " << i << " neither classified as internal nor as surface. Algorithm has failed";
            return;
        }

        // Get position and radius
        glm::vec3 atomCenter = atoms[i].center;
        float atomExtRadius = atoms[i].radius + mProbeRadius;

        // Count samples which are classified as internal
        int internalSamples = 0;

        // Do some samples per atom
        for(int j = 0; j < mSurfaceTestAtomSampleCount; j++)
        {
            // Generate samples (http://mathworld.wolfram.com/SpherePointPicking.html)
            float u = (float)((double)std::rand() / (double)RAND_MAX);
            float v = (float)((double)std::rand() / (double)RAND_MAX);
            float theta = 2.f * glm::pi<float>() * u;
            float phi = glm::acos(2.f * v - 1);

            // Generate sample point
            glm::vec3 samplePosition(
                atomExtRadius * glm::sin(phi) * glm::cos(theta),
                atomExtRadius * glm::cos(phi),
                atomExtRadius * glm::sin(phi) * glm::sin(theta));
            samplePosition += atomCenter;

            // Go over all atoms and test, whether sample is inside in at least one
            bool inside = false;
            for(unsigned int k : inputIndices)
            {
                // Test not against atom that generated sample
                if(k == i) { continue; };

                // Actual test
                if(glm::distance(samplePosition, atoms[k].center) < (atoms[k].radius + mProbeRadius))
                {
                    inside = true;
                    break;
                }
            }

            // Check result
            if(inside)
            {
                internalSamples++;
            }
            else
            {
                // Push back to vector only, if NOT inside
                samples.push_back(samplePosition);

                // If sample was created by internal atom and is not classified as internal in test, something went terrible wrong
                if(internalAtom)
                {
                    // Sample is not inside any other atom's extended hull but should be
                    std::cout << "Atom " << i << " is proably wrongly classified as internal by algorithm";

                    // Increment internalSampleFailures
                    internalSampleFailures++;
                }
            }
        }

        // If all samples are classified internal and the atom was classified as surface by the algorithm something MAY have went wront
        if((internalSamples == mSurfaceTestAtomSampleCount) && !internalAtom)
        {
            surfaceAtomsFailures++;
        }
    }

    // Bind vertex array object
    glBindVertexArray(mSurfaceValidationVAO);

    // Fill vertex buffer with vertices
    glBindBuffer(GL_ARRAY_BUFFER, mSurfaceValidationVBO);
    glBufferData(GL_ARRAY_BUFFER, samples.size() * sizeof(glm::vec3), samples.data(), GL_DYNAMIC_DRAW);

    // Bind it to shader program
    GLint posAttrib = glGetAttribLocation(mupSurfaceValidationProgram->getProgramHandle(), "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Unbind everything
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Remember about complete count of samples for drawing
    mSurfaceValidationSampleCount = samples.size();

    // Draw output of test to GUI
    mTestOutput = "Wrong classification as internal for " + std::to_string(internalSampleFailures) + " samples.\n";
    mTestOutput += "Maybe wrong classification as surface for " + std::to_string(surfaceAtomsFailures) + " atoms.";
}

void SurfaceDynamicsVisualization::runCPPImplementation(bool threaded)
{
    /*
    std::cout << "CPP implementation used!" << std::endl;

    // Note
    // No input indices used, just complete molecule is processed.
    // No support for layer removal.

    // Copy atoms of protein to stack
    auto atoms = mupGPUProtein->getAtoms();

    // # Prepare output vectors
    std::vector<unsigned int> internalIndices;
    std::vector<unsigned int> surfaceIndices;

    // # Simulate compute shader
    CPPImplementation cppImplementation;

    // Do it for each atom
    double time = glfwGetTime();
    std::cout << "*** ALGORITHM OUTPUT START ***" << std::endl;

    if(threaded)
    {
        // Do it in threads
        std::vector<std::vector<unsigned int> > internalIndicesSubvectors;
        std::vector<std::vector<unsigned int> > surfaceIndicesSubvectors;
        internalIndicesSubvectors.resize(mCPPThreads);
        surfaceIndicesSubvectors.resize(mCPPThreads);
        std::vector<std::thread> threads;

        // Lauch threads
        for(int i = 0; i < mCPPThreads; i++)
        {
            // Calculate min and max index
            int count = atoms.size() / mCPPThreads;
            int offset = count * i;
            threads.push_back(
                std::thread([&](
                    int minIndex,
                    int maxIndex,
                    std::vector<unsigned int>& internalIndicesSubvector,
                    std::vector<unsigned int>& surfaceIndicesSubvector)
                {
                    CPPImplementation threadCppImplementation;
                    for(int a = minIndex; a <= maxIndex; a++)
                    {
                        threadCppImplementation.execute(
                            a,
                            atoms.size(),
                            mProbeRadius,
                            atoms,
                            internalIndicesSubvector,
                            surfaceIndicesSubvector);
                    }
                },
                offset, // minIndex
                i == mCPPThreads - 1 ? atoms.size() - 1 : offset+count-1, // maxIndex
                std::ref(internalIndicesSubvectors[i]), // internal indices
                std::ref(surfaceIndicesSubvectors[i]))); // external indices
        }

        // Join threads
        for(int i = 0; i < mCPPThreads; i++)
        {
            // Joint thread i
            threads[i].join();

            // Collect results from it
            internalIndices.insert(
                internalIndices.end(),
                internalIndicesSubvectors[i].begin(),
                internalIndicesSubvectors[i].end());
            surfaceIndices.insert(
                surfaceIndices.end(),
                surfaceIndicesSubvectors[i].begin(),
                surfaceIndicesSubvectors[i].end());
        }
    }
    else
    {
        // Do all atoms in main thread
        for(int a = 0; a < atoms.size(); a++)
        {
            cppImplementation.execute(a, atoms.size(), mProbeRadius, atoms, internalIndices, surfaceIndices);
        }
    }

    std::cout << "*** ALGORITHM OUTPUT END ***" << std::endl;
    float computationTime = (float) (1000.0 * (glfwGetTime() - time));

    // Fill output atom indices to image buffers
    glBindBuffer(GL_TEXTURE_BUFFER, mInternalIndicesBuffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(GLuint) * internalIndices.size(), internalIndices.data(), GL_STATIC_DRAW);
    glBindBuffer(0, mInternalIndicesBuffer);

    glBindBuffer(GL_TEXTURE_BUFFER, mSurfaceIndicesBuffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(GLuint) * surfaceIndices.size(), surfaceIndices.data(), GL_STATIC_DRAW);
    glBindBuffer(0, mSurfaceIndicesBuffer);

    // Fetch count
    mInputCount = atoms.size();
    mInternalCount = internalIndices.size();
    mSurfaceCount = surfaceIndices.size();

    // Update compute information
    updateComputationInformation(
        threaded ? ("Multi-Core CPU (" + std::to_string(mCPPThreads) + " Threads)") : "Single-Core CPU",
        computationTime);
    */
}

void SurfaceDynamicsVisualization::runGLSLImplementation()
{
    std::cout << "GLSL implementation used!" << std::endl;

    for(const auto& rupGPUProtein : mGPUProteins)
    {
        mGPUSurfaces.push_back(std::move(mupGPUSurfaceExtraction->calculateSurface(rupGPUProtein.get(), mProbeRadius, true)));
    }

    // Update compute information
    // updateComputationInformation("GPGPU", mupGPUSurface->getComputationTime());
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
