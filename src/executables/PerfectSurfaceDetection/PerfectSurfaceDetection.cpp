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

    // Test protein extent
    mupProtein->minMax(); // first, one has to calculate min and max value of protein
    glm::vec3 min = mupProtein->getMin();
    glm::vec3 max = mupProtein->getMax();
    std::cout << "Min extent of protein: " << min.x << ", " << min.y << ", " << min.z << std::endl;
    std::cout << "Max extent of protein: " << max.x << ", " << max.y << ", " << max.z << std::endl;

    // Test atom radii
    std::vector<Atom*>* pAtoms = mupProtein->getAtoms();
    AtomLUT atomLUT;
    for(int i = 0; i < pAtoms->size(); i++)
    {
        std::string element = pAtoms->at(i)->getElement();
        std::cout << "Atom: " <<  element << " Radius: " << atomLUT.vdW_radii_picometer.at(element) << std::endl;
    }

    // ### Create structure to find neighbors fast ###

    // Do it later, could improve performance
    // - determine size of volume and grid cell count
    // - do something similar to per pixel linked list to save all atoms per cell
    // - since only used in built up of cutting face list...really necessary?

    // ### Calculate Solvent Accesible Surface ###

    // Everything in one kernel? -> Yes :D Yolo
    // - build up cutting face list (cut molecule with neighbors)
    //  - input: atoms
    //  - output: per atom: cutting face list
    //  - algorithm: test atom's extented sphere against neighbors and save cutting planes
    // - build up / use intersection line list (consisting of nodes storing up to two endpoints)
    //  - (is in paper partley started in cutting face list buildup to stop if already internal)
    //  - input: cutting face list per atom
    //  - output: intersection line list or just boolean?
    //  - algorithm:
    //    - determine end points of intersection line of ALL pairs of cutting faces
    //    - if end point not cut away by ANY other cutting face, then keep it in list
    // - build array of indices of surface atoms
    //   - if intersection line list is empty, add index of atom to some global memory

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

    // # Prepare atoms input

    // # Prepare uint image to write indices of surface atoms

    // # Prepare atomic counter for writing results

    // # Execute compute shader to determine surface atoms

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
