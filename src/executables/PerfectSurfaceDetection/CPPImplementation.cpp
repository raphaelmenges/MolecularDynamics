#include "CPPImplementation.h"

#include <iostream>

void CPPImplementation::setup()
{
    cuttingFaceCount = 0;
    cuttingFaceIndicesCount = 0;
}

// ## Distance point to plane
// http://stackoverflow.com/questions/15688232/check-which-side-of-a-plane-points-are-on
bool CPPImplementation::pointInHalfspaceOfPlane(
    float faceDistance,
    glm::vec3 faceNormal,
    glm::vec3 point) const
{
    return 0 < (glm::dot(faceNormal, point - faceDistance) + faceDistance);
}

// ## Intersection line of two planes
// http://stackoverflow.com/questions/6408670/line-of-intersection-between-two-planes
void CPPImplementation::intersectPlanes(
    float faceDistance,
    glm::vec3 faceNormal,
    float otherFaceDistance,
    glm::vec3 otherFaceNormal,
    glm::vec3 &linePoint,
    glm::vec3 &lineDir) const
{
    // Direction of line
    lineDir = cross(faceNormal, otherFaceNormal);

    // Determinant (should no be zero since no parallel planes tested)
    float determinant = glm::length(lineDir);
    determinant = determinant * determinant;

    // Point on line
    linePoint =
        (cross(lineDir, otherFaceNormal) * faceDistance
        + (cross(faceNormal, lineDir) * otherFaceDistance))
        / determinant;

    // Normalize direction of line
    lineDir = glm::normalize(lineDir);
}

// ## Part under square root of intersection line and sphere
// https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection
float CPPImplementation::underSQRT(
    glm::vec3 linePoint,
    glm::vec3 lineDir,
    glm::vec3 sphereCenter,
    float sphereRadius) const
{
    float underSQRT1 = glm::dot(lineDir, (linePoint - sphereCenter));
    underSQRT1 = underSQRT1 * underSQRT1;
    float underSQRT2 = glm::length(linePoint - sphereCenter);
    underSQRT2 = underSQRT2 * underSQRT2;
    return (underSQRT1 - underSQRT2 + (sphereRadius * sphereRadius));
}

// ## Function to test whether endpoint is NOT cut away. Called after cutting face list is minimized
bool CPPImplementation::testEndpoint(glm::vec3 endpoint) const
{
    std::cout << "Testing an endpoint: " << endpoint.x << ", " << endpoint.y << ", " << endpoint.z << std::endl;

    // Some epsilon to prohibit cutting away by faces that created this endpoint
    float epsilon = 0.0001f;

    // Iterate over cuttingFaceIndices entries
    for(int i = 0; i < cuttingFaceIndicesCount; i++)
    {
        // Index of cutting face
        int index = cuttingFaceIndices[i];

        // TODO: USE EPSILON (how?)
        float distance = cuttingFaceDistances[index];
        if(pointInHalfspaceOfPlane(
            distance,
            cuttingFaceNormals[index],
            endpoint) > 0)
        {
            std::cout << "Endpoint killed by cutting face" << std::endl;
            return false;
        }
    }
    std::cout << "Endpoint survived" << std::endl;
    return true;
}

