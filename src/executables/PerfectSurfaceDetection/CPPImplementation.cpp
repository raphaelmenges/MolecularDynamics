#include "CPPImplementation.h"

#include <iostream>

void CPPImplementation::setup()
{
    cuttingFaceCount = 0;
    cuttingFaceIndicesCount = 0;
}

// ## Determines whether point lies in halfspace of plane's normal direction
// http://stackoverflow.com/questions/15688232/check-which-side-of-a-plane-points-are-on
bool CPPImplementation::pointInHalfspaceOfPlane(
    glm::vec4 plane,
    glm::vec3 point) const
{
    // Use negative distance of plane to subtract it from distance between point and plane
    return 0 < glm::dot(plane, glm::vec4(point, -1));
}

// ## Intersection line of two planes (Planes should not be parallel, which is impossible due to cutting face tests)
// http://stackoverflow.com/questions/6408670/line-of-intersection-between-two-planes
void CPPImplementation::intersectPlanes(
    glm::vec4 plane,
    glm::vec4 otherPlane,
    glm::vec3 &linePoint,
    glm::vec3 &lineDir) const
{
    // Direction of line
    lineDir = glm::cross(glm::vec3(plane.x, plane.y, plane.z), glm::vec3(otherPlane.x, otherPlane.y, otherPlane.z));

    // Determinant (should not be zero since no parallel planes tested)
    float determinant = glm::length(lineDir);
    determinant = determinant * determinant;

    // Point on line
    linePoint =
        (cross(lineDir, glm::vec3(otherPlane.x, otherPlane.y, otherPlane.z)) * (-plane.w)
        + (cross(glm::vec3(plane.x, plane.y, plane.z), lineDir) * (-otherPlane.w)))
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

// ## Function to test whether endpoint is NOT cut away. Called after cutting face list is optimized
bool CPPImplementation::testEndpoint(glm::vec3 endpoint, int excludeA, int excludeB) const
{
    if(logging) { std::cout << "Testing an endpoint: " << endpoint.x << ", " << endpoint.y << ", " << endpoint.z << std::endl; }

    // Iterate over cuttingFaceIndices entries
    for(int i = 0; i < cuttingFaceIndicesCount; i++)
    {
        // Index of cutting face
        int index = cuttingFaceIndices[i];

        // Do not test against faces which created endpoint
        if(index == excludeA || index == excludeB) { continue; }

        // Test whether endpoint is in positive halfspace of cut away part
        if(pointInHalfspaceOfPlane(
            cuttingFaces[index],
            endpoint))
        {
            if(logging) { std::cout << "Endpoint killed by cutting face" << std::endl; }
            return false;
        }
    }
    if(logging) { std::cout << "Endpoint survived" << std::endl; }
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

    if(logging) { std::cout << std::endl; }
    if(logging) { std::cout << "### Execution for atom: " << atomIndex << std::endl; }

    // When no endpoint was generated at all, atom is surface (value is false then)
    bool endpointGenerated = false;

    // When one endpoint survives cutting, atom is surface (value is true then)
    bool endpointSurvivesCut = false;

    // Own center
    glm::vec3 atomCenter = atoms[atomIndex].center;
    if(logging) { std::cout << "Atom center: " << atomCenter.x << ", " << atomCenter.y << ", " << atomCenter.z << std::endl; }

    // Own extended radius
    float atomExtRadius = atoms[atomIndex].radius + probeRadius;
    if(logging) { std::cout << "Atom extended radius: " << atomExtRadius << std::endl; }

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

        /*
        // Do they intersect with extended radii?
        if(atomsDistance >= (atomExtRadius + otherAtomExtRadius)) { continue; }
        if(logging) { std::cout << "Neighbor atom distance: " << atomsDistance << std::endl; }

        if(logging) { std::cout << "Working on cutting face: " << cuttingFaceCount << std::endl; }

        // NOTE: Following cases are NOT considered
        // - both spheres touch each other in one point (excluded by neighboring test)
        // - one sphere lies completely inside other
        // - one sphere lies completely inside other and touches other's surface in one point
        */

        // Test atoms are either too far away or just touch each other (then continue)
        if(atomsDistance >= (atomExtRadius + otherAtomExtRadius)) { continue; }

        // Test atoms are either too far away (then continue)
        // if(atomsDistance > (atomExtRadius + otherAtomExtRadius)) { continue; }

        // Test whether atom is completely covering other
        if(atomExtRadius >= (otherAtomExtRadius + atomsDistance)) { continue; }

        // Test whether atom is completely covered by other
        if((atomExtRadius + atomsDistance) <= otherAtomExtRadius)
        {
            // Since it is completely covered, it is not at surface
            return;
        }

        // ### INTERSECTION WITH OTHER ATOMS ###

        // Calculate center of intersection
        // http://gamedev.stackexchange.com/questions/75756/sphere-sphere-intersection-and-circle-sphere-intersection
        float h =
            0.5
            + ((atomExtRadius * atomExtRadius)
            - (otherAtomExtRadius * otherAtomExtRadius))
            / (2.0 * (atomsDistance * atomsDistance));
        if(logging) { std::cout << "h: " << h << std::endl; }

        // ### CUTTING FACE LIST ###

        // Use connection between centers as line
        //cuttingFaceCenters[cuttingFaceCount] = atomCenter + h * connection;
        //if(logging) { std::cout << "Cutting face center: " << cuttingFaceCenters[cuttingFaceCount].x << ", " << cuttingFaceCenters[cuttingFaceCount].y << ", " << cuttingFaceCenters[cuttingFaceCount].z << std::endl; }

        // Calculate radius of intersection
        //
        //cuttingFaceRadii[cuttingFaceCount] =
        //    sqrt((atomExtRadius * atomExtRadius)
        //    - (h * h * atomsDistance * atomsDistance));
        //if(logging) { std::cout << "Cutting face radius: " << cuttingFaceRadii[cuttingFaceCount] << std::endl; }

        // Calculate normal of intersection
        //cuttingFaceNormals[cuttingFaceCount] = normalize(connection);
        //if(logging) { std::cout << "Cutting face normal: " << cuttingFaceNormals[cuttingFaceCount].x << ", " << cuttingFaceNormals[cuttingFaceCount].y << ", " << cuttingFaceNormals[cuttingFaceCount].z << std::endl; }

        // Distance of face from origin
        //cuttingFaceDistances[cuttingFaceCount] = glm::dot(cuttingFaceCenters[cuttingFaceCount], cuttingFaceNormals[cuttingFaceCount]);

        // Save center of face
        glm::vec3 faceCenter = atomCenter + (h * connection);
        cuttingFaceCenters[cuttingFaceCount] = faceCenter;
        if(logging) { std::cout << "Cutting face center: " << faceCenter.x << ", " << faceCenter.y << ", " << faceCenter.z << std::endl; }

        // Save plane equation of face
        glm::vec3 faceNormal = glm::normalize(connection);
        float faceDistance = glm::dot(faceCenter, faceNormal);
        cuttingFaces[cuttingFaceCount] = glm::vec4(faceNormal, faceDistance);
        if(logging) { std::cout << "Cutting face distance: " << faceDistance << std::endl; }

        // Initialize cutting face indicator with: 1 == was not cut away (yet)
        cuttingFaceIndicators[cuttingFaceCount] = 1;

        // Increment cutting face list index and break if max count of neighbors reached
        cuttingFaceCount++;
        if(cuttingFaceCount == neighborsMaxCount) { std::cout << "TOO MANY NEIGHBORS" << std::endl; break; }
    }

    // CALCULATE WHICH CUTTING FACES ARE USED FOR ENDPOINT CALCULATION
    for(int i = 0; i < cuttingFaceCount - 1; i++)
    {
        // Already cut away
        if(cuttingFaceIndicators[i] == 0) { continue; }

        // Values of cutting face
        glm::vec4 face = cuttingFaces[i];
        glm::vec3 faceCenter = cuttingFaceCenters[i];

        // Test every cutting face for intersection line with other
        for(int j = i+1; j < cuttingFaceCount; j++)
        {
            if(logging) { std::cout << "Testing cutting faces: " << i << ", " << j << std::endl; }

            // Already cut away
            if((cuttingFaceIndicators[i] == 0) || (cuttingFaceIndicators[j] == 0)) { continue; }

            // Values of other cutting face
            glm::vec4 otherFace = cuttingFaces[j];
            glm::vec3 otherFaceCenter = cuttingFaceCenters[j];

            // Check for parallelity, first
            bool notCutEachOther = (1.0 <= glm::abs(glm::dot(glm::vec3(face.x, face.y, face.z), glm::vec3(otherFace.x, otherFace.y, otherFace.z)))); // If already parallel, they do not cut

            // Do further checking when not parallel
            if(!notCutEachOther)
            {
                // Intersection of planes, resulting in line
                glm::vec3 linePoint; glm::vec3 lineDir;
                intersectPlanes(
                    face,
                    otherFace,
                    linePoint,
                    lineDir);

                if(logging) { std::cout << "Cutting faces " << i << " and " << j << " do intersect" << std::endl; }
                if(logging) { std::cout << "Line point: " << linePoint.x << ", " << linePoint.y << ", " << linePoint.z << std::endl; }
                if(logging) { std::cout << "Line direction: " << lineDir.x << ", " << lineDir.y << ", " << lineDir.z << std::endl; }

                // Intersection of line with sphere, resulting in two, one or no endpoints
                // https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection
                float valueUnderSQRT = underSQRT(linePoint, lineDir, atomCenter, atomExtRadius);
                if(logging) { std::cout << "Value under SQRT: " << valueUnderSQRT << std::endl; }

                // Only interesting case is for zero endpoints, because then there is no cut on atom's sphere
                notCutEachOther = (valueUnderSQRT < 0);
            }

            // ### CHECK WHETHER CUTTING FACE CAN BE FORGOT ###

            // Faces do not cut each other on sphere, so they produce not later endpoints. Check them now
            if(notCutEachOther)
            {
                if(logging) { std::cout << "Following cutting faces do not cut each other on surface: " << i << ", " << j << std::endl; }

                // Connection between faces' center (vector from face to other face)
                glm::vec3 connection = otherFaceCenter - faceCenter;

                // Test point
                glm::vec3 testPoint = faceCenter + 0.5f * connection;

                if(glm::dot(glm::vec3(face.x, face.y, face.z), connection) > 0 == glm::dot(glm::vec3(otherFace.x, otherFace.y, otherFace.z), connection) > 0)
                {
                    // Inclusion
                    if(pointInHalfspaceOfPlane(face, testPoint))
                    {
                        cuttingFaceIndicators[j] = 0;
                    }
                    else
                    {
                        cuttingFaceIndicators[i] = 0;
                    }
                }
                else
                {
                    // Maybe complete atom is cut away
                    if(pointInHalfspaceOfPlane(face, testPoint))
                    {
                        return;
                    }
                }
            }

            if(logging) { std::cout << std::endl; }
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

    if(logging) { std::cout << "Cutting face count: " << cuttingFaceCount << ". After optimization: " << cuttingFaceIndicesCount << std::endl; }

    // ### GO OVER OPTIMIZED CUTTING FACE LIST AND TEST ENDPOINTS ###

    for(int i = 0; i < cuttingFaceIndicesCount - 1; i++)
    {
        // Values of cutting face
        int index = cuttingFaceIndices[i];
        glm::vec4 face = cuttingFaces[index];

        // Test every cutting face for intersection line with other
        for(int j = i+1; j < cuttingFaceIndicesCount; j++)
        {
            // Values of other cutting face
            int otherIndex = cuttingFaceIndices[j];
            glm::vec4 otherFace = cuttingFaces[otherIndex];

            // Intersection of faces, resulting in line
            glm::vec3 lineDir; glm::vec3 linePoint;
            intersectPlanes(
                face,
                otherFace,
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
                // Some endpoint was generated, at least
                endpointGenerated = true;

                // Right part of equation
                float right = glm::sqrt(valueUnderSQRT);

                // First endpoint
                float d = left + right;
                if(testEndpoint(linePoint + (d * lineDir), index, otherIndex))
                {
                    // Break out of for loop (and outer)
                    endpointSurvivesCut = true;
                    break;
                }

                // Second endpoint
                d = left - right;
                if(testEndpoint(linePoint + (d * lineDir), index, otherIndex))
                {
                    // Break out of for loop (and outer)
                    endpointSurvivesCut = true;
                    break;
                }
            }
            else if(valueUnderSQRT == 0)
            {
                // Some endpoint was generated, at least generated
                endpointGenerated = true;

                // Just test the one endpoint
                float d = left;
                if(testEndpoint(linePoint + (d * lineDir), index, otherIndex))
                {
                    // Break out of for loop (and outer)
                    endpointSurvivesCut = true;
                    break;
                }
            }
            // else, no endpoint is generated since cutting faces do not intersect
        }

        if(endpointSurvivesCut) { break; }
    }

    // ### ATOM IS SURFACE ATOM ###

    // If no endpoint was generated at all or one or more survived cutting, add this atom to surface
    if((!endpointGenerated) || endpointSurvivesCut)
    {
        surfaceAtomsIndices.push_back((unsigned int) atomIndex);
    }
}
