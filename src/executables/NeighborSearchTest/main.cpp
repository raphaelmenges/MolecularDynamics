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
#include <Utils/PickingTexture.h>

// framework includes
#include "ShaderTools/Renderer.h"
#include "ShaderTools/ShaderProgram.h"
#include "Utils/Logger.h"

// local includes
#include "ProteinLoader.h"
#include "Utils/OrbitCamera.h"
#include "NeighborSearch/NeighborhoodSearch.h"






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
std::unique_ptr<OrbitCamera>    mp_camera;
glm::vec2                       m_CameraDeltaMovement;
float                           m_CameraSmoothTime;
bool                            m_rotateCamera = false;
PickingTexture                  m_pickingTexture;


// protein
ProteinLoader   m_proteinLoader;
int             m_selectedAtom = -1;
int             m_selectedProtein = 0;
float           m_proteinMoveSpeed = 2.f;


// imgui gpu
GLint m_maxStorageBufferBindings = -1;
GLint m_maxVertShaderStorageBlocks = -1;
GLint m_maxFragShaderStorageBlocks = -1;
GLint m_maxComputeShaderStorageBlocks = -1;
int   m_work_grp_cnt[3];
int   m_work_grp_size[3];


// rendering
glm::vec3   m_lightDirection;
static bool m_drawGrid  = true;
ShaderProgram m_impostorProgram;
ShaderProgram m_linesProgram;
ShaderProgram m_searchRadiusProgram;
ShaderProgram m_pickingProgram;
GLuint m_pointsVBO;
GLuint m_pointsVAO;
int    m_numVBOEntries;


// neighborhood search
NeighborhoodSearch  m_search;
float               m_searchRadius;
glm::ivec3          m_gridRes;
static bool         m_findOnlySelectedAtomsNeighbors = false;
bool                m_updateNeighborhoodSearch = false;
ShaderProgram       m_extractElementPositionsShader;
ShaderProgram       m_findSelectedAtomsNeighborsShader;
ShaderProgram       m_colorAtomsInRadiusShader;
GLuint m_atomsSSBO;
GLuint* m_positionsSSBO;
GLuint* m_searchResultsSSBO;


// time
Timer m_runTimer;
Timer m_applicationTimer;


// for debug
int m_debugOffset = 0;






/*
 * forward declarations
 */
void setup();
void compileShaderPrograms();
void setupBuffers();

void keyCallback(int key, int scancode, int action, int mods);
void mouseButtonCallback(int button, int action, int mods);
void scrollCallback(double xoffset, double yoffset);
int  getAtomBeneathCursor();
void moveProteinInsideGrid(glm::vec3 offset);

void retrieveGPUInfos();
void updateAtomsSSBO();
void initNeighborhoodSearch(glm::vec3 gridResolution, float searchRadius);

void run();
void setupLinesBuffer();
void drawGrid(ShaderProgram linesProgram);
void drawSearchRadius(ShaderProgram searchRadiusProgram);

void findNeighbors(Neighborhood& neighborhood);
void findSelectedAtomsNeighbors(Neighborhood& neighborhood, int selectedAtomIdx);
void colorAtomsInRadius(Neighborhood& neighborhood);

