#include "PerfectSurfaceDetection.h"

#include "CPPImplementation.h"
#include "imgui/imgui.h"
#include "imgui/examples/opengl3_example/imgui_impl_glfw_gl3.h"

#include "OrbitCamera.h"
#include "ShaderTools/Renderer.h"
#include "Molecule/MDtrajLoader/MdTraj/MdTrajWrapper.h"
#include "Molecule/MDtrajLoader/Data/Protein.h"
#include "SimpleLoader.h"

#include <glm/gtx/component_wise.hpp>

#include <iostream>

// ### Class implementation ###

PerfectSurfaceDetection::PerfectSurfaceDetection()
{
    // # Setup members
    mSurfaceCount = 0;
    mCameraDeltaMovement = glm::vec2(0,0);
    mCameraSmoothTime = 1.f;
    mLightDirection = glm::normalize(glm::vec3(-0.5, -0.75, -0.3));

    // Create window
    mpWindow = generateWindow();

    // Init ImGui
    ImGui_ImplGlfwGL3_Init(mpWindow, true);
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();

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
    // paths.push_back(std::string(RESOURCES_PATH) + "/molecules/PDB/1a19.pdb");
    // paths.push_back(std::string(RESOURCES_PATH) + "/molecules/PDB/1vis.pdb");
    paths.push_back(std::string(RESOURCES_PATH) + "/molecules/PDB/2mp3.pdb");
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

    // Vector which is used as data for SSBO and CPP implementation
    for(Atom const * pAtom : *(upProtein->getAtoms()))
    {
        // Push back all atoms (CONVERTING PICOMETER TO ANGSTROM)
        mAtomStructs.push_back(
            AtomStruct(
                pAtom->getPosition(),
                0.01f * mAtomLUT.vdW_radii_picometer.at(
                    pAtom->getElement())));
    }
    */

    // /*
    // Simple PDB loader
    // mAtomStructs = parseSimplePDB(std::string(RESOURCES_PATH) + "/molecules/SimplePDB/PDB_Polymerase-of-E-coli-DNA.txt", mProteinMinExtent, mProteinMaxExtent);
    mAtomStructs = parseSimplePDB(std::string(RESOURCES_PATH) + "/molecules/SimplePDB/PDB_Myoglobin.txt", mProteinMinExtent, mProteinMaxExtent);
    // mAtomStructs = parseSimplePDB(std::string(RESOURCES_PATH) + "/molecules/SimplePDB/PDB_Nitrogen-Paracoccus-Cytochrome-C550.txt", mProteinMinExtent, mProteinMaxExtent);
    // mAtomStructs = parseSimplePDB(std::string(RESOURCES_PATH) + "/molecules/SimplePDB/8AtomsIntersection.txt", mProteinMinExtent, mProteinMaxExtent);
    // */

    // Count of atoms
    mAtomCount = mAtomStructs.size();

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

    /*
    // Test atom radii
    for(const auto& rAtom : mAtomStructs)
    {
        std::cout << "Radius in Angstrom: " << rAtom.radius << std::endl;
    }
    */

    // Output atom count
    std::cout << "Atom count: " << mAtomCount << std::endl;

    /*
    // # Some simple rotation matrix for easy test of rotation invariance
    // glm::mat4 rotation = glm::rotate(glm::mat4(1.f), glm::radians(45.f), glm::normalize(glm::vec3(-1,1,1)));
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.f), glm::radians(0.f), glm::normalize(glm::vec3(0.6f,-1.f,0.f)));
    glm::vec3 translation = glm::vec3(3.544,-8.454,1);
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.f), translation);

    // Go over atom structs and transform them
    for(auto& rAtomStruct : mAtomStructs)
    {
        glm::vec4 newAtomCenter = translationMatrix * (rotationMatrix * glm::vec4(rAtomStruct.center, 1));
        rAtomStruct.center = glm::vec3(newAtomCenter.x, newAtomCenter.y, newAtomCenter.z);
    }
    */

    // # Create camera
    glm::vec3 cameraCenter = (mProteinMinExtent + mProteinMaxExtent) / 2.f;
    float cameraRadius = glm::compMax(mProteinMaxExtent - cameraCenter);
    mupCamera = std::unique_ptr<OrbitCamera>(
        new OrbitCamera(
            cameraCenter,
            90.f,
            90.f,
            cameraRadius,
            cameraRadius / 2.f,
            5.f * cameraRadius,
            45.f,
            0.05f));

    // Create query to measure execution time
    glGenQueries(1, &mQuery);

     // Variable to measure elapsed time
    GLuint timeElapsed = 0;

    // # Prepare atoms input (position + radius)

    // Start query for time measurement
    glBeginQuery(GL_TIME_ELAPSED, mQuery);

    // Fill into SSBO
    glGenBuffers(1, &mAtomsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mAtomsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(AtomStruct) * mAtomStructs.size(), mAtomStructs.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // # Prepare uint image to write indices of surface atoms. Lets call it image list in shader for easier understanding

    // Buffers
    glGenBuffers(1, &mInternalIndicesBuffer); // just generate, filled in GLSL or CPP implementatin method
    glGenBuffers(1, &mSurfaceIndicesBuffer); // just generate, filled in GLSL or CPP implementatin method

    // TODO: understand why buffer to texture binding is necessary in GLSL method and in renderLoop()
    // Texture (which will be bound as image, connected to buffer in renderLoop(), since it seems that is has to be done after buffer fill)
    glGenTextures(1, &mInternalIndicesTexture);
    glGenTextures(1, &mSurfaceIndicesTexture);

    // Print time for data transfer
    glEndQuery(GL_TIME_ELAPSED);
    GLuint done = 0;
    while(done == 0)
    {
        glGetQueryObjectuiv(mQuery, GL_QUERY_RESULT_AVAILABLE, &done);
    }
    glGetQueryObjectuiv(mQuery, GL_QUERY_RESULT, &timeElapsed);
    std::cout << "Time for data transfer on GPU: " << std::to_string(timeElapsed) << "ns" << std::endl;

    // # Run implementation to extract surface atoms
    if(mUseGLSLImplementation)
    {
        runGLSLImplementation();
    }
    else
    {
        runCPPImplementation();
    }

    // Output count
    std::cout << "Internal atom count: " << mInternalCount << std::endl;
    std::cout << "Surface atom count: " << mSurfaceCount << std::endl;
}

PerfectSurfaceDetection::~PerfectSurfaceDetection()
{
    // TODO: Delete OpenGL stuff
    // - ssbo
    // - texture
    // - buffer

    // Delete query object
    glDeleteQueries(1, &mQuery);
}

void PerfectSurfaceDetection::renderLoop()
{
    // Setup OpenGL
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    // Point size for rendering
    glPointSize(15.f);

    // Cursor
    float prevCursorX, prevCursorY = 0;

    // Prepare shader programs for rendering
    ShaderProgram pointProgram = ShaderProgram("/PerfectSurfaceDetection/point.vert", "/PerfectSurfaceDetection/point.frag");
    ShaderProgram impostorProgram = ShaderProgram("/PerfectSurfaceDetection/impostor.vert", "/PerfectSurfaceDetection/impostor.geom", "/PerfectSurfaceDetection/impostor.frag");

    // Bind SSBO with atoms
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mAtomsSSBO);

    // Bind buffer to texture (may not be done before buffer filling? Probably not necessary for GLSL implementation since already bound there and filled on GPU)
    glBindTexture(GL_TEXTURE_BUFFER, mInternalIndicesTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, mInternalIndicesBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glBindTexture(GL_TEXTURE_BUFFER, mSurfaceIndicesTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, mSurfaceIndicesBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

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

        // Drawing
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

            // Draw internal
            if(mShowInternal)
            {
                glBindImageTexture(
                    1,
                    mInternalIndicesTexture,
                    0,
                    GL_TRUE,
                    0,
                    GL_READ_ONLY,
                    GL_R32UI);
                impostorProgram.update("color", glm::vec3(1.f,1.f,1.f));
                glDrawArrays(GL_POINTS, 0, mInternalCount);
            }

            if(mShowSurface)
            {
                // Draw surface
                glBindImageTexture(
                    1,
                    mSurfaceIndicesTexture,
                    0,
                    GL_TRUE,
                    0,
                    GL_READ_ONLY,
                    GL_R32UI);
                impostorProgram.update("color", glm::vec3(1.f,0.f,0.f));
                glDrawArrays(GL_POINTS, 0, mSurfaceCount);
            }
        }
        else
        {
            // Prepare point drawing
            pointProgram.use();
            pointProgram.update("view", mupCamera->getViewMatrix());
            pointProgram.update("projection", mupCamera->getProjectionMatrix());
            pointProgram.update("selectedIndex", mSelectedAtom);

            // Draw internal
            if(mShowInternal)
            {
                glBindImageTexture(
                    1,
                    mInternalIndicesTexture,
                    0,
                    GL_TRUE,
                    0,
                    GL_READ_ONLY,
                    GL_R32UI);
                pointProgram.update("color", glm::vec3(1.f,1.f,1.f));
                glDrawArrays(GL_POINTS, 0, mInternalCount);
            }

            // Draw surface
            if(mShowSurface)
            {
                glBindImageTexture(
                    1,
                    mSurfaceIndicesTexture,
                    0,
                    GL_TRUE,
                    0,
                    GL_READ_ONLY,
                    GL_R32UI);
                pointProgram.update("color", glm::vec3(1.f,0.f,0.f));
                glDrawArrays(GL_POINTS, 0, mSurfaceCount);
            }
        }

        // GUI rendering
        renderGUI();
    });
}

