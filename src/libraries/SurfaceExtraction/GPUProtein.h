// Author: Raphael Menges
// Protein on GPU.

#ifndef GPU_PROTEIN_H
#define GPU_PROTEIN_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>

// Forward declaration
class Protein;

class GPUProtein
{
public:

    // Constructor
    GPUProtein(Protein * const pProtein);

    // Destructor
    virtual ~GPUProtein();

    // Bind SSBOs (readonly)
    void bind(GLuint radiiSlot, GLuint trajectorySlot) const;

    // Get count of atoms in protein
    int getAtomCount() const { return mspRadii->size(); }

    // Get count of frames available in trajectory
    int getFrameCount() const { return mspTrajectory->size(); }

    // Get shared pointer to atom radii
    std::shared_ptr<std::vector<float> > getRadii() const;

    // Get shared pointer to trajectory (position per atom per frame)
    std::shared_ptr<
        std::vector<
            std::vector<glm::vec3> > > getTrajectory() const;

private:

    // Vector of radii
    std::shared_ptr<std::vector<float> > mspRadii;

    // Vector of trajectory (outer is for frames, inner for atoms in each frame)
    std::shared_ptr<std::vector<std::vector<glm::vec3> > > mspTrajectory;

    // SSBO of radii
    GLuint mRadiiSSBO;

    // SSBO of trajectory
    GLuint mTrajectorySSBO;

};

#endif // GPU_PROTEIN_H
