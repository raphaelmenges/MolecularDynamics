#include "GPUSurfaceExtraction.h"
#include "Utils/AtomicCounter.h"
#include <GLFW/glfw3.h>
#include <thread>
#include <functional>

GPUSurfaceExtraction::GPUSurfaceExtraction()
{
    // Create shader program which is used
    mupComputeProgram = std::unique_ptr<ShaderProgram>(new ShaderProgram(GL_COMPUTE_SHADER, "/SurfaceExtraction/surface.comp"));

    // Create query object for time measurement
    glGenQueries(1, &mQuery);
}

GPUSurfaceExtraction::~GPUSurfaceExtraction()
{
    // Delete query object
    glDeleteQueries(1, &mQuery);
}

void GPUSurfaceExtraction::CPUSurfaceExtraction::setup()
{
    mCuttingFaceCount = 0;
    mCuttingFaceIndicesCount = 0;
}

// ## Check for parallelism
bool GPUSurfaceExtraction::CPUSurfaceExtraction::checkParallelism(
    glm::vec4 plane,
    glm::vec4 otherPlane) const
{
    return (1.0 <= glm::abs(glm::dot(glm::vec3(plane.x, plane.y, plane.z), glm::vec3(otherPlane.x, otherPlane.y, otherPlane.z))));
}

// ## Determines whether point lies in halfspace of plane's normal direction
// http://stackoverflow.com/questions/15688232/check-which-side-of-a-plane-points-are-on
bool GPUSurfaceExtraction::CPUSurfaceExtraction::pointInHalfspaceOfPlane(
    glm::vec4 plane,
    glm::vec3 point) const
{
    // Use negative distance of plane to subtract it from distance between point and plane
    return 0 < glm::dot(plane, glm::vec4(point, -1));
}

