#include "Path.h"

Path::Path()
{
    // Shader
    mupProgram = std::unique_ptr<ShaderProgram>(new ShaderProgram("/SurfaceDynamicsVisualization/path.vert", "/SurfaceDynamicsVisualization/path.frag"));

    // Generate and bind vertex array object
    glGenVertexArrays(1, &mVAO);
    glBindVertexArray(mVAO);

    // Generate vertex buffer but do not fill it
    glGenBuffers(1, &mVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);

    // Bind it to shader program
    mPositionAttribue = glGetAttribLocation(mupProgram->getProgramHandle(), "position");
    glEnableVertexAttribArray(mPositionAttribue);
    glVertexAttribPointer(mPositionAttribue, 3, GL_FLOAT, GL_FALSE, 0, 0); // called again at path creation

    // Unbind everything
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

Path::~Path()
{
    glDeleteVertexArrays(1, &mVAO);
    glDeleteBuffers(1, &mVBO);
}

void Path::update(
    GPUProtein const * pGPUProtein,
    const std::set<GLuint>& atomIndices,
    int smoothFrameRadius)
{
    // Clear accumulated length
    mAccLengths.clear();
    mAccLengths.reserve(pGPUProtein->getFrameCount() - 1); // distance between path vertices

    // Path
    auto rTrajectoy = pGPUProtein->getTrajectory();

    // Go over frames and calculate average position of all used atoms
    std::vector<glm::vec3> path;
    path.reserve(pGPUProtein->getFrameCount());
    for(const auto& frame : *(rTrajectoy.get()))
    {
        // Go over analysed atoms and accumulate position in frame
        glm::vec3 accPosition(0,0,0);
        for(int atomIndex : atomIndices)
        {
            accPosition += frame.at(atomIndex);
        }

        // Calculate average
        glm::vec3 avgPosition = accPosition / atomIndices.size();

        // Caluclate and accumulate distance
        if(!path.empty())
        {
            // Save length
            float length = glm::distance(avgPosition, path.back());
            if(mAccLengths.empty())
            {
                mAccLengths.push_back(length);
            }
            else
            {
                mAccLengths.push_back(mAccLengths.back() + length);
            }
        }

        // Push new average position value to vector after calculating distance
        path.push_back(avgPosition);
    }

    // Create VBO which is filled within next if clause
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);

    // If necessary, smooth those calculated positions over time
    if(smoothFrameRadius > 0)
    {
        // Create vector for smoothed path
        std::vector<glm::vec3> smoothedPath;
        smoothedPath.reserve(pGPUProtein->getFrameCount());

        // Go over all positions
        int pathVertexCount = path.size();
        for(int i = 0; i < pathVertexCount; i++)
        {
            // Do accumulation for each frame
            glm::vec3 accPosition(0,0,0);

            // Get all position within radius
            int startFrame = glm::max(0, i - smoothFrameRadius);
            int endFrame = glm::min(pathVertexCount-1, i + smoothFrameRadius);
            for(int j = startFrame; j <= endFrame; j++)
            {
                accPosition += path.at(j);
            }
            accPosition /= ((endFrame - startFrame) + 1);

            // Push it back to smoothed path vector
            smoothedPath.push_back(accPosition);
        }

        // Fill smoothed path to vertex buffer
        glBufferData(GL_ARRAY_BUFFER, smoothedPath.size() * sizeof(glm::vec3), smoothedPath.data(), GL_DYNAMIC_DRAW);
        mVertexCount = smoothedPath.size(); // should be the same as path.size(), but ok...
    }
    else
    {
        // Fill unsmoothed path to vertex buffer
        glBufferData(GL_ARRAY_BUFFER, path.size() * sizeof(glm::vec3), path.data(), GL_DYNAMIC_DRAW);
        mVertexCount = path.size();
    }

    // Unbind buffer
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Path::draw(
    int frame,
    int drawFrameRadius,
    glm::mat4 viewMatrix,
    glm::mat4 projectionMatrix,
    glm::vec3 pastColor,
    glm::vec3 futureColor,
    float pointSize) const
{
    if(mVertexCount > 0)
    {
        glDisable(GL_DEPTH_TEST);
        glBindVertexArray(mVAO);
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);

        // Point size for rendering
        glPointSize(pointSize);

        // Decide which path parts are rendered
        int offset = glm::clamp(frame - drawFrameRadius, 0, mVertexCount); // on which frame path starts
        glVertexAttribPointer(mPositionAttribue, 3, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(GLfloat) * 3 * offset));
        int count = glm::min(((drawFrameRadius * 2) + 1), mVertexCount - offset);

        // General shader setup
        mupProgram->use();
        mupProgram->update("view", viewMatrix);
        mupProgram->update("projection", projectionMatrix);
        mupProgram->update("pastColor", pastColor);
        mupProgram->update("futureColor", futureColor);
        mupProgram->update("frame", frame);
        mupProgram->update("offset", offset);
        mupProgram->update("frameRadius", drawFrameRadius);

        // Drawing
        glDrawArrays(GL_LINE_STRIP, 0, count); // path as line
        glDrawArrays(GL_POINTS, 0, count); // path as points

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
    }
}

float Path::getCompleteLength() const
{
    if(mAccLengths.empty())
    {
        return 0.f;
    }
    else
    {
        return mAccLengths.back();
    }
}

float Path::getLength(int startFrame, int endFrame) const
{
    if(mAccLengths.empty() || startFrame >= endFrame)
    {
        return 0.f;
    }
    else if(startFrame == 0)
    {
        return mAccLengths.at(endFrame - 1);
    }
    else
    {
        return (mAccLengths.at(endFrame - 1) - mAccLengths.at(startFrame - 1));
    }
}
