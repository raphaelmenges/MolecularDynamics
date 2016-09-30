//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

// Protein on GPU.

#ifndef GPU_PROTEIN_H
#define GPU_PROTEIN_H

#include "SurfaceExtraction/GPUBuffer.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>

// Forward declaration
class Protein;

class GPUProtein
{
public:

    // Structure to hold aminoacid information
    struct AminoAcid
    {
        AminoAcid(std::string name, int startIndex, int endIndex)
        {
            this->name = name;
            this->startIndex = startIndex;
            this->endIndex = endIndex;
        }

        std::string name;
        int startIndex;
        int endIndex;
    };

    // Constructors
    GPUProtein(Protein * const pProtein);
    GPUProtein(const std::vector<glm::vec4>& rAtoms); // vec3 center + float radius

    // Destructor
    virtual ~GPUProtein();

    // Bind SSBOs with radii and trajectory
    void bind(GLuint radiiSlot, GLuint trajectorySlot) const;

    // Bind radii SSBO
    void bindRadii(GLuint slot) const;

    // Bind trjactory SSBO
    void bindTrajectory(GLuint slot) const;

    // Bind SSBO with colors according to element
    void bindColorsElement(GLuint slot) const;

    // Bind SSBO with colors according to aminoacid
    void bindColorsAminoacid(GLuint slot) const;

    // Get count of atoms in protein
    int getAtomCount() const { return mspRadii->size(); }

    // Get count of frames available in trajectory
    int getFrameCount() const { return mspTrajectory->size(); }

    // Get shared pointer to atom radii
    std::shared_ptr<const std::vector<float> > getRadii() const;

    // Get shared pointer to trajectory (position per atom per frame)
    std::shared_ptr<const std::vector<std::vector<glm::vec3> > > getTrajectory() const;

    // Get center of protein at specific frame
    glm::vec3 getCenterOfMass(int frame) const { return mCentersOfMass.at(frame); }

    // Get element
    std::string getElement(int atomIndex) const { return mElements.at(atomIndex); }

    // Get aminoacid
    std::string getAminoacid(int atomIndex) const { return mAminoacids.at(atomIndex); }

    // Get minimum coordinates of initial structure
    glm::vec3 getMinCoordinates() const { return mMinCoordinates; }

    // Get maximum coordinates of initial structure
    glm::vec3 getMaxCoordinates() const { return mMaxCoordinates; }

    // Get minimum coordinates at a frame
    glm::vec3 getMinCoordinates(int frame) const { return mMinCoordinatesAnimation.at(frame); }

    // Get maximum coordinates at a frame
    glm::vec3 getMaxCoordinates(int frame) const { return mMaxCoordinatesAnimation.at(frame); }

    // Get amino acid information
    std::vector<AminoAcid> getAminoAcids() const { return mAminoAcids; }

    // Get radius of biggest atom
    float getMaxRadius() const { return mMaxRadius; }

private:

    // Initialize SSBOs
    void initSSBOs(int atomCount, int frameCount);

    // Vector of radii
    std::shared_ptr<std::vector<float> > mspRadii;

    // Vector of trajectory (outer is for frames, inner for atoms in each frame)
    std::shared_ptr<std::vector<std::vector<glm::vec3> > > mspTrajectory;

    // SSBO of radii
    GPUBuffer<float> mRadiiBuffer;

    // SSBO of trajectory
    GPUBuffer<glm::vec3> mTrajectoryBuffer;

    // Vector which holds the center of mass for each frame (ok, mass is not yet taken into account)
    std::vector<glm::vec3> mCentersOfMass;

    // Strings which hold element of atoms
    std::vector<std::string> mElements;

    // Strings which hold aminoacid of atoms
    std::vector<std::string> mAminoacids;

    // SSBO with colors for atoms according to element
    GPUBuffer<glm::vec3> mColorsElementBuffer;

    // SSBO with colors for atoms according to aminoacid
    GPUBuffer<glm::vec3> mColorsAminoacidBuffer;

    // Save mininum and maximum values of initial atom coordinate values
    glm::vec3 mMinCoordinates;
    glm::vec3 mMaxCoordinates;

    // Save minimum and maxmium postions per frame
    std::vector<glm::vec3> mMinCoordinatesAnimation;
    std::vector<glm::vec3> mMaxCoordinatesAnimation;

    // Vector with amino acids information
    std::vector<AminoAcid> mAminoAcids;

    // Radius of biggest atom
    float mMaxRadius;
};

#endif // GPU_PROTEIN_H
