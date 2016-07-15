#include "SurfaceDynamicsVisualization.h"
#include "Utils/OrbitCamera.h"
#include "ShaderTools/Renderer.h"
#include "Molecule/MDtrajLoader/MdTraj/MdTrajWrapper.h"
#include "Molecule/MDtrajLoader/Data/Protein.h"
#include "SimpleLoader.h"
#include "imgui/imgui.h"
#include "imgui/examples/opengl3_example/imgui_impl_glfw_gl3.h"
#include <glm/gtx/component_wise.hpp>
#include <sstream>
#include <iomanip>

// stb_image wants those defines
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

// ### Class implementation ###

SurfaceDynamicsVisualization::SurfaceDynamicsVisualization()
{
    // # Setup members
    mCameraDeltaRotation = glm::vec2(0,0);
    mCameraRotationSmoothTime = 1.f;
    mLightDirection = glm::normalize(glm::vec3(-0.5, -0.75, -0.3));
    mWindowWidth = mInitialWindowWidth;
    mWindowHeight = mInitialWindowHeight;

    // Create window (which initializes OpenGL)
    mpWindow = generateWindow(mWindowTitle, mWindowWidth, mWindowHeight);

    // Create empty outline atoms indices after OpenGL initialization
    mupOutlineAtomIndices = std::unique_ptr<GPUTextureBuffer>(new GPUTextureBuffer(0)); // create empty outline atom indices buffer

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
        0x25cb, 0x25cb, // circle
        0x212b, 0x212b, // angstrom
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

    // # Prepare framebuffers for rendering
    mupMoleculeFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(mWindowWidth, mWindowHeight));
    mupMoleculeFramebuffer->addAttachment(Framebuffer::ColorFormat::RGBA); // color
    mupMoleculeFramebuffer->addAttachment(Framebuffer::ColorFormat::RGB); // picking index
    mupOverlayFramebuffer = std::unique_ptr<Framebuffer>(new Framebuffer(mWindowWidth, mWindowHeight));
    mupOverlayFramebuffer->addAttachment(Framebuffer::ColorFormat::RGBA);

    // # Ascension rendering
    mupAscension = std::unique_ptr<GPUTextureBuffer>(new GPUTextureBuffer(0));

    // # Prepare cubemap
    std::vector<std::string> cubemapFullpaths;
    /*
    cubemapFullpaths.push_back(std::string(std::string(RESOURCES_PATH) + "/cubemaps/NissiBeach/posx.jpg"));
    cubemapFullpaths.push_back(std::string(std::string(RESOURCES_PATH) + "/cubemaps/NissiBeach/negx.jpg"));
    cubemapFullpaths.push_back(std::string(std::string(RESOURCES_PATH) + "/cubemaps/NissiBeach/posy.jpg"));
    cubemapFullpaths.push_back(std::string(std::string(RESOURCES_PATH) + "/cubemaps/NissiBeach/negy.jpg"));
    cubemapFullpaths.push_back(std::string(std::string(RESOURCES_PATH) + "/cubemaps/NissiBeach/posz.jpg"));
    cubemapFullpaths.push_back(std::string(std::string(RESOURCES_PATH) + "/cubemaps/NissiBeach/negz.jpg"));
    */
    ///*
    cubemapFullpaths.push_back(std::string(std::string(RESOURCES_PATH) + "/cubemaps/Simple/posx.png"));
    cubemapFullpaths.push_back(std::string(std::string(RESOURCES_PATH) + "/cubemaps/Simple/negx.png"));
    cubemapFullpaths.push_back(std::string(std::string(RESOURCES_PATH) + "/cubemaps/Simple/posy.png"));
    cubemapFullpaths.push_back(std::string(std::string(RESOURCES_PATH) + "/cubemaps/Simple/negy.png"));
    cubemapFullpaths.push_back(std::string(std::string(RESOURCES_PATH) + "/cubemaps/Simple/posz.png"));
    cubemapFullpaths.push_back(std::string(std::string(RESOURCES_PATH) + "/cubemaps/Simple/negz.png"));
    //*/

    // Setup stb_image
    // stbi_set_flip_vertically_on_load(true);
    int width, height, channelCount;

    // Create texture
    glGenTextures(1, &mCubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mCubemapTexture);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

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

    // # Create path

    // Shader
    mupPathProgram = std::unique_ptr<ShaderProgram>(new ShaderProgram("/SurfaceDynamicsVisualization/path.vert", "/SurfaceDynamicsVisualization/path.frag"));

    // Generate and bind vertex array object
    glGenVertexArrays(1, &mPathVAO);
    glBindVertexArray(mPathVAO);

    // Generate vertex buffer but do not fill it
    glGenBuffers(1, &mPathVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mPathVBO);

    // Bind it to shader program
    mPathPositionAttribute = glGetAttribLocation(mupPathProgram->getProgramHandle(), "position");
    glEnableVertexAttribArray(mPathPositionAttribute);
    glVertexAttribPointer(mPathPositionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0); // called again at path creation

    // Unbind everything
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

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
    mupGPUProtein = std::unique_ptr<GPUProtein>(new GPUProtein(upProtein.get()));

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
            0.1f));

    // # Run implementation to extract surface atoms
    computeLayers(0, 0, mInitiallyUseGLSLImplementation);

    // # Other

    // Prepare validation of the surface
    mupSurfaceValidation = std::unique_ptr<SurfaceValidation>(new SurfaceValidation());

    // Set endframe in GUI to maximum number
    mComputationEndFrame = mupGPUProtein->getFrameCount() - 1;
}

