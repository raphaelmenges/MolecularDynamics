//
// Created by ubundrian on 09.06.16.
//

#include "NeighborSearch.h"

//___________________________________PUBLIC_________________________________//

//__________________________________________________________________________//
//                                CONSTRUCTOR                               //
//__________________________________________________________________________//
NeighborSearch::NeighborSearch()
{
    /*
     * init window
     */
    mp_Window = generateWindow();

    /*
     * init imgui
     */
    ImGui_ImplGlfwGL3_Init(mp_Window, true);
    ImGuiIO& io = ImGui::GetIO();

    /*
     * register callbacks
     */
    std::function<void(int, int, int, int)> kC = [&](int k, int s, int a, int m)
    {
        // Check whether ImGui is handling this
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureKeyboard)
        {
            return;
        }
        this->keyCallback(k, s, a, m);
    };
    setKeyCallback(mp_Window, kC);

    std::function<void(int, int, int)> kB = [&](int b, int a, int m)
    {
        // Check whether ImGui is handling this
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureMouse)
        {
            return;
        }
        this->mouseButtonCallback(b, a, m);
    };
    setMouseButtonCallback(mp_Window, kB);

    std::function<void(double, double)> kS = [&](double x, double y)
    {
        // Check whether ImGui is handling this
        ImGuiIO& io = ImGui::GetIO();
        if(io.WantCaptureMouse)
        {
            return;
        }
        this->scrollCallback(x,y);
    };
    setScrollCallback(mp_Window, kS);

    /*
     * init opengl
     */
    glClearColor(0.f, 0.f, 0.f, 1.f);

    /*
     * init protein loader
     */
    m_proteinLoader = ProteinLoader();
}

NeighborSearch::~NeighborSearch()
{

}





//__________________________________PRIVATE_________________________________//

//__________________________________________________________________________//
//                                  METHODS                                 //
//__________________________________________________________________________//


//____________________________________CORE__________________________________//
void NeighborSearch::loadProtein(std::string fileName)
{

    /*
     * concatenate full path
     */
    std::string subfolder = "/molecules/";
    std::string filePath = RESOURCES_PATH + subfolder + fileName;
    std::cout << "Load protein from path: " << filePath << std::endl;

    /*
     * load protein from pdb file
     */
    SimpleProtein protein;
    m_proteinLoader.loadPDB(filePath, protein, protein.bbMin, protein.bbMax);

    /*
     * generate, bind, fill and then unbind atom ssbo buffer
     */
    glGenBuffers(1, &protein.atomsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, protein.atomsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(SimpleAtom) * protein.atoms.size(), protein.atoms.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    m_proteins.push_back(protein);

}

void NeighborSearch::execute(){

    /*
     * setup opengl
     */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    /*
     * setup shader programs
     */
    ShaderProgram impostorProgram = ShaderProgram("/NeighborSearch/impostor.vert", "/NeighborSearch/impostor.geom", "/NeighborSearch/impostor.frag");

    /*
     * bind atoms in ssbo
     * ssbo's are used to work with compute shader
     */
    // todo: find out how to buffer multiple proteins
    for (int i = 0; i < m_proteins.size(); i++) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_proteins.at(i).atomsSSBO);
    }

    /*
     * setup camera
     */
    glm::vec3 cameraCenter = glm::vec3(0.0, 0.0, 0.0);
    float cameraRadius = 0.0f;
    for (int i = 0; i < m_proteins.size(); i++)
    {
        SimpleProtein protein = m_proteins.at(i);
        cameraCenter = cameraCenter + ((protein.bbMax + protein.bbMin) / 2);
        float radius = glm::length(protein.bbMax - protein.bbMin);
        cameraRadius = (radius > cameraRadius) ? radius : cameraRadius;
    }
    std::cout << "Camera radius is " << cameraRadius << std::endl;

    mp_camera = std::unique_ptr<OrbitCamera>(
            new OrbitCamera(
                    cameraCenter,
                    90.0f,
                    90.0f,
                    cameraRadius,
                    cameraRadius / 2.0f,
                    5.0f * cameraRadius,
                    45.0f,
                    0.05f
            )
    );

    /*
     * setup cursor position
     */
    float prevCursorX, prevCursorY = 0;

    /*
     * renderloop
     * call render function of Renderer.h and provide it with a function
     */
    render(mp_Window, [&] (float deltaTime)
    {

        /*
         * clear buffer
         */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplGlfwGL3_NewFrame();

        /*
         * update viewport
         */
        glm::vec2 resolution = getResolution(mp_Window);
        glViewport(0,0,resolution.x, resolution.y);

        /*
         * Calculate cursor movement
         */
        double cursorX, cursorY;
        glfwGetCursorPos(mp_Window, &cursorX, &cursorY);
        GLfloat cursorDeltaX = (float)cursorX - prevCursorX;
        GLfloat cursorDeltaY = (float)cursorY - prevCursorY;
        prevCursorX = cursorX;
        prevCursorY = cursorY;

        /*
         * update camera
         */
        if(m_rotateCamera) {
            m_CameraDeltaMovement = glm::vec2(cursorDeltaX, cursorDeltaY);
            m_CameraSmoothTime = 1.f;
        }
        glm::vec2 cameraMovement = glm::lerp(glm::vec2(0), m_CameraDeltaMovement, m_CameraSmoothTime);
        mp_camera->setAlpha(mp_camera->getAlpha() + 0.25f * cameraMovement.x);
        mp_camera->setBeta(mp_camera->getBeta() - 0.25f * cameraMovement.y);
        mp_camera->update(resolution.x, resolution.y, true);

        /*
         * light direction equals view direction of the camera
         */
        m_lightDirection = glm::normalize(-mp_camera->getPosition() + mp_camera->getCenter());

        /*
         * draw proteins as impostor
         */
        impostorProgram.use();
        impostorProgram.update("view", mp_camera->getViewMatrix());
        impostorProgram.update("projection", mp_camera->getProjectionMatrix());
        impostorProgram.update("cameraWorldPos", mp_camera->getPosition());
        impostorProgram.update("probeRadius", 0.f);
        impostorProgram.update("lightDir", m_lightDirection);
        impostorProgram.update("selectedIndex", m_selectedAtom);
        impostorProgram.update("color", glm::vec3(1.f,1.f,1.f));

        /*
         * Draw atoms
         */
        for (int i = 0; i < m_proteins.size(); i++) {
            glDrawArrays(GL_POINTS, 0, (GLsizei)m_proteins.at(i).atoms.size());
        }


        /*
         * update gui
         */
        updateGUI();
    });
}

