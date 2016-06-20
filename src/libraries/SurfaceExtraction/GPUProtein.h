// Author: Raphael Menges
// Protein on GPU.

#ifndef GPU_PROTEIN_H
#define GPU_PROTEIN_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

// Forward declaration
class Protein;

// Struct which holds necessary information about single atom
struct GPUAtom
{
    // Constructor
    GPUAtom(
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

class GPUProtein
{
public:

    // Constructor
    GPUProtein(Protein * const pProtein);

    // Destructor
    virtual ~GPUProtein();

    // Bind SSBO with atoms (readonly)
    void bind(int slot);

    // Get count of atoms in protein
    int getAtomCount() const { return mAtomCount; }

    // Get copy of GPUAtoms
    std::vector<GPUAtom> getAtoms() const { return mAtoms; }

private:

    // Vector of atoms
    std::vector<GPUAtom> mAtoms;

    // SSBO with atoms
    GLuint mSSBO;

    // Count of atoms in SSBO
    int mAtomCount;

};

#endif // GPU_PROTEIN_H
