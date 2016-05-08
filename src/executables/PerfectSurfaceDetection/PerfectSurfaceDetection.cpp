#include "PerfectSurfaceDetection.h"

#include "CPPImplementation.h"

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
    mRotateCamera = false;
    mSurfaceAtomCount = 0;
    mRenderImpostor = false;
    mRenderWithProbeRadius = false;
    mCameraDeltaMovement = glm::vec2(0,0);
    mCameraSmoothTime = 1.f;
    mLightDirection = glm::normalize(glm::vec3(-0.5, -0.75, -0.3));
    mSelectedAtom = 0;

    // Create window
    mpWindow = generateWindow();

    // Clear color
    glClearColor(0.f, 0.f, 0.f, 1.f);

    // # Callbacks

    // Register keyboard callback
    std::function<void(int, int, int, int)> kC = [&](int k, int s, int a, int m)
    {
        this->keyCallback(k, s, a, m);
    };
    setKeyCallback(mpWindow, kC);

    // Register mouse button callback
    std::function<void(int, int, int)> kB = [&](int b, int a, int m)
    {
        this->mouseButtonCallback(b, a, m);
    };
    setMouseButtonCallback(mpWindow, kB);

    // Register scroll callback
    std::function<void(double, double)> kS = [&](double x, double y)
    {
        this->scrollCallback(x,y);
    };
    setScrollCallback(mpWindow, kS);

    // # Load protein

    // /*
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
    // */

    /*
    // Simple PDB loader
    // mAtomStructs = parseSimplePDB(std::string(RESOURCES_PATH) + "/molecules/SimplePDB/PDB_Polymerase-of-E-coli-DNA.txt", mProteinMinExtent, mProteinMaxExtent);
    // mAtomStructs = parseSimplePDB(std::string(RESOURCES_PATH) + "/molecules/SimplePDB/PDB_Myoglobin.txt", mProteinMinExtent, mProteinMaxExtent);
    mAtomStructs = parseSimplePDB(std::string(RESOURCES_PATH) + "/molecules/SimplePDB/PDB_Nitrogen-Paracoccus-Cytochrome-C550.txt", mProteinMinExtent, mProteinMaxExtent);
    // mAtomStructs = parseSimplePDB(std::string(RESOURCES_PATH) + "/molecules/SimplePDB/8AtomsIntersection.txt", mProteinMinExtent, mProteinMaxExtent);
    */

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
            5.f * cameraRadius));

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

    // Buffer
    glGenBuffers(1, &mSurfaceAtomBuffer); // just generate, filled in GLSL or CPP implementatin method

    // Texture (which will be bound as image)
    glGenTextures(1, &mSurfaceAtomTexture); // connected to buffer in renderLoop(), since it seems that is has to be done after buffer fill

    // Print time for data transfer
    glEndQuery(GL_TIME_ELAPSED);
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
    std::cout << "Surface atom count: " << mSurfaceAtomCount << std::endl;
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
    ShaderProgram proteinPointProgram = ShaderProgram("/PerfectSurfaceDetection/proteinPoint.vert", "/PerfectSurfaceDetection/point.frag");
    ShaderProgram surfacePointProgram = ShaderProgram("/PerfectSurfaceDetection/surfacePoint.vert", "/PerfectSurfaceDetection/point.frag");
    ShaderProgram selectionPointProgram = ShaderProgram("/PerfectSurfaceDetection/selectionPoint.vert", "/PerfectSurfaceDetection/point.frag");
    ShaderProgram proteinImpostorProgram = ShaderProgram("/PerfectSurfaceDetection/proteinImpostor.vert", "/PerfectSurfaceDetection/impostor.geom", "/PerfectSurfaceDetection/impostor.frag");
    ShaderProgram surfaceImpostorProgram = ShaderProgram("/PerfectSurfaceDetection/surfaceImpostor.vert", "/PerfectSurfaceDetection/impostor.geom", "/PerfectSurfaceDetection/impostor.frag");
    ShaderProgram selectionImpostorProgram = ShaderProgram("/PerfectSurfaceDetection/selectionImpostor.vert", "/PerfectSurfaceDetection/impostor.geom", "/PerfectSurfaceDetection/impostor.frag");

    // Bind SSBO with atoms
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mAtomsSSBO);

    // Bind buffer to texture (may not be done before buffer filling? Probably not necessary for GLSL implementation since already bound there and filled on GPU)
    glBindTexture(GL_TEXTURE_BUFFER, mSurfaceAtomTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, mSurfaceAtomBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    // Bind image with buffer where indices of surface atoms are read from
    glBindImageTexture(
        1,
        mSurfaceAtomTexture,
        0,
        GL_TRUE,
        0,
        GL_READ_ONLY,
        GL_R32UI);

    // Projection matrix (hardcoded viewport size)
    glm::mat4 projection = glm::perspective(glm::radians(45.f), (GLfloat)1280 / (GLfloat)720, 0.1f, 1000.f);
    /*
    GLfloat halfWidth = ((GLfloat) 1280) / 2.f;
    GLfloat halfHeight = ((GLfloat) 720) / 2.f;
    GLfloat zoom = 0.1f;
    glm::mat4 projection = glm::ortho(
        zoom * -halfWidth,
        zoom * halfWidth,
        zoom * -halfHeight,
        zoom * halfHeight,
        0.1f,
        100.0f);
    */
    proteinPointProgram.use();
    proteinPointProgram.update("projection", projection);
    surfacePointProgram.use();
    surfacePointProgram.update("projection", projection);
    selectionPointProgram.use();
    selectionPointProgram.update("projection", projection);
    proteinImpostorProgram.use();
    proteinImpostorProgram.update("projection", projection);
    surfaceImpostorProgram.use();
    surfaceImpostorProgram.update("projection", projection);
    selectionImpostorProgram.use();
    selectionImpostorProgram.update("projection", projection);

    // Call render function of Rendering.h with lambda function
    render(mpWindow, [&] (float deltaTime)
    {
        // Clear buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
        mupCamera->update();

        // Light direction
        if(mRotateLight)
        {
            mLightDirection = - glm::normalize(mupCamera->getPosition() - mupCamera->getCenter());
        }

        // Drawing
        if(mRenderImpostor)
        {
            // Draw complete atoms with impostors
            proteinImpostorProgram.use();
            proteinImpostorProgram.update("view", mupCamera->getViewMatrix());
            proteinImpostorProgram.update("cameraWorldPos", mupCamera->getPosition());
            proteinImpostorProgram.update("probeRadius", mRenderWithProbeRadius ? mProbeRadius : 0.f);
            proteinImpostorProgram.update("lightDir", mLightDirection);
            glDrawArrays(GL_POINTS, 0, mAtomCount);

            // Draw surface atoms with impostors
            surfaceImpostorProgram.use();
            surfaceImpostorProgram.update("view", mupCamera->getViewMatrix());
            surfaceImpostorProgram.update("cameraWorldPos", mupCamera->getPosition());
            surfaceImpostorProgram.update("probeRadius", mRenderWithProbeRadius ? mProbeRadius : 0.f);
            surfaceImpostorProgram.update("lightDir", mLightDirection);
            glDrawArrays(GL_POINTS, 0, mSurfaceAtomCount);

            // Draw selected atom with impostor
            selectionImpostorProgram.use();
            selectionImpostorProgram.update("view", mupCamera->getViewMatrix());
            selectionImpostorProgram.update("cameraWorldPos", mupCamera->getPosition());
            selectionImpostorProgram.update("probeRadius", mRenderWithProbeRadius ? mProbeRadius : 0.f);
            selectionImpostorProgram.update("lightDir", mLightDirection);
            selectionImpostorProgram.update("atomIndex", mSelectedAtom);
            glDrawArrays(GL_POINTS, 0, 1);
        }
        else
        {
            // Draw complete protein with points
            proteinPointProgram.use();
            proteinPointProgram.update("view", mupCamera->getViewMatrix());
            glDrawArrays(GL_POINTS, 0, mAtomCount);

            // Draw surface atoms with points
            surfacePointProgram.use();
            surfacePointProgram.update("view", mupCamera->getViewMatrix());
            glDrawArrays(GL_POINTS, 0, mSurfaceAtomCount);

            // Draw selected atom with point
            selectionPointProgram.use();
            selectionPointProgram.update("view", mupCamera->getViewMatrix());
            selectionPointProgram.update("atomIndex", mSelectedAtom);
            glDrawArrays(GL_POINTS, 0, 1);
        }
    });
}

