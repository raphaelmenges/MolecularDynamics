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
    for(int i = 0; i < frameCount; i++) // go over frames
    {
        glm::vec3 accPosition(0, 0, 0);
        for(int j = 0; j < atomCount; j++) // go over atoms
        {
            // Position of that atom in that frame
            glm::vec3 position = pProtein->getAtoms()->at(j)->getPositionAtFrame(i);

            // Collect trajectory (already correctly sized)
            mspTrajectory->at(i).at(j) = position;

            // Accumulate position
            accPosition += position;
        }

        // Save center
        mCentersOfMass.push_back(accPosition / atomCount);
    }

    // Init SSBOs
    initSSBOs(atomCount, frameCount);
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

