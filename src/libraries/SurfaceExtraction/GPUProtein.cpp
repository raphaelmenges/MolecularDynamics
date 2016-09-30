//============================================================================
// Distributed under the MIT License. Author: Raphael Menges
//============================================================================

#include "GPUProtein.h"
#include "Molecule/MDtrajLoader/Data/Protein.h"
#include "Molecule/MDtrajLoader/Data/AtomLUT.h"

GPUProtein::GPUProtein(Protein * const pProtein)
{
    // Create structures for CPU
    int atomCount  = pProtein->getAtoms()->size();
    int frameCount = pProtein->getAtomAt(0)->getCountOfFrames(); // TODO: what if no atoms in protein?
    mspRadii = std::shared_ptr<std::vector<float> >(new std::vector<float>);
    mspRadii->reserve(atomCount);

    // Already size trajectory vector
    mspTrajectory = std::shared_ptr<
            std::vector<
                std::vector<glm::vec3> > >(
                    new std::vector<std::vector<glm::vec3> >);
    mspTrajectory->resize(frameCount);
    for(int i = 0; i < frameCount; i++)
    {
        mspTrajectory->at(i).resize(atomCount);
    }

    // Reserve space in other vectors (which are all assumed to be empty)
    mCentersOfMass.reserve(frameCount);
    mElements.reserve(atomCount);
    mAminoacids.reserve(atomCount);

    // Fill radii, elements and aminoacids on CPU
    mMaxRadius = std::numeric_limits<float>::min();
    mMinCoordinates = glm::vec3(
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max());
    mMaxCoordinates = glm::vec3(
        std::numeric_limits<float>::min(),
        std::numeric_limits<float>::min(),
        std::numeric_limits<float>::min());
    for(int i = 0; i < atomCount; i++) // go over atoms
    {
        // Collect radius
        mspRadii->push_back(pProtein->getRadiusAt(i));

        // Element
        mElements.push_back(pProtein->getAtomAt(i)->getElement());

        // Aminoacid
        mAminoacids.push_back(pProtein->getAtomAt(i)->getAmino());

        // Update max radius
        mMaxRadius = mMaxRadius < pProtein->getRadiusAt(i) ? pProtein->getRadiusAt(i) : mMaxRadius;

        // Update min / max coordinate values
        glm::vec3 position = pProtein->getAtomAt(i)->getPosition();
        mMinCoordinates.x = mMinCoordinates.x > position.x ? position.x : mMinCoordinates.x;
        mMinCoordinates.y = mMinCoordinates.y > position.y ? position.y : mMinCoordinates.y;
        mMinCoordinates.z = mMinCoordinates.z > position.z ? position.z : mMinCoordinates.z;
        mMaxCoordinates.x = mMaxCoordinates.x < position.x ? position.x : mMaxCoordinates.x;
        mMaxCoordinates.y = mMaxCoordinates.y < position.y ? position.y : mMaxCoordinates.y;
        mMaxCoordinates.z = mMaxCoordinates.z < position.z ? position.z : mMaxCoordinates.z;
    }

    // Fill trajectory on CPU
    mMinCoordinatesAnimation.reserve(frameCount);
    mMaxCoordinatesAnimation.reserve(frameCount);
    for(int i = 0; i < frameCount; i++) // go over frames
    {
        glm::vec3 minCoords = glm::vec3(
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max());
        glm::vec3 maxCoords = glm::vec3(
            std::numeric_limits<float>::min(),
            std::numeric_limits<float>::min(),
            std::numeric_limits<float>::min());
        glm::vec3 accPosition(0, 0, 0);
        for(int j = 0; j < atomCount; j++) // go over atoms
        {
            // Position of that atom in that frame
            glm::vec3 position = pProtein->getAtoms()->at(j)->getPositionAtFrame(i);

            // Collect trajectory (already correctly sized)
            mspTrajectory->at(i).at(j) = position;

            // Accumulate position
            accPosition += position;

            // Update min / max coordinate values
            minCoords.x = minCoords.x > position.x ? position.x : minCoords.x;
            minCoords.y = minCoords.y > position.y ? position.y : minCoords.y;
            minCoords.z = minCoords.z > position.z ? position.z : minCoords.z;
            maxCoords.x = maxCoords.x < position.x ? position.x : maxCoords.x;
            maxCoords.y = maxCoords.y < position.y ? position.y : maxCoords.y;
            maxCoords.z = maxCoords.z < position.z ? position.z : maxCoords.z;
        }

        // Push back min and max coordinates
        mMinCoordinatesAnimation.push_back(minCoords);
        mMaxCoordinatesAnimation.push_back(maxCoords);

        // Save center
        mCentersOfMass.push_back(accPosition / atomCount);
    }

    // Init SSBOs
    initSSBOs(atomCount, frameCount);

    // Extract amino acids (here should be const pointers :( )
    std::vector<std::string>* pAminoAcids = pProtein->getAminoNames();

    // Assumption: index range with no missing indices
    for(std::string name : *pAminoAcids)
    {
        std::vector<Atom*>* pAtoms  = pProtein->getAtomsFromAmino(name);
        int minIndex = atomCount;
        int maxIndex = 0;
        for(Atom* pAtom : *pAtoms)
        {
            int index = pAtom->getIndex() - 1;
            minIndex = minIndex > index ? index : minIndex;
            maxIndex = maxIndex < index ? index : maxIndex;
        }

        mAminoAcids.push_back(AminoAcid(name, minIndex, maxIndex));
    }
}

