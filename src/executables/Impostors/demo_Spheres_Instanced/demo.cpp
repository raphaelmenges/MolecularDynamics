#include <vector>
#include "ShaderTools/Renderer.h"
#include "ShaderTools/RenderPass.h"
#include "ShaderTools/ComputeProgram.h"
#include "AssetTools/Texture.h"
#include "AssetTools/Mesh.h"
#include "ShaderTools/VertexArrayObjects/Quad.h"
#include "ShaderTools/VertexArrayObjects/ImpostorSpheres.h"

using namespace glm;

bool useAtomicCounters = true;
bool vsync = true;

float r(float size) {
    return size * 2 * static_cast <float> (rand()) / static_cast <float> (RAND_MAX) - size;
}

float r2(float size) {
    return size * static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
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
}


int main(int argc, char *argv[]) {
    GLFWwindow* window = generateWindow();

    printProperties();

    float rotX = 0.0f;
    float rotY = 0.0f;
    float distance = 80.0;
    float scale = 1.0;
    bool updateVisibilityMap = false;
    bool updateVisibilityMapLock = false; // L: lock U: unlock reduced elements for current frame
    bool pingPongOff = true; // reduce number of elements every other frame O: off P: Pingpong On

    ImpostorSpheres *impSph = new ImpostorSpheres(!useAtomicCounters);
    int num_balls = ImpostorSpheres::num_balls;
    mat4 projection = perspective(45.0f, getRatio(window), 0.1f, 100.0f);
    float asd = getRatio(window);

    ShaderProgram spRenderImpostor;
    ShaderProgram spRenderDiscs;
    ShaderProgram spRenderBalls;
    if (useAtomicCounters)
    {
        spRenderImpostor = ShaderProgram("/Impostor/impostorSpheres_InstancedUA.vert", "/Filters/solidColorInstanceCount.frag");
        spRenderDiscs = ShaderProgram("/Impostor/impostorSpheres_InstancedUA.vert", "/Impostor/impostorSpheres_discardFragments_Instanced.frag");
        spRenderBalls = ShaderProgram("/Impostor/impostorSpheres_InstancedUA.vert", "/Impostor/impostorSpheres_Instanced.frag");
    }
    else
    {
        spRenderImpostor = ShaderProgram("/Impostor/impostorSpheres_Instanced.vert", "/Filters/solidColorInstanceCount.frag");
        spRenderDiscs = ShaderProgram("/Impostor/impostorSpheres_Instanced.vert", "/Impostor/impostorSpheres_discardFragments_Instanced.frag");
        spRenderBalls = ShaderProgram("/Impostor/impostorSpheres_Instanced.vert", "/Impostor/impostorSpheres_Instanced.frag");
    }


    /// Renderpass to render impostors/fake geometry
    RenderPass* renderBalls = new RenderPass(
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
    RenderPass* detectVisible = new RenderPass(
                new Quad(),
                new ShaderProgram("/Filters/fullscreen.vert","/RenderTechniques/DetectVisible/DetectVisibleInstanceIDs.frag"),
                getWidth(window),
                getHeight(window));

    // prepare 1D buffer for entries
    Texture* bufferTex = new Texture(GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_INT);
    bufferTex->genUimageBuffer(num_balls);
    detectVisible->texture("visibilityBuffer", bufferTex->getHandle());
    detectVisible->texture("tex", renderBalls->get("InstanceID"));

    /// renderpass to display result frame
    auto result = new RenderPass(
                new Quad(),
                new ShaderProgram("/Filters/fullscreen.vert","/Filters/toneMapperLinearInstanceCount.frag"));
    result->texture("tex", renderBalls->get("fragColor"));

    /// compute shader to process a list of visible IDs (with the actual instanceID of the first general draw)
    auto computeVisibleIDs = new ComputeProgram(new ShaderProgram("/RenderTechniques/DetectVisible/CreateVisibleIDList.comp"));

    // 1D buffer for visible IDs
    Texture* visibleIDsBuff = new Texture(GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT);
    visibleIDsBuff->genUimageBuffer(num_balls);

    computeVisibleIDs->texture("visibilityBuffer", bufferTex->getHandle());
    computeVisibleIDs->texture("visibleIDsBuff", visibleIDsBuff->getHandle());

    // atomic counter buffer for consecutive index access in compute shader
    GLuint atomBuff;
    glGenBuffers(1, &atomBuff);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomBuff);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 3, atomBuff);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    int positions_size = sizeof(glm::vec4) * impSph->instance_positions_s.instance_positions.size();
    int colors_size = sizeof(glm::vec4) * impSph->instance_colors_s.instance_colors.size();

    // SSBO setup
    GLuint SSBO[2];
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
    const GLuint zero = 0;
    int byteCount = sizeof(unsigned int)* num_balls;
    unsigned char * zeros = new unsigned char[byteCount];
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
    std::vector<GLuint> identityInstancesMap;
    identityInstancesMap.clear();
    for (GLuint i = 0; i < num_balls; i++)
        identityInstancesMap.push_back(i);

    glBindTexture(GL_TEXTURE_1D, visibleIDsBuff->getHandle());
    glTexImage1D(GL_TEXTURE_1D, 0, GL_R32UI, num_balls, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &identityInstancesMap[0]);

    // map to set all instances visible
    std::vector<GLint> mapAllVisible;
    mapAllVisible.resize(num_balls);
    std::fill(mapAllVisible.begin(), mapAllVisible.end(), 1);

    bool animate = false;
    float lastTime = 0;
    float elapsedTime = 0;

    float fps           = 0.0f;
    float frameInterval = 0.0f;
    int numberOfFrames  = 0;

    // time query
    GLuint timeQuery;
    GLuint queryTime;
    glGenQueries(1, &timeQuery);

    glEnable(GL_DEPTH_TEST);

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
        if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) distance += deltaTime * 5;
        if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) distance = max(distance - deltaTime * 5, 0.0f);
        if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS) scale += deltaTime*4;
        if (glfwGetKey(window, GLFW_KEY_COMMA) == GLFW_PRESS) scale = glm::max(scale - deltaTime*4, 0.01f);
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {updateVisibilityMapLock = true; updateVisibilityMap = true;}
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) updateVisibilityMapLock = false;
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) pingPongOff = false;
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) pingPongOff = true;
        if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS) { vsync = !vsync; vsync ? glfwSwapInterval(1) : glfwSwapInterval(0); vsync ? std::cout << "VSync enabled\n" : std::cout << "VSync disabled\n"; }

        // Render impostor geometry
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        {
            renderBalls->setShaderProgram(&spRenderImpostor);
            renderBalls->texture("visibleIDsBuff", visibleIDsBuff->getHandle());
            result->update("maxRange", 1.0f);
            result->texture("tex", renderBalls->get("fragColor"));
        }
        // Render impostor geometry as disc
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        {
            renderBalls->setShaderProgram(&spRenderDiscs);
            renderBalls->texture("visibleIDsBuff", visibleIDsBuff->getHandle());
            result->update("maxRange", 1.0f);
            result->texture("tex", renderBalls->get("fragColor"));
        }
        // Render faked geometry
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        {
            renderBalls->setShaderProgram(&spRenderBalls);
            renderBalls->texture("visibleIDsBuff", visibleIDsBuff->getHandle());
            result->update("maxRange", 1.0f);
            result->texture("tex", renderBalls->get("fragColor"));
        }
        // Render instance IDs geometry
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
        {
            renderBalls->setShaderProgram(&spRenderBalls);
            renderBalls->texture("visibleIDsBuff", visibleIDsBuff->getHandle());
            result->update("maxRange", float(ImpostorSpheres::num_balls));
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
        glBindTexture(GL_TEXTURE_1D, bufferTex->getHandle());
        glTexImage1D(GL_TEXTURE_1D, 0, GL_R8UI, num_balls, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, zeros);
        //glBindTexture(GL_TEXTURE_1D, visibleIDsBuff->getHandle());
        //glTexImage1D(GL_TEXTURE_1D, 0, GL_R32UI, num_balls, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &identityInstancesMap[0]);

        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomBuff);
        glClearBufferSubData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, 0, sizeof(GLuint), GL_RED_INTEGER, GL_UNSIGNED_INT, zero);

        renderBalls->clear(-1,-1,-1,-1); // the clear color may not interfere with the detected instance IDs
        renderBalls->clearDepth();
        renderBalls->update("scale", vec2(scale));
        renderBalls->update("view", view);
        //renderBalls->update("elapsedTime", elapsedTime);
        renderBalls->run();


        // Depending on user input: sort out instances for the next frame or not,
        // or lock the current set of visible instances
        if(useAtomicCounters)
            if (updateVisibilityMap && !pingPongOff)
            {
                // the following shaders in detectVisible look at what has been written to the screen (framebuffer0)
                // better render the instance stuff to a texture and read from there
                detectVisible->run();
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT|GL_BUFFER_UPDATE_BARRIER_BIT);
                glBeginQuery(GL_TIME_ELAPSED, timeQuery);
                computeVisibleIDs->run(16,1,1); // 16 work groups * 1024 work items = 16384 atoms and IDs
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
                    glBindTexture(GL_TEXTURE_1D, visibleIDsBuff->getHandle());
                    glTexImage1D(GL_TEXTURE_1D, 0, GL_R32UI, num_balls, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &identityInstancesMap[0]);
                    impSph->instancesToRender = impSph->num_balls;
                    updateVisibilityMap = true; // sort out every other frame
                }
            }
        else
            if (updateVisibilityMap && !pingPongOff)
            {
                detectVisible->run();
                // detect visible instances
                glBeginQuery(GL_TIME_ELAPSED, timeQuery);
                GLuint visibilityMapFromBuff[ImpostorSpheres::num_balls];
                glBindTexture(GL_TEXTURE_1D, bufferTex->getHandle());
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

        result->clear(num_balls,num_balls,num_balls,num_balls);
        result->run();
    });
}


