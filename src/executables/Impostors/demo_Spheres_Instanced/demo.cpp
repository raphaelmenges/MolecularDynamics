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
    float distance = 40.0;
    float scale = 1.0;

    mat4 projection = perspective(45.0f, getRatio(window), 0.1f, 100.0f);




    ShaderProgram spRenderImpostor = ShaderProgram("/Impostor/impostorSpheres_Instanced.vert", "/Filters/solidColor.frag");
    ShaderProgram spRenderDiscs = ShaderProgram("/Impostor/impostorSpheres_Instanced.vert", "/Impostor/impostorSpheres_discardFragments_Instanced.frag");
    ShaderProgram spRenderBalls = ShaderProgram("/Impostor/impostorSpheres_Instanced.vert", "/Impostor/impostorSpheres_Instanced.frag");
    
    RenderPass* renderBalls = new RenderPass(
                new ImpostorSpheres(true, true),
                &spRenderBalls,
                getWidth(window),
                getHeight(window));

    // get the texture the instanceID is rendered to and modify its properties
    GLuint IDTextureHandle = renderBalls->get("InstanceID");
    glBindTexture(GL_TEXTURE_2D, IDTextureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, getWidth(window), getHeight(window), 0, GL_R, GL_UNSIGNED_INT, 0);
    GLuint currentAttachment = GL_COLOR_ATTACHMENT0 + renderBalls->getShaderProgram()->outputMap.at("InstanceID").location;
    glFramebufferTexture2D(GL_FRAMEBUFFER, currentAttachment, GL_TEXTURE_2D, IDTextureHandle, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    renderBalls->update("projection", projection);

    // define projection matrix for other shader programs
    renderBalls->setShaderProgram(&spRenderDiscs);
    renderBalls->update("projection", projection);
    renderBalls->setShaderProgram(&spRenderImpostor);
    renderBalls->update("projection", projection);

    auto result = (new RenderPass(
                       new Quad(),
                       new ShaderProgram("/Filters/fullscreen.vert","/Filters/toneMapperLinear.frag")));
    result->texture("tex", renderBalls->get("fragColor"));

    bool animate = false;
    float lastTime = 0;
    float elapsedTime = 0;


    // SSBO stuff

//    const int n =ImpostorSpheres::num_balls;

//    struct drawCounterData
//    {
//        std::vector<int> drawCount;
//    }drawCounterData_SSBO;

//    drawCounterData_SSBO.drawCount.resize(n);

//    GLuint m_drawCounterSSBO = 0;
//    glGenBuffers(1, &m_drawCounterSSBO);
//    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_drawCounterSSBO);
//    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(drawCounterData_SSBO), &drawCounterData_SSBO, GL_DYNAMIC_COPY);
//    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

//    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_drawCounterSSBO);
//    GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
//    memcpy(p, &drawCounterData_SSBO, sizeof(drawCounterData_SSBO))
//            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);


    glEnable(GL_DEPTH_TEST);
    render(window, [&] (float deltaTime) {
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) (rotY - deltaTime < 0)? rotY -= deltaTime + 6.283 : rotY -= deltaTime;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) (rotY + deltaTime > 6.283)? rotY += deltaTime - 6.283 : rotY += deltaTime;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) (rotX - deltaTime < 0)? rotX -= deltaTime + 6.283 : rotX -= deltaTime;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) (rotX + deltaTime > 6.283)? rotX += deltaTime - 6.283 : rotX += deltaTime;
        if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) distance += deltaTime * 3;
        if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) distance = max(distance - deltaTime * 3, 0.0f);
        if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS) scale += deltaTime;
        if (glfwGetKey(window, GLFW_KEY_COMMA) == GLFW_PRESS) scale = glm::max(scale - deltaTime, 0.01f);

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
            result->update("maxRange", 1000.0f);
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
        vec3 xyzOffset = vec3(sin(elapsedTime),cos(elapsedTime/2),sin(elapsedTime/3));

        renderBalls->clear(0,0,0,999);
        renderBalls->clearDepth();
        renderBalls->update("scale", vec2(scale));
        renderBalls->update("view", view);
        //renderBalls->update("xyzOffset", xyzOffset);
        renderBalls->update("elapsedTime", elapsedTime);

        renderBalls->run();
        result->clear();
        result->run();
    });
}


