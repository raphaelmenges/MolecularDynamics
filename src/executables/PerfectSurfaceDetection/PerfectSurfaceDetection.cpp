#include "PerfectSurfaceDetection.h"

#include "Molecule/MDtrajLoader/MdTraj/MdTrajWrapper.h"
#include "Molecule/MDtrajLoader/Data/Protein.h"
#include "Molecule/MDtrajLoader/Data/AtomLUT.h"

#include <GL/glew.h>
#include <iostream>

// ### Shader implementation ###
const char* pSurfaceDetectionShaderSource =
"#version 430 core\n"

// Workgroup layout (just linear of atoms)
"layout(local_size_x=8, local_size_y=1, local_size_z=1) in;\n"

// Main function
"void main()\n"
"{\n"
"   // TODO;\n"
"}\n";

// ### Class implementation ###

PerfectSurfaceDetection::PerfectSurfaceDetection()
{
    // ### Load molecule ###

    // Path to protein molecule
    std::vector<std::string> paths;
    paths.push_back(std::string(RESOURCES_PATH) + "/molecules/PDB/1crn.pdb");

    // Load protein
    MdTrajWrapper mdwrap;
    mupProtein = mdwrap.load(paths);

    // Load LUT
    AtomLUT atomLUT;

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
    for(int i = 0; i < pAtoms->size(); i++)
    {
        std::string element = pAtoms->at(i)->getElement();
        std::cout << "Atom: " <<  element << " Radius: " << atomLUT.vdW_radii_picometer.at(element) << std::endl;
    }

    // # Set up compute shader

    // Compile shader
    GLint surfaceDetectionProgram = glCreateProgram();
    GLint surfaceDetectionShader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(surfaceDetectionShader, 1, &pSurfaceDetectionShaderSource, NULL);
    glCompileShader(surfaceDetectionShader);

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

    // # Prepare uint image to write indices of surface atoms

    // # Prepare atomic counter for writing results

    // # Execute compute shader to determine surface atoms

    // Bind everything
    // - Shader Program
    // - Image
    // - Atomic counter
    // - Atoms

    // Dispatch

    // ### Render protein with marked surface atoms ###

    // # Bind image with results

    // # Do simple impostor rendering (TODO: split into smaller tasks)

}

// ### Main function ###

int main()
{
    PerfectSurfaceDetection detection;
    return 0;
}
