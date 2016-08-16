//
// Created by ubundrian on 09.06.16.
//

#ifndef OPENGL_FRAMEWORK_PROTEINLOADER_H
#define OPENGL_FRAMEWORK_PROTEINLOADER_H

#define FLOAT_MIN std::numeric_limits<float>::min()
#define FLOAT_MAX std::numeric_limits<float>::max()

// standard includes
#include <limits>

// framework includes
#include "Molecule/MDtrajLoader/MdTraj/MdTrajWrapper.h"
#include "Utils/Logger.h"

// project specific includes
#include "SimpleProtein.h"

class ProteinLoader {

public:
    //__________________PUBLIC__________________//
    //_____________________________________//
    //           CONSTRUCTOR               //
    //_____________________________________//
    ProteinLoader();
    ~ProteinLoader();

    int getNumberOfProteins();
    std::vector<SimpleProtein*> getProteins();
    SimpleProtein* getProteinAt(int i);
    std::vector<SimpleAtom> getAllAtoms();
    void updateAtoms();
    int getNumberOfAllAtoms();
    void getBoundingBoxAroundProteins(glm::vec3& min, glm::vec3& max);


    //_____________________________________//
    //             METHODS                 //
    //_____________________________________//
    SimpleProtein* loadProtein(std::string fileName);
    void loadPDB(std::string filePath, SimpleProtein &protein, glm::vec3 &minPosition, glm::vec3 &maxPosition);
private:
    std::vector<SimpleProtein*> m_proteins;
    std::vector<SimpleAtom> m_allAtoms;
    float m_currentProteinIdx;
};


#endif //OPENGL_FRAMEWORK_PROTEINLOADER_H
