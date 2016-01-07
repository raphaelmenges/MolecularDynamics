#include "ShaderTools/Renderer.h"
#include "ShaderTools/RenderPass.h"
#include "AssetTools/Mesh.h"
#include "ShaderTools/VertexArrayObjects/Cube.h"
#include "ShaderTools/VertexArrayObjects/Tetrahedron.h"

using namespace std;
using namespace glm;

int main(int argc, char *argv[]) {
    GLFWwindow* window = generateWindow();

    float rotX = 0.0f;
    float rotY = 0.0f;
    float distance = 40.0;

    vec4 color_torus = vec4(1,0,0,0);
    vec4 color_sphere = vec4(0,1,0,0);
    vec4 color_tetra = vec4(0,0,1,0);

    mat4 projection = perspective(45.0f, getRatio(window), 1.0f, 100.0f);

    RenderPass* torus = (new RenderPass(
                             new Cube(),
                             new ShaderProgram("/3DObject/modelViewProjection.vert","/Impostor/Impostor3DTorus.frag")))
            ->update("projection", projection)
            ->update("color", color_torus);

    RenderPass* torus_2 = (new RenderPass(
                               new Cube(),
                               new ShaderProgram("/3DObject/modelViewProjection.vert","/Impostor/Impostor3DTorus.frag"),
                               torus->getFrameBufferObject()))
            ->update("projection", projection)
            ->update("color", color_torus);

    RenderPass* torus_3 = (new RenderPass(
                               new Cube(),
                               new ShaderProgram("/3DObject/modelViewProjection.vert","/Impostor/Impostor3DTorus.frag"),
                               torus->getFrameBufferObject()))
            ->update("projection", projection)
            ->update("color", color_torus);

    RenderPass* sphere_1 = (new RenderPass(
                                new Cube(),
                                new ShaderProgram("/3DObject/modelViewProjection.vert","/Impostor/Impostor3DSphere.frag"),
                                torus->getFrameBufferObject()))
            ->update("projection", projection)
            ->update("color", color_sphere);

    RenderPass* sphere_2 = (new RenderPass(
                                new Cube(),
                                new ShaderProgram("/3DObject/modelViewProjection.vert","/Impostor/Impostor3DSphere.frag"),
                                torus->getFrameBufferObject()))
            ->update("projection", projection)
            ->update("color", color_sphere);

    RenderPass* sphere_3 = (new RenderPass(
                                new Cube(),
                                new ShaderProgram("/3DObject/modelViewProjection.vert","/Impostor/Impostor3DSphere.frag"),
                                torus->getFrameBufferObject()))
            ->update("projection", projection)
            ->update("color", color_sphere);

    RenderPass* tetra = (new RenderPass(
                             new Tetrahedron(),
                             new ShaderProgram("/3DObject/modelViewProjection.vert","/Impostor/Impostor3DSphereTetrahedron.frag"),
                             torus->getFrameBufferObject()))
            ->update("projection", projection)
            ->update("color", color_tetra);

    render(window, [&] (float deltaTime) {
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) (rotY - deltaTime < 0)? rotY -= deltaTime + 6.283 : rotY -= deltaTime;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) (rotY + deltaTime > 6.283)? rotY += deltaTime - 6.283 : rotY += deltaTime;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) (rotX - deltaTime < 0)? rotX -= deltaTime + 6.283 : rotX -= deltaTime;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) (rotX + deltaTime > 6.283)? rotX += deltaTime - 6.283 : rotX += deltaTime;
        if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) distance += deltaTime * 10;
        if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) distance = glm::max(distance - deltaTime * 10.0f, 0.0f);

        mat4 view = translate(mat4(1), vec3(0,0,-distance)) * eulerAngleXY(-rotX, -rotY);
        mat4 model_torus = scale(mat4(1), vec3(1,1,1));
        mat4 model_sphere_1 = translate(mat4(1), vec3(1.609,0,0));
        mat4 model_sphere_2 = translate(mat4(1), vec3(-1.609,0,0));

        mat4 model_sphere_3 = translate(mat4(1), vec3(0,0,2.787));
//        model_sphere_3 = rotate(model_sphere_3, 3.14f/3.0f, vec3(0,1,0));
//        model_sphere_3 = translate(model_sphere_3, vec3(-1.609 * 2,0,0));

        mat4 model_torus_2 = translate(mat4(1), vec3(1.609,0,0));
        model_torus_2 = rotate(model_torus_2, 3.14f/3.0f, vec3(0,1,0));
        model_torus_2 = translate(model_torus_2, vec3(-1.609,0,0));

        mat4 model_torus_3 = translate(mat4(1), vec3(-1.609,0,0));
        model_torus_3 = rotate(model_torus_3, -3.14f/3.0f, vec3(0,1,0));
        model_torus_3 = translate(model_torus_3, vec3(1.609,0,0));

        mat4 model_tetra = translate(mat4(1), vec3(0,0,1.393));





        float elapsedTime = glfwGetTime();
        if (elapsedTime > 628)
        {
            elapsedTime = 0;
            glfwSetTime(0);
        }
        // Translation, die auch wieder rausgerechnet werden muss um den Sehstrahl in Weltkoordinaten zu bekommen
        //model = translate (model, vec3(10*sin(elapsedTime),1,1));

        glEnable(GL_DEPTH_TEST);
        //glDisable(GL_DEPTH_TEST);

        torus
                ->clearDepth()
                ->clear()
                ->update("model", model_torus)
                ->update("view", view)
                ->run();

        torus_2
                ->update("model", model_torus_2)
                ->update("view", view)
                ->run();

        torus_3
                ->update("model", model_torus_3)
                ->update("view", view)
                ->run();


        tetra
                //                ->clearDepth()
                //                ->clear()
                ->update("model", model_tetra)
                ->update("view", view)
                ->run();

        sphere_1
                ->update("model", model_sphere_1)
                ->update("view", view)
                ->run();
        sphere_2
                ->update("model", model_sphere_2)
                ->update("view", view)
                ->run();
        sphere_3
                ->update("model", model_sphere_3)
                ->update("view", view)
                ->run();
    });
}


