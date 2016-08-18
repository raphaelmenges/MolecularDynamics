//
// Created by ubundrian on 12.08.16.
//
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
#include "Search/NeighborhoodSearch.h"
#include "Search/GPUHandler.h"



/*
 * DEFINES
 */
#define WIDTH 1280
#define HEIGHT 720



/*
 * VARIABLES
 */
// window
GLFWwindow* mp_Window;

// interaction
std::unique_ptr<OrbitCamera> mp_camera;
glm::vec2 m_CameraDeltaMovement;
float m_CameraSmoothTime;
bool m_rotateCamera = false;

// rendering
glm::vec3 m_lightDirection;
bool m_drawDebug = false;
static bool m_drawGrid  = false;

// protein
ProteinLoader m_proteinLoader;
int m_selectedAtom = 0;
int m_selectedProtein = 0;
float m_proteinMoveSpeed = 5.f;

// gpu
GPUHandler m_gpuHandler;
GLuint m_atomsSSBO;
GLuint m_pointsVBO;
int    m_numVBOEntries;
GLuint m_pointsVAO;

// neighborhood search
NeighborhoodSearch m_search;



/*
 * forward declaration
 */
void setup();
void keyCallback(int key, int scancode, int action, int mods);
void mouseButtonCallback(int button, int action, int mods);
void scrollCallback(double xoffset, double yoffset);
void moveProteinInsideGrid(glm::vec3 offset);
void printGPUInfos();
void updateAtomsSSBO();
void initNeighborhoodSearch(glm::vec3 gridResolution, float searchRadius);

void run();
void initBuffers();
void updateBuffers();
void drawGrid(ShaderProgram linesProgram);
void updateGUI();



/*
 * INITIALIZATION
 */
void setup()
{
    /*
     * init window
     */
    mp_Window = generateWindow("Test", WIDTH, HEIGHT);

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
        keyCallback(k, s, a, m);
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
        mouseButtonCallback(b, a, m);
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
        scrollCallback(x,y);
    };
    setScrollCallback(mp_Window, kS);

    /*
     * init opengl
     */
    glClearColor(0.2f, 0.2f, 0.2f, 1.f);

    /*
     * init protein loader
     */
    m_proteinLoader = ProteinLoader();
}



/*
 * INTERACTION
 */
void keyCallback(int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_P) {
        if (action == GLFW_PRESS) {
            m_selectedProtein = (m_selectedProtein+1) % m_proteinLoader.getNumberOfProteins();
        }
    }
    else if (key == GLFW_KEY_W) {
        moveProteinInsideGrid(glm::vec3(0,m_proteinMoveSpeed, 0));
    }
    else if (key == GLFW_KEY_A) {
        moveProteinInsideGrid(glm::vec3(-m_proteinMoveSpeed, 0, 0));
    }
    else if (key == GLFW_KEY_S) {
        moveProteinInsideGrid(glm::vec3(0,-m_proteinMoveSpeed, 0));
    }
    else if (key == GLFW_KEY_D) {
        moveProteinInsideGrid(glm::vec3(m_proteinMoveSpeed, 0, 0));
    }
    else if (key == GLFW_KEY_Q) {
        moveProteinInsideGrid(glm::vec3(0, 0, -m_proteinMoveSpeed));
    }
    else if (key == GLFW_KEY_E) {
        moveProteinInsideGrid(glm::vec3(0, 0, m_proteinMoveSpeed));
    }
}

void mouseButtonCallback(int button, int action, int mods)
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

void scrollCallback(double xoffset, double yoffset)
{
    mp_camera->setRadius(mp_camera->getRadius() - 2.f * (float)yoffset);
}