void PerfectSurfaceDetection::keyCallback(int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
            case GLFW_KEY_ESCAPE: { glfwSetWindowShouldClose(mpWindow, GL_TRUE); break; }
            case GLFW_KEY_I: { mRenderImpostor = !mRenderImpostor; break; }
            case GLFW_KEY_P: { mRenderWithProbeRadius = !mRenderWithProbeRadius; break; }
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

    // # Prepare output vector
    std::vector<unsigned int> surfaceAtomIndices;

    // # Simulate compute shader
    CPPImplementation cppImplementation;

    // Do it for each atom
    std::cout << "*** ALGORITHM OUTPUT START ***" << std::endl;
    for(int i = 0; i < mAtomCount; i++) // all atoms
    // for(int i = 0; i < 1; i++) // just first atom
    {
        cppImplementation.execute(i, mAtomCount, mProbeRadius, mAtomStructs, surfaceAtomIndices);
    }
    std::cout << "*** ALGORITHM OUTPUT END ***" << std::endl;

    // Fill surface atom indices to mSurfaceAtomBuffer
    glBindBuffer(GL_TEXTURE_BUFFER, mSurfaceAtomBuffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(GLuint) * surfaceAtomIndices.size(), surfaceAtomIndices.data(), GL_STATIC_DRAW);
    glBindBuffer(0, mSurfaceAtomBuffer);

    // Fetch count
    mSurfaceAtomCount = surfaceAtomIndices.size();
}

