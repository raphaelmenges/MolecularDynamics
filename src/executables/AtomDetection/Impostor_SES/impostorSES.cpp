#include "impostorSES.h"
#include "Molecule/MDtrajLoader/MdTraj/MdTrajWrapper.h"

#include "DynamicVertexArrayObject.h"
#include "MoleculeSESAtomImpostor.h"
#include "MoleculeSESSpherePatchImpostor.h"
#include "MoleculeSESToroidalPatchImpostor.h"

ImpostorSES::ImpostorSES()
{

}

bool renderAtoms = true;
bool renderSpherePatches = false;
bool renderToroidalPatches = false;
bool vsync = true;
bool animate = false;

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        case GLFW_KEY_ESCAPE: { glfwSetWindowShouldClose(window, GL_TRUE); break; }
        case GLFW_KEY_B: { vsync = !vsync; vsync ? glfwSwapInterval(1) : glfwSwapInterval(0); vsync ? std::cout << "VSync enabled\n" : std::cout << "VSync disabled\n"; break; }
        case GLFW_KEY_F1: { renderAtoms = !renderAtoms; break; }
        case GLFW_KEY_F2: { renderSpherePatches = !renderSpherePatches; break; }
        case GLFW_KEY_F3: { renderToroidalPatches = !renderToroidalPatches; break; }
        case GLFW_KEY_F9: { animate = !animate; break; }
        }
    }
}

void updateInputMapping(RenderPass* rp, ShaderProgram &sp)
{
    auto vao = static_cast<DynamicVertexArrayObject*>(rp->vertexArrayObject);
    vao->enableVertexAttribArrays(sp.inputMap);
}

