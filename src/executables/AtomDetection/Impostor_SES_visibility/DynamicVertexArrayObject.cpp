#include "DynamicVertexArrayObject.h"


DynamicVertexArrayObject::DynamicVertexArrayObject()
{
    glGenVertexArrays(1, &vertexArrayObjectHandle);
    glBindVertexArray(vertexArrayObjectHandle);

    glGenBuffers(1, &vertexBufferObjectHandle);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjectHandle);
}

DynamicVertexArrayObject::~DynamicVertexArrayObject()
{
    glDeleteBuffers(1, &vertexBufferObjectHandle);
    glDeleteVertexArrays(1, &vertexArrayObjectHandle);
}