void PerfectSurfaceDetection::keyCallback(int key, int scancode, int action, int mods)
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
                if(mSelectedAtom >= mAtomCount) { mSelectedAtom = 0; }
                std::cout << "Selected atom: " << mSelectedAtom << std::endl;
                break;
            }
            case GLFW_KEY_LEFT:
            {
                mSelectedAtom--;
                if(mSelectedAtom < 0) { mSelectedAtom = mAtomCount - 1; }
                std::cout << "Selected atom: " << mSelectedAtom << std::endl;
                break;
            }
            case GLFW_KEY_C: { mUsePerspectiveCamera = !mUsePerspectiveCamera; break; }
            case GLFW_KEY_I: { mShowInternal = !mShowInternal; break; }
            case GLFW_KEY_S: { mShowSurface = !mShowSurface; break; }
        }
    }
}

void PerfectSurfaceDetection::mouseButtonCallback(int button, int action, int mods)
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

void PerfectSurfaceDetection::scrollCallback(double xoffset, double yoffset)
{
    mupCamera->setRadius(mupCamera->getRadius() - 0.5f * (float)yoffset);
}

GLuint PerfectSurfaceDetection::readAtomicCounter(GLuint atomicCounter) const
{
    // Read atomic counter
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicCounter);

    GLuint *mapping = (GLuint*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER,
                                                0,
                                                sizeof(GLuint),
                                                GL_MAP_READ_BIT);

    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    return mapping[0];
}