void ImpostorSES::init()
{
    window = generateWindow();

    // load a file
    std::vector<std::string> paths;
    paths.push_back("/home/nlichtenberg/1crn.pdb");
    //paths.push_back("/home/nlichtenberg/1vis.pdb");
    //paths.push_back("/home/nlichtenberg/Develop/Mol_Sandbox/resources/TrajectoryFiles/1aon.pdb");

    MdTrajWrapper mdwrap;
    prot = mdwrap.load(paths);
    prot->recenter();


    impSph = new ImpostorSpheres(false, false);
    impSph->setProteinData(prot.get());
    impSph->init();
    num_balls = impSph->num_balls;

    if(perspectiveProj)
        projection = perspective(45.0f, getRatio(window), 0.1f, 100.0f);
    else
        projection = ortho(-30.0f, 30.0f, -30.0f, 30.0f, 0.0f, 100.0f);


    spRenderImpostor = ShaderProgram("/VisibleAtomsDetection/Impostor/impostorSpheres_InstancedUA.vert",
                                     "/VisibleAtomsDetection/Detection/solidColorInstanceCount.frag");
    spRenderDiscs = ShaderProgram("/VisibleAtomsDetection//Impostor/impostorSpheres_InstancedUA.vert",
                                  "/VisibleAtomsDetection//Impostor/impostorSpheres_discardFragments_Instanced.frag");
    if(perspectiveProj)
        spRenderBalls = ShaderProgram("/VisibleAtomsDetection/Base/modelViewProjectionInstancedUA.vert",
                                      "/VisibleAtomsDetection/Impostor/Impostor3DSphere.frag");
    else
        spRenderBalls = ShaderProgram("/VisibleAtomsDetection/Base/modelViewProjectionInstancedUA.vert",
                                      "/VisibleAtomsDetection/Impostor/Impostor3DSphere_Ortho.frag");


    /// Renderpass to render impostors/fake geometry
    renderBalls = new RenderPass(
                impSph,
                &spRenderBalls,
                getWidth(window),
                getHeight(window));

    renderBalls->update("projection", projection);

    // define projection matrix for other shader programs
    renderBalls->setShaderProgram(&spRenderDiscs);
    renderBalls->update("projection", projection);
    renderBalls->setShaderProgram(&spRenderImpostor);
    renderBalls->update("projection", projection);

    /// Renderpass to detect the visible instances
    collectSurfaceIDs = new RenderPass(
                new Quad(),
                new ShaderProgram("/VisibleAtomsDetection/Base/fullscreen.vert",
                                  "/VisibleAtomsDetection/Detection/DetectVisibleInstanceIDs.frag"),
                getWidth(window),
                getHeight(window));

    // prepare 1D buffer for entries
    tex_collectedIDsBuffer = new Texture(GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_INT);
    tex_collectedIDsBuffer->genUimageBuffer(num_balls);
    collectSurfaceIDs->texture("collectedIDsBuffer", tex_collectedIDsBuffer);
    collectSurfaceIDs->texture("tex", renderBalls->get("InstanceID"));

    /// renderpass to display result frame
    result = new RenderPass(
                new Quad(),
                new ShaderProgram("/VisibleAtomsDetection/Base/fullscreen.vert",
                                  "/VisibleAtomsDetection/Detection/toneMapperLinearInstanceCount.frag"));
    result->texture("tex", renderBalls->get("fragColor"));

    /// compute shader to process a list of visible IDs (with the actual instanceID of the first general draw)
    computeSortedIDs = new ComputeProgram(new ShaderProgram("/VisibleAtomsDetection/Detection/CreateVisibleIDList.comp"));

    // 1D buffer for visible IDs
    tex_sortedVisibleIDsBuffer = new Texture(GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT);
    tex_sortedVisibleIDsBuffer->genUimageBuffer(num_balls);

    computeSortedIDs->texture("collectedIDsBuffer", tex_collectedIDsBuffer);
    computeSortedIDs->texture("sortedVisibleIDsBuffer", tex_sortedVisibleIDsBuffer);

    // atomic counter buffer for consecutive index access in compute shader
    glGenBuffers(1, &atomBuff);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomBuff);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 3, atomBuff);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    positions_size = sizeof(glm::vec4) * impSph->instance_positions_s.instance_positions.size();
    colors_size = sizeof(glm::vec4) * impSph->instance_colors_s.instance_colors.size();

    // SSBO setup
    glGenBuffers(2, SSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, positions_size, &impSph->instance_positions_s.instance_positions[0], GL_DYNAMIC_COPY);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, SSBO[0], 0, positions_size);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, colors_size, &impSph->instance_colors_s.instance_colors[0], GL_DYNAMIC_COPY);
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, SSBO[1], 0, colors_size);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // SSBO copy data
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO[0]);
    GLvoid* p_ = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(p_, &impSph->instance_positions_s.instance_positions[0], positions_size);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO[1]);
    p_ = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(p_, &impSph->instance_colors_s.instance_colors[0], colors_size);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    // SSBO bind to shaders
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO[0]);
    GLuint block_index = 0;
    block_index = glGetProgramResourceIndex(spRenderBalls.getProgramHandle(), GL_SHADER_STORAGE_BLOCK, "instance_positions_t");
    glShaderStorageBlockBinding(spRenderBalls.getProgramHandle(), block_index, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO[1]);
    block_index = glGetProgramResourceIndex(spRenderBalls.getProgramHandle(), GL_SHADER_STORAGE_BLOCK, "instance_colors_t");
    glShaderStorageBlockBinding(spRenderBalls.getProgramHandle(), block_index, 1);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO[0]);
    block_index = glGetProgramResourceIndex(spRenderDiscs.getProgramHandle(), GL_SHADER_STORAGE_BLOCK, "instance_positions_t");
    glShaderStorageBlockBinding(spRenderBalls.getProgramHandle(), block_index, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO[1]);
    block_index = glGetProgramResourceIndex(spRenderDiscs.getProgramHandle(), GL_SHADER_STORAGE_BLOCK, "instance_colors_t");
    glShaderStorageBlockBinding(spRenderBalls.getProgramHandle(), block_index, 1);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO[0]);
    block_index = glGetProgramResourceIndex(spRenderImpostor.getProgramHandle(), GL_SHADER_STORAGE_BLOCK, "instance_positions_t");
    glShaderStorageBlockBinding(spRenderBalls.getProgramHandle(), block_index, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO[1]);
    block_index = glGetProgramResourceIndex(spRenderImpostor.getProgramHandle(), GL_SHADER_STORAGE_BLOCK, "instance_colors_t");
    glShaderStorageBlockBinding(spRenderBalls.getProgramHandle(), block_index, 1);


    // prepare data to reset the buffer that holds the visible instance IDs
    int byteCount = sizeof(unsigned int)* num_balls;
    zeros = new unsigned char[byteCount];
    unsigned int f = 0;
    unsigned char const * p = reinterpret_cast<unsigned char const *>(&f);

    for (int i = 0; i < byteCount; i+=4)
    {
        zeros[i] = p[0];
        zeros[i+1] = p[1];
        zeros[i+2] = p[2];
        zeros[i+3] = p[3];
    }

    // prepare buffer with index = value
    // used to draw all istances

    identityInstancesMap.clear();
    for (GLuint i = 0; i < num_balls; i++)
        identityInstancesMap.push_back(i);

    glBindTexture(GL_TEXTURE_1D, tex_sortedVisibleIDsBuffer->getHandle());
    glTexImage1D(GL_TEXTURE_1D, 0, GL_R32UI, num_balls, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &identityInstancesMap[0]);

    // map to set all instances visible
    mapAllVisible.resize(num_balls);
    std::fill(mapAllVisible.begin(), mapAllVisible.end(), 1);

    // time query
    GLuint timeQuery;
    glGenQueries(1, &timeQuery);

    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);
    //glCullFace(GL_FRONT);
}