void fillPickingTexture(ShaderProgram pickingProgram);
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
     * setup opengl
     */
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

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

    /*
     * init picking buffer
     */
    m_pickingTexture.Init(WIDTH, HEIGHT);
}
void compileShaderPrograms()
{
    /*
     * setup shader programs
     */
    m_impostorProgram     = ShaderProgram("/NeighborSearch/renderAtoms/impostor.vert", "/NeighborSearch/renderAtoms/impostor.geom", "/NeighborSearch/renderAtoms/impostor.frag");
    m_linesProgram        = ShaderProgram("/NeighborSearch/renderLines/lines.vert", "/NeighborSearch/renderLines/lines.frag");
    m_searchRadiusProgram = ShaderProgram("/NeighborSearch/renderSearchRadius/radius.vert", "/NeighborSearch/renderSearchRadius/radius.geom", "/NeighborSearch/renderSearchRadius/radius.frag");
    m_pickingProgram      = ShaderProgram("/NeighborSearch/atomPicking/picking.vert", "/NeighborSearch/atomPicking/picking.geom", "/NeighborSearch/atomPicking/picking.frag");
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        //Logger::instance().print("GLerror after init shader programs: " + std::to_string(err), Logger::Mode::ERROR);
    }

    /*
     * setup compute shader
     */
    m_extractElementPositionsShader = ShaderProgram("/NeighborSearch/neighborhoodSearch/extractElementPositions.comp");
    m_findSelectedAtomsNeighborsShader  = ShaderProgram("/NeighborSearch/searchApplication/findSelectedAtomsNeighbors.comp");
    m_colorAtomsInRadiusShader          = ShaderProgram("/NeighborSearch/searchApplication/colorAtomsInRadius.comp");
}

void setupBuffers()
{
    /*
     * generate, bind, fill and then unbind atom ssbo buffer
     */
    m_atomsSSBO;
    glGenBuffers(1, &m_atomsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_atomsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(SimpleAtom) * m_proteinLoader.getNumberOfAllAtoms(), m_proteinLoader.getAllAtoms().data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_atomsSSBO);

    /*
     * init positions ssbo
     */
    m_positionsSSBO = new GLuint;
    GPUHandler::initSSBO<glm::vec4>(m_positionsSSBO, m_proteinLoader.getNumberOfAllAtoms());

    /*
     * init search results ssbo
     */
    m_searchResultsSSBO = new GLuint;
    GPUHandler::initSSBO<int>(m_searchResultsSSBO, m_proteinLoader.getNumberOfAllAtoms());
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
    if (key == GLFW_KEY_O) {
        if (action == GLFW_PRESS) {
            m_selectedAtom = (m_selectedAtom+1) % m_proteinLoader.getNumberOfAllAtoms();
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
    else if (key == GLFW_KEY_RIGHT) {
        if (action == GLFW_PRESS) {
            m_debugOffset++;
        }
    }
    else if (key == GLFW_KEY_LEFT) {
        if (action == GLFW_PRESS) {
            m_debugOffset--;
        }
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

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        int atomIdx = getAtomBeneathCursor();
        m_selectedAtom = atomIdx;
    }
}

void scrollCallback(double xoffset, double yoffset)
{
    mp_camera->setRadius(mp_camera->getRadius() - 2.f * (float)yoffset);
}

int getAtomBeneathCursor()
{
    // Variables to collect results
    int foundIndex = -1;

    // extract atom id from pixel info
    double cursorX, cursorY;
    glfwGetCursorPos(mp_Window, &cursorX, &cursorY);
    PickingTexture::PixelInfo pixel = m_pickingTexture.ReadPixel((uint)cursorX, HEIGHT - (uint)(cursorY) - 1);
    if (pixel.ObjectID >= 1) {
        foundIndex = (int)pixel.ObjectID;
    }

    return foundIndex;
}

void moveProteinInsideGrid(glm::vec3 offset)
{
    SimpleProtein* protein = m_proteinLoader.getProteinAt(m_selectedProtein);

    glm::vec3 min = protein->bbMin + offset;
    glm::vec3 max = protein->bbMax + offset;
    glm::vec3 gridMin, gridMax;
    m_search.getGridMinMax(gridMin, gridMax);

    // checking lower bounds
    if (min.x < gridMin.x) offset.x += gridMin.x - min.x;
    if (min.y < gridMin.y) offset.y += gridMin.y - min.y;
    if (min.z < gridMin.z) offset.z += gridMin.z - min.z;

    // checking upper bounds
    if (max.x > gridMax.x) offset.x -= max.x - gridMax.x;
    if (max.y > gridMax.y) offset.y -= max.y - gridMax.y;
    if (max.z > gridMax.z) offset.z -= max.z - gridMax.z;

    protein->move(offset);
}



/*
 * NEIGHBORHOOD SEARCH
 */
void retrieveGPUInfos()
{
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &m_maxStorageBufferBindings);
    glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS,   &m_maxVertShaderStorageBlocks);
    glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &m_maxFragShaderStorageBlocks);
    glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS,  &m_maxComputeShaderStorageBlocks);
    glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &m_work_grp_cnt[0]);
    glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &m_work_grp_cnt[1]);
    glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &m_work_grp_cnt[2]);
    glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &m_work_grp_size[0]);
    glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &m_work_grp_size[1]);
    glGetIntegeri_v (GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &m_work_grp_size[2]);
}