void PerfectSurfaceDetection::resetAtomicCounter(GLuint atomicCounter) const
{
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicCounter);

    // Map the buffer
    GLuint* mapping = (GLuint*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER,
                                                0 ,
                                                sizeof(GLuint),
                                                GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
    // Set memory to new value
    memset(mapping, 0, sizeof(GLuint));

    // Unmap the buffer
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}

void PerfectSurfaceDetection::runCPPImplementation()
{
    std::cout << "CPP implementation used!" << std::endl;

    // # Prepare output vectors
    std::vector<unsigned int> internalIndices;
    std::vector<unsigned int> surfaceIndices;

    // # Simulate compute shader
    CPPImplementation cppImplementation;

    // Do it for each atom
    double time = glfwGetTime();
    std::cout << "*** ALGORITHM OUTPUT START ***" << std::endl;
    for(int i = 0; i < mAtomCount; i++) // all atoms
    // for(int i = 0; i < 1; i++) // just first atom
    {
        cppImplementation.execute(i, mAtomCount, mProbeRadius, mAtomStructs, internalIndices, surfaceIndices);
    }
    std::cout << "*** ALGORITHM OUTPUT END ***" << std::endl;
    std::cout << "Time for execution on CPU: " << std::to_string(1000.0 * (glfwGetTime() - time)) << "ms" << std::endl;

    // Fill output atom indices to image buffers
    glBindBuffer(GL_TEXTURE_BUFFER, mInternalIndicesBuffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(GLuint) * internalIndices.size(), internalIndices.data(), GL_STATIC_DRAW);
    glBindBuffer(0, mInternalIndicesBuffer);

    glBindBuffer(GL_TEXTURE_BUFFER, mSurfaceIndicesBuffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(GLuint) * surfaceIndices.size(), surfaceIndices.data(), GL_STATIC_DRAW);
    glBindBuffer(0, mSurfaceIndicesBuffer);

    // Fetch count
    mInternalCount = internalIndices.size();
    mSurfaceCount = surfaceIndices.size();
}

