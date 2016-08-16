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

void GPUHandler::copyDataToSSBOUInt(GLuint* targetHandler, uint* data, int length)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *targetHandler);
    GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(p, &data, sizeof(uint)*length);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void GPUHandler::copyDataToSSBOFloat(GLuint* targetHandler, float* data, int length)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *targetHandler);
    GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(p, &data, sizeof(float)*length);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void GPUHandler::copyDataToSSBOFloat3(GLuint* targetHandler, glm::vec3* data, int length)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *targetHandler);
    GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(p, &data, sizeof(glm::vec3)*length);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}



void GPUHandler::printSSBODataInt(GLuint* ssboHandler, int length)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
    GLuint *ptr;
    ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    Logger::instance().print("SSBO int:"); Logger::instance().tabIn();
    for (int i = 0; i < length; i++) {
        int val = *(((int*)ptr)+i);
        Logger::instance().print(std::to_string(i) + ": " + std::to_string(val));
    }
    Logger::instance().tabOut();
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void GPUHandler::printSSBODataUInt(GLuint* ssboHandler, int length)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
    GLuint *ptr;
    ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    Logger::instance().print("SSBO uint:"); Logger::instance().tabIn();
    for (int i = 0; i < length; i++) {
        uint val = *(((uint*)ptr)+i);
        Logger::instance().print(std::to_string(i) + ": " + std::to_string(val));
    }
    Logger::instance().tabOut();
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void GPUHandler::printSSBODataFloat(GLuint* ssboHandler, int length)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
    GLuint *ptr;
    ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    Logger::instance().print("SSBO uint:"); Logger::instance().tabIn();
    for (int i = 0; i < length; i++) {
        float val = *(((float*)ptr)+i);
        Logger::instance().print(std::to_string(i) + ": " + std::to_string(val));
    }
    Logger::instance().tabOut();
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void GPUHandler::printSSBODataFloat3(GLuint* ssboHandler, int length)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
    GLuint *ptr;
    ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    Logger::instance().print("SSBO float 3:"); Logger::instance().tabIn();
    int idx = 0;
    for (int i = 0; i < length*4; i=i+4) {
        float x = *(((float*)ptr)+i);
        float y = *(((float*)ptr)+i+1);
        float z = *(((float*)ptr)+i+2);
        Logger::instance().print(std::to_string(idx) + ": " + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z));
        idx++;
    }
    Logger::instance().tabOut();
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}



void GPUHandler::deleteSSBO(GLuint* ssboHandler)
{
    glDeleteBuffers(1, ssboHandler);
}