// ## Intersection line of two planes (Planes should not be parallel, which is impossible due to cutting face tests)
// http://stackoverflow.com/questions/6408670/line-of-intersection-between-two-planes
void GPUSurfaceExtraction::CPUSurfaceExtraction::intersectPlanes(
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
float GPUSurfaceExtraction::CPUSurfaceExtraction::underSQRT(
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
bool GPUSurfaceExtraction::CPUSurfaceExtraction::testEndpoint(glm::vec3 endpoint, int excludeA, int excludeB) const
{
    /* if(mLogging) { std::cout << "Testing an endpoint: " << endpoint.x << ", " << endpoint.y << ", " << endpoint.z << std::endl; } */

    // Iterate over mCuttingFaceIndices entries
    for(int i = 0; i < mCuttingFaceIndicesCount; i++)
    {
        // Index of cutting face
        int index = mCuttingFaceIndices[i];

        // Do not test against faces which created endpoint
        if(index == excludeA || index == excludeB) { continue; }

        // Test whether endpoint is in positive halfspace of cut away part
        if(pointInHalfspaceOfPlane(
            mCuttingFaces[index],
            endpoint))
        {
            /* if(mLogging) { std::cout << "Endpoint killed by cutting face" << std::endl; } */
            return false;
        }
    }
    /* if(mLogging) { std::cout << "Endpoint survived" << std::endl; } */
    return true;
}

// ## Execution function
void GPUSurfaceExtraction::CPUSurfaceExtraction::execute(
    int executionIndex,
    int atomCount,
    float probeRadius,
    const std::vector<GPUAtom>& atoms,
    std::vector<unsigned int>& internalIndices,
    std::vector<unsigned int>& surfaceIndices)
{
    // Reset members for new execution
    setup();

    // Index
    int atomIndex = executionIndex;

    // Check whether in range
    /* if(atomIndex >= atomCount) { return; } */

    /* if(mLogging) { std::cout << std::endl; } */
    /* if(mLogging) { std::cout << "### Execution for atom: " << atomIndex << std::endl; } */

    // When no endpoint was generated at all, atom is surface (value is false then)
    bool endpointGenerated = false;

    // When one endpoint survives cutting, atom is surface (value is true then)
    bool endpointSurvivesCut = false;

    // Own center
    glm::vec3 atomCenter = atoms[atomIndex].center;
    /* if(mLogging) { std::cout << "Atom center: " << atomCenter.x << ", " << atomCenter.y << ", " << atomCenter.z << std::endl; } */

    // Own extended radius
    float atomExtRadius = atoms[atomIndex].radius + probeRadius;
    /* if(mLogging) { std::cout << "Atom extended radius: " << atomExtRadius << std::endl; } */

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

        // Test atoms are either too far away or just touch each other (then continue)
        if(atomsDistance >= (atomExtRadius + otherAtomExtRadius)) { continue; }

        // Test atoms are either too far away (then continue)
        // if(atomsDistance > (atomExtRadius + otherAtomExtRadius)) { continue; }

        // Test whether atom is completely covering other
        if(atomExtRadius >= (otherAtomExtRadius + atomsDistance)) { continue; }

        // Test whether atom is completely covered by other
        if((atomExtRadius + atomsDistance) <= otherAtomExtRadius)
        {
            // Since it is completely covered, it is internal
            internalIndices.push_back((unsigned int) atomIndex); return;
        }

        // ### INTERSECTION WITH OTHER ATOMS ###

        // Calculate center of intersection
        // http://gamedev.stackexchange.com/questions/75756/sphere-sphere-intersection-and-circle-sphere-intersection
        float h =
            0.5
            + ((atomExtRadius * atomExtRadius)
            - (otherAtomExtRadius * otherAtomExtRadius))
            / (2.0 * (atomsDistance * atomsDistance));
        /* if(mLogging) { std::cout << "h: " << h << std::endl; } */

        // ### CUTTING FACE LIST ###

        // Calculate radius of intersection
        //
        //cuttingFaceRadii[mCuttingFaceCount] =
        //    sqrt((atomExtRadius * atomExtRadius)
        //    - (h * h * atomsDistance * atomsDistance));
        ///* if(mLogging) { std::cout << "Cutting face radius: " << cuttingFaceRadii[mCuttingFaceCount] << std::endl; } */

        // Save center of face
        glm::vec3 faceCenter = atomCenter + (h * connection);
        mCuttingFaceCenters[mCuttingFaceCount] = faceCenter;
        /* if(mLogging) { std::cout << "Cutting face center: " << faceCenter.x << ", " << faceCenter.y << ", " << faceCenter.z << std::endl; } */

        // Save plane equation of face
        glm::vec3 faceNormal = glm::normalize(connection);
        float faceDistance = glm::dot(faceCenter, faceNormal);
        mCuttingFaces[mCuttingFaceCount] = glm::vec4(faceNormal, faceDistance);
        /* if(mLogging) { std::cout << "Cutting face distance: " << faceDistance << std::endl; } */

        // Initialize cutting face indicator with: 1 == was not cut away (yet)
        mCuttingFaceIndicators[mCuttingFaceCount] = 1;

        // Increment cutting face list index and break if max count of neighbors reached
        mCuttingFaceCount++;
        if(mCuttingFaceCount == mNeighborsMaxCount) { std::cout << "TOO MANY NEIGHBORS" << std::endl; break; } // TODO: Pipe cout to GUI
    }

    // CALCULATE WHICH CUTTING FACES ARE USED FOR ENDPOINT CALCULATION
    for(int i = 0; i < mCuttingFaceCount - 1; i++)
    {
        // Already cut away
        if(mCuttingFaceIndicators[i] == 0) { continue; }

        // Values of cutting face
        glm::vec4 face = mCuttingFaces[i];
        glm::vec3 faceCenter = mCuttingFaceCenters[i];

        // Test every cutting face for intersection line with other
        for(int j = i+1; j < mCuttingFaceCount; j++)
        {
            /* if(mLogging) { std::cout << "Testing cutting faces: " << i << ", " << j << std::endl; } */

            // Already cut away
            if(mCuttingFaceIndicators[j] == 0) { continue; }

            // Values of other cutting face
            glm::vec4 otherFace = mCuttingFaces[j];
            glm::vec3 otherFaceCenter = mCuttingFaceCenters[j];

            // Check for parallelism, first
            bool notCutEachOther = checkParallelism(face, otherFace); // If already parallel, they do not cut

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

                /* if(mLogging) { std::cout << "Cutting faces " << i << " and " << j << " do intersect" << std::endl; } */
                /* if(mLogging) { std::cout << "Line point: " << linePoint.x << ", " << linePoint.y << ", " << linePoint.z << std::endl; } */
                /* if(mLogging) { std::cout << "Line direction: " << lineDir.x << ", " << lineDir.y << ", " << lineDir.z << std::endl; } */

                // Intersection of line with sphere, resulting in two, one or no endpoints
                // https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection
                float valueUnderSQRT = underSQRT(linePoint, lineDir, atomCenter, atomExtRadius);
                /* if(mLogging) { std::cout << "Value under SQRT: " << valueUnderSQRT << std::endl; } */

                // Only interesting case is for zero endpoints, because then there is no cut on atom's sphere
                notCutEachOther = (valueUnderSQRT < 0);
            }

            // ### CHECK WHETHER CUTTING FACE CAN BE FORGOT ###

            // Faces do not cut each other on sphere, so they produce not later endpoints. Check them now
            if(notCutEachOther)
            {
                /* if(mLogging) { std::cout << "Following cutting faces do not cut each other on surface: " << i << ", " << j << std::endl; } */

                // Connection between faces' center (vector from face to other face)
                glm::vec3 connection = otherFaceCenter - faceCenter;

                // Test point
                glm::vec3 testPoint = faceCenter + 0.5f * connection;

                if((glm::dot(glm::vec3(face.x, face.y, face.z), connection) > 0) == (glm::dot(glm::vec3(otherFace.x, otherFace.y, otherFace.z), connection) > 0))
                {
                    // Inclusion
                    if(pointInHalfspaceOfPlane(face, testPoint))
                    {
                        mCuttingFaceIndicators[j] = 0;
                    }
                    else
                    {
                        mCuttingFaceIndicators[i] = 0;
                    }
                }
                else
                {
                    // Maybe complete atom is cut away
                    if(pointInHalfspaceOfPlane(face, testPoint))
                    {
                        internalIndices.push_back((unsigned int) atomIndex); return;
                    }
                }
            }

            /* if(mLogging) { std::cout << std::endl; } */
        }
    }

    // ### GO OVER CUTTING FACES AND COLLECT NOT CUT AWAY ONES ###

    for(int i = 0; i < mCuttingFaceCount; i++)
    {
        // Check whether cutting face is still there after preprocessing
        if(mCuttingFaceIndicators[i] == 1)
        {
            // Save index of that cutting face
            mCuttingFaceIndices[mCuttingFaceIndicesCount] = i;

            // Increase count of those cutting faces
            mCuttingFaceIndicesCount++;
        }
    }

    /* if(mLogging) { std::cout << "Cutting face count: " << mCuttingFaceCount << ". After optimization: " << mCuttingFaceIndicesCount << std::endl; } */

    // ### GO OVER OPTIMIZED CUTTING FACE LIST AND TEST ENDPOINTS ###

    for(int i = 0; i < mCuttingFaceIndicesCount - 1; i++)
    {
        // Values of cutting face
        int index = mCuttingFaceIndices[i];
        glm::vec4 face = mCuttingFaces[index];

        // Test every cutting face for intersection line with other
        for(int j = i+1; j < mCuttingFaceIndicesCount; j++)
        {
            // Values of other cutting face
            int otherIndex = mCuttingFaceIndices[j];
            glm::vec4 otherFace = mCuttingFaces[otherIndex];

            // Check for parallelism
            if(checkParallelism(face, otherFace)) { continue; }

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
        surfaceIndices.push_back((unsigned int) atomIndex);
    }
    else
    {
        internalIndices.push_back((unsigned int) atomIndex);
    }
}

std::unique_ptr<GPUSurface> GPUSurfaceExtraction::calculateSurface(
    GPUProtein const * pGPUProtein,
    float probeRadius,
    bool extractLayers,
    bool useCPU,
    int CPUThreadCount) const
{
    // Prepare atomic counters for writing results to unique positions in images
    AtomicCounter internalCounter;
    AtomicCounter surfaceCounter;

    // Input count
    int inputCount = pGPUProtein->getAtomCount(); // at first run, all are input

    // Create GPUSurface
    std::unique_ptr<GPUSurface> upGPUSurface = std::unique_ptr<GPUSurface>(new GPUSurface(inputCount));

    // Miliseconds for computation
    float computationTime = 0;

    // Decide which device to use for computation
    if(useCPU)
    {
        // Copy atoms of protein to stack
        auto atoms = pGPUProtein->getAtoms();

        // Create vector for indices
        std::vector<unsigned int> internalIndices;
        std::vector<unsigned int> surfaceIndices;

        // Start measuring time
        double time = glfwGetTime();

        // Do it in threads
        std::vector<std::vector<unsigned int> > internalIndicesSubvectors;
        std::vector<std::vector<unsigned int> > surfaceIndicesSubvectors;
        internalIndicesSubvectors.resize(CPUThreadCount);
        surfaceIndicesSubvectors.resize(CPUThreadCount);
        std::vector<std::thread> threads;

        // Launch threads
        for(int i = 0; i < CPUThreadCount; i++)
        {
            // Calculate min and max index
            int count = atoms.size() / CPUThreadCount;
            int offset = count * i;
            threads.push_back(
                std::thread([&](
                    int minIndex,
                    int maxIndex,
                    std::vector<unsigned int>& internalIndicesSubvector,
                    std::vector<unsigned int>& surfaceIndicesSubvector)
                {
                    CPUSurfaceExtraction threadCPUSurfaceExtraction;
                    for(int a = minIndex; a <= maxIndex; a++)
                    {
                        threadCPUSurfaceExtraction.execute(
                            a,
                            atoms.size(),
                            probeRadius,
                            atoms,
                            internalIndicesSubvector,
                            surfaceIndicesSubvector);
                    }
                },
                offset, // minIndex
                i == CPUThreadCount - 1 ? atoms.size() - 1 : offset+count-1, // maxIndex
                std::ref(internalIndicesSubvectors[i]), // internal indices
                std::ref(surfaceIndicesSubvectors[i]))); // external indices
        }

        // Join threads
        for(int i = 0; i < CPUThreadCount; i++)
        {
            // Join thread i
            threads[i].join();

            // Collect results from it
            internalIndices.insert(
                internalIndices.end(),
                internalIndicesSubvectors[i].begin(),
                internalIndicesSubvectors[i].end());
            surfaceIndices.insert(
                surfaceIndices.end(),
                surfaceIndicesSubvectors[i].begin(),
                surfaceIndicesSubvectors[i].end());
        }

        // Save computation time
        computationTime = (float) (1000.0 * (glfwGetTime() - time));

        // Fill structures in GPUSurface (TODO: what about input indices? No extra fill necessary?)
        int layer = (upGPUSurface->addLayer((int)atoms.size()) - 1);
        upGPUSurface->fillInternalBuffer(layer, internalIndices);
        upGPUSurface->fillSurfaceBuffer(layer, surfaceIndices);
        upGPUSurface->mInternalCounts.at(layer) = (int)internalIndices.size();
        upGPUSurface->mSurfaceCounts.at(layer) = (int)surfaceIndices.size();
    }
    else
    {
        // Use compute shader program
        mupComputeProgram->use();

        // Probe radius
        mupComputeProgram->update("probeRadius", probeRadius);

        // Bind SSBO with atoms
        pGPUProtein->bind(0);

        // Bind atomic counter
        internalCounter.bind(1);
        surfaceCounter.bind(2);

        // Start query for time measurement
        glBeginQuery(GL_TIME_ELAPSED, mQuery);

        // Do it as often as indicated
        bool firstRun = true;
        while(firstRun || (extractLayers && (inputCount > 0)))
        {
            // Remember the first run
            firstRun = false;

            // Reset atomic counter
            internalCounter.reset();
            surfaceCounter.reset();

            // Tell shader program about count of input atoms
            mupComputeProgram->update("inputCount", inputCount);

            // Add new layer to GPUSurface with buffers which could take all indices
            // It is more reserved than later used, therefore count of internal and surface must be saved extra
            int layer = upGPUSurface->addLayer(inputCount) - 1;

            // Bind that layer
            upGPUSurface->bindForComputation(layer, 3, 4, 5);

            // Dispatch
            glDispatchCompute((inputCount / 64) + 1, 1, 1);
            glMemoryBarrier(GL_ALL_BARRIER_BITS);

            // Save count of internal as next count of input atoms
            inputCount = (int)internalCounter.read();

            // Tell added layer about counts calculated on graphics card
            upGPUSurface->mInternalCounts.at(layer) = inputCount;
            upGPUSurface->mSurfaceCounts.at(layer) = (int)surfaceCounter.read();
        }

        // Print time for execution
        glEndQuery(GL_TIME_ELAPSED);
        GLuint done = 0;
        while(done == 0)
        {
            glGetQueryObjectuiv(mQuery, GL_QUERY_RESULT_AVAILABLE, &done);
        }
        GLuint timeElapsed = 0; // nanoseconds
        glGetQueryObjectuiv(mQuery, GL_QUERY_RESULT, &timeElapsed);
        computationTime = timeElapsed / 1000000.f; // miliseconds
    }

    // Fill computation time to GPUSurface
    upGPUSurface->mComputationTime = computationTime;

    // Return unique pointer
    return std::move(upGPUSurface);
}