void PerfectSurfaceDetection::runGLSLImplementation()
{
    std::cout << "GLSL implementation used!" << std::endl;

    // # Compile shader
    ShaderProgram computeProgram(GL_COMPUTE_SHADER, "/PerfectSurfaceDetection/surface.comp");

    // # Prepare atomic counter for writing results to unique position in image
    GLuint internalCounter;
    glGenBuffers(1, &internalCounter);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, internalCounter);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
    resetAtomicCounter(internalCounter);

    GLuint surfaceCounter;
    glGenBuffers(1, &surfaceCounter);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, surfaceCounter);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
    resetAtomicCounter(surfaceCounter);

    // # Prepare output buffer
    glBindBuffer(GL_TEXTURE_BUFFER, mInternalIndicesBuffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(GLuint) * mAtomCount, 0, GL_DYNAMIC_COPY); // DYNAMIC_COPY because filled by GPU and read by GPU
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

    glBindBuffer(GL_TEXTURE_BUFFER, mSurfaceIndicesBuffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(GLuint) * mAtomCount, 0, GL_DYNAMIC_COPY); // DYNAMIC_COPY because filled by GPU and read by GPU
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

    // Bind buffer to texture which is used as image
    glBindTexture(GL_TEXTURE_BUFFER, mInternalIndicesTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, mInternalIndicesBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glBindTexture(GL_TEXTURE_BUFFER, mSurfaceIndicesTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, mSurfaceIndicesBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    // # Execute compute shader to determine surface atoms

    // Variable to measure elapsed time
    GLuint timeElapsed = 0;

    // Start query for time measurement
    glBeginQuery(GL_TIME_ELAPSED, mQuery);

    // Use compute shader program
    computeProgram.use();

    // Tell shader about count of atoms
    computeProgram.update("atomCount", mAtomCount);

    // Probe radius
    computeProgram.update("probeRadius", mProbeRadius);

    // Bind SSBO with atoms
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mAtomsSSBO);

    // Bind atomic counter
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, internalCounter);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, surfaceCounter);

    // Bind image where indices of surface atoms are written to
    glBindImageTexture(3,
                       mInternalIndicesTexture,
                       0,
                       GL_TRUE,
                       0,
                       GL_WRITE_ONLY,
                       GL_R32UI);
    glBindImageTexture(4,
                       mSurfaceIndicesTexture,
                       0,
                       GL_TRUE,
                       0,
                       GL_WRITE_ONLY,
                       GL_R32UI);


    // Print time for setup
    glEndQuery(GL_TIME_ELAPSED);
    GLuint done = 0;
    while(done == 0)
    {
        glGetQueryObjectuiv(mQuery, GL_QUERY_RESULT_AVAILABLE, &done);
    }
    glGetQueryObjectuiv(mQuery, GL_QUERY_RESULT, &timeElapsed);
    std::cout << "Time for setup on GPU: " << std::to_string(timeElapsed) << "ns" << std::endl;

    // Start query for time measurement
    glBeginQuery(GL_TIME_ELAPSED, mQuery);

    // Dispatch
    glDispatchCompute((mAtomCount / 16) + 1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    // Print time for execution
    glEndQuery(GL_TIME_ELAPSED);
    done = 0;
    while(done == 0)
    {
        glGetQueryObjectuiv(mQuery, GL_QUERY_RESULT_AVAILABLE, &done);
    }
    glGetQueryObjectuiv(mQuery, GL_QUERY_RESULT, &timeElapsed);
    std::cout << "Time for execution on GPU: " << std::to_string(timeElapsed) << "ns (= " << std::to_string(timeElapsed / 1000000.f) << "ms)" << std::endl;

    // Fetch count
    mInternalCount = readAtomicCounter(internalCounter);
    mSurfaceCount = readAtomicCounter(surfaceCounter);

    // TODO: Delete atomic counter
}

void PerfectSurfaceDetection::renderGUI()
{
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
                if(ImGui::MenuItem("Show Points", "P", false, true)) { mRenderImpostor = false; }
            }
            else
            {
                if(ImGui::MenuItem("Show Impostor", "P", false, true)) { mRenderImpostor = true; }
            }

            // Camera
            if(mUsePerspectiveCamera)
            {
                if(ImGui::MenuItem("Orthographic Projection", "C", false, true)) { mUsePerspectiveCamera = false; }
            }
            else
            {
                if(ImGui::MenuItem("Perspective Projection", "C", false, true)) { mUsePerspectiveCamera = true; }
            }

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

            ImGui::EndMenu();
        }

        // Frametime
        std::string fps = "FPS: " + std::to_string(ImGui::GetIO().Framerate);
        ImGui::MenuItem(fps.c_str(), "", false, false);

        ImGui::EndMainMenuBar();
    }
    ImGui::Render();
}

// ### Main function ###

int main()
{
    PerfectSurfaceDetection detection;
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
