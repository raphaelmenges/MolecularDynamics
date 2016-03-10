#include "surfaceExtraction.h"
#include "Molecule/MDtrajLoader/MdTraj/MdTrajWrapper.h"
#include "Molecule/MDtrajLoader/Data/Protein.h"

SurfaceExtraction::SurfaceExtraction()
{

}


glm::mat4 rotationMatrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;

    return glm::mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}


void SurfaceExtraction::init()
{
    window = generateWindow(256,256);

    rot90Y = rotationMatrix(glm::vec3(0,1,0), 90.0f/180.0f * 3.14159265358979323846264338327950288f);

    // load a file
    std::vector<std::string> paths;
    //paths.push_back("/home/nlichtenberg/Files/PDB/1crn.pdb");
    //paths.push_back("/home/nlichtenberg/Files/PDB/2plt.pdb");
    paths.push_back("/home/nlichtenberg/Files/PDB/1a19.pdb");
    //paths.push_back("/home/nlichtenberg/Files/PDB/155C.pdb");
    //paths.push_back("/home/nlichtenberg/Files/PDB/1vis.pdb");
    //paths.push_back("/home/nlichtenberg/Files/PDB/Develop/Mol_Sandbox/resources/TrajectoryFiles/1aon.pdb");
    MdTrajWrapper mdwrap;
    std::auto_ptr<Protein> prot = mdwrap.load(paths);
    //prot->getAtoms()->resize(727);
    prot->recenter();
    std::cout << "Protein " << prot->getName() << " with " << prot->getAtoms()->size() << " atoms" << std::endl;
    impSph = new ImpostorSpheres(!useAtomicCounters, true);
    impSph->setProteinData(prot.get());
    impSph->init();
    num_balls = impSph->num_balls;

    if(perspectiveProj)
        projection = perspective(45.0f, getRatio(window), 0.1f, 100.0f);
    else
        projection = ortho(-20.0f, 20.0f, -20.0f, 20.0f, -200.0f, 200.0f);

    if (useAtomicCounters)
    {
        spRenderImpostor = ShaderProgram("/SurfaceAtomsDetection/Impostor/impostorSpheres_InstancedUA.vert",
                                         "/SurfaceAtomsDetection/Detection/solidColorInstanceCount.frag");
        spRenderDiscs = ShaderProgram("/SurfaceAtomsDetection//Impostor/impostorSpheres_InstancedUA.vert",
                                      "/SurfaceAtomsDetection//Impostor/impostorSpheres_discardFragments_Instanced.frag");
        spRenderBalls_p = ShaderProgram("/SurfaceAtomsDetection/Base/modelViewProjectionInstancedUA.vert",
                                        "/SurfaceAtomsDetection/Impostor/Impostor3DSphere.frag");
        if(perspectiveProj)
            spRenderBalls = ShaderProgram("/SurfaceAtomsDetection/Base/modelViewProjectionInstancedUA.vert",
                                          "/SurfaceAtomsDetection/Impostor/Impostor3DSphere.frag");
        else
            spRenderBalls = ShaderProgram("/SurfaceAtomsDetection/Base/modelViewProjectionInstancedUA.vert",
                                          "/SurfaceAtomsDetection/Impostor/Impostor3DSphere_Ortho_StoreIntervals_2.frag");
    }
    else
    {
        spRenderImpostor = ShaderProgram("/SurfaceAtomsDetection/Impostor/impostorSpheres_Instanced.vert",
                                         "/SurfaceAtomsDetection/Detection/solidColorInstanceCount.frag");
        spRenderDiscs = ShaderProgram("/SurfaceAtomsDetection/Impostor/impostorSpheres_Instanced.vert",
                                      "/SurfaceAtomsDetection/Impostor/impostorSpheres_discardFragments_Instanced.frag");
        spRenderBalls = ShaderProgram("/SurfaceAtomsDetection/Impostor/impostorSpheres_Instanced.vert",
                                      "/SurfaceAtomsDetection/Impostor/Impostor3DSphere.frag");
    }

    // Setup semaphore texture for atomic fragment access blocks
    tex_Semaphore = new Texture(GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT);
    tex_Semaphore->gen2DTexture(getWidth(window), getHeight(window));

    // Setup 3D texture to store depth intervals and ID references
    tex_3DintervalStorageBuffer = new Texture(GL_RGBA16F, GL_RGBA, GL_FLOAT);
    tex_3DintervalStorageBuffer->gen3DTexture(getWidth(window), getHeight(window), perPixelDepth);

    /// Renderpass to render impostors/fake geometry
    renderBalls = new RenderPass(
                impSph,
                &spRenderBalls,
                getWidth(window),
                getHeight(window));

    renderBalls->update("projection", projection);
    renderBalls->texture("semaphore",tex_Semaphore);
    renderBalls->texture("intervalBuffer",tex_3DintervalStorageBuffer);
    renderBalls->update("width", getWidth(window));
    renderBalls->update("height", getHeight(window));
    renderBalls->update("perPixelDepth", perPixelDepth);
    renderBalls->update("rotY", rot90Y);


    // define projection matrix for other shader programs
    renderBalls->setShaderProgram(&spRenderBalls_p);
    renderBalls->update("projection", projection);
    renderBalls->setShaderProgram(&spRenderDiscs);
    renderBalls->update("projection", projection);
    renderBalls->setShaderProgram(&spRenderImpostor);
    renderBalls->update("projection", projection);

    /// Renderpass to detect the visible instances
    collectSurfaceIDs = new RenderPass(
                new Quad(),
                new ShaderProgram("/SurfaceAtomsDetection/Base/fullscreen.vert",
                                  "/SurfaceAtomsDetection/Detection/CollectVisibleIDs.frag"),
                getWidth(window),
                getHeight(window));

    // prepare 1D buffer for entries
    tex_collectedIDsBuffer = new Texture(GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_INT);
    tex_collectedIDsBuffer->genUimageBuffer(num_balls);
    collectSurfaceIDs->texture("collectedIDsBuffer", tex_collectedIDsBuffer);
    collectSurfaceIDs->texture("intervalBuffer", tex_3DintervalStorageBuffer);

    /// renderpass to display result frame
    result = new RenderPass(
                new Quad(),
                new ShaderProgram("/SurfaceAtomsDetection/Base/fullscreen.vert",
                                  "/SurfaceAtomsDetection/Detection/toneMapperLinearInstanceCount.frag"));
    result->texture("tex", renderBalls->get("fragColor"));

    /// compute shader to process a list of visible IDs (with the actual instanceID of the first general draw)
    computeSortedIDs = new ComputeProgram(new ShaderProgram("/SurfaceAtomsDetection/Detection/SortVisibleIDList.comp"));

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
    //glCullFace(GL_BACK);
    //glDisable(GL_DEPTH_TEST);
}