void moveProteinInsideGrid(glm::vec3 offset)
{
    SimpleProtein* protein = m_proteinLoader.getProteinAt(m_selectedProtein);

    glm::vec3 min = protein->bbMin + offset;
    glm::vec3 max = protein->bbMax + offset;
    glm::vec3 gridMin, gridMax;
    m_search.getGridMinMax(gridMin, gridMax);

    Logger::instance().print("Grid move:"); Logger::instance().tabIn();
    Logger::instance().print("Min: " + std::to_string(min.x) + ", " + std::to_string(min.y) + ", " + std::to_string(min.z));
    Logger::instance().print("Max: " + std::to_string(max.x) + ", " + std::to_string(max.y) + ", " + std::to_string(max.z));
    Logger::instance().print("Gridmin: " + std::to_string(gridMin.x) + ", " + std::to_string(gridMin.y) + ", " + std::to_string(gridMin.z));
    Logger::instance().print("Gridmax: " + std::to_string(gridMax.x) + ", " + std::to_string(gridMax.y) + ", " + std::to_string(gridMax.z));
    Logger::instance().print("Offset before: " + std::to_string(offset.x) + ", " + std::to_string(offset.y) + ", " + std::to_string(offset.z));

    // checking lower bounds
    if (min.x < gridMin.x) offset.x += gridMin.x - min.x;
    if (min.y < gridMin.y) offset.y += gridMin.y - min.y;
    if (min.z < gridMin.z) offset.z += gridMin.z - min.z;

    // checking upper bounds
    if (max.x > gridMax.x) offset.x -= max.x - gridMax.x;
    if (max.y > gridMax.y) offset.y -= max.y - gridMax.y;
    if (max.z > gridMax.z) offset.z -= max.z - gridMax.z;

    Logger::instance().print("Offset after: " + std::to_string(offset.x) + ", " + std::to_string(offset.y) + ", " + std::to_string(offset.z));
    Logger::instance().tabOut();

    protein->move(offset);
}



/*
 * NEIGHBORHOOD SEARCH
 */
void printGPUInfos()
{
    Logger::instance().print("GPU limits:"); Logger::instance().tabIn();

    GLint maxStorageBufferBindings = -1;
    GLint maxStorageBlockSize = -1;
    GLint maxVertShaderStorageBlocks = -1;
    GLint maxFragShaderStorageBlocks = -1;
    GLint maxComputeShaderStorageBlocks = -1;
    GLint maxCombinedShaderStorageBlocks = -1;
    int   work_grp_cnt[3];
    int   work_grp_size[3];

    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &maxStorageBufferBindings);
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE,      &maxStorageBlockSize);
    glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,   &maxVertShaderStorageBlocks);
    glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &maxFragShaderStorageBlocks);
    glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,  &maxComputeShaderStorageBlocks);
    glGetIntegerv(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, &maxCombinedShaderStorageBlocks);
    glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
    glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
    glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
    glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
    glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
    glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);

    Logger::instance().print("Max shader storage buffer bindings: " + std::to_string(maxStorageBufferBindings));
    Logger::instance().print("Max shader storage block size:      " + std::to_string(maxStorageBlockSize));
    Logger::instance().print("Max vertex shader storage blocks:   " + std::to_string(maxVertShaderStorageBlocks));
    Logger::instance().print("Max fragment shader storage blocks: " + std::to_string(maxFragShaderStorageBlocks));
    Logger::instance().print("Max compute shader storage blocks:  " + std::to_string(maxComputeShaderStorageBlocks));
    Logger::instance().print("Max combined shader storage blocks: " + std::to_string(maxCombinedShaderStorageBlocks));
    Logger::instance().print("Max global work group size x: " + std::to_string(work_grp_cnt[0])
                                                     + " y: " + std::to_string(work_grp_cnt[1])
                                                     + " z: " + std::to_string(work_grp_cnt[2]));
    Logger::instance().print( "Max local work group size x: " + std::to_string(work_grp_size[0])
                                                     + " y: " + std::to_string(work_grp_size[1])
                                                     + " z: " + std::to_string(work_grp_size[2]));

    Logger::instance().tabOut();
}

void updateAtomsSSBO()
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_atomsSSBO);
    GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(p, m_proteinLoader.getAllAtoms().data(), sizeof(SimpleAtom)*m_proteinLoader.getNumberOfAllAtoms());
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void initNeighborhoodSearch(glm::vec3 gridResolution, float searchRadius)
{
    // at least one protein should have been loaded at that time
    if(m_proteinLoader.getNumberOfProteins() <= 0) {
        Logger::instance().print("Error while initializing neighborhood search: No proteins loaded", Logger::Mode::ERROR);
        exit(-1);
    }

    /*
     * get min and max of all proteins
     * and get number of all atoms in all proteins
     */
    glm::fvec3 min, max;
    m_proteinLoader.getCenteredBoundingBoxAroundProteins(min, max);

    m_search.init(m_proteinLoader.getNumberOfAllAtoms(), min, max, gridResolution, searchRadius);
}



