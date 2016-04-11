#include "PerfectSurfaceDetection.h"

#include "ShaderTools/Renderer.h"
#include "Molecule/MDtrajLoader/MdTraj/MdTrajWrapper.h"
#include "Molecule/MDtrajLoader/Data/Protein.h"
#include "Molecule/MDtrajLoader/Data/AtomLUT.h"

#include <iostream>

// ### Shader implementation ###
const char* pSurfaceDetectionShaderSource =
"#version 430 core\n"

// Workgroup layout (just linear of atoms)
"layout(local_size_x=8, local_size_y=1, local_size_z=1) in;\n"

// Structs
"struct AtomStruct\n"
"{\n"
"   vec3 position;\n"
"   float radius;\n"
"};\n"

// SSBOs
"layout(std430, binding = 0) restrict readonly buffer AtomBuffer\n"
"{\n"
"	AtomStruct atoms[];\n"
"};\n"

// Uniforms
"uniform int atomCount;\n"

// Atomic counter
"layout(binding = 0) uniform atomic_uint index;\n"

// Image with output indices of surface atoms
"layout(binding = 1, r32ui) restrict writeonly uniform uimageBuffer list;\n"

// Main function
"void main()\n"
"{\n"
    // Check whether in range
"   if(gl_GlobalInvocationID.x < atomCount);\n"
"   {\n"
"       uint idx = atomicCounterIncrement(index);\n"
"   }\n"
"}\n";

// ### Class implementation ###

PerfectSurfaceDetection::PerfectSurfaceDetection()
{
    // Create window
    mpWindow = generateWindow();

    // Register keyboard callback
    std::function<void(int, int, int, int)> kC = [&](int k, int s, int a, int m)
    {
        this->keyCallback(k, s, a, m);
    };
    setKeyCallback(mpWindow, kC);

    // ### Load molecule ###

    // Path to protein molecule
    std::vector<std::string> paths;
    paths.push_back(std::string(RESOURCES_PATH) + "/molecules/PDB/1crn.pdb");

    // Load protein
    MdTrajWrapper mdwrap;
    mupProtein = mdwrap.load(paths);

    // Load LUT
    AtomLUT atomLUT;

    // Atom count
    int atomCount = (int) mupProtein->getAtoms()->size();

    // Output atom count
    std::cout << "Atom count: " << atomCount << std::endl;

    // Test protein extent
    mupProtein->minMax(); // first, one has to calculate min and max value of protein
    glm::vec3 proteinMinExtent = mupProtein->getMin();
    glm::vec3 proteinMaxExtent = mupProtein->getMax();
    std::cout
        << "Min extent of protein: "
        << proteinMinExtent.x << ", "
        << proteinMinExtent.y << ", "
        << proteinMinExtent.z << std::endl;
    std::cout
        << "Max extent of protein: "
        << proteinMaxExtent.x << ", "
        << proteinMaxExtent.y << ", "
        << proteinMaxExtent.z << std::endl;

    // Test atom radii
    std::vector<Atom*>* pAtoms = mupProtein->getAtoms();
    for(int i = 0; i < (int)pAtoms->size(); i++)
    {
        std::string element = pAtoms->at(i)->getElement();
        std::cout << "Atom: " <<  element << " Radius: " << atomLUT.vdW_radii_picometer.at(element) << std::endl;
    }

    // # Set up compute shader

    // Compile shader
    GLint surfaceDetectionProgram = glCreateProgram();
    GLint surfaceDetectionShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(surfaceDetectionShader, 1, &pSurfaceDetectionShaderSource, NULL);
    // glCompileShader(surfaceDetectionShader);

    // Log if error
    GLint log_length = 0;
    glGetShaderiv(surfaceDetectionShader, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length > 1)
    {
        // Copy log to chars
        GLchar *log = new GLchar[log_length];
        glGetShaderInfoLog(surfaceDetectionShader, log_length, NULL, log);

        // Print it
        std::cout << log << std::endl;

        // Delete chars
        delete[] log;
    }

    // Attach shader, link program and delete shader
    glAttachShader(surfaceDetectionProgram, surfaceDetectionShader);
    glLinkProgram(surfaceDetectionProgram);
    glDetachShader(surfaceDetectionProgram, surfaceDetectionShader);
    glDeleteShader(surfaceDetectionShader);

    // # Prepare atoms input (position + radius)

    // Struct which holds necessary information about singe atom
    struct AtomStruct
    {
        // Constructor
        AtomStruct(
            glm::vec3 position,
            float radius)
        {
            this->position = position;
            this->radius = radius;
        }

        // Fields
        glm::vec3 position;
        float radius;
    };

    // Vector which is used as data for SSBO
    std::vector<AtomStruct> atomStructs;
    for(Atom const * pAtom : *(mupProtein->getAtoms()))
    {
        // Push back all atoms
        atomStructs.push_back(
            AtomStruct(
                pAtom->getPosition(),
                atomLUT.vdW_radii_picometer.at(
                    pAtom->getElement())));
    }

    // Fill into ssbo
    GLuint atomsSSBO;
    glGenBuffers(1, &atomsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, atomsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(AtomStruct) * atomStructs.size(), atomStructs.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // # Prepare atomic counter for writing results to unique position in image

    GLuint atomicCounter;
    glGenBuffers(1, &atomicCounter);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicCounter);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_STATIC_DRAW);
    resetAtomicCounter(atomicCounter);

    // # Prepare uint image to write indices of surface atoms. Lets call it list for easier understanding

    // Buffer
    GLuint listBuffer;
    glGenBuffers(1, &listBuffer);
    glBindBuffer(GL_TEXTURE_BUFFER, listBuffer);
    glBufferData(GL_TEXTURE_BUFFER, sizeof(GLuint) * atomCount, 0, GL_STATIC_DRAW);
    glBindBuffer(GL_TEXTURE_BUFFER, 0);

    // Texture (which will be bound as image)
    GLuint listTexture;
    glGenTextures(1, &listTexture);
    glBindTexture(GL_TEXTURE_BUFFER, listTexture);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, listBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    // # Execute compute shader to determine surface atoms

    // Use compute shader program
    glUseProgram(surfaceDetectionProgram);

    // Bind SSBO with atoms
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, atomsSSBO);

    // Tell shader about count of atoms
    glUniform1i(glGetUniformLocation(surfaceDetectionProgram, "atomCount"), atomCount);

    // Bind atomic counter
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, atomicCounter);

    // Bind image where indices of surface atoms are written to
    glBindImageTexture(1,
                       listTexture,
                       0,
                       GL_TRUE,
                       0,
                       GL_WRITE_ONLY,
                       GL_R32UI);

    // Dispatch
    glDispatchCompute((atomCount / 8) + 1, 1, 1);
    glMemoryBarrier(GL_ALL_BARRIER_BITS);

    // ### Render protein with marked surface atoms ###

    // # Bind image with results

    // # Do simple impostor rendering (TODO: split into smaller tasks)

    // TODO: delete OpenGL objects
}

