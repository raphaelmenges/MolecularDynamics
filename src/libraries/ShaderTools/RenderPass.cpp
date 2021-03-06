#include "RenderPass.h"

RenderPass::RenderPass(VertexArrayObject* vertexArrayObject, ShaderProgram* shaderProgram)
	: vertexArrayObject(vertexArrayObject), shaderProgram(shaderProgram), frameBufferObject(0)
{
	frameBufferObject = new FrameBufferObject();
}

RenderPass::RenderPass(VertexArrayObject* vertexArrayObject, ShaderProgram* shaderProgram, int width, int height)
	: vertexArrayObject(vertexArrayObject), shaderProgram(shaderProgram), frameBufferObject(0)
{
	autoGenerateFrameBufferObject(width, height);
}

RenderPass::RenderPass(VertexArrayObject* vertexArrayObject, ShaderProgram* shaderProgram, FrameBufferObject* frameBufferObject)
	: vertexArrayObject(vertexArrayObject), shaderProgram(shaderProgram), frameBufferObject(frameBufferObject)
{
}

RenderPass* RenderPass::run() {
	frameBufferObject->bind();
	shaderProgram->use();
	vertexArrayObject->draw();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return this;
}

void RenderPass::autoGenerateFrameBufferObject(int width, int height) {
	if (frameBufferObject) delete frameBufferObject;
	frameBufferObject = new FrameBufferObject(&(shaderProgram->outputMap), width, height);
}

RenderPass* RenderPass::texture(std::string name, GLuint textureHandle) {
    shaderProgram->texture(name, textureHandle);
	return this;
}

RenderPass* RenderPass::texture(std::string name, GLuint textureHandle, GLuint samplerHandle) {
	shaderProgram->texture(name, textureHandle, samplerHandle);
    return this;
}

RenderPass *RenderPass::texture(std::string name, Texture *texture)
{
    shaderProgram->texture(name, texture);
    return this;
}

RenderPass *RenderPass::texture(std::string name, Texture *texture, GLuint samplerHandle)
{
    shaderProgram->texture(name, texture, samplerHandle);
    return this;
}

RenderPass* RenderPass::clear(float r, float g, float b, float a) {
	frameBufferObject->clear(r, g, b, a);
	return this;
}

RenderPass* RenderPass::clear() {
	frameBufferObject->clear();
	return this;
}

RenderPass* RenderPass::clearDepth() {
	frameBufferObject->clearDepth();
	return this;
}

GLuint RenderPass::get(std::string name) {
	return frameBufferObject->get(name);
}

ShaderProgram* RenderPass::getShaderProgram() {
	return shaderProgram;
}

VertexArrayObject* RenderPass::getVertexArrayObject() {
	return vertexArrayObject;
}

FrameBufferObject* RenderPass::getFrameBufferObject() {
	return frameBufferObject;
}

RenderPass* RenderPass::setShaderProgram(ShaderProgram* shaderProgram) {
	this->shaderProgram = shaderProgram;
	return this;
}

RenderPass* RenderPass::setVertexArrayObject(VertexArrayObject* vertexArrayObject) {
	this->vertexArrayObject = vertexArrayObject;
	return this;
}

RenderPass* RenderPass::setFrameBufferObject(FrameBufferObject* frameBufferObject) {
	this->frameBufferObject = frameBufferObject;
	return this;
}