/*
 * SETUP AND MAINLOOP
 */
void run()
{

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
    ShaderProgram debugProgram    = ShaderProgram("/NeighborSearch/dummy.vert", "/NeighborSearch/fullscreenQuad.geom", "/NeighborSearch/debug.frag");
    ShaderProgram linesProgram    = ShaderProgram("/NeighborSearch/lines.vert", "/NeighborSearch/lines.frag");
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        Logger::instance().print("GLerror after init shader programs: " + std::to_string(err), Logger::Mode::ERROR);
    }

    /*
     * generate, bind, fill and then unbind atom ssbo buffer
     */
    m_atomsSSBO;
    glGenBuffers(1, &m_atomsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_atomsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(SimpleAtom) * m_proteinLoader.getNumberOfAllAtoms(), m_proteinLoader.getAllAtoms().data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_atomsSSBO);

    initBuffers();


    /*
     * setup camera
     */
    glm::vec3 cameraCenter = glm::vec3(0.0, 0.0, 0.0);
    float cameraRadius = 0.0f;
    glm::vec3 globalMin;
    glm::vec3 globalMax;
    m_proteinLoader.getBoundingBoxAroundProteins(globalMin, globalMax);
    cameraCenter = (globalMax + globalMin) / 2;
    float radius = glm::length(globalMax - globalMin);
    cameraRadius = (radius > cameraRadius) ? radius : cameraRadius;

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


        /*
         * request new frame for ImGui
         */
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
         * set light direction
         */
        //m_lightDirection = glm::normalize(-mp_camera->getPosition() + mp_camera->getCenter()); // light direction equals view direction of the camera
        m_lightDirection = glm::normalize(glm::vec3(0, -1, 0));

        /*
         * update atom positions
         */
        updateAtomsSSBO();

        /*
         * run neighborhood search
         */
        m_search.run();

        /*
         * do the actual drawing
         */
        if (!m_drawDebug) {
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
            impostorProgram.update("proteinNum", (int)m_proteinLoader.getNumberOfProteins());
            impostorProgram.update("selectedProtein", m_selectedProtein);
            glDrawArrays(GL_POINTS, 0, (GLsizei)m_proteinLoader.getNumberOfAllAtoms());
        } else {
            /*
             * draw debug view
             */
            Logger::instance().print("grid num: " + std::to_string(m_search.getTotalGridNum()));
            debugProgram.use();
            debugProgram.update("totalNumElements", (int)m_proteinLoader.getNumberOfAllAtoms());
            debugProgram.update("numCells", m_search.getTotalGridNum());
            debugProgram.update("width", WIDTH);
            debugProgram.update("height", HEIGHT);
            glDrawArrays(GL_POINTS, 0, 1);
        }

        /*
         * draw the grid
         */
        if (m_drawGrid) {
            drawGrid(linesProgram);
        }

        /*
         * update gui
         */
        updateGUI();
    });
}