void ImpostorSES::initSES()
{
    // one timestep so far ...
    //prot->frames.resize(1);
    prot->recenter();
    prot->setupSimpleAtoms();
    prot->calculatePatches(probeRadius);

    // --- ATOM

    spAtomImpostorQuad              = ShaderProgram("/SGROSS_Molecules/atom_impostor.vert", "/SGROSS_Molecules/atom_impostor.geom", "/SGROSS_Molecules/atom_impostor__quad.frag");
    spAtomImpostorSphere            = ShaderProgram("/SGROSS_Molecules/atom_impostor.vert", "/SGROSS_Molecules/atom_impostor.geom", "/SGROSS_Molecules/atom_impostor__sphere.frag");
    spAtomImpostorSphereRaymarching = ShaderProgram("/SGROSS_Molecules/atom_impostor.vert", "/SGROSS_Molecules/atom_impostor.geom", "/SGROSS_Molecules/atom_impostor__sphere_raymarching.frag");
    spAtomImpostorNormal            = ShaderProgram("/SGROSS_Molecules/atom_impostor.vert", "/SGROSS_Molecules/atom_impostor.geom", "/SGROSS_Molecules/atom_impostor__normal.frag");
    spAtomImpostorFull              = ShaderProgram("/SGROSS_Molecules/atom_impostor.vert", "/SGROSS_Molecules/atom_impostor.geom", "/SGROSS_Molecules/atom_impostor__full.frag");
    spAtomImpostorFullColored       = ShaderProgram("/SGROSS_Molecules/atom_impostor.vert", "/SGROSS_Molecules/atom_impostor.geom", "/SGROSS_Molecules/atom_impostor__full_colored.frag");

    rpAtoms = new RenderPass(new MoleculeSESAtomImpostor(prot), &spAtomImpostorQuad);

    updateInputMapping(rpAtoms, spAtomImpostorQuad);
    rpAtoms->update("projection", projection);

    // --- SPHERE PATCH

    spSpherePatchImpostorTriangle    = ShaderProgram("/SGROSS_Molecules/sphere_patch_impostor.vert", "/SGROSS_Molecules/sphere_patch_impostor__triangle.geom", "/SGROSS_Molecules/sphere_patch_impostor__triangle.frag");
    spSpherePatchImpostorTetrahedron = ShaderProgram("/SGROSS_Molecules/sphere_patch_impostor.vert", "/SGROSS_Molecules/sphere_patch_impostor.geom",           "/SGROSS_Molecules/sphere_patch_impostor__tetrahedron.frag");
    spSpherePatchImpostorNormal      = ShaderProgram("/SGROSS_Molecules/sphere_patch_impostor.vert", "/SGROSS_Molecules/sphere_patch_impostor.geom",           "/SGROSS_Molecules/sphere_patch_impostor__normal.frag");
    spSpherePatchImpostorFull        = ShaderProgram("/SGROSS_Molecules/sphere_patch_impostor.vert", "/SGROSS_Molecules/sphere_patch_impostor.geom",           "/SGROSS_Molecules/sphere_patch_impostor__full.frag");
    spSpherePatchImpostorFullColored = ShaderProgram("/SGROSS_Molecules/sphere_patch_impostor.vert", "/SGROSS_Molecules/sphere_patch_impostor.geom",           "/SGROSS_Molecules/sphere_patch_impostor__full_colored.frag");

    rpSpherePatches = new RenderPass(new MoleculeSESSpherePatchImpostor(prot), &spSpherePatchImpostorTriangle);

    updateInputMapping(rpSpherePatches, spSpherePatchImpostorTriangle);
    rpSpherePatches->update("projection", projection);

    // --- TOROIDAL PATCH

    spToroidalPatchImpostorFrustum     = ShaderProgram("/SGROSS_Molecules/toroidal_patch_impostor.vert", "/SGROSS_Molecules/toroidal_patch_impostor.geom", "/SGROSS_Molecules/toroidal_patch_impostor__frustum.frag");
    spToroidalPatchImpostorNormal      = ShaderProgram("/SGROSS_Molecules/toroidal_patch_impostor.vert", "/SGROSS_Molecules/toroidal_patch_impostor.geom", "/SGROSS_Molecules/toroidal_patch_impostor__normal.frag");
    spToroidalPatchImpostorFull        = ShaderProgram("/SGROSS_Molecules/toroidal_patch_impostor.vert", "/SGROSS_Molecules/toroidal_patch_impostor.geom", "/SGROSS_Molecules/toroidal_patch_impostor__full.frag");
    spToroidalPatchImpostorFullColored = ShaderProgram("/SGROSS_Molecules/toroidal_patch_impostor.vert", "/SGROSS_Molecules/toroidal_patch_impostor.geom", "/SGROSS_Molecules/toroidal_patch_impostor__full_colored.frag");

    rpToroidalPatches = new RenderPass(new MoleculeSESToroidalPatchImpostor(prot), &spToroidalPatchImpostorFrustum);

    updateInputMapping(rpToroidalPatches, spToroidalPatchImpostorFrustum);
    rpToroidalPatches->update("projection", projection);
}

