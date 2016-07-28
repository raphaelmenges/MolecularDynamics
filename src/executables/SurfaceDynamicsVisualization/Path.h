// Author: Raphael Menges
// Path of atom group.

#ifndef PATH_H
#define PATH_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <set>
#include "SurfaceExtraction/GPUProtein.h"
#include "ShaderTools/ShaderProgram.h"

class Path
{
public:

    // Constructor
    Path();

    // Destructor
    virtual ~Path();

    // Update path
    void update(
        GPUProtein const * pGPUProtein,
        const std::set<GLuint>& atomIndices,
        int smoothFrameRadius);

    // Draw path
    void draw(
        int frame,
        int drawFrameRadius,
        glm::mat4 viewMatrix,
        glm::mat4 projectionMatrix,
        glm::vec3 pastColor,
        glm::vec3 futureColor,
        float pointSize) const;

    // Get length of complete path
    float getCompleteLength() const;

    // Get length of subpath
    float getLength(int start, int end) const;

    // Get count of vertices in path
    int getVertexCount() const { return mVertexCount; }

private:

    // Members
    float mlength = 0;
    GLuint mVBO = 0;
    GLuint mVAO = 0;
    int mVertexCount;
    std::unique_ptr<ShaderProgram> mupProgram;
    int mPositionAttribue = 0;
    std::vector<float> mAccLengths; // saving accumulated lengths for easier computations. Size = mVertexCount - 1

};

#endif // PATH_H