void initBuffers()
{
    glm::vec3 min, max;
    m_search.getGridMinMax(min, max);
    float cellSize = m_search.getCellSize();
    glm::ivec3 gridRes = m_search.getGridResolution();

    Logger::instance().print("Grid min max:"); Logger::instance().tabIn();
    Logger::instance().print("Min:" + std::to_string(min.x) + ", " + std::to_string(min.y) + ", " + std::to_string(min.z));
    Logger::instance().print("Max:" + std::to_string(max.x) + ", " + std::to_string(max.y) + ", " + std::to_string(max.z));
    Logger::instance().tabOut();

    /*
     * grid hull
     */
    std::vector<glm::vec4> points;
    glm::vec4 p1 = glm::vec4(min.x, min.y, min.z, 1);
    glm::vec4 p2 = glm::vec4(max.x, min.y, min.z, 1);
    glm::vec4 p3 = glm::vec4(min.x, min.y, max.z, 1);
    glm::vec4 p4 = glm::vec4(max.x, min.y, max.z, 1);
    glm::vec4 p5 = glm::vec4(min.x, max.y, min.z, 1);
    glm::vec4 p6 = glm::vec4(max.x, max.y, min.z, 1);
    glm::vec4 p7 = glm::vec4(min.x, max.y, max.z, 1);
    glm::vec4 p8 = glm::vec4(max.x, max.y, max.z, 1);
    // bottom
    points.push_back(p1);
    points.push_back(p2);
    points.push_back(p1);
    points.push_back(p3);
    points.push_back(p2);
    points.push_back(p4);
    points.push_back(p3);
    points.push_back(p4);
    // side
    points.push_back(p1);
    points.push_back(p5);
    points.push_back(p2);
    points.push_back(p6);
    points.push_back(p3);
    points.push_back(p7);
    points.push_back(p4);
    points.push_back(p8);
    // top
    points.push_back(p5);
    points.push_back(p6);
    points.push_back(p5);
    points.push_back(p7);
    points.push_back(p6);
    points.push_back(p8);
    points.push_back(p7);
    points.push_back(p8);

    m_numVBOEntries = 24;

    /*
     * grid cell divisions
     */
    float xs[2] = {min.x, max.x};
    float ys[2] = {min.y, max.y};
    float zs[2] = {min.z, max.z};

    // x resolution
    for (int i = 0; i < gridRes.x; i++) {
        float x = min.x + (i * cellSize);
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 2; k++) {
                float y = ys[j];
                float z = zs[k];
                points.push_back(glm::vec4(x, y, z, 1));
                m_numVBOEntries++;
            }
        }
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 2; k++) {
                float y = ys[k];
                float z = zs[j];
                points.push_back(glm::vec4(x, y, z, 1));
                m_numVBOEntries++;
            }
        }
    }

    // y resolution
    for (int i = 0; i < gridRes.y; i++) {
        float y = min.y + (i * cellSize);
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 2; k++) {
                float x = xs[j];
                float z = zs[k];
                points.push_back(glm::vec4(x, y, z, 1));
                m_numVBOEntries++;
            }
        }
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 2; k++) {
                float x = xs[k];
                float z = zs[j];
                points.push_back(glm::vec4(x, y, z, 1));
                m_numVBOEntries++;
            }
        }
    }

    // z resolution
    for (int i = 0; i < gridRes.z; i++) {
        float z = min.z + (i * cellSize);
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 2; k++) {
                float x = xs[j];
                float y = ys[k];
                points.push_back(glm::vec4(x, y, z, 1));
                m_numVBOEntries++;
            }
        }
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 2; k++) {
                float x = xs[k];
                float y = ys[j];
                points.push_back(glm::vec4(x, y, z, 1));
                m_numVBOEntries++;
            }
        }
    }

    m_pointsVBO = 0;
    glGenBuffers(1, &m_pointsVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_pointsVBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * 4 * sizeof(float), points.data(), GL_STATIC_DRAW);

    m_pointsVAO = 0;
    glGenVertexArrays(1, &m_pointsVAO);
    glBindVertexArray(m_pointsVAO);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

    // disable the vao and vbo
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void drawGrid(ShaderProgram linesProgram)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(m_pointsVAO);

    linesProgram.use();
    linesProgram.update("viewMat", mp_camera->getViewMatrix());
    linesProgram.update("projMat", mp_camera->getProjectionMatrix());
    glDrawArrays(GL_LINES, 0, m_numVBOEntries);

    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