void NeighborSearch::updateGUI()
{
    // Main menu bar
    if (ImGui::BeginMainMenuBar())
    {
        // General menu
        if (ImGui::BeginMenu("Menu"))
        {
            if(ImGui::MenuItem("Quit", "Esc", false, true)) { glfwSetWindowShouldClose(mp_Window, GL_TRUE); }
            ImGui::EndMenu();
        }

        // Rendering menu
        if (ImGui::BeginMenu("Rendering"))
        {
            // Impostors
            if(ImGui::MenuItem("Show Internal", "P", false, true)) { m_showInternal; }
            if(ImGui::MenuItem("Show Surface", "P", false, true)) { m_showSurface; }

            ImGui::EndMenu();
        }

        // Window menu
        if (ImGui::BeginMenu("Window"))
        {
            // Computation window
            static bool mShowComputationWindow = false;
            if(mShowComputationWindow)
            {
                if(ImGui::MenuItem("Hide Computation", "", false, true)) { mShowComputationWindow = false; }
            }
            else
            {
                if(ImGui::MenuItem("Show Computation", "", false, true)) { mShowComputationWindow = true; }
            }

            ImGui::EndMenu();
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

    ImGui::Render();
}

void NeighborSearch::findNeighbors()
{

}



//________________________________INTERACTION_______________________________//
void NeighborSearch::keyCallback(int key, int scancode, int action, int mods)
{

}

void NeighborSearch::mouseButtonCallback(int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        m_rotateCamera = true;
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        m_rotateCamera = false;
    }
}

void NeighborSearch::scrollCallback(double xoffset, double yoffset)
{
    mp_camera->setRadius(mp_camera->getRadius() - 0.5f * (float)yoffset);
    std::cout << "Camera radius is now " << mp_camera->getRadius() << std::endl;
}





//__________________________________________________________________________//
//                                   MAIN                                   //
//__________________________________________________________________________//
int main() {

    NeighborSearch neighborSearch;
    neighborSearch.loadProtein("PDB/1a19.pdb");
    neighborSearch.execute();

    return 0;
}