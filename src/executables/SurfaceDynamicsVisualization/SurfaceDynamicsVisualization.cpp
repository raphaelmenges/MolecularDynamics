#include "SurfaceDynamicsVisualization.h"

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
#include <sstream>
#include <iomanip>
#include <thread>
#include <functional>

// ### Class implementation ###

SurfaceDynamicsVisualization::SurfaceDynamicsVisualization()
{
    // # Setup members
    mSurfaceCount = 0;
    mCameraDeltaMovement = glm::vec2(0,0);
    mCameraSmoothTime = 1.f;
    mLightDirection = glm::normalize(glm::vec3(-0.5, -0.75, -0.3));

    // Create window
    mpWindow = generateWindow("Surface Dynamics Visualization");

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

    // # Load PDB protein

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

    /*
    // Simple PDB loader (provided by scientist who wrote paper where the surface extraction algorithm bases on)
    // mAtomStructs = parseSimplePDB(std::string(RESOURCES_PATH) + "/molecules/SimplePDB/PDB_Polymerase-of-E-coli-DNA.txt", mProteinMinExtent, mProteinMaxExtent);
    mAtomStructs = parseSimplePDB(std::string(RESOURCES_PATH) + "/molecules/SimplePDB/PDB_Myoglobin.txt", mProteinMinExtent, mProteinMaxExtent);
    // mAtomStructs = parseSimplePDB(std::string(RESOURCES_PATH) + "/molecules/SimplePDB/PDB_Nitrogen-Paracoccus-Cytochrome-C550.txt", mProteinMinExtent, mProteinMaxExtent);
    // mAtomStructs = parseSimplePDB(std::string(RESOURCES_PATH) + "/molecules/SimplePDB/8AtomsIntersection.txt", mProteinMinExtent, mProteinMaxExtent);
    */

    // TESTING

    // Protein animation loader (PDB and XTC)
    std::vector<std::string> paths;
    paths.push_back("/home/raphael/ownCloud/Daten/GIIIA_Native.pdb");
    paths.push_back("/home/raphael/ownCloud/Daten/MD_GIIIA_Native_100ns.xtc"); // XTC file is not allowed to be published :(

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
        // Decide which frame to extract
        int frame = (int)(0.0f * (float)(pAtom->getFrameCount()-1));
        std::cout << pAtom->getFrameCount() << std::endl;

        // Push back all atoms (CONVERTING PICOMETER TO ANGSTROM)
        mAtomStructs.push_back(
            AtomStruct(
                glm::vec3(
                    pAtom->getXAtFrame(frame),
                    pAtom->getYAtFrame(frame),
                    pAtom->getZAtFrame(frame)),
                0.01f * mAtomLUT.vdW_radii_picometer.at(
                    pAtom->getElement())));
    }


    // END OF TESTING

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
            mCameraDefaultAlpha,
            mCameraDefaultBeta,
            cameraRadius,
            cameraRadius / 2.f,
            5.f * cameraRadius,
            45.f,
            0.05f));

    // # Create query to measure execution time
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

    // # Prepare buffers

    // Buffers
    glGenBuffers(1, &mInputIndicesBuffer);
    glGenBuffers(1, &mInternalIndicesBuffer);
    glGenBuffers(1, &mSurfaceIndicesBuffer);

    // TODO: understand why buffer to texture binding is necessary in GLSL method and in renderLoop()
    // Texture (which will be bound as image, connected to buffer in renderLoop(), since it seems that is has to be done after buffer fill)
    glGenTextures(1, &mInputIndicesTexture);
    glGenTextures(1, &mInternalIndicesTexture);
    glGenTextures(1, &mSurfaceIndicesTexture);

    // Intially filling of mInputIndicesBuffer
    resetInputIndicesBuffer();

    // Prepare output buffer with maximum of necessary space
    glBindBuffer(GL_TEXTURE_BUFFER, mInternalIndicesBuffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(GLuint) * mAtomCount, 0, GL_DYNAMIC_COPY); // DYNAMIC_COPY because filled by GPU and read by GPU
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

    glBindBuffer(GL_TEXTURE_BUFFER, mSurfaceIndicesBuffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(GLuint) * mAtomCount, 0, GL_DYNAMIC_COPY); // DYNAMIC_COPY because filled by GPU and read by GPU
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

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
    if(mInitiallyUseGLSLImplementation)
    {
        runGLSLImplementation();
    }
    else
    {
        runCPPImplementation(true);
    }

    // Output count
    std::cout << "Internal atom count: " << mInternalCount << std::endl;
    std::cout << "Surface atom count: " << mSurfaceCount << std::endl;

    // Prepare testing the surface
    glGenBuffers(1, &mSurfaceTestVBO);
    glGenVertexArrays(1, &mSurfaceTestVAO);
    mupSurfaceTestProgram = std::unique_ptr<ShaderProgram>(new ShaderProgram("/SurfaceDynamicsVisualization/sample.vert", "/SurfaceDynamicsVisualization/sample.frag"));
}