void ImpostorSES::run()
{
    const GLuint zero = 0;

    render(window, [&] (float deltaTime) {

        numberOfFrames++;
        frameInterval += deltaTime;

        if (frameInterval > 1.0f)
        {
            fps = numberOfFrames / frameInterval;

            std::cout << "FPS: " << fps << std::endl;

            numberOfFrames = 0;
            frameInterval = 0.0f;
        }
        glfwSetKeyCallback(window, keyCallback);

        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) (rotY - deltaTime < 0)? rotY -= deltaTime + 6.283 : rotY -= deltaTime;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) (rotY + deltaTime > 6.283)? rotY += deltaTime - 6.283 : rotY += deltaTime;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) (rotX - deltaTime < 0)? rotX -= deltaTime + 6.283 : rotX -= deltaTime;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) (rotX + deltaTime > 6.283)? rotX += deltaTime - 6.283 : rotX += deltaTime;
        if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) distance += deltaTime * 50;
        if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) distance = max(distance - deltaTime * 50, 0.0f);
        if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS) scale += deltaTime*4;
        if (glfwGetKey(window, GLFW_KEY_COMMA) == GLFW_PRESS) scale = glm::max(scale - deltaTime*4, 0.01f);
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {updateVisibilityMapLock = true; updateVisibilityMap = true;}
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) updateVisibilityMapLock = false;
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) pingPongOff = false;
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) pingPongOff = true;

        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)         { rpAtoms->setShaderProgram(&spAtomImpostorQuad);                           updateInputMapping(rpAtoms, spAtomImpostorQuad);                           rpAtoms->update("projection", projection); }
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)         { rpAtoms->setShaderProgram(&spAtomImpostorSphere);                         updateInputMapping(rpAtoms, spAtomImpostorSphere);                         rpAtoms->update("projection", projection); }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)         { rpAtoms->setShaderProgram(&spAtomImpostorSphereRaymarching);              updateInputMapping(rpAtoms, spAtomImpostorSphereRaymarching);              rpAtoms->update("projection", projection); }
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)         { rpAtoms->setShaderProgram(&spAtomImpostorNormal);                         updateInputMapping(rpAtoms, spAtomImpostorNormal);                         rpAtoms->update("projection", projection); }
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)         { rpAtoms->setShaderProgram(&spAtomImpostorFull);                           updateInputMapping(rpAtoms, spAtomImpostorFull);                           rpAtoms->update("projection", projection); }
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)         { rpAtoms->setShaderProgram(&spAtomImpostorFullColored);                    updateInputMapping(rpAtoms, spAtomImpostorFullColored);                    rpAtoms->update("projection", projection); }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)         { rpSpherePatches->setShaderProgram(&spSpherePatchImpostorTriangle);        updateInputMapping(rpSpherePatches, spSpherePatchImpostorTriangle);        rpSpherePatches->update("projection", projection); }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)         { rpSpherePatches->setShaderProgram(&spSpherePatchImpostorTetrahedron);     updateInputMapping(rpSpherePatches, spSpherePatchImpostorTetrahedron);     rpSpherePatches->update("projection", projection); }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)         { rpSpherePatches->setShaderProgram(&spSpherePatchImpostorNormal);          updateInputMapping(rpSpherePatches, spSpherePatchImpostorNormal);          rpSpherePatches->update("projection", projection)->update("probe_radius", probeRadius); }
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)         { rpSpherePatches->setShaderProgram(&spSpherePatchImpostorFull);            updateInputMapping(rpSpherePatches, spSpherePatchImpostorFull);            rpSpherePatches->update("projection", projection)->update("probe_radius", probeRadius); }
        if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS)         { rpSpherePatches->setShaderProgram(&spSpherePatchImpostorFullColored);     updateInputMapping(rpSpherePatches, spSpherePatchImpostorFullColored);     rpSpherePatches->update("projection", projection)->update("probe_radius", probeRadius); }
        if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)         { rpToroidalPatches->setShaderProgram(&spToroidalPatchImpostorFrustum);     updateInputMapping(rpToroidalPatches, spToroidalPatchImpostorFrustum);     rpToroidalPatches->update("projection", projection); }
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)         { rpToroidalPatches->setShaderProgram(&spToroidalPatchImpostorNormal);      updateInputMapping(rpToroidalPatches, spToroidalPatchImpostorNormal);      rpToroidalPatches->update("projection", projection)->update("probe_radius", probeRadius); }
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)         { rpToroidalPatches->setShaderProgram(&spToroidalPatchImpostorFull);        updateInputMapping(rpToroidalPatches, spToroidalPatchImpostorFull);        rpToroidalPatches->update("projection", projection)->update("probe_radius", probeRadius); }
        if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS)         { rpToroidalPatches->setShaderProgram(&spToroidalPatchImpostorFullColored); updateInputMapping(rpToroidalPatches, spToroidalPatchImpostorFullColored); rpToroidalPatches->update("projection", projection)->update("probe_radius", probeRadius); }


        // Render impostor geometry
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        {
            renderBalls->setShaderProgram(&spRenderImpostor);
            renderBalls->texture("sortedVisibleIDsBuffer", tex_sortedVisibleIDsBuffer);
            result->update("maxRange", 1.0f);
            result->texture("tex", renderBalls->get("fragColor"));
        }
        // Render impostor geometry as disc
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        {
            renderBalls->setShaderProgram(&spRenderDiscs);
            renderBalls->texture("sortedVisibleIDsBuffer", tex_sortedVisibleIDsBuffer);
            result->update("maxRange", 1.0f);
            result->texture("tex", renderBalls->get("fragColor"));
        }
        // Render faked geometry
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        {
            renderBalls->setShaderProgram(&spRenderBalls);
            renderBalls->texture("sortedVisibleIDsBuffer", tex_sortedVisibleIDsBuffer);
            result->update("maxRange", 1.0f);
            result->texture("tex", renderBalls->get("fragColor"));
        }
        // Render instance IDs geometry
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
        {
            renderBalls->setShaderProgram(&spRenderBalls);
            renderBalls->texture("sortedVisibleIDsBuffer", tex_sortedVisibleIDsBuffer);
            result->update("maxRange", float(impSph->num_balls));
            result->texture("tex", renderBalls->get("InstanceID"));
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            if(animate)
            {
                animate = false;
                lastTime = glfwGetTime();
            }
            else
            {
                animate = true;
                glfwSetTime(lastTime);
            }
        }

        if (animate)
        {
            elapsedTime = glfwGetTime();
            if (elapsedTime > 628)
            {
                elapsedTime = 0;
                glfwSetTime(0);
            }
        }

        mat4 view = translate(mat4(1), vec3(0,0,-distance)) * eulerAngleXY(-rotX, -rotY);

        // reset the detected instance IDs
        glBindTexture(GL_TEXTURE_1D, tex_collectedIDsBuffer->getHandle());
        glTexImage1D(GL_TEXTURE_1D, 0, GL_R8UI, num_balls, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, zeros);
        //glBindTexture(GL_TEXTURE_1D, visibleIDsBuff->getHandle());
        //glTexImage1D(GL_TEXTURE_1D, 0, GL_R32UI, num_balls, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &identityInstancesMap[0]);

        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomBuff);
        glClearBufferSubData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, 0, sizeof(GLuint), GL_RED_INTEGER, GL_UNSIGNED_INT, zero);

        renderBalls->clear(-1,-1,-1,-1); // the clear color may not interfere with the detected instance IDs
        renderBalls->clearDepth();
        renderBalls->update("scale", vec2(scale));
        renderBalls->update("view", view);
        renderBalls->update("probeRadius", probeRadius);
        renderBalls->run();


        // Depending on user input: sort out instances for the next frame or not,
        // or lock the current set of visible instances
        if (updateVisibilityMap && !pingPongOff)
        {
            // the following shaders in detectVisible look at what has been written to the screen (framebuffer0)
            // better render the instance stuff to a texture and read from there
            collectSurfaceIDs->run();
            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);
            glBeginQuery(GL_TIME_ELAPSED, timeQuery);
            computeSortedIDs->run(16,1,1); // 16 work groups * 1024 work items = 16384 atoms and IDs
            glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT|GL_SHADER_IMAGE_ACCESS_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);
            glEndQuery(GL_TIME_ELAPSED);

            glGetQueryObjectuiv(timeQuery, GL_QUERY_RESULT, &queryTime);
            std::cout << "compute shader time: " << queryTime << std::endl;

            // Check buffer data
            //                GLuint visibilityMapFromBuff[ImpostorSpheres::num_balls];
            //                GLuint visibleIDsFromBuff[ImpostorSpheres::num_balls];
            //                glBindTexture(GL_TEXTURE_1D, bufferTex->getHandle());
            //                glGetTexImage(GL_TEXTURE_1D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, visibilityMapFromBuff);
            //                glBindTexture(GL_TEXTURE_1D, visibleIDsBuff->getHandle());
            //                glGetTexImage(GL_TEXTURE_1D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, visibleIDsFromBuff);

            //get the value of the atomic counter
            glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomBuff);
            GLuint* counterVal = (GLuint*) glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint),  GL_MAP_READ_BIT );
            glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

            impSph->instancesToRender = *counterVal;
            std::cout << "Number of visible instances by atomic Counter: " << *counterVal << std::endl;

            updateVisibilityMap = false;
        }else
        {
            if(!updateVisibilityMapLock)
            {
                // ToDo
                // instead of uploading the data, swap the buffer
                glBindTexture(GL_TEXTURE_1D, tex_sortedVisibleIDsBuffer->getHandle());
                glTexImage1D(GL_TEXTURE_1D, 0, GL_R32UI, num_balls, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &identityInstancesMap[0]);
                impSph->instancesToRender = impSph->num_balls;
                updateVisibilityMap = true; // sort out every other frame
            }
        }

        result->clear(0.15f, 0.15f, 0.15f, 1.0f);
        result->run();

        // just to clear the framebuffer (always)
        rpAtoms
                ->clear(0.15f, 0.15f, 0.15f, 1.0f)
                ->clearDepth();

        if (renderAtoms)
            rpAtoms
                    ->update("view", view)
                    ->run();

        if (renderToroidalPatches)
            rpToroidalPatches
                    ->update("view", view)
                    ->run();

        if (renderSpherePatches)
            rpSpherePatches
                    ->update("view", view)
                    ->run();
    });
}

