#ifndef CPP_IMPLEMENTATION_H
#define CPP_IMPLEMENTATION_H

#include <glm/glm.hpp>
#include <vector>
#include "AtomStruct.h"

class CPPImplementation
{
public:

    void execute(
        int executionIndex,
        int atomCount,
        float probeRadius,
        const std::vector<AtomStruct>& atoms,
        std::vector<unsigned int>& surfaceAtomsIndices);

private:

    void setup();

    void intersectPlanes(
        glm::vec3 faceCenter,
        glm::vec3 faceNormal,
        glm::vec3 otherFaceCenter,
        glm::vec3 otherFaceNormal,
        glm::vec3 &linePoint,
        glm::vec3 &lineDir) const;

    float underSQRT(
        glm::vec3 linePoint,
        glm::vec3 lineDir,
        glm::vec3 atomCenter,
        float atomRadius) const;

    bool testEndpoint(glm::vec3 endpoint) const;

    // Members
    static const int neighborsMaxCount = 10;

    // All cutting faces, also those who gets cut away by others
    int cuttingFaceCount = 0;
    glm::vec3 cuttingFaceCenters[neighborsMaxCount];
    float cuttingFaceRadii[neighborsMaxCount];
    glm::vec3 cuttingFaceNormals[neighborsMaxCount];

    // Selection of cutting faces which get intersected pairwaise and produce endpoints
    int cuttingFaceIndicators[neighborsMaxCount]; // Indicator whether cutting face was cut away by other (1 == not cut away)
    int cuttingFaceIndicesCount = 0; // Count of not cut away cutting faces
    int cuttingFaceIndices[neighborsMaxCount]; // Indices of cutting faces which are not cut away by other
};

#endif // CPP_IMPLEMENTATION_H
