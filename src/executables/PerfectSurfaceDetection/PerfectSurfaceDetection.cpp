#include "PerfectSurfaceDetection.h"

#include "Molecule/MDtrajLoader/MdTraj/MdTrajWrapper.h"
#include "Molecule/MDtrajLoader/Data/Protein.h"

#include <iostream>

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

    // Test protein
    mupProtein->minMax();
    glm::vec3 min = mupProtein->getMin();
    std::cout << min.x << ", " << min.y << ", " << min.z << std::endl;

    // Create structure to find neighbors fast

    // Calculate Solvent Accesible Surface
}

// ### Main function ###

int main()
{
    PerfectSurfaceDetection detection;
    return 0;
}