void PerfectSurfaceDetection::renderLoop()
{
    // Call render function of Rendering.h with lambda function
    render(mpWindow, [&] (float deltaTime)
    {
    });
}

void PerfectSurfaceDetection::keyCallback(int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        switch (key)
        {
        case GLFW_KEY_ESCAPE: { glfwSetWindowShouldClose(mpWindow, GL_TRUE); break; } // Does not work, but method gets called?!
        }
    }
}

GLuint PerfectSurfaceDetection::readAtomicCounter(GLuint atomicCounter) const
{
    // Read atomic counter
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicCounter);

    GLuint *mapping = (GLuint*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER,
                                                0,
                                                sizeof(GLuint),
                                                GL_MAP_READ_BIT);

    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    return mapping[0];
}

void PerfectSurfaceDetection::resetAtomicCounter(GLuint atomicCounter) const
{
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicCounter);

    // Map the buffer
    GLuint* mapping = (GLuint*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER,
                                                0 ,
                                                sizeof(GLuint),
                                                GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
    // Set memory to new value
    memset(mapping, 0, sizeof(GLuint));

    // Unmap the buffer
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}


// ### Main function ###

int main()
{
    PerfectSurfaceDetection detection;
    detection.renderLoop();
    return 0;
}

// ### Snippets ###

/*
    // Read back SSBO
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, atomsSSBO);
    AtomStruct *ptr;
    ptr = (AtomStruct *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for(int i = 0; i < atomCount; i++)
    {
        std::cout << i << ". " << ptr[i].radius << std::endl;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);


*/