SurfaceDynamicsVisualization::~SurfaceDynamicsVisualization()
{
    // TODO: Delete OpenGL stuff
    // - ssbo
    // - texture
    // - buffer

    // Testing surface
    glDeleteVertexArrays(1, &mSurfaceTestVAO);
    glDeleteBuffers(1, &mSurfaceTestVBO);

    // Delete query object
    glDeleteQueries(1, &mQuery);
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
            impostorProgram.update("cuttingPlane", mCuttingPlane);

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
                impostorProgram.update("color", mInternalAtomColor);
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
                impostorProgram.update("color", mSurfaceAtomColor);
                glDrawArrays(GL_POINTS, 0, mSurfaceCount);
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
            pointProgram.update("cuttingPlane", mCuttingPlane);

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
                pointProgram.update("color", mInternalAtomColor);
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
                pointProgram.update("color", mSurfaceAtomColor);
                glDrawArrays(GL_POINTS, 0, mSurfaceCount);
            }
        }

        // Drawing of surface test
        if(mShowSamplePoint)
        {
            glPointSize(mSamplePointSize);
            mupSurfaceTestProgram->use();
            mupSurfaceTestProgram->update("view", mupCamera->getViewMatrix());
            mupSurfaceTestProgram->update("projection", mupCamera->getProjectionMatrix());
            mupSurfaceTestProgram->update("color", mSamplePointColor);
            glBindVertexArray(mSurfaceTestVAO);
            glDrawArrays(GL_POINTS, 0, mSurfaceTestSampleCount);
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
                if(mSelectedAtom >= mAtomCount) { mSelectedAtom = 0; }
                break;
            }
            case GLFW_KEY_LEFT:
            {
                mSelectedAtom--;
                if(mSelectedAtom < 0) { mSelectedAtom = mAtomCount - 1; }
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

GLuint SurfaceDynamicsVisualization::readAtomicCounter(GLuint atomicCounter) const
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

void SurfaceDynamicsVisualization::resetAtomicCounter(GLuint atomicCounter) const
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

void SurfaceDynamicsVisualization::resetInputIndicesBuffer()
{
    std::vector<GLuint> inputIndices;
    inputIndices.reserve(mAtomCount);
    for(unsigned int i = 0; i < (unsigned int)mAtomCount; i++) { inputIndices.push_back(i); }
    glBindBuffer(GL_TEXTURE_BUFFER, mInputIndicesBuffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(GLuint) * inputIndices.size(), inputIndices.data(), GL_DYNAMIC_COPY); // Same copy policy as other buffers
    glBindBuffer(0, mInternalIndicesBuffer);
}

std::vector<GLuint> SurfaceDynamicsVisualization::readTextureBuffer(GLuint buffer, int size) const
{
    std::vector<GLuint> data;
    data.resize(size);
    glBindBuffer(GL_TEXTURE_BUFFER, buffer);
    glGetBufferSubData(GL_TEXTURE_BUFFER, 0, sizeof(GLuint) * size, data.data());
    return data;
}

void SurfaceDynamicsVisualization::updateComputationInformation(std::string device, float computationTime)
{
    std::stringstream stream;
    stream <<
        device << " used" << "\n"
        << "Probe radius: " + std::to_string(mProbeRadius) << "\n"
        << "Time: " << computationTime << "ms" << "\n"
        << "Atom count: " + std::to_string(mAtomCount) << "\n"
        << "Internal atoms: " + std::to_string(mInternalCount) << "\n"
        << "Surface atoms: " + std::to_string(mSurfaceCount);
    mComputeInformation = stream.str();
}

void SurfaceDynamicsVisualization::updateGUI()
{
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));

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

    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 0.75f));

    // Computation window
    if(mShowSurfaceComputationWindow)
    {
        ImGui::Begin("Surface Computation", NULL, 0);
        ImGui::SliderFloat("Probe radius", &mProbeRadius, 0.f, 2.f, "%.1f");
        ImGui::SliderInt("Layer removal count", &mLayerRemovalCount, 0, 5);
        ImGui::SliderInt("CPU Cores", &mCPPThreads, 1, 24);
        if(ImGui::Button("Run GPGPU")) { runGLSLImplementation(); }
        if(ImGui::Button("Run Single-Core CPU")) { runCPPImplementation(false); }
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
        ImGui::SliderFloat("Cutting Plane Offset", &mCuttingPlane, 0.f, 200.f, "%.1f");
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

    // Debugging window
    if(mShowDebuggingWindow)
    {
        ImGui::Begin("Debugging", NULL, 0);
        ImGui::Text(std::string("Selected Atom: " + std::to_string(mSelectedAtom)).c_str());
        ImGui::End();
    }

    ImGui::PopStyleColor();

    ImGui::Render();
}