GPUProtein::GPUProtein(const std::vector<glm::vec4>& rAtoms)
{
    // Create structures for CPU
    int atomCount  = rAtoms.size();
    mspRadii = std::shared_ptr<std::vector<float> >(new std::vector<float>);
    mspRadii->resize(atomCount);
    mspTrajectory = std::shared_ptr<
            std::vector<
                std::vector<glm::vec3> > >(
                    new std::vector<std::vector<glm::vec3> >);
    mspTrajectory->resize(1);
    mspTrajectory->at(0).resize(atomCount);

    // Fill structures for CPU
    for(int i = 0; i < atomCount; i++)
    {
        // Collect radius
        mspRadii->at(i) = rAtoms.at(i).w;

        // Collect trajectory
        mspTrajectory->at(0).at(i) = glm::vec3(rAtoms.at(i).x, rAtoms.at(i).y, rAtoms.at(i).z);
    }

    // TODO: Elements and aminoacids are not filled here

    // Init SSBOs
    initSSBOs(atomCount, 0);
}

GPUProtein::~GPUProtein()
{
    // Nothing to do
}

void GPUProtein::bind(GLuint radiiSlot, GLuint trajectorySlot) const
{
    mRadiiBuffer.bind(radiiSlot);
    mTrajectoryBuffer.bind(trajectorySlot);
}

void GPUProtein::bindRadii(GLuint slot) const
{
    mRadiiBuffer.bind(slot);
}

void GPUProtein::bindTrajectory(GLuint slot) const
{
    mTrajectoryBuffer.bind(slot);
}

void GPUProtein::bindColorsElement(GLuint slot) const
{
    mColorsElementBuffer.bind(slot);
}

void GPUProtein::bindColorsAminoacid(GLuint slot) const
{
    mColorsAminoacidBuffer.bind(slot);
}

std::shared_ptr<const std::vector<float> > GPUProtein::getRadii() const
{
    return mspRadii;
}

std::shared_ptr<const std::vector<std::vector<glm::vec3> > > GPUProtein::getTrajectory() const
{
    return mspTrajectory;
}

void GPUProtein::initSSBOs(int atomCount, int frameCount)
{
    // For copying it to OpenGL, store it linear
    std::vector<glm::vec3> linearTrajectory;
    linearTrajectory.reserve(frameCount * atomCount);
    for(int i = 0; i < frameCount; i++)
    {
        linearTrajectory.insert(linearTrajectory.end(), mspTrajectory->at(i).begin(), mspTrajectory->at(i).end());
    }

    // Create structures of radii and trajectory on GPU
    mRadiiBuffer.fill(*mspRadii.get(), GL_STATIC_DRAW);
    mTrajectoryBuffer.fill(linearTrajectory, GL_STATIC_DRAW);

    // Get atom lookup
    AtomLUT lut;

    // Create structure for coloring according to element on GPU
    std::vector<glm::vec3> elementColors;
    elementColors.reserve(atomCount);
    for(int i = 0; i < atomCount; i++)
    {
        auto color = lut.cpk_colorcode[mElements.at(i)];
        elementColors.push_back(glm::vec3(color.r, color.g, color.b));
    }
    mColorsElementBuffer.fill(elementColors, GL_STATIC_DRAW);

    // Create structure for coloring according to aminoacid on GPU
    std::vector<glm::vec3> aminoacidColors;
    aminoacidColors.reserve(atomCount);
    for(int i = 0; i < atomCount; i++)
    {
        auto color = lut.fetchAminoColor(mAminoacids.at(i));
        aminoacidColors.push_back(glm::vec3(color.r, color.g, color.b));
    }
    mColorsAminoacidBuffer.fill(aminoacidColors, GL_STATIC_DRAW);
}