void PerfectSurfaceDetection::runGLSLImplementation()
{
    std::cout << "GLSL implementation used!" << std::endl;

    // # Compile shader
    ShaderProgram computeProgram(GL_COMPUTE_SHADER, "/PerfectSurfaceDetection/surface.comp");

    // # Prepare atomic counter for writing results to unique position in image
    GLuint atomicCounter;
    glGenBuffers(1, &atomicCounter);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicCounter);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
    resetAtomicCounter(atomicCounter);

    // # Prepare output buffer
    glBindBuffer(GL_TEXTURE_BUFFER, mSurfaceAtomBuffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(GLuint) * mAtomCount, 0, GL_DYNAMIC_COPY); // DYNAMIC_COPY because filled by GPU and read by GPU
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

    // Bind buffer to texture which is used as image
    glBindTexture(GL_TEXTURE_BUFFER, mSurfaceAtomTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, mSurfaceAtomBuffer);
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
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 1, atomicCounter);

    // Bind image where indices of surface atoms are written to
    glBindImageTexture(2,
                       mSurfaceAtomTexture,
                       0,
                       GL_TRUE,
                       0,
                       GL_WRITE_ONLY,
                       GL_R32UI);

    // Print time for setup
    glEndQuery(GL_TIME_ELAPSED);
    glGetQueryObjectuiv(mQuery, GL_QUERY_RESULT, &timeElapsed);
    std::cout << "Time for setup on GPU: " << std::to_string(timeElapsed) << "ns" << std::endl;

    // Start query for time measurement
    glBeginQuery(GL_TIME_ELAPSED, mQuery);

    // Dispatch
    glDispatchCompute((mAtomCount / 16) + 1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    // Print time for execution
    glEndQuery(GL_TIME_ELAPSED);
    glGetQueryObjectuiv(mQuery, GL_QUERY_RESULT, &timeElapsed);
    std::cout << "Time for execution on GPU: " << std::to_string(timeElapsed) << "ns (= " << std::to_string(timeElapsed / 1000000.f) << "ms)" << std::endl;

    // Fetch count
    mSurfaceAtomCount = readAtomicCounter(atomicCounter);

    // TODO: Delete atomic counter
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