void SurfaceDynamicsVisualization::testSurface()
{
    std::cout << "*** SURFACE TEST START ***" << std::endl;

    // Read data back from OpenGL buffers
    std::vector<GLuint> inputIndices = readTextureBuffer(mInputIndicesBuffer, mInputCount);
    std::vector<GLuint> internalIndices = readTextureBuffer(mInternalIndicesBuffer, mInternalCount);
    std::vector<GLuint> surfaceIndices = readTextureBuffer(mSurfaceIndicesBuffer, mSurfaceCount);

    // Seed
    std::srand(mSurfaceTestSeed);

    // Vector of samples
    std::vector<glm::vec3> samples;

    // Count cases of failure
    int internalSampleFailures = 0;
    int surfaceAtomsFailures = 0;

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
        glm::vec3 atomCenter = mAtomStructs[i].center;
        float atomExtRadius = mAtomStructs[i].radius + mProbeRadius;

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
                if(glm::distance(samplePosition, mAtomStructs[k].center) < (mAtomStructs[k].radius + mProbeRadius))
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
    glBindVertexArray(mSurfaceTestVAO);

    // Fill vertex buffer with vertices
    glBindBuffer(GL_ARRAY_BUFFER, mSurfaceTestVBO);
    glBufferData(GL_ARRAY_BUFFER, samples.size() * sizeof(glm::vec3), samples.data(), GL_DYNAMIC_DRAW);

    // Bind it to shader program
    GLint posAttrib = glGetAttribLocation(mupSurfaceTestProgram->getProgramHandle(), "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Unbind everything
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Remember about complete count of samples for drawing
    mSurfaceTestSampleCount = samples.size();

    // Draw output of test to GUI
    mTestOutput = "Wrong classification as internal for " + std::to_string(internalSampleFailures) + " samples.\n";
    mTestOutput += "Maybe wrong classification as surface for " + std::to_string(surfaceAtomsFailures) + " atoms.";

    std::cout << "*** SURFACE TEST END ***" << std::endl;
}

void SurfaceDynamicsVisualization::runCPPImplementation(bool threaded)
{
    std::cout << "CPP implementation used!" << std::endl;

    // Note
    // No input indices used, just complete molecule is processed.
    // No support for layer removal.

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
            int count = mAtomCount / mCPPThreads;
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
                            mAtomCount,
                            mProbeRadius,
                            mAtomStructs,
                            internalIndicesSubvector,
                            surfaceIndicesSubvector);
                    }
                },
                offset, // minIndex
                i == mCPPThreads - 1 ? mAtomCount - 1 : offset+count-1, // maxIndex
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
        for(int a = 0; a < mAtomCount; a++)
        {
            cppImplementation.execute(a, mAtomCount, mProbeRadius, mAtomStructs, internalIndices, surfaceIndices);
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
    mInputCount = mAtomCount;
    mInternalCount = internalIndices.size();
    mSurfaceCount = surfaceIndices.size();

    // Update compute information
    updateComputationInformation(
        threaded ? ("Multi-Core CPU (" + std::to_string(mCPPThreads) + " Threads)") : "Single-Core CPU",
        computationTime);
}

