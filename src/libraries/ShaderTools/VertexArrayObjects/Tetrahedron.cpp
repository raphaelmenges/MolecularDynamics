#include "Tetrahedron.h"

Tetrahedron::Tetrahedron() {
	const float size = 1.0f;
	mode = GL_TRIANGLES;

    glGenVertexArrays(1, &vertexArrayObjectHandle);
    glBindVertexArray(vertexArrayObjectHandle);

    // 1: 0,0,1.393
    // 2: 1.609,0,-1.393
    // 3: -1.609,0,-1.393
    // 4: 0, 1.3027, -0.46433
    // alt: 2.627
    GLuint vertexBufferHandles[2];
    glGenBuffers(2, vertexBufferHandles);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferHandles[0]);
    float positions[] = {
                 0,0,1.393, 1.609,0,-1.393, -1.609,0,-1.393,    // 1,2,3
                //
                0,0,1.393, 0, 1.3027,-0.46433, -1.609,0,-1.393, //1,4,3
                //
                0,0,1.393, 1.609,0,-1.393, 0, 1.3027,-0.46433, // 1,2,4
                //
                1.609,0,-1.393, -1.609,0,-1.393, 0, 1.3027,-0.46433 // 2,3,4

    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    GLfloat uvCoordinates[] = {
        // Front face
        0,0, 1,0, 1,1,
        // Right face
        0,0, 1,0, 1,1,
        // Back face
        0,0, 1,0, 1,1,
        // Left face
        0,0, 1,0, 1,1,
    };
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferHandles[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uvCoordinates), uvCoordinates, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);
}

void Tetrahedron::draw() {
    glBindVertexArray(vertexArrayObjectHandle);
    glDrawArrays(mode, 0, 4*3);
}
