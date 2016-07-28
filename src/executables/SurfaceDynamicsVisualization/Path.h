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

    // Get path length
    float getLength() const { return _length; }

private:

    // Members
    float _length = 0;
    GLuint _VBO = 0;
    GLuint _VAO = 0;
    int _vertexCount;
    std::unique_ptr<ShaderProgram> mupProgram;
    int _positionAttribue = 0;

};

#endif // PATH_H
