#include "PerfectSurfaceDetection.h"

#include "Molecule/MDtrajLoader/MdTraj/MdTrajWrapper.h"
#include "Molecule/MDtrajLoader/Data/Protein.h"
#include "Molecule/MDtrajLoader/Data/AtomLUT.h"

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

    // Create structure to find neighbors fast

    // Calculate Solvent Accesible Surface
}

// ### Main function ###

int main()
{
    PerfectSurfaceDetection detection;
    return 0;
}
