//
// Created by ubundrian on 09.06.16.
//

#ifndef OPENGL_FRAMEWORK_PROTEINLOADER_H
#define OPENGL_FRAMEWORK_PROTEINLOADER_H

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



    //_____________________________________//
    //             METHODS                 //
    //_____________________________________//
    void loadPDB(std::string filePath, SimpleProtein &protein, glm::vec3 &minPosition, glm::vec3 &maxPosition);

};


#endif //OPENGL_FRAMEWORK_PROTEINLOADER_H