void SurfaceDynamicsVisualization::runGLSLImplementation()
{
    std::cout << "GLSL implementation used!" << std::endl;

    // # Compile shader
    ShaderProgram computeProgram(GL_COMPUTE_SHADER, "/SurfaceDynamicsVisualization/surface.comp");

    // # Prepare atomic counter for writing results to unique position in image
    GLuint internalCounter;
    glGenBuffers(1, &internalCounter);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, internalCounter);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);

    GLuint surfaceCounter;
    glGenBuffers(1, &surfaceCounter);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, surfaceCounter);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);

    // Bind buffer to texture which is used as image
    glBindTexture(GL_TEXTURE_BUFFER, mInputIndicesTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, mInputIndicesBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glBindTexture(GL_TEXTURE_BUFFER, mInternalIndicesTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, mInternalIndicesBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glBindTexture(GL_TEXTURE_BUFFER, mSurfaceIndicesTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, mSurfaceIndicesBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    // Variable to measure elapsed time
    GLuint timeElapsed = 0;

    // Use compute shader program
    computeProgram.use();

    // Probe radius
    computeProgram.update("probeRadius", mProbeRadius);

    // Bind SSBO with atoms
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mAtomsSSBO);

    // Bind atomic counter
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, internalCounter);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 2, surfaceCounter);

    // Start query for time measurement
    glBeginQuery(GL_TIME_ELAPSED, mQuery);

    // # Do it as often as indicated
    for(int i = 0; i <= mLayerRemovalCount; i++)
    {
        if(i == 0)
        {
            // Reset input indices buffer (just set 0..mAtomCount-1);
            resetInputIndicesBuffer();

            // All atoms are input
            mInputCount = mAtomCount;
        }
        else
        {
            // Swap input and internal indices buffer
            GLuint a = mInputIndicesTexture;
            GLuint b = mInputIndicesBuffer;
            mInputIndicesTexture = mInternalIndicesTexture;
            mInputIndicesBuffer = mInternalIndicesBuffer;
            mInternalIndicesTexture = a;
            mInternalIndicesBuffer = b;

            // Old internal are now input
            mInputCount = (int)readAtomicCounter(internalCounter);
        }

        // Reset atomic counter
        resetAtomicCounter(surfaceCounter);
        resetAtomicCounter(internalCounter);

        // Tell shader about count of input atoms
        computeProgram.update("inputCount", mInputCount);

        // Bind texture as image where input indices are listed
        glBindImageTexture(3,
                           mInputIndicesTexture,
                           0,
                           GL_TRUE,
                           0,
                           GL_READ_ONLY,
                           GL_R32UI);

        // Bind textures as images where output indices are written to
        glBindImageTexture(4,
                           mInternalIndicesTexture,
                           0,
                           GL_TRUE,
                           0,
                           GL_WRITE_ONLY,
                           GL_R32UI);
        glBindImageTexture(5,
                           mSurfaceIndicesTexture,
                           0,
                           GL_TRUE,
                           0,
                           GL_WRITE_ONLY,
                           GL_R32UI);

        // Dispatch
        glDispatchCompute((mInputCount / 64) + 1, 1, 1);
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
    }

    // Print time for execution
    glEndQuery(GL_TIME_ELAPSED);
    GLuint done = 0;
    while(done == 0)
    {
        glGetQueryObjectuiv(mQuery, GL_QUERY_RESULT_AVAILABLE, &done);
    }
    glGetQueryObjectuiv(mQuery, GL_QUERY_RESULT, &timeElapsed);
    float computationTime = timeElapsed / 1000000.f;

    // Fetch count
    mInternalCount = readAtomicCounter(internalCounter);
    mSurfaceCount = readAtomicCounter(surfaceCounter);

    // Update compute information
    updateComputationInformation("GPGPU", computationTime);

    // Delete atomic counter buffers
    glDeleteBuffers(1, &internalCounter);
    glDeleteBuffers(1, &surfaceCounter);
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
