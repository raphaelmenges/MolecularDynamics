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
    glm::vec3 pos;
    float radius;
    glm::vec4 proteinID;
};

#endif //OPENGL_FRAMEWORK_SIMPLEATOM_H
