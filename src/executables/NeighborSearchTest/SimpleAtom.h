//============================================================================
// Distributed under the MIT License. Author: Adrian Derstroff
//============================================================================

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