void SurfaceExtraction::run()
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

        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) (rotY - deltaTime < 0)? rotY -= deltaTime + 6.283 : rotY -= deltaTime;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) (rotY + deltaTime > 6.283)? rotY += deltaTime - 6.283 : rotY += deltaTime;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) (rotX - deltaTime < 0)? rotX -= deltaTime + 6.283 : rotX -= deltaTime;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) (rotX + deltaTime > 6.283)? rotX += deltaTime - 6.283 : rotX += deltaTime;
        if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) distance += deltaTime * 50;
        if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) distance = max(distance - deltaTime * 50, 0.0f);
        if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS) probeRadius += 0.1;
        if (glfwGetKey(window, GLFW_KEY_COMMA) == GLFW_PRESS) probeRadius = glm::max(probeRadius - 0.1f, 0.01f);
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {pingPongOff = true; updateVisibilityMapLock = true; updateVisibilityMap = true; projection = perspective(45.0f, getRatio(window), 0.1f, 100.0f);
            renderBalls->setShaderProgram(&spRenderBalls_p);renderBalls->update("projection", projection); }
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {updateVisibilityMapLock = false; projection = ortho(-15.0f, 15.0f, -15.0f, 15.0f, 1.0f, 300.0f); renderBalls->setShaderProgram(&spRenderBalls);renderBalls->update("projection", projection);}
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) pingPongOff = false;
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) pingPongOff = true;
        if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS) { vsync = !vsync; vsync ? glfwSwapInterval(1) : glfwSwapInterval(0); vsync ? std::cout << "VSync enabled\n" : std::cout << "VSync disabled\n"; }

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
        //mat4 view2 = translate(mat4(1), vec3(0,0,-distance)) * eulerAngleXY(-rotX, -rotY- 90.0f/180.0f * 3.14159265358979323846264338327950288f);

        // reset the detected instance IDs
        glBindTexture(GL_TEXTURE_1D, tex_collectedIDsBuffer->getHandle());
        glTexImage1D(GL_TEXTURE_1D, 0, GL_R8UI, num_balls, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, zeros);

        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomBuff);
        glClearBufferSubData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, 0, sizeof(GLuint), GL_RED_INTEGER, GL_UNSIGNED_INT, zero);

        // reset 3D tex
        tex_3DintervalStorageBuffer->reset();
        tex_Semaphore->reset();

        renderBalls->clear(-1,-1,-1,-1); // the clear color may not interfere with the detected instance IDs
        renderBalls->clearDepth();
        renderBalls->update("scale", vec2(scale));
        renderBalls->update("view", view);
        //renderBalls->update("view_second", view2);
        renderBalls->update("probeRadius", probeRadius);

        glBeginQuery(GL_TIME_ELAPSED, timeQuery);
        renderBalls->run();
        glEndQuery(GL_TIME_ELAPSED);
        glGetQueryObjectuiv(timeQuery, GL_QUERY_RESULT, &queryTime);
        //std::cout << "render/interval shader time: " << queryTime/1000000000.0 << std::endl;

        // Depending on user input: sort out instances for the next frame or not,
        // or lock the current set of visible instances
        if(useAtomicCounters)
            if (updateVisibilityMap && !pingPongOff)
            {
                // the following shaders in detectVisible look at what has been written to the screen (framebuffer0)
                // better render the instance stuff to a texture and read from there
                collectSurfaceIDs->run();
                //std::cout << "collect IDs shader time: " << queryTime/1000000000.0 << std::endl;
                //glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);
                computeSortedIDs->run(16,1,1); // 16 work groups * 1024 work items = 16384 atoms and IDs
                //glMemoryBarrier(GL_ATOMIC_COUNTER_BARRIER_BIT|GL_SHADER_IMAGE_ACCESS_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);
                //std::cout << "compute shader time: " << queryTime/1000000000.0 << std::endl;

                //                int x = tex_3DintervalStorageBuffer->getX();
                //                int y = tex_3DintervalStorageBuffer->getY();
                //                int z = tex_3DintervalStorageBuffer->getZ();
                //                int slice = 0;
                ////                // Check buffer data
                //                GLfloat *bufferData = new GLfloat[4 * x * y * z];
                //                glBindTexture(GL_TEXTURE_3D, tex_3DintervalStorageBuffer->getHandle());
                //                //glBindImageTexture(0, tex_3DintervalStorageBuffer->getHandle(), 0, GL_TRUE, 0, GL_READ_WRITE, tex_3DintervalStorageBuffer->getInternalFormat());
                //                glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_FLOAT, bufferData);

                //                int numIntervals_3_0 = 0;
                //                int numIntervals_0_3 = 0;
                //                int numIntervals_1_3 = 0;
                //                int numIntervals_3_1 = 0;
                //                int numIntervals_0_0 = 0;
                //                int maxIntervals = 0;
                //                GLuint visibleIDsFromBuff[impSph->num_balls];
                //                for(int i = 0; i < y; i++){
                //                    if (!(300 < i && i < 400))
                //                        continue;
                //                    for(int j = 0; j < x; j++){
                //                        if (!(140 < j && j < 220))
                //                            continue;
                //                   //     int start =  ((x * y * slice) + (i * x) + j) * 4;
                //      //                  if ((float)bufferData[start] == 0 /*|| (float)bufferData[start+2] == (float)bufferData[start+3]*/)
                //      //                      continue;
                //                //        std::cout << "Texel at " << i << " " << j << " " << slice << " has color " << (float)bufferData[start] << " " << (float)bufferData[start + 1] << " " << (float)bufferData[start + 2] << " " << (float)bufferData[start + 3] << std::endl;
                //                 //       std::cout << "Number of intervals: " << (float)bufferData[((x * y * 62) + (i * x) + j) * 4] << std::endl;
                //                        if( (float)bufferData[((x * y * 63) + (i * x) + j) * 4] != 0)
                //                        {
                //                            float slices = (float)bufferData[((x * y * 63) + (i * x) + j) * 4];
                //                            for( int n = 0; n < 1; n++)
                //                            {
                //                              int start =  ((x * y * n) + (i * x) + j) * 4;
                //                              if(!((float)bufferData[start + 2] == (float)bufferData[start + 3]))
                //                                  continue;
                //                              std::cout << "Slice " << n << std::endl;
                //                              std::cout << "Texel at " << i << " " << j << " " << slice << " has color " << (float)bufferData[start] << " " << (float)bufferData[start + 1] << " " << (float)bufferData[start + 2] << " " << (float)bufferData[start + 3] << std::endl;
                //                            }
                //                        }
                //                        maxIntervals = max(maxIntervals, (int)bufferData[((x * y * 63) + (i * x) + j) * 4]);

                //        //                  Check buffer data
                //                        //glBindTexture(GL_TEXTURE_1D, tex_visibleIDsBuffer->getHandle());
                //                        //glGetTexImage(GL_TEXTURE_1D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, visibleIDsFromBuff);

                //                        if((float)bufferData[start+2] == 0 && (float)bufferData[start+3] == 3)
                //                            numIntervals_0_3++;
                //                        if((float)bufferData[start+2] == 3 && (float)bufferData[start+3] == 0)
                //                            numIntervals_3_0++;
                //                        if((float)bufferData[start+2] == 1 && (float)bufferData[start+3] == 3)
                //                            numIntervals_1_3++;
                //                        if((float)bufferData[start+2] == 3 && (float)bufferData[start+3] == 1)
                //                            numIntervals_3_1++;
                //                        if((float)bufferData[start+2] == 0&& (float)bufferData[start+3] == 0)
                //                            numIntervals_0_0++;
                //                    }
                //                }

                //                std::cout << "number of 3_0 intervals: " << numIntervals_3_0 << std::endl;
                //                std::cout << "number of 0_3 intervals: " << numIntervals_0_3 << std::endl;
                //                std::cout << "number of 1_3 intervals: " << numIntervals_1_3 << std::endl;
                //                std::cout << "number of 3_1 intervals: " << numIntervals_3_1 << std::endl;
                //                std::cout << "number of 0_0 intervals: " << numIntervals_0_0 << std::endl;
                //                std::cout << "max intervals: " << maxIntervals << std::endl;

                //                delete(bufferData);

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
        else
            if (updateVisibilityMap && !pingPongOff)
            {
                collectSurfaceIDs->run();
                // detect visible instances
                glBeginQuery(GL_TIME_ELAPSED, timeQuery);
                GLuint visibilityMapFromBuff[impSph->num_balls];
                glBindTexture(GL_TEXTURE_1D, tex_collectedIDsBuffer->getHandle());
                glGetTexImage(GL_TEXTURE_1D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, visibilityMapFromBuff);

                // copy visible instance information to a vector with size = num_balls
                int num_vis = 0;
                std::vector<GLint> newMap;
                newMap.resize(num_balls);
                for (int i = 0; i < num_balls; i++)
                {
                    newMap[i] = (int)visibilityMapFromBuff[i];
                    if(visibilityMapFromBuff[i] != 0)
                        num_vis++;
                }
                glEndQuery(GL_TIME_ELAPSED);
                glGetQueryObjectuiv(timeQuery, GL_QUERY_RESULT, &queryTime);
                std::cout << "cpu time: " << queryTime << std::endl;

                // print number of visible instances
                std::cout << "Number of visible instances: " << num_vis << std::endl;
                impSph->updateVisibilityMap(newMap);
                updateVisibilityMap = false;
            }else
            {
                if(!updateVisibilityMapLock)
                {
                    impSph->updateVisibilityMap(mapAllVisible);
                    updateVisibilityMap = true; // sort out every other frame
                }
            }

        //result->clear(0.3,0.3,0.3,1);
        //result->run();
    });
}

void printProperties()
{
    generateWindow();
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

    //    int maxAtomicCountersFrag;
    //    glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS, &maxAtomicCountersFrag);
    //    std::cout << "GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS: " << maxAtomicCountersFrag << std::endl;
}

int main(int argc, char *argv[]) {
    printProperties();

    SurfaceExtraction demo;
    demo.perspectiveProj = false;
    demo.init();



    demo.run();
}