void updateAtomsSSBO()
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_atomsSSBO);
    GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(p, m_proteinLoader.getAllAtoms().data(), sizeof(SimpleAtom)*m_proteinLoader.getNumberOfAllAtoms());
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    int numBlocks = ceil((float)m_proteinLoader.getNumberOfAllAtoms() / BLOCK_SIZE);

    // update element positions
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_atomsSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, *m_positionsSSBO);

    m_extractElementPositionsShader.use();
    m_extractElementPositionsShader.update("pnum", m_proteinLoader.getNumberOfAllAtoms());
    glDispatchCompute(numBlocks, 1, 1);
    glMemoryBarrier (GL_ALL_BARRIER_BITS);
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
     * generate lines for the grid
     */
    setupLinesBuffer();

    /*
     * setup camera
     */
    glm::vec3 cameraCenter = glm::vec3(0.0, 0.0, 0.0);
    float cameraRadius = 0.0f;
    glm::vec3 globalMin;
    glm::vec3 globalMax;
    m_proteinLoader.getCenteredBoundingBoxAroundProteins(globalMin, globalMax);
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
     * setup neighborhood
     */
    Neighborhood neighborhood;


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
         * update neighborhood search if requested
         */
        if (m_updateNeighborhoodSearch) {
            m_updateNeighborhoodSearch = false;

            glm::fvec3 min, max;
            m_proteinLoader.getCenteredBoundingBoxAroundProteins(min, max);

            m_search.update(m_proteinLoader.getNumberOfAllAtoms(), min, max, m_gridRes, m_searchRadius);

            setupLinesBuffer();
        }

        /*
         * setup neighborhood search
         */
        m_runTimer.start();
        m_search.run(m_positionsSSBO, neighborhood);
        m_runTimer.stop();

        /*
         * find neighbors
         */
        m_applicationTimer.start();
        findNeighbors(neighborhood);
        m_applicationTimer.stop();

        /*
         * draw proteins as impostor
         */
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_atomsSSBO);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, *m_searchResultsSSBO);
        m_impostorProgram.use();
        m_impostorProgram.update("view", mp_camera->getViewMatrix());
        m_impostorProgram.update("projection", mp_camera->getProjectionMatrix());
        m_impostorProgram.update("cameraWorldPos", mp_camera->getPosition());
        m_impostorProgram.update("probeRadius", 0.f);
        m_impostorProgram.update("lightDir", m_lightDirection);
        m_impostorProgram.update("selectedIndex", m_selectedAtom);
        m_impostorProgram.update("proteinNum", (int)m_proteinLoader.getNumberOfProteins());
        m_impostorProgram.update("selectedProtein", m_selectedProtein);
        glDrawArrays(GL_POINTS, 0, (GLsizei)m_proteinLoader.getNumberOfAllAtoms());

        /*
         * draw the grid
         */
        if (m_drawGrid) {
            drawGrid(m_linesProgram);
        }

        /*
         * drawing a circle around the selected atom
         */
        drawSearchRadius(m_searchRadiusProgram);

        /*
         * filling the picking texture with the visible
         * atom ids for every pixel
         */
        fillPickingTexture(m_pickingProgram);

        /*
         * update gui
         */
        updateGUI();
    });
}

