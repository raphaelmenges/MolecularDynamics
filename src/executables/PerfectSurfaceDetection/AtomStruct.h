#ifndef ATOM_STRUCT_H
#define ATOM_STRUCT_H

#include <glm/glm.hpp>

// Struct which holds necessary information about singe atom
struct AtomStruct
{
    // Constructor
    AtomStruct(
        glm::vec3 center,
        float radius)
    {
        this->center = center;
        this->radius = radius;
    }

    // Fields
    glm::vec3 center;
    float radius;
};

#endif // ATOM_STRUCT_H
