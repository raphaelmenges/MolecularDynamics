#include "ComputeProgram.h"

ComputeProgram::ComputeProgram(ShaderProgram* shaderProgram)
    : shaderProgram(shaderProgram)
{
}

ComputeProgram* ComputeProgram::run(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z)
{
    shaderProgram->use();
    glDispatchCompute(num_groups_x,num_groups_y,num_groups_z);
    return this;
}


ShaderProgram* ComputeProgram::getShaderProgram() {
    return shaderProgram;
}

ComputeProgram* ComputeProgram::setShaderProgram(ShaderProgram* shaderProgram)
{
    this->shaderProgram = shaderProgram;
    return this;
}

ComputeProgram* ComputeProgram::texture(std::string name, GLuint textureHandle)
{
    this->shaderProgram->texture(name, textureHandle);
    return this;
}

ComputeProgram* ComputeProgram::texture(std::string name, GLuint textureHandle, GLuint samplerHandle)
{
    this->shaderProgram->texture(name, textureHandle, samplerHandle);
    return this;
}

ComputeProgram *ComputeProgram::texture(std::string name, Texture *texture)
{
    this->shaderProgram->texture(name, texture);
    return this;
}

ComputeProgram *ComputeProgram::texture(std::string name, Texture *texture, GLuint samplerHandle)
{
    this->shaderProgram->texture(name, texture, samplerHandle);
    return this;
}
