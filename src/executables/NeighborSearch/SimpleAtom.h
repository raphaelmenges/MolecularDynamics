//
// Created by ubundrian on 09.06.16.
//

#ifndef OPENGL_FRAMEWORK_SIMPLEATOM_H
#define OPENGL_FRAMEWORK_SIMPLEATOM_H

#include <glm/glm.hpp>

class SimpleAtom {
public:
    //_____________________________________//
    //            VARIABLES                //
    //_____________________________________//
    std::string name;
    glm::vec3 pos;
    float radius;

    //_____________________________________//
    //           CONSTRUCTOR               //
    //_____________________________________//
    SimpleAtom(std::string name, glm::vec3 pos, float radius)
    {
        this->name = name;
        this->pos = pos;
        this->radius = radius;
    }

    ~SimpleAtom()
    {

    }

};

#endif //OPENGL_FRAMEWORK_SIMPLEATOM_H
