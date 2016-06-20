#include "GPUProtein.h"
#include "Molecule/MDtrajLoader/Data/Protein.h"

GPUProtein::GPUProtein(Protein * const pProtein)
{
    // Create structures on CPU
    mAtomCount = pProtein->getAtoms()->size();
    for(int i = 0; i < mAtomCount; i++)
    {
        // Push back all atoms (CONVERTING PICOMETER TO ANGSTROM)
        mAtoms.push_back(
            GPUAtom(
                pProtein->getAtoms()->at(i)->getPosition(),
                pProtein->getRadiusAt(i)));
    }

    // Create structures on GPU
    glGenBuffers(1, &mSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GPUAtom) * mAtoms.size(), mAtoms.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

}

GPUProtein::~GPUProtein()
{
    glDeleteBuffers(1, &mSSBO);
}

void GPUProtein::bind(GLuint slot) const
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot, mSSBO);
}