void setupLinesBuffer()
{
    glm::vec3 min, max;
    m_search.getGridMinMax(min, max);
    float cellSize = m_search.getCellSize();
    glm::ivec3 gridRes = m_search.getGridResolution();

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

    if (m_pointsVBO != 0) glDeleteBuffers(1, &m_pointsVBO);
    m_pointsVBO = 0;
    glGenBuffers(1, &m_pointsVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_pointsVBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * 4 * sizeof(float), points.data(), GL_STATIC_DRAW);

    if (m_pointsVAO != 0) glDeleteVertexArrays(1, &m_pointsVAO);
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

void drawSearchRadius(ShaderProgram searchRadiusProgram)
{
    // only draw when an atom is selected
    if (m_selectedAtom >= 0) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        searchRadiusProgram.use();
        SimpleAtom selectedAtom = m_proteinLoader.getAllAtoms().at(m_selectedAtom);
        searchRadiusProgram.update( "selectedAtomPosition", selectedAtom.pos                 );
        searchRadiusProgram.update( "searchRadius",         m_searchRadius                   );
        searchRadiusProgram.update( "view",                 mp_camera->getViewMatrix()       );
        searchRadiusProgram.update( "projection",           mp_camera->getProjectionMatrix() );
        glDrawArrays(GL_POINTS, 0, 1);
        glBindVertexArray(0);

        glDisable(GL_BLEND);
    }
}



/*
 * applying the neighborhood informations
 */
void findNeighbors(Neighborhood& neighborhood)
{
    m_applicationTimer.start();

    if (m_findOnlySelectedAtomsNeighbors) {
        findSelectedAtomsNeighbors(neighborhood, m_selectedAtom);
    } else {
        colorAtomsInRadius(neighborhood);
    }

    m_applicationTimer.stop();
}

void findSelectedAtomsNeighbors(Neighborhood& neighborhood, int selectedAtomIdx)
{
    // reset search results
    GPUHandler::fillSSBO<int>(m_searchResultsSSBO, m_proteinLoader.getNumberOfAllAtoms(), 0);

    int numBlocks = ceil((float)m_proteinLoader.getNumberOfAllAtoms() / BLOCK_SIZE);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_atomsSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, *neighborhood.dp_particleCell);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, *neighborhood.dp_particleCellIndex);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, *neighborhood.dp_gridCellCounts);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, *neighborhood.dp_gridCellOffsets);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, *neighborhood.dp_grid);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, *neighborhood.dp_particleOriginalIndex);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, *m_searchResultsSSBO);

    m_findSelectedAtomsNeighborsShader.use();
    m_findSelectedAtomsNeighborsShader.update("selectedAtomUndx", selectedAtomIdx);
    m_findSelectedAtomsNeighborsShader.update("pnum",             m_proteinLoader.getNumberOfAllAtoms());
    m_findSelectedAtomsNeighborsShader.update("radius2",          neighborhood.searchRadius * neighborhood.searchRadius);
    m_findSelectedAtomsNeighborsShader.update("gridAdjCnt",       neighborhood.numberOfSearchCells);
    m_findSelectedAtomsNeighborsShader.update("searchCellOff",    neighborhood.startCellOffset);
    glUniform1iv(glGetUniformLocation(m_findSelectedAtomsNeighborsShader.getProgramHandle(),"gridAdj"), 216, neighborhood.p_searchCellOffsets);
    glDispatchCompute(numBlocks, 1, 1);
    glMemoryBarrier (GL_ALL_BARRIER_BITS);
}

