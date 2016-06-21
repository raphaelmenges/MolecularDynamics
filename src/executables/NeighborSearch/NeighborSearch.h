//
// Created by ubundrian on 09.06.16.
//

#ifndef OPENGL_FRAMEWORK_NEIGHBORSEARCH_H
#define OPENGL_FRAMEWORK_NEIGHBORSEARCH_H

// external includes
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/examples/opengl3_example/imgui_impl_glfw_gl3.h>
#include <sstream>
#include <iomanip>

// framework includes
#include "ShaderTools/Renderer.h"
#include "ShaderTools/ShaderProgram.h"
#include "Utils/Logger.h"

// local includes
#include "ProteinLoader.h"
#include "Utils/OrbitCamera.h"

class NeighborSearch {

public:
    //__________________PUBLIC__________________//
    //_____________________________________//
    //           CONSTRUCTOR               //
    //_____________________________________//
    NeighborSearch();
    ~NeighborSearch();



    //_____________________________________//
    //             METHODS                 //
    //_____________________________________//
    void loadProtein(std::string fileName);
    void execute();


private:
    //_________________PRIVATE__________________//
    //_____________________________________//
    //            VARIABLES                //
    //_____________________________________//
    // window
    GLFWwindow* mp_Window;

    // interaction
    std::unique_ptr<OrbitCamera> mp_camera;
    glm::vec2 m_CameraDeltaMovement;
    float m_CameraSmoothTime;
    bool m_rotateCamera = false;

    // rendering
    glm::vec3 m_lightDirection;

    // gui
    bool m_showInternal;
    bool m_showSurface;

    // protein
    ProteinLoader m_proteinLoader;
    std::vector<SimpleProtein> m_proteins;
    int m_selectedAtom = 0;


    //_____________________________________//
    //             METHODS                 //
    //_____________________________________//
    // core
    void updateGUI();
    void findNeighbors();

    // interaction
    void keyCallback(int key, int scancode, int action, int mods);
    void mouseButtonCallback(int button, int action, int mods);
    void scrollCallback(double xoffset, double yoffset);

};


#endif //OPENGL_FRAMEWORK_NEIGHBORSEARCH_H