// ## Execution function
void CPPImplementation::execute(
    int executionIndex,
    int atomCount,
    float probeRadius,
    const std::vector<AtomStruct>& atoms,
    std::vector<unsigned int>& surfaceAtomsIndices)
{
    // Reset members for new execution
    setup();

    // Index
    int atomIndex = executionIndex;

    // Check whether in range
    if(atomIndex >= atomCount) { return; }

    std::cout << std::endl;
    std::cout << "### Execution for atom: " << atomIndex << std::endl;

    // When no endpoint was generated at all, atom is surface (value is false then)
    bool endpointGenerated = false;

    // When one endpoint survives cutting, atom is surface (value is true then)
    bool endpointSurvivesCut = false;

    // Own extended radius
    float atomExtRadius = atoms[atomIndex].radius + probeRadius;
    std::cout << "Atom extended radius: " << atomExtRadius << std::endl;

    // Own center
    glm::vec3 atomCenter = atoms[atomIndex].center;
    std::cout << "Atom center: " << atomCenter.x << ", " << atomCenter.y << ", " << atomCenter.z << std::endl;

    // ### BUILD UP OF CUTTING FACE LIST ###

    // Go over other atoms and build cutting face list
    for(int i = 0; i < atomCount; i++)
    {
        // Do not cut with itself
        if(i == atomIndex) { continue; }

        // ### OTHER'S VALUES ###

        // Get values from other atom
        glm::vec3 otherAtomCenter = atoms[i].center;
        float otherAtomExtRadius = atoms[i].radius + probeRadius;

        // ### INTERSECTION TEST ###

        // Vector from center to other's
        glm::vec3 connection = otherAtomCenter - atomCenter;

        // Distance between atoms
        float atomsDistance = glm::length(connection);

        // Do they intersect with extended radii?
        if(atomsDistance >= (atomExtRadius + otherAtomExtRadius)) { continue; }
        std::cout << "Neighbor atom distance: " << atomsDistance << std::endl;

        std::cout << "Working on cutting face: " << cuttingFaceCount << std::endl;

        // NOTE: Following cases are NOT considered
        // - both spheres touch each other in one point
        // - one sphere lies completely inside other
        // - one sphere lies completely inside other and touches other's surface in one point

        // ### INTERSECTION ###

        // Calculate center of intersection
        // http://gamedev.stackexchange.com/questions/75756/sphere-sphere-intersection-and-circle-sphere-intersection
        float h =
            0.5
            + ((atomExtRadius * atomExtRadius)
            - (otherAtomExtRadius * otherAtomExtRadius))
            / (2 * (atomsDistance * atomsDistance));
        std::cout << "h: " << h << std::endl;

        // ### CUTTING FACE LIST ###

        // Use connection between centers as line
        cuttingFaceCenters[cuttingFaceCount] = atomCenter + h * connection;
        std::cout << "Cutting face center: " << cuttingFaceCenters[cuttingFaceCount].x << ", " << cuttingFaceCenters[cuttingFaceCount].y << ", " << cuttingFaceCenters[cuttingFaceCount].z << std::endl;

        // Calculate radius of intersection
        /*
        cuttingFaceRadii[cuttingFaceCount] =
            sqrt((atomExtRadius * atomExtRadius)
            - (h * h * atomsDistance * atomsDistance));
        std::cout << "Cutting face radius: " << cuttingFaceRadii[cuttingFaceCount] << std::endl;
        */

        // Calculate normal of intersection
        cuttingFaceNormals[cuttingFaceCount] = normalize(connection);
        std::cout << "Cutting face normal: " << cuttingFaceNormals[cuttingFaceCount].x << ", " << cuttingFaceNormals[cuttingFaceCount].y << ", " << cuttingFaceNormals[cuttingFaceCount].z << std::endl;

        // Distance of face from origin (TODO: understand minus: https://en.wikipedia.org/wiki/Plane_(geometry) )
        cuttingFaceDistances[cuttingFaceCount] = -glm::dot(cuttingFaceCenters[cuttingFaceCount], cuttingFaceNormals[cuttingFaceCount]);

        // Initialize cutting face indicator with: 1 == was not cut away (yet)
        cuttingFaceIndicators[cuttingFaceCount] = 1;

        // Increment cutting face list index and break if max count of neighbors reached
        if((++cuttingFaceCount) == neighborsMaxCount) { break; }
    }

    // FROM HERE ON: TEST INTERSECTION LINE STUFF (CAN BE DELETED LATER ON)
    for(int i = 0; i < cuttingFaceCount - 1; i++)
    {
        // Already cut away (think about where this is save to test)
        // if(cuttingFaceIndicators[i] == 0) { continue; }

        // Values of cutting face
        glm::vec3 faceCenter = cuttingFaceCenters[i];
        float faceDistance = cuttingFaceDistances[i];
        glm::vec3 faceNormal = cuttingFaceNormals[i];

        // Test every cutting face for intersection line with other
        for(int j = i+1; j < cuttingFaceCount; j++)
        {
            // Already cut away (think about where this is save to test)
            // if(cuttingFaceIndicators[j] == 0) { continue; }

            // Values of other cutting face
            glm::vec3 otherFaceCenter = cuttingFaceCenters[j];
            float otherFaceDistance = cuttingFaceDistances[j];
            glm::vec3 otherFaceNormal = cuttingFaceNormals[j];

            // Check for parallelity, first
            bool notCutEachOther = (1.0 == glm::abs(dot(faceNormal, otherFaceNormal))); // If already parallel, they do not cut

            // Do further checking if not already parallel
            if(!notCutEachOther)
            {
                // Intersection of planes, resulting in line
                glm::vec3 lineDir; glm::vec3 linePoint;
                intersectPlanes(
                    faceDistance,
                    faceNormal,
                    otherFaceDistance,
                    otherFaceNormal,
                    linePoint,
                    lineDir);

                std::cout << "Cutting faces " << i << " and " << j << " were intersected" << std::endl;
                std::cout << "Line point: " << linePoint.x << ", " << linePoint.y << ", " << linePoint.z << std::endl;
                std::cout << "Line direction: " << lineDir.x << ", " << lineDir.y << ", " << lineDir.z << std::endl;

                // Intersection of line with sphere, resulting in two, one or no endpoints
                // https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection
                float valueUnderSQRT = underSQRT(linePoint, lineDir, atomCenter, atomExtRadius);
                std::cout << "Value under SQRT: " << valueUnderSQRT << std::endl;

                // Only interesting case is for zero endpoints, because then there is no cut on atom surface
                notCutEachOther = valueUnderSQRT < 0;
            }

            // ### CHECK WHETHER CUTTING FACE CAN BE FORGOT ###

            // Faces do not cut each other, so they produce not later endpoints. Check them now
            if(notCutEachOther)
            {
                // Connection between faces' center
                glm::vec3 connection = otherFaceCenter - faceCenter;

                // TODO

                // - Face includes other
                // - Faces cut complete atom away
            }
        }
    }

    // ### GO OVER CUTTING FACES AND COLLECT NOT CUT AWAY ONES ###

    for(int i = 0; i < cuttingFaceCount; i++)
    {
        // Check whether cutting face is still there after preprocessing
        if(cuttingFaceIndicators[i] == 1)
        {
            // Save index of that cutting face
            cuttingFaceIndices[cuttingFaceIndicesCount] = i;

            // Increase count of those cutting faces
            cuttingFaceIndicesCount++;
        }
    }

    std::cout << "Cutting face count: " << cuttingFaceCount << ". After minimization: " << cuttingFaceIndicesCount << std::endl;

    // ### GO OVER MINIMIZED CUTTING FACE LIST AND TEST ENDPOINTS ###

    for(int i = 0; i < cuttingFaceIndicesCount; i++)
    {
        // Values of cutting face
        int index = cuttingFaceIndices[i];
        glm::vec3 faceCenter = cuttingFaceCenters[index];
        float faceDistance = cuttingFaceDistances[index];
        glm::vec3 faceNormal = cuttingFaceNormals[index];

        // Test every cutting face for intersection line with other
        for(int j = i+1; j < cuttingFaceIndicesCount; j++)
        {
            // Values of other cutting face
            int otherIndex = cuttingFaceIndices[j];
            glm::vec3 otherFaceCenter = cuttingFaceCenters[otherIndex];
            float otherFaceDistance = cuttingFaceDistances[otherIndex];
            glm::vec3 otherFaceNormal = cuttingFaceNormals[otherIndex];

            // Intersection of planes, resulting in line
            glm::vec3 lineDir; glm::vec3 linePoint;
            intersectPlanes(
                faceDistance,
                faceNormal,
                otherFaceDistance,
                otherFaceNormal,
                linePoint,
                lineDir);

            // Intersection of line with sphere, resulting in two, one or no endpoints
            // https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection
            float valueUnderSQRT = underSQRT(linePoint, lineDir, atomCenter, atomExtRadius);

            // Left part of equation
            float left = -(glm::dot(lineDir, (linePoint - atomCenter)));

            // Check value under square root
            if(valueUnderSQRT > 0)
            {
                // Some endpoint was at least generated
                endpointGenerated = true;

                // Right part of equation
                float right = sqrt(valueUnderSQRT);

                // First endpoint
                float d = left + right;
                if(testEndpoint(linePoint + (d * lineDir)))
                {
                    // Break out of for loop (and outer)
                    endpointSurvivesCut = true;
                    break;
                }

                // Second endpoint
                d = left - right;
                if(testEndpoint(linePoint + (d * lineDir)))
                {
                    // Break out of for loop (and outer)
                    endpointSurvivesCut = true;
                    break;
                }
            }
            else if(valueUnderSQRT == 0)
            {
                // Some endpoint was at least generated
                endpointGenerated = true;

                // Just test the one endpoint
                float d = left;
                if(testEndpoint(linePoint + (d * lineDir)))
                {
                    // Break out of for loop (and outer)
                    endpointSurvivesCut = true;
                    break;
                }
            }
            // else, no endpoint is generated since cutting faces do not intersect (should not happen)
        }

        if(endpointSurvivesCut) { break; }
    }

    // ### ATOM IS SURFACE ATOM ###

    // If no endpoint was generated at all or one or more survived cutting, add this atom to surface
    if(!endpointGenerated || endpointSurvivesCut)
    {
        surfaceAtomsIndices.push_back((unsigned int) atomIndex);
    }
}