void updateGUI()
{
    // Main menu bar
    if (ImGui::BeginMainMenuBar())
    {
        /*
         * General menu
         */
        if (ImGui::BeginMenu("Menu"))
        {
            if(ImGui::MenuItem("Quit", "Esc", false, true)) { glfwSetWindowShouldClose(mp_Window, GL_TRUE); }
            ImGui::EndMenu();
        }

        /*
         * Shortcut infos
         */
        if (ImGui::BeginMenu("Shortcuts"))
        {
            ImGui::Text("P: Switch between proteins");
            ImGui::Text("W: Move selected protein up");
            ImGui::Text("A: Move selected protein left");
            ImGui::Text("S: Move selected protein down");
            ImGui::Text("D: Move selected protein right");
            ImGui::EndMenu();
        }

        /*
         * Protein infos
         */
        if (ImGui::BeginMenu("Proteins"))
        {
            if (m_proteinLoader.getNumberOfProteins() > 0) {
                for (int i = 0; i < m_proteinLoader.getNumberOfProteins(); i++) {
                    SimpleProtein* protein = m_proteinLoader.getProteinAt(i);
                    std::string text = std::to_string(i) + ": " + protein->name + " - atom number: " + std::to_string(protein->atoms.size());
                    ImGui::Text(text.c_str());
                }
            } else {
                ImGui::Text("No proteins loaded!");
            }
            std::string atomText = "Total number of atoms: " + std::to_string(m_proteinLoader.getNumberOfAllAtoms());
            ImGui::Text(atomText.c_str());
            ImGui::EndMenu();
        }

        /*
         * Protein infos
         */
        if (ImGui::BeginMenu("Grid"))
        {
            glm::vec3 min, max;
            m_search.getGridMinMax(min, max);

            std::string gridSizeText = "Gridsize: " + std::to_string(m_search.getGridSize().x)
                                             + ", " + std::to_string(m_search.getGridSize().y)
                                             + ", " + std::to_string(m_search.getGridSize().z);
            std::string gridResText  = "Gridres: "  + std::to_string(m_search.getGridResolution().x)
                                             + ", " + std::to_string(m_search.getGridResolution().y)
                                             + ", " + std::to_string(m_search.getGridResolution().z);
            std::string gridMinText  = "Gridmin: "  + std::to_string(min.x)
                                             + ", " + std::to_string(min.y)
                                             + ", " + std::to_string(min.z);
            std::string gridMaxText  = "Gridmax: "  + std::to_string(max.x)
                                             + ", " + std::to_string(max.y)
                                             + ", " + std::to_string(max.z);
            std::string gridCellNumText = "#Gridcells: " + std::to_string(m_search.getTotalGridNum());
            std::string cellSizeText = "Cellsize: " + std::to_string(m_search.getCellSize());
            std::string gridSearchText = "Gridsearch: " + std::to_string(m_search.getGridSearch());

            ImGui::Text(gridSizeText.c_str());
            ImGui::Text(gridResText.c_str());
            ImGui::Text(gridMinText.c_str());
            ImGui::Text(gridMaxText.c_str());
            ImGui::Text(gridCellNumText.c_str());
            ImGui::Text(gridSearchText.c_str());
            ImGui::Text(cellSizeText.c_str());
            ImGui::Checkbox("Show grid", &m_drawGrid);

            ImGui::EndMenu();
        }

        /*
         * Frametime
         */
        float framerate = ImGui::GetIO().Framerate;
        std::stringstream stream;
        stream << std::fixed << std::setprecision(0) << framerate;
        std::string fps = "FPS: " + stream.str();
        ImGui::MenuItem(fps.c_str(), "", false, false);

        /*
         * End main menu bar
         */
        ImGui::EndMainMenuBar();
    }

    ImGui::Render();
}



int main()
{
    Logger::instance().changeTab("     ");
    Logger::instance().print("Start Neighborhood search"); Logger::instance().tabIn();

    setup();

    printGPUInfos();

    SimpleProtein* proteinA = m_proteinLoader.loadProtein("PDB/1a19.pdb");

    SimpleProtein* proteinB = m_proteinLoader.loadProtein("PDB/1crn.pdb");
    SimpleProtein* proteinC = m_proteinLoader.loadProtein("PDB/1mbn.pdb");

    proteinA->center();
    proteinB->center();
    proteinC->center();
    proteinB->move(glm::vec3(proteinA->extent().x/2 + proteinB->extent().x/2, 0, 0));
    proteinC->move(glm::vec3(-proteinA->extent().x/2 - proteinC->extent().x/2, 0, 0));


    glm::vec3 gridResolution = glm::vec3(3,3,3);
    float searchRadius = 0.5;
    initNeighborhoodSearch(gridResolution, searchRadius);

    run();

    Logger::instance().tabOut(); Logger::instance().print("Exit Neighborhood search");

    return 0;
}