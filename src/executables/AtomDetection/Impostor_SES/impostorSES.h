#ifndef ImpostorSES_H
#define ImpostorSES_H

#include "ShaderTools/Renderer.h"
#include "ShaderTools/RenderPass.h"
#include "ShaderTools/ComputeProgram.h"
#include "AssetTools/Texture.h"
#include "AssetTools/Mesh.h"
#include "ShaderTools/VertexArrayObjects/Quad.h"
#include "ShaderTools/VertexArrayObjects/ImpostorSpheres.h"
#include "Molecule/MDtrajLoader/Data/Protein.h"
#include "Molecule/MDtrajLoader/Data/SesPatchesProtein.h"

using namespace glm;

class ImpostorSES
{
public:
    ImpostorSES();

    GLFWwindow* window;

    mat4 projection;

    // program switches
    bool useAtomicCounters = true;
    bool perspectiveProj = true;

    bool vsync = true;

    // Shader Programs
    ShaderProgram spRenderDiscs;
    ShaderProgram spRenderBalls;
    ShaderProgram spRenderImpostor;

    // Render Passes
    RenderPass* renderBalls;
    RenderPass* detectVisible;
    RenderPass* result;

    ComputeProgram* computeVisibleIDs;

    // Textures/Buffers
    Texture* bufferTex;
    Texture* visibleIDsBuff;

    // Handles
    GLuint atomBuff;
    GLuint SSBO[2];
    GLuint timeQuery;
    GLuint queryTime;

    // variables
    float rotX = 0.0f;
    float rotY = 0.0f;
    float distance = 80.0;
    float scale = 1.0;
    float probeRadius = 1.4; // standard proberadius
    bool updateVisibilityMap = false;
    bool updateVisibilityMapLock = false; // L: lock U: unlock reduced elements for current frame
    bool pingPongOff = true; // reduce number of elements every other frame O: off P: Pingpong On

    ImpostorSpheres *impSph;
    int num_balls;

    int positions_size;
    int colors_size;

    unsigned char * zeros;

    std::vector<GLuint> identityInstancesMap;
    std::vector<GLint> mapAllVisible;

    // frame control
    bool animate = false;
    float lastTime = 0;
    float elapsedTime = 0;

    float fps           = 0.0f;
    float frameInterval = 0.0f;
    int numberOfFrames  = 0;

    void init();
    void initSES();
    void run();

    std::shared_ptr<Protein> prot;

    bool renderAtoms = true;
    bool renderSpherePatches = false;
    bool renderToroidalPatches = false;

    ShaderProgram spAtomImpostorQuad;
    ShaderProgram spAtomImpostorSphere;
    ShaderProgram spAtomImpostorSphereRaymarching;
    ShaderProgram spAtomImpostorNormal;
    ShaderProgram spAtomImpostorFull;
    ShaderProgram spAtomImpostorFullColored;

    RenderPass* rpAtoms;

    // --- SPHERE PATCH

    ShaderProgram spSpherePatchImpostorTriangle;
    ShaderProgram spSpherePatchImpostorTetrahedron;
    ShaderProgram spSpherePatchImpostorNormal;
    ShaderProgram spSpherePatchImpostorFull;
    ShaderProgram spSpherePatchImpostorFullColored;

    RenderPass* rpSpherePatches;

    // --- TOROIDAL PATCH

    ShaderProgram spToroidalPatchImpostorFrustum;
    ShaderProgram spToroidalPatchImpostorNormal;
    ShaderProgram spToroidalPatchImpostorFull;
    ShaderProgram spToroidalPatchImpostorFullColored;

    RenderPass* rpToroidalPatches;


    float r(float size) {
        return size * 2 * static_cast <float> (rand()) / static_cast <float> (RAND_MAX) - size;
    }

    float r2(float size) {
        return size * static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    }
};

#endif // ImpostorSES_H
