#include <vector>
#include "ShaderTools/Renderer.h"
#include "ShaderTools/RenderPass.h"
#include "AssetTools/Texture.h"
#include "AssetTools/Mesh.h"
#include "ShaderTools/VertexArrayObjects/Quad.h"
#include "ShaderTools/VertexArrayObjects/ImpostorSpheres.h"

using namespace glm;

float r(float size) {
    return size * 2 * static_cast <float> (rand()) / static_cast <float> (RAND_MAX) - size;
}

float r2(float size) {
    return size * static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
}

int main(int argc, char *argv[]) {
    GLFWwindow* window = generateWindow();

    float rotX = 0.0f;
    float rotY = 0.0f;
    float distance = 80.0;
    float scale = 1.0;
    bool updateVisibilityMap = false;
    bool updateVisibilityMapLock = false; // L: lock U: unlock reduced elements for current frame
    bool pingPongOff = true; // reduce number of elements every other frame O: off P: Pingpong On

    ImpostorSpheres *impSph = new ImpostorSpheres();
    int num_balls = ImpostorSpheres::num_balls;
    mat4 projection = perspective(45.0f, getRatio(window), 0.1f, 100.0f);

    ShaderProgram spRenderImpostor = ShaderProgram("/Impostor/impostorSpheres_Instanced.vert", "/Filters/solidColorInstanceCount.frag");
    ShaderProgram spRenderDiscs = ShaderProgram("/Impostor/impostorSpheres_Instanced.vert", "/Impostor/impostorSpheres_discardFragments_Instanced.frag");
    ShaderProgram spRenderBalls = ShaderProgram("/Impostor/impostorSpheres_Instanced.vert", "/Impostor/impostorSpheres_Instanced.frag");
    

    // Renderpass to render impostors/fake geometry
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

    // Renderpass to detect the visible instances
    RenderPass* detectVisible = new RenderPass(
                new Quad(),
                new ShaderProgram("/Filters/fullscreen.vert","/RenderTechniques/DetectVisible/DetectVisibleInstanceIDs.frag"),
                getWidth(window),
                getHeight(window));

    // prepare 1D buffer for entries
    Texture* bufferTex = new Texture;
    bufferTex->genUimageBuffer(num_balls);
    detectVisible->texture("visibilityBuffer", bufferTex->getHandle());
    detectVisible->texture("tex", renderBalls->get("InstanceID"));

    // renderpass to display result frame
    auto result = new RenderPass(
                new Quad(),
                new ShaderProgram("/Filters/fullscreen.vert","/Filters/toneMapperLinearInstanceCount.frag"));
    result->texture("tex", renderBalls->get("fragColor"));


    // prepare data to reset the buffer that holds the visible instance IDs
    int byteCount = sizeof(float) * 4 * num_balls;
    unsigned char * data = new unsigned char[byteCount];
    float f = 0.0f;
    unsigned char const * p = reinterpret_cast<unsigned char const *>(&f);

    for (int i = 0; i < byteCount; i+=4)
    {
        data[i] = p[0];
        data[i+1] = p[1];
        data[i+2] = p[2];
        data[i+3] = p[3];
    }

    // map to set all instances visible
    std::vector<GLint> mapAllVisible;
    mapAllVisible.resize(num_balls);
    std::fill(mapAllVisible.begin(), mapAllVisible.end(), 1);

    bool animate = false;
    float lastTime = 0;
    float elapsedTime = 0;

    glEnable(GL_DEPTH_TEST);
    render(window, [&] (float deltaTime) {
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) (rotY - deltaTime < 0)? rotY -= deltaTime + 6.283 : rotY -= deltaTime;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) (rotY + deltaTime > 6.283)? rotY += deltaTime - 6.283 : rotY += deltaTime;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) (rotX - deltaTime < 0)? rotX -= deltaTime + 6.283 : rotX -= deltaTime;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) (rotX + deltaTime > 6.283)? rotX += deltaTime - 6.283 : rotX += deltaTime;
        if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) distance += deltaTime * 5;
        if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) distance = max(distance - deltaTime * 5, 0.0f);
        if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS) scale += deltaTime*3;
        if (glfwGetKey(window, GLFW_KEY_COMMA) == GLFW_PRESS) scale = glm::max(scale - deltaTime*3, 0.01f);
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) updateVisibilityMapLock = true;
        if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) updateVisibilityMapLock = false;
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) pingPongOff = false;
        if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) pingPongOff = true;

        // Render impostor geometry
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        {
            renderBalls->setShaderProgram(&spRenderImpostor);
            result->update("maxRange", 1.0f);
            result->texture("tex", renderBalls->get("fragColor"));
        }
        // Render impostor geometry as disc
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        {
            renderBalls->setShaderProgram(&spRenderDiscs);
            result->update("maxRange", 1.0f);
            result->texture("tex", renderBalls->get("fragColor"));
        }
        // Render faked geometry
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        {
            renderBalls->setShaderProgram(&spRenderBalls);
            result->update("maxRange", 1.0f);
            result->texture("tex", renderBalls->get("fragColor"));
        }
        // Render instance IDs geometry
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
        {
            renderBalls->setShaderProgram(&spRenderBalls);
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

        renderBalls->clear(-1,-1,-1,-1); // the clear color may not interfere with the detected instance IDs
        renderBalls->clearDepth();
        renderBalls->update("scale", vec2(scale));
        renderBalls->update("view", view);
        renderBalls->update("elapsedTime", elapsedTime);
        renderBalls->run();

        result->clear(num_balls,num_balls,num_balls,num_balls);
        result->run();

        // detect visible instances
        GLfloat *iPixel= new GLfloat[ImpostorSpheres::num_balls * 4];
        // why use RGBA float?
        // ToDo: use GL_R and GL_INT
        //      glTexImage1D(GL_TEXTURE_1D, 0, GL_R32UI, num_entries, 0, GL_R, GL_UNSIGNED_INT, data);

        // reset the detected instance IDs
        glBindTexture(GL_TEXTURE_1D, bufferTex->getHandle());
        glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA16F, num_balls, 0, GL_RGBA, GL_FLOAT, data);
        glBindTexture(GL_TEXTURE_1D, 0);

        // the following shaders in detectVisible look at what has been written to the screen (framebuffer0)
        // better render the instance stuff to a texture and read from there
        detectVisible->run();

        // get the visible instance IDs
        glBindTexture(GL_TEXTURE_1D, bufferTex->getHandle());
        glGetTexImage(GL_TEXTURE_1D, 0, GL_RGBA, GL_FLOAT, iPixel);
        glBindTexture(GL_TEXTURE_1D, 0);


        // Depending on user input: sort out instances for the next frame or not,
        // or lock the current set of visible instances
        if (updateVisibilityMap && !pingPongOff)
        {
            // copy visible instance information to a vector with size = num_balls
            // since we are currently working on RGBA ( size = num_balls*4)
            int num_vis = 0;
            std::vector<GLint> newMap;
            newMap.resize(num_balls);
            for (int i = 0; i < num_balls; i++)
            {
                newMap[i] = (int)iPixel[i*4];
                if(iPixel[i*4] != 0)
                    num_vis++;
            }
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

        delete iPixel;
    });
}