void colorAtomsInRadius(Neighborhood& neighborhood)
{
    int numBlocks = ceil((float)m_proteinLoader.getNumberOfAllAtoms() / BLOCK_SIZE);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_atomsSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, *neighborhood.dp_particleCell);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, *neighborhood.dp_particleCellIndex);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, *neighborhood.dp_gridCellCounts);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, *neighborhood.dp_gridCellOffsets);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, *neighborhood.dp_grid);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, *neighborhood.dp_particleOriginalIndex);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, *m_searchResultsSSBO);

    m_colorAtomsInRadiusShader.use();
    m_colorAtomsInRadiusShader.update("pnum",             m_proteinLoader.getNumberOfAllAtoms());
    m_colorAtomsInRadiusShader.update("radius2",          neighborhood.searchRadius * neighborhood.searchRadius);
    m_colorAtomsInRadiusShader.update("gridAdjCnt",       neighborhood.numberOfSearchCells);
    m_colorAtomsInRadiusShader.update("searchCellOff",    neighborhood.startCellOffset);
    glUniform1iv(glGetUniformLocation(m_colorAtomsInRadiusShader.getProgramHandle(),"gridAdj"), 216, neighborhood.p_searchCellOffsets);
    glDispatchCompute(numBlocks, 1, 1);
    glMemoryBarrier (GL_ALL_BARRIER_BITS);
}