SurfaceDynamicsVisualization::~SurfaceDynamicsVisualization()
{
    // Delete cubemap
    glDeleteTextures(1, &mCubemapTexture);

    // Delete path
    glDeleteVertexArrays(1, &mPathVAO);
    glDeleteBuffers(1, &mPathVBO);
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

    // Shader program for rendering the protein
    ShaderProgram hullProgram("/SurfaceDynamicsVisualization/hull.vert", "/SurfaceDynamicsVisualization/impostor.geom", "/SurfaceDynamicsVisualization/impostor.frag");
    ShaderProgram ascensionProgram("/SurfaceDynamicsVisualization/ascension.vert", "/SurfaceDynamicsVisualization/impostor.geom", "/SurfaceDynamicsVisualization/impostor.frag");

    // Shader program for screenfilling quad rendering
    ShaderProgram screenFillingProgram("/SurfaceDynamicsVisualization/screenfilling.vert", "/SurfaceDynamicsVisualization/screenfilling.geom", "/SurfaceDynamicsVisualization/screenfilling.frag");

    // Shader program for cubemap
    ShaderProgram cubemapProgram("/SurfaceDynamicsVisualization/cubemap.vert", "/SurfaceDynamicsVisualization/cubemap.geom", "/SurfaceDynamicsVisualization/cubemap.frag");

    // Shader program for outline rendering
    ShaderProgram outlineProgram("/SurfaceDynamicsVisualization/hull.vert", "/SurfaceDynamicsVisualization/impostor.geom", "/SurfaceDynamicsVisualization/outline.frag");

    // Bind SSBOs
    mupGPUProtein->bind(0, 1);

    // Call render function of Rendering.h with lambda function
    render(mpWindow, [&] (float deltaTime)
    {
        // Viewport size
        glm::vec2 resolution = getResolution(mpWindow);
        mWindowWidth = resolution.x;
        mWindowHeight = resolution.y;

        // # Update everything before drawing

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

        // Calculate cursor movement
        double cursorX, cursorY;
        glfwGetCursorPos(mpWindow, &cursorX, &cursorY);
        GLfloat cursorDeltaX = (float)cursorX - prevCursorX;
        GLfloat cursorDeltaY = (float)cursorY - prevCursorY;
        bool lockCursorPosition = false;

        // Rotate camera
        if(mRotateCamera)
        {
            mCameraDeltaRotation = glm::vec2(cursorDeltaX, cursorDeltaY);
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

        // # Fill overlay framebuffer
        mupOverlayFramebuffer->bind();
        mupOverlayFramebuffer->resize(mWindowWidth, mWindowHeight);
        glViewport(0, 0, mWindowWidth, mWindowHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Get count of atoms which will get a outline (size of buffer can be used here because all elements are valid)
        int outlineAtomCount = mupOutlineAtomIndices->getSize();
        if(outlineAtomCount > 0)
        {
            // Bind texture buffer with input atoms
            mupOutlineAtomIndices->bindAsImage(2, GPUTextureBuffer::GPUAccess::READ_ONLY);

            // Probe radius
            float probeRadius = mRenderWithProbeRadius ? mProbeRadius : 0.f;

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
                mInternalSamplePointColor,
                mSurfaceSamplePointColor,
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

        // Drawing of path
        if(mShowPath && mPathVertexCount > 0)
        {
            glDisable(GL_DEPTH_TEST);
            glBindVertexArray(mPathVAO);
            glBindBuffer(GL_ARRAY_BUFFER, mPathVBO);

            // Point size for rendering
            glPointSize(mPathPointSize);

            // Decide which path parts are rendered
            int offset = glm::clamp(mFrame - mPathFrameRadius, 0, mPathVertexCount); // on which frame path starts
            glVertexAttribPointer(mPathPositionAttribute, 3, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(GLfloat) * 3 * offset));
            int count = glm::min(((mPathFrameRadius * 2) + 1), mPathVertexCount - offset);

            // General shader setup
            mupPathProgram->use();
            mupPathProgram->update("view", mupCamera->getViewMatrix());
            mupPathProgram->update("projection", mupCamera->getProjectionMatrix());
            mupPathProgram->update("pastColor", mPastPathColor);
            mupPathProgram->update("futureColor", mFuturePathColor);
            mupPathProgram->update("frame", mFrame);
            mupPathProgram->update("offset", offset);

            // Drawing
            glDrawArrays(GL_LINE_STRIP, 0, count); // path as line
            glDrawArrays(GL_POINTS, 0, count); // path as points

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            glEnable(GL_DEPTH_TEST);
        }

        // Unbind framebuffer for overlay
        mupOverlayFramebuffer->unbind();

         // # Fill molecule framebuffer
        mupMoleculeFramebuffer->bind();
        mupMoleculeFramebuffer->resize(mWindowWidth, mWindowHeight);
        glViewport(0, 0, mWindowWidth, mWindowHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Decide about surface rendering
        switch(mSurfaceRendering)
        {
        case SurfaceRendering::HULL:

            // Prepare shader program
            hullProgram.use();
            hullProgram.update("view", mupCamera->getViewMatrix());
            hullProgram.update("projection", mupCamera->getProjectionMatrix());
            hullProgram.update("cameraWorldPos", mupCamera->getPosition());
            hullProgram.update("probeRadius", mRenderWithProbeRadius ? mProbeRadius : 0.f);
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
                mGPUSurfaces.at(mFrame - mComputedStartFrame)->bindInternalIndicesForDrawing(mLayer, 2);
                hullProgram.update("color", mInternalAtomColor);
                glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfInternalAtoms(mLayer));
            }

            // Draw surface
            if(mShowSurface)
            {
                mGPUSurfaces.at(mFrame - mComputedStartFrame)->bindSurfaceIndicesForDrawing(mLayer, 2);
                hullProgram.update("color", mSurfaceAtomColor);
                glDrawArrays(GL_POINTS, 0, mGPUSurfaces.at(mFrame - mComputedStartFrame)->getCountOfSurfaceAtoms(mLayer));
            }

            break;

        case SurfaceRendering::ASCENSION:

            // Bind texture buffer with ascension information as image
            mupAscension->bindAsImage(2, GPUTextureBuffer::GPUAccess::READ_ONLY);

            // Prepare shader program
            ascensionProgram.use();
            ascensionProgram.update("view", mupCamera->getViewMatrix());
            ascensionProgram.update("projection", mupCamera->getProjectionMatrix());
            ascensionProgram.update("cameraWorldPos", mupCamera->getPosition());
            ascensionProgram.update("probeRadius", mRenderWithProbeRadius ? mProbeRadius : 0.f);
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
            ascensionProgram.update("hotColor", mAscensionHotColor);
            ascensionProgram.update("coldColor", mAscensionColdColor);
            glDrawArrays(GL_POINTS, 0, mupGPUProtein->getAtomCount());

            break;
        }

        // Unbind molecule framebuffer
        mupMoleculeFramebuffer->unbind();

        // # Fill standard framebuffer
        glViewport(0, 0, mWindowWidth, mWindowHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Disable depth test
        glDisable(GL_DEPTH_TEST);

        // Render cubemap
        glDisable(GL_DEPTH_TEST);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mCubemapTexture);
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

void SurfaceDynamicsVisualization::setWindowTitle(float progress)
{
    if(progress != 1.f)
    {
        int intProgress = (int)(progress * 100);
        std::string stringProgress = intProgress < 10 ? "0" + std::to_string(intProgress) : std::to_string(intProgress);
        glfwSetWindowTitle(mpWindow, std::string(mWindowTitle + " [" + stringProgress + "%]").c_str());
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
        << "Probe radius: " + std::to_string(mProbeRadius) << "\n"
        << "Extracted layers: " << (mExtractLayers ? "yes" : "no") << "\n"
        << "Start frame: " << mComputedStartFrame << " End frame: " << mComputedEndFrame << "\n"
        << "Count of frames: " << (mComputedEndFrame - mComputedStartFrame + 1) << "\n"
        << "Time: " << computationTime << "ms";
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

            ImGui::EndMenu();
        }

        // Help menu
        if (ImGui::BeginMenu("Help"))
        {
            ImGui::BeginPopup("Help");
            ImGui::Text("LMB: Rotate camera");
            ImGui::Text("MMB: Move camera");
            ImGui::Text("RMB: Select atom");
            ImGui::EndPopup();
        }

        // About menu
        if (ImGui::BeginMenu("About"))
        {
            ImGui::BeginPopup("About");
            ImGui::Text("Author: Raphael Menges");
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
        ImGui::Text("Computation");
        ImGui::SliderFloat("Probe Radius", &mProbeRadius, 0.f, 2.f, "%.1f");
        ImGui::Checkbox("Extract Layers", &mExtractLayers);
        ImGui::SliderInt("Start Frame", &mComputationStartFrame, 0, mComputationEndFrame);
        ImGui::SliderInt("End Frame", &mComputationEndFrame, mComputationStartFrame, mupGPUProtein->getFrameCount() - 1);
        ImGui::SliderInt("CPU Threads", &mCPUThreads, 1, 24);
        if(ImGui::Button("\u2794 GPGPU")) { computeLayers(mComputationStartFrame, mComputationEndFrame, true); }
        ImGui::SameLine();
        if(ImGui::Button("\u2794 CPU")) { computeLayers(mComputationStartFrame, mComputationEndFrame, false); }
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

        // Render points / spheres
        ImGui::Text("Rendering");

        // Surface rendering
        ImGui::Combo("##SurfaceRenderingCombo", (int*)&mSurfaceRendering, "Hull\0Ascension\0");

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
        ImGui::Text("Animation Smoothing");
        ImGui::SliderInt("Smooth Radius", &mSmoothAnimationRadius, 0, 10);
        ImGui::SliderFloat("Smooth Max Deviation", &mSmoothAnimationMaxDeviation, 0, 100, "%.1f");
        ImGui::Separator();

        // Effects
        ImGui::Text("Effects");
        if(ImGui::Button("Spot Light"))
        {
            mLightDirection = -glm::normalize(mupCamera->getPosition() - mupCamera->getCenter());
        }

        ImGui::SliderFloat("Depth Darkening Start", &mDepthDarkeningStart, 0, mDepthDarkeningEnd, "%.1f");
        ImGui::SliderFloat("Depth Darkening End", &mDepthDarkeningEnd, mDepthDarkeningStart, mDepthDarkeningMaxEnd, "%.1f");

        ImGui::End();
        ImGui::PopStyleColor(); // window background
    }

    // Information window
    if(mShowInformationWindow)
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.5f, 0.0f, 0.75f)); // window background
        ImGui::Begin("Information", NULL, 0);

        // General infos
        ImGui::Text(std::string("Atom Count: " + std::to_string(mupGPUProtein->getAtomCount())).c_str());
        ImGui::Text(std::string("Selected Atom: " + std::to_string(mSelectedAtom)).c_str());

        // Show available GPU memory
        int availableMemory;
        glGetIntegerv(0x9049, &availableMemory); // Nvidia only
        availableMemory = availableMemory / 1000;
        ImGui::Text(std::string("Available VRAM: " + std::to_string(availableMemory) + "MB").c_str());
        // TODO: one may want to clean up glGetError if query failed and show failure message on GUI (for example on Intel or AMD)
        ImGui::Separator();

        // Testing
        ImGui::Text("Testing");
        std::vector<float> curve;
        curve.push_back(0.5f);
        curve.push_back(0.6f);
        ImGui::PlotLines("Curve", curve.data(), curve.size());

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
                mupGPUProtein.get(),
                mGPUSurfaces.at(mFrame - mComputedStartFrame).get(),
                mFrame,
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
            // Information about atom
            ImGui::Text("%05d", atomIndex);
            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 60);

            // Select that atom (use ## to add number for an unique button id)
            if(ImGui::Button(std::string("\u25cb##" + std::to_string(atomIndex)).c_str()))
            {
                mSelectedAtom = atomIndex;
            }
            if(ImGui::IsItemHovered()) { ImGui::SetTooltip("Select Atom"); } // tooltip
            ImGui::SameLine();

            // Remove that atom from analysis atoms (use ## to add number for an unique button id)
            if(ImGui::Button(std::string("\u00D7##" + std::to_string(atomIndex)).c_str()))
            {
                toBeRemoved.push_back(atomIndex);
                analysisAtomsChanged = true;
            }
            if(ImGui::IsItemHovered()) { ImGui::SetTooltip("Remove Atom"); } // tooltip
        }

        // Remove atoms
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
            auto rTrajectoy = mupGPUProtein->getTrajectory();

            // Go over frames
            std::vector<glm::vec3> path;
            mPathLength = 0;
            for(const auto& frame : *(rTrajectoy.get()))
            {
                // Go over analysed atoms and accumulate position in frame
                glm::vec3 accPosition(0,0,0);
                for(int atomIndex : mAnalyseAtoms)
                {
                    accPosition += frame.at(atomIndex);
                }

                // Calculate average
                glm::vec3 avgPosition = accPosition / mAnalyseAtoms.size();

                // Caluclate and accumulate distance
                if(!path.empty())
                {
                    mPathLength += glm::distance(avgPosition, path.back());
                }

                // Push new average position value to vector after calculating distance
                path.push_back(avgPosition);
            }

            // Fill it to vertex buffer of path
            glBindBuffer(GL_ARRAY_BUFFER, mPathVBO);
            glBufferData(GL_ARRAY_BUFFER, path.size() * sizeof(glm::vec3), path.data(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            mPathVertexCount = path.size();
        }

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
        ImGui::SameLine();

        // Display length of path
        std::ostringstream stringPathLength;
        stringPathLength << std::fixed << std::setprecision(2) << mPathLength;
        ImGui::Text(std::string("Path Length: " + stringPathLength.str() + " \u212b").c_str());

        // Radius of frames in path visualization
        ImGui::SliderInt("Path Radius", &mPathFrameRadius, 1, 100);

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
        // Process one frame
        if(useGPU)
        {
            mGPUSurfaces.push_back(std::move(mupGPUSurfaceExtraction->calculateSurface(mupGPUProtein.get(), i, mProbeRadius, mExtractLayers)));
        }
        else
        {
            mGPUSurfaces.push_back(std::move(mupGPUSurfaceExtraction->calculateSurface(mupGPUProtein.get(), i, mProbeRadius, mExtractLayers, true, mCPUThreads)));
        }
        computationTime += mGPUSurfaces.back()->getComputationTime();

        // Show progress
        float progress = (float)(i- startFrame + 1) / (float)(endFrame - startFrame + 1);
        setWindowTitle(progress);
    }

    // Remember which frames were computed
    mComputedStartFrame = startFrame;
    mComputedEndFrame = endFrame;

    // Update compute information
    updateComputationInformation(
        (useGPU ? "GPU" : "CPU with " + std::to_string(mCPUThreads) + " threads"), computationTime);

    // Set to first animation frame
    setFrame(mComputedStartFrame);

    // Calculate ascension for visualization
    // TODO: fill with computation of mupAscension

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
