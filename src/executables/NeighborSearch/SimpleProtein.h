//
// Created by ubundrian on 09.06.16.
//

#ifndef OPENGL_FRAMEWORK_SIMPLEPROTEIN_H
#define OPENGL_FRAMEWORK_SIMPLEPROTEIN_H

// external includes
#include <GL/glew.h>
#include <glm/glm.hpp>

// project specific includes
#include "SimpleAtom.h"

class SimpleProtein {

public:
    //_____________________________________//
    //            VARIABLES                //
    //_____________________________________//
    std::string name;
    std::vector<SimpleAtom> atoms;
    GLuint atomsSSBO;
    glm::vec3 bbMin;
    glm::vec3 bbMax;

    //_____________________________________//
    //           CONSTRUCTOR               //
    //_____________________________________//
    SimpleProtein(std::string name = "Protein") : name(name)
    {
        bbMin = glm::vec3(0.0f, 0.0f, 0.0f);
        bbMax = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    void setName(std::string name)
    {
        this->name = name;
    }

    void createAtom(std::string name, glm::vec3 pos, float radius)
    {
        SimpleAtom atom;
        atom.pos = pos;
        atom.radius = radius;
        atoms.push_back(std::move(atom));
    }

};

#endif //OPENGL_FRAMEWORK_SIMPLEPROTEIN_H
