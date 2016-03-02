#ifndef DYNAMIC_VERTEX_ARRAY_OBJECT_H
#define DYNAMIC_VERTEX_ARRAY_OBJECT_H

#include "ShaderTools/VertexArrayObject.h"
#include "ShaderTools/ShaderProgram.h"


class DynamicVertexArrayObject : public VertexArrayObject
{
public:
    DynamicVertexArrayObject();
    virtual ~DynamicVertexArrayObject();

    virtual void draw() = 0;
    virtual void enableVertexAttribArrays(std::map<std::string, ShaderProgram::Info> &inputMap) = 0;

protected:
    GLuint vertexBufferObjectHandle;
};

#endif // DYNAMIC_VERTEX_ARRAY_OBJECT_H