void fillPickingTexture(ShaderProgram pickingProgram)
{
    m_pickingTexture.EnableWriting();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    pickingProgram.use();
    pickingProgram.update("view", mp_camera->getViewMatrix());
    pickingProgram.update("projection", mp_camera->getProjectionMatrix());
    glDrawArrays(GL_POINTS, 0, (GLsizei)m_proteinLoader.getNumberOfAllAtoms());

    m_pickingTexture.DisableWriting();
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
        if (ImGui::BeginMenu("Controls"))
        {
            ImGui::Text("Rotate the camera by holding left mouse button while moving the mouse");
            ImGui::Text("Select an atom with right click");
            ImGui::Text("P: Switch between proteins");
            ImGui::Text("O: Switch between atoms");
            ImGui::Text("W: Move selected protein up");
            ImGui::Text("A: Move selected protein left");
            ImGui::Text("S: Move selected protein down");
            ImGui::Text("D: Move selected protein right");
            ImGui::Text("Q: Move selected protein back");
            ImGui::Text("E: Move selected protein forth");
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
            ImGui::Separator();
            std::string atomText = "Total number of atoms: " + std::to_string(m_proteinLoader.getNumberOfAllAtoms());
            ImGui::Text(atomText.c_str());
            ImGui::EndMenu();
        }

        /*
         * Grid infos
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
            ImGui::Separator();
            ImGui::Checkbox("Show grid", &m_drawGrid);

            ImGui::EndMenu();
        }

        /*
         * Gpu infos
         */
        if (ImGui::BeginMenu("GPU"))
        {
            ImGui::Text("Limits");
            std::string maxSSBBindingsText = "Maximum shader storage buffer bindings: " + std::to_string(m_maxStorageBufferBindings);
            std::string maxVSBlocksText = "Maximum vertex shader storage blocks: " + std::to_string(m_maxVertShaderStorageBlocks);
            std::string maxFSBlocksText = "Maximum fragment shader storage blocks: " + std::to_string(m_maxFragShaderStorageBlocks);
            std::string maxCSBlocksText = "Maximum compute shader storage blocks: " + std::to_string(m_maxComputeShaderStorageBlocks);
            std::string maxGWGSizeText = "Maximum global work group size: " + std::to_string(m_work_grp_cnt[0]) + ", " + std::to_string(m_work_grp_cnt[1]) + ", " + std::to_string(m_work_grp_cnt[2]);
            std::string maxLWGSizeText = "Maximum local work group size: " + std::to_string(m_work_grp_size[0]) + ", " + std::to_string(m_work_grp_size[1]) + ", " + std::to_string(m_work_grp_size[2]);
            ImGui::Text(maxSSBBindingsText.c_str());
            ImGui::Text(maxVSBlocksText.c_str());
            ImGui::Text(maxFSBlocksText.c_str());
            ImGui::Text(maxCSBlocksText.c_str());
            ImGui::Text(maxGWGSizeText.c_str());
            ImGui::Text(maxLWGSizeText.c_str());

            ImGui::Separator();

            ImGui::Text("Current Computation");
            std::string numBlocksAtomsText = "Number of used workgroups for the atoms: " + std::to_string(m_search.getNumberOfBlocksForElementsComputation());
            std::string numThreadsAtomsText = "Number of workitems per workgroup for the atoms: " + std::to_string(m_search.getNumberOfThreadsPerBlockForElementsComputation());
            std::string numBlocksGridText = "Number of used workgroups for the grid: " + std::to_string(m_search.getNumberOfBlocksForGridComputation());
            std::string numThreadsGridText = "Number of workitems per workgroup for the grid: " + std::to_string(m_search.getNumberOfThreadsPerBlockForGridComputation());
            ImGui::Text(numBlocksAtomsText.c_str());
            ImGui::Text(numThreadsAtomsText.c_str());
            ImGui::Text(numBlocksGridText.c_str());
            ImGui::Text(numThreadsGridText.c_str());

            ImGui::EndMenu();
        }

        /*
         * Neighborhood search infos
         */
        if (ImGui::BeginMenu("Neighborhood search"))
        {
            float oldSearchRadius = m_searchRadius;
            glm::ivec3 oldGridRes = m_gridRes;
            ImGui::SliderInt("Grid resolution x", &m_gridRes.x, 1, 20);
            ImGui::SliderInt("Grid resolution y", &m_gridRes.y, 1, 20);
            ImGui::SliderInt("Grid resolution z", &m_gridRes.z, 1, 20);
            ImGui::SliderFloat("Search radius", &m_searchRadius, 0, m_search.getMaxSearchRadius());
            if (oldSearchRadius != m_searchRadius ||
                    m_gridRes.x != oldGridRes.x   ||
                    m_gridRes.y != oldGridRes.y   ||
                    m_gridRes.z != oldGridRes.z) {
                m_updateNeighborhoodSearch = true;
            }
            std::string setupTimeText = "Setup time: " + std::to_string(m_runTimer.getDuration()/1000.0) + " ms";
            std::string searchTimeText = "Neighborhood search time: " + std::to_string(m_applicationTimer.getDuration()/1000.0) + " ms";
            ImGui::Text(setupTimeText.c_str());
            ImGui::Text(searchTimeText.c_str());
            ImGui::Checkbox("Find only neighbors of selected atom", &m_findOnlySelectedAtomsNeighbors);

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

    /*
     * general setup
     */
    setup();
    compileShaderPrograms();

    /*
     * get gpu infos for imgui
     */
    retrieveGPUInfos();

    /*
     * load proteins
     */
    SimpleProtein* proteinA = m_proteinLoader.loadProtein("PDB/1a19.pdb");
    SimpleProtein* proteinB = m_proteinLoader.loadProtein("PDB/5bs0.pdb");
    proteinA->center();
    proteinB->center();
    proteinB->move(glm::vec3(proteinA->extent().x/2 + proteinB->extent().x/2, 0, 0));

    /*
     * initialize neighborhood search
     */
    m_gridRes = glm::vec3(10, 10, 10);
    m_searchRadius = 20.f;
    initNeighborhoodSearch(m_gridRes, m_searchRadius);
    m_searchRadius = m_search.getMaxSearchRadius() - 0.01f;
    m_updateNeighborhoodSearch = true;

    /*
     * some setups that require the number of atoms
     */
    setupBuffers();

    /*
     * run application
     */
    run();

    Logger::instance().tabOut(); Logger::instance().print("Exit Neighborhood search");

    return 0;
}
