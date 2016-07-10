#include "GPUProtein.h"
#include "Molecule/MDtrajLoader/Data/Protein.h"

GPUProtein::GPUProtein(Protein * const pProtein)
{
    // Create structures for CPU
    int atomCount  = pProtein->getAtoms()->size();
    int frameCount = pProtein->getAtomAt(0)->getCountOfFrames(); // TODO: what if no atoms in protein?
    mspRadii = std::shared_ptr<std::vector<float> >(new std::vector<float>);
    mspRadii->resize(atomCount);
    mspTrajectory = std::shared_ptr<
            std::vector<
                std::vector<glm::vec3> > >(
                    new std::vector<std::vector<glm::vec3> >);
    mspTrajectory->resize(frameCount);
    for(int i = 0; i < frameCount; i++)
    {
        mspTrajectory->at(i).resize(atomCount);
    }

    // Fill structures for CPU
    for(int i = 0; i < atomCount; i++)
    {
        // Collect radius
        mspRadii->at(i) = pProtein->getRadiusAt(i);

        // Collect trajectory
        for(int j = 0; j < frameCount; j++)
        {
            mspTrajectory->at(j).at(i) = pProtein->getAtoms()->at(i)->getPositionAtFrame(j);
        }
    }

    // Create structures on GPU
    glGenBuffers(1, &mRadiiSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mRadiiSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * atomCount, mspRadii->data(), GL_STATIC_DRAW);
    glGenBuffers(1, &mTrajectorySSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mTrajectorySSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec3) * frameCount * atomCount, mspTrajectory->data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

GPUProtein::~GPUProtein()
{
    glDeleteBuffers(1, &mRadiiSSBO);
    glDeleteBuffers(1, &mTrajectorySSBO);
}

void GPUProtein::bind(GLuint radiiSlot, GLuint trajectorySlot) const
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, radiiSlot, mRadiiSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, trajectorySlot, mTrajectorySSBO);
}

std::shared_ptr<std::vector<float> > GPUProtein::getRadii() const
{
    return mspRadii;
}

std::shared_ptr<
    std::vector<
        std::vector<glm::vec3> > > GPUProtein::getTrajectory() const
{
    return mspTrajectory;
}
