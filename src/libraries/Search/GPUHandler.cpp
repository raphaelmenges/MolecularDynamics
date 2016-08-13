//
// Created by ubundrian on 12.08.16.
//

#include "GPUHandler.h"



void GPUHandler::initSSBOInt(GLuint* ssboHandler, int length)
{
    glGenBuffers(1, ssboHandler);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int)*length, NULL, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        Logger::instance().print("Error while initializing SSBO: " + std::to_string(err), Logger::Mode::ERROR);
    }
}

void GPUHandler::initSSBOUInt(GLuint* ssboHandler, int length)
{
    glGenBuffers(1, ssboHandler);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint)*length, NULL, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        Logger::instance().print("Error while initializing SSBO: " + std::to_string(err), Logger::Mode::ERROR);
    }
}

void GPUHandler::initSSBOFloat(GLuint* ssboHandler, int length)
{
    glGenBuffers(1, ssboHandler);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float)*length, NULL, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        Logger::instance().print("Error while initializing SSBO: " + std::to_string(err), Logger::Mode::ERROR);
    }
}

void GPUHandler::initSSBOFloat3(GLuint* ssboHandler, int length)
{
    glGenBuffers(1, ssboHandler);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float)*3*length, NULL, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        Logger::instance().print("Error while initializing SSBO: " + std::to_string(err), Logger::Mode::ERROR);
    }
}



void GPUHandler::fillSSBOInt(GLuint* ssboHandler, int length, int value)
{
    int* values = new int[length];
    memset(values, value, length*sizeof(int));

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
    GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(p, &values, sizeof(values));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}



void GPUHandler::copyDataToSSBOInt(GLuint* targetHandler, int* data, int length)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *targetHandler);
    GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(p, &data, sizeof(int)*length);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void copyDataToSSBOUInt(GLuint* targetHandler, uint* data, int length)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *targetHandler);
    GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(p, &data, sizeof(uint)*length);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void copyDataToSSBOFloat(GLuint* targetHandler, float* data, int length)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *targetHandler);
    GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(p, &data, sizeof(float)*length);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void copyDataToSSBOFloat3(GLuint* targetHandler, glm::vec3* data, int length)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *targetHandler);
    GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(p, &data, sizeof(glm::vec3)*length);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}



void GPUHandler::deleteSSBO(GLuint* ssboHandler)
{
    glDeleteBuffers(1, ssboHandler);
}

