#include "ShaderTools/Renderer.h"
#include "ShaderTools/RenderPass.h"
#include "AssetTools/Mesh.h"
#include "ShaderTools/VertexArrayObjects/Cube.h"
#include "ShaderTools/VertexArrayObjects/Tetrahedron.h"

using namespace std;
using namespace glm;

float r(float size) {
    return size * 2 * static_cast <float> (rand()) / static_cast <float> (RAND_MAX) - size;
}

int main(int argc, char *argv[]) {
    GLFWwindow* window = generateWindow(1024,768);

    float rotX = 0.0f;
    float rotY = 0.0f;
    float distance = 40.0;
    float probeRadius = 1.3;
    float sphereRadius = 0.999;
    float toroidalRadius = 1.6;

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


    int iterations = 1;

    std::vector<mat4> translations;
    mat4 t;

    translations.push_back(translate(mat4(1), vec3(0,0,0)));

//    int rr = 20;
//    for (int i = 0; i < iterations-1; i++)
//    {
//        translations.push_back(translate(mat4(1), vec3(r(rr),r(rr),r(rr))));
//    }

    render(window, [&] (float deltaTime) {
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) (rotY - deltaTime < 0)? rotY -= deltaTime + 6.283 : rotY -= deltaTime;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) (rotY + deltaTime > 6.283)? rotY += deltaTime - 6.283 : rotY += deltaTime;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) (rotX - deltaTime < 0)? rotX -= deltaTime + 6.283 : rotX -= deltaTime;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) (rotX + deltaTime > 6.283)? rotX += deltaTime - 6.283 : rotX += deltaTime;
        if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) distance += deltaTime * 10;
        if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) distance = glm::max(distance - deltaTime * 10.0f, 0.0f);

        if ((glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS) && glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) sphereRadius = glm::max(sphereRadius + deltaTime * 1.0f, 0.0f);
        if ((glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS) && glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) sphereRadius = glm::max(sphereRadius - deltaTime * 1.0f, 0.0f);
        if ((glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS) && glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) toroidalRadius = glm::max(toroidalRadius + deltaTime * 1.0f, 0.0f);
        if ((glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS) && glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) toroidalRadius = glm::max(toroidalRadius - deltaTime * 1.0f, 0.0f);
        if ((glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS) && glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) probeRadius = glm::max(probeRadius + deltaTime * 1.0f, 0.0f);
        if ((glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS) && glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) probeRadius = glm::max(probeRadius - deltaTime * 1.0f, 0.0f);


        /**
         * Alle hard-gecodeten Werte basieren auf der Annahme: Atome r=1, Probe r=1.3 und dass der Torusauschnitt der Länge 1 entspricht.
         * Auch in den Shadern!
         *
         * ... warum auch nicht?
         */

//        mat4 view = translate(mat4(1), vec3(0,0,-distance)) * eulerAngleXY(-rotX, -rotY);
//        mat4 model_torus = scale(mat4(1), vec3(1,1,1));
//        mat4 model_sphere_1 = translate(mat4(1), vec3(1.609,0,0));
//        mat4 model_sphere_2 = translate(mat4(1), vec3(-1.609,0,0));

//        mat4 model_sphere_3 = translate(mat4(1), vec3(0,0,2.787));

//        mat4 model_torus_2 = translate(mat4(1), vec3(1.609,0,0));
//        model_torus_2 = rotate(model_torus_2, 3.14f/3.0f, vec3(0,1,0));
//        model_torus_2 = translate(model_torus_2, vec3(-1.609,0,0));

//        mat4 model_torus_3 = translate(mat4(1), vec3(-1.609,0,0));
//        model_torus_3 = rotate(model_torus_3, -3.14f/3.0f, vec3(0,1,0));
//        model_torus_3 = translate(model_torus_3, vec3(1.609,0,0));

//        mat4 model_tetra = translate(mat4(1), vec3(0,0,1.393));

        glEnable(GL_DEPTH_TEST);

        mat4 view = translate(mat4(1), vec3(0,0,-distance)) * eulerAngleXY(-rotX, -rotY);

        tetra
                ->clearDepth()
                ->clear();
        for ( int i = 0; i< iterations; i++)
        {

            t = translations.at(i);
           mat4 model_sphere_1 = t;
           mat4 model_sphere_2 = t;
           mat4 model_sphere_3 = t;
           mat4 model_torus = t;
           mat4 model_torus_2 = t;
           mat4 model_torus_3 = t;
           mat4 model_tetra = t;

             model_torus = scale(model_torus, vec3(1,1,1));
             model_sphere_1 = translate(model_sphere_1, vec3(1.609,0,0));
             model_sphere_2 = translate(model_sphere_2, vec3(-1.609,0,0));

             model_sphere_3 = translate(model_sphere_3, vec3(0,0,2.787));

             model_torus_2 = translate(model_torus_2, vec3(1.609,0,0));
            model_torus_2 = rotate(model_torus_2, 3.14f/3.0f, vec3(0,1,0));
            model_torus_2 = translate(model_torus_2, vec3(-1.609,0,0));

             model_torus_3 = translate(model_torus_3, vec3(-1.609,0,0));
            model_torus_3 = rotate(model_torus_3, -3.14f/3.0f, vec3(0,1,0));
            model_torus_3 = translate(model_torus_3, vec3(1.609,0,0));

             model_tetra = translate(model_tetra, vec3(0,0,1.393));

        tetra
                ->update("model", model_tetra)
                ->update("view", view)
                ->update("probeRadius", probeRadius)
                ->run();


        sphere_1
                ->update("model", model_sphere_1)
                ->update("view", view)
                ->update("sphereRadius", sphereRadius)
                ->run();
        sphere_2
                ->update("model", model_sphere_2)
                ->update("view", view)
                ->update("sphereRadius", sphereRadius)
                ->run();
        sphere_3
                ->update("model", model_sphere_3)
                ->update("view", view)
                ->update("sphereRadius", sphereRadius)
                ->run();

        torus
                ->update("model", model_torus)
                ->update("view", view)
                ->update("probeRadius", probeRadius)
                ->update("toroidalRadius", toroidalRadius)
                ->run();

        torus_2
                ->update("model", model_torus_2)
                ->update("view", view)
                ->update("probeRadius", probeRadius)
                ->update("toroidalRadius", toroidalRadius)
                ->run();

        torus_3
                ->update("model", model_torus_3)
                ->update("view", view)
                ->update("probeRadius", probeRadius)
                ->update("toroidalRadius", toroidalRadius)
                ->run();





        }
    });
}


