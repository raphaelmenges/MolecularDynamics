//============================================================================
// Distributed under the MIT License. Author: Adrian Derstroff
//============================================================================

#ifndef OPENGL_FRAMEWORK_SIMPLEPROTEIN_H
#define OPENGL_FRAMEWORK_SIMPLEPROTEIN_H

#define FLOAT_MIN std::numeric_limits<float>::min()
#define FLOAT_MAX std::numeric_limits<float>::max()

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
    glm::vec3 bbMin;
    glm::vec3 bbMax;

    //_____________________________________//
    //           CONSTRUCTOR               //
    //_____________________________________//
    SimpleProtein(std::string name = "Protein") : name(name)
    {
        bbMin = glm::vec3(0, 0, 0);
        bbMax = glm::vec3(0, 0, 0);
    }

    void setName(std::string name)
    {
        this->name = name;
    }

    void move(glm::vec3 offset)
    {
        for (int i = 0; i < atoms.size(); i++) {
            atoms.at(i).pos += offset;
        }
        bbMin += offset;
        bbMax += offset;
    }

    glm::vec3 getCenterOfGravity()
    {
        glm::vec3 cog = glm::vec3(0,0,0);
        for (int i = 0; i < atoms.size(); i++) {
            cog += atoms.at(i).pos;
        }
        if (atoms.size() > 0) cog /= atoms.size();
        return cog;
    }

    void center()
    {
        glm::vec3 cog = getCenterOfGravity();
        for (int i = 0; i < atoms.size(); i++) {
            atoms.at(i).pos -= cog;
        }
        bbMin -= cog;
        bbMax -= cog;
    }

    void recalculateBB()
    {
        if (atoms.size() > 0) {
            bbMin = glm::vec3(FLOAT_MAX, FLOAT_MAX, FLOAT_MAX);
            bbMax = glm::vec3(FLOAT_MIN, FLOAT_MIN, FLOAT_MIN);
            for (int i = 0; i < atoms.size(); i++) {
                SimpleAtom atom = atoms.at(i);
                bbMin.x = glm::min(bbMin.x, atom.pos.x);
                bbMin.y = glm::min(bbMin.y, atom.pos.y);
                bbMin.z = glm::min(bbMin.z, atom.pos.z);
                bbMax.x = glm::max(bbMax.x, atom.pos.x);
                bbMax.y = glm::max(bbMax.y, atom.pos.y);
                bbMax.z = glm::max(bbMax.z, atom.pos.z);
            }
        } else {
            Logger::instance().print("No atoms to recalculate BB!", Logger::Mode::WARNING);
        }
    }

    glm::vec3 extent()
    {
        //recalculateBB();
        return (bbMax - bbMin);
    }

};

#endif //OPENGL_FRAMEWORK_SIMPLEPROTEIN_H