void printProperties()
{
    // Compute Shader Props
    int groupMax_x;
    int groupMax_y;
    int groupMax_z;
    int maxInvocations;

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &groupMax_x);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &groupMax_y);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &groupMax_z);
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxInvocations);

    std::cout << "Max work group size as (x,y,z): (" << groupMax_x << "," << groupMax_y << "," << groupMax_z << ")" << std::endl;
    std::cout << "Max work group invocations: " << maxInvocations << std::endl;

    // Texture Props
    int maxTexture;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexture);
    std::cout << "Max texture size: " << maxTexture << std::endl;

    int maxTexture3D;
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &maxTexture3D);
    std::cout << "Max 3D texture size: " << maxTexture3D << std::endl;

    //    GLuint tex3d;
    //    glGenTextures(1, &tex3d);
    //    glBindTexture(GL_TEXTURE_3D, tex3d);
    //    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, maxTexture3D, maxTexture3D, 64, 0, GL_RGBA, GL_FLOAT, 0);

    int err = glGetError();
    printf("%d\n", err);


    //    int maxAtomicCountersFrag;
    //    glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS, &maxAtomicCountersFrag);
    //    std::cout << "GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS: " << maxAtomicCountersFrag << std::endl;
}


int main(int argc, char *argv[]) {
    ImpostorSES demo;
    demo.perspectiveProj = true;

    demo.init();
    demo.initSES();

    printProperties();

    demo.run();
}

