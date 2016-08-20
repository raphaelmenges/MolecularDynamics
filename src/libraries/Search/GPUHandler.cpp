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

void GPUHandler::initSSBOFloat4(GLuint* ssboHandler, int length)
{
    glGenBuffers(1, ssboHandler);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float)*4*length, NULL, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        Logger::instance().print("Error while initializing SSBO: " + std::to_string(err), Logger::Mode::ERROR);
    }
}



void GPUHandler::fillSSBOInt(GLuint* ssboHandler, int length, int value)
{
    int* values = new int[length];
    memset(values, value, sizeof(int)*length);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
    GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(p, values, sizeof(int)*length);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        Logger::instance().print("Error while filling SSBO with ints: " + std::to_string(err), Logger::Mode::ERROR);
    }

    delete[] values;
}

void GPUHandler::fillSSBOUInt(GLuint* ssboHandler, int length, uint value)
{
    uint* values = new uint[length];
    memset(values, value, sizeof(uint)*length);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
    GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(p, values, sizeof(uint)*length);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        Logger::instance().print("Error while filling SSBO with ints: " + std::to_string(err), Logger::Mode::ERROR);
    }

    delete[] values;
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



void GPUHandler::copySSBOtoSSBOUInt(GLuint* targetHandler, GLuint* dataHandler, int length) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *targetHandler);
    GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(p, dataHandler, sizeof(uint)*length);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void GPUHandler::copySSBOtoSSBOFloat4(GLuint* targetHandler, GLuint* dataHandler, int length) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *targetHandler);
    GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(p, dataHandler, sizeof(float)*length*4);
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

void GPUHandler::printSSBODataFloat4(GLuint* ssboHandler, int length)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
    GLuint *ptr;
    ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    Logger::instance().print("SSBO float 4:"); Logger::instance().tabIn();
    int idx = 0;
    for (int i = 0; i < length*4; i=i+4) {
        float x = *(((float*)ptr)+i);
        float y = *(((float*)ptr)+i+1);
        float z = *(((float*)ptr)+i+2);
        float w = *(((float*)ptr)+i+3);
        Logger::instance().print(std::to_string(idx) + ": " + std::to_string(x)
                                                     + ", " + std::to_string(y)
                                                     + ", " + std::to_string(z)
                                                     + ", " + std::to_string(w));
        idx++;
    }
    Logger::instance().tabOut();
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}



void GPUHandler::assertAtomsAreInBounds(GLuint* ssboHandler, int length, glm::vec3 min, glm::vec3 max)
{
    int numOutlier = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
    GLuint *ptr;
    ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    int idx = 0;
    for (int i = 0; i < length*4; i=i+4) {
        float x = *(((float*)ptr)+i);
        float y = *(((float*)ptr)+i+1);
        float z = *(((float*)ptr)+i+2);
        float w = *(((float*)ptr)+i+3);
        bool xInBounds = x >= min.x && x <= max.x;
        bool yInBounds = y >= min.y && y <= max.y;
        bool zInBounds = z >= min.z && z <= max.z;
        if (!(xInBounds && yInBounds && zInBounds)) {
            Logger::instance().print(std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + "not in bounds", Logger::Mode::ERROR);
            numOutlier++;
        }
        idx++;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    if (numOutlier > 0) {
        Logger::instance().print("Assertion error: Some atoms are outside the grid!", Logger::Mode::ERROR);
        exit(-1);
    }
}

void GPUHandler::assertSum(GLuint* ssboHandler, int length, int sum)
{
    int testSum = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
    GLuint *ptr;
    ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (int i = 0; i < length; i++) {
        int val = *(((int*)ptr)+i);
        testSum += val;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    if (testSum != sum) {
        Logger::instance().print("Assertion error: " + std::to_string(testSum) + " does not match expected " + std::to_string(sum), Logger::Mode::ERROR);
        exit(-1);
    }
}

void GPUHandler::assertBelowLimit(GLuint* ssboHandler, int length, int limit)
{
    int numOutlier = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
    GLuint *ptr;
    ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (int i = 0; i < length; i++) {
        int val = *(((int*)ptr)+i);
        if (val >= limit) {
            Logger::instance().print(std::to_string(val) + " not below " + std::to_string(limit), Logger::Mode::ERROR);
            numOutlier++;
        }
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    if (numOutlier > 0) {
        Logger::instance().print("Assertion error: Number of values not below limit (" + std::to_string(limit) + "): " + std::to_string(numOutlier) + "/" + std::to_string(length), Logger::Mode::ERROR);
        exit(-1);
    }
}

void GPUHandler::assertAboveLimit(GLuint* ssboHandler, int length, int limit)
{
    int numOutlier = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
    GLuint *ptr;
    ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (int i = 0; i < length; i++) {
        int val = *(((int*)ptr)+i);
        if (val <= limit) {
            //Logger::instance().print(std::to_string(val) + " not above " + std::to_string(limit), Logger::Mode::ERROR);
            numOutlier++;
        }
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    if (numOutlier > 0) {
        Logger::instance().print("Assertion error: Number of values not above limit (" + std::to_string(limit) + "): " + std::to_string(numOutlier) + "/" + std::to_string(length), Logger::Mode::ERROR);
        exit(-1);
    }
}

void GPUHandler::assertAboveLimit(GLuint* ssboHandler, int length, uint limit)
{
    int numOutlier = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
    GLuint *ptr;
    ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (int i = 0; i < length; i++) {
        uint val = *(((uint*)ptr)+i);
        if (val <= limit) {
            //Logger::instance().print(std::to_string(val) + " not above " + std::to_string(limit), Logger::Mode::ERROR);
            numOutlier++;
        }
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    if (numOutlier > 0) {
        Logger::instance().print("Assertion error: Number of values not above limit (" + std::to_string(limit) + "): " + std::to_string(numOutlier) + "/" + std::to_string(length), Logger::Mode::ERROR);
        exit(-1);
    }
}

void GPUHandler::assertAboveEqLimit(GLuint* ssboHandler, int length, int limit)
{
    int numOutlier = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
    GLuint *ptr;
    ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (int i = 0; i < length; i++) {
        int val = *(((int*)ptr)+i);
        if (val < limit) {
            //Logger::instance().print(std::to_string(val) + " not above " + std::to_string(limit), Logger::Mode::ERROR);
            numOutlier++;
        }
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    if (numOutlier > 0) {
        Logger::instance().print("Assertion error: Number of values not above limit (" + std::to_string(limit) + "): " + std::to_string(numOutlier) + "/" + std::to_string(length), Logger::Mode::ERROR);
        exit(-1);
    }
}

void GPUHandler::assertAboveEqLimit(GLuint* ssboHandler, int length, uint limit)
{
    int numOutlier = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
    GLuint *ptr;
    ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (int i = 0; i < length; i++) {
        uint val = *(((uint*)ptr)+i);
        if (val < limit) {
            //Logger::instance().print(std::to_string(val) + " not above " + std::to_string(limit), Logger::Mode::ERROR);
            numOutlier++;
        }
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    if (numOutlier > 0) {
        Logger::instance().print("Assertion error: Number of values not above limit (" + std::to_string(limit) + "): " + std::to_string(numOutlier) + "/" + std::to_string(length), Logger::Mode::ERROR);
        exit(-1);
    }
}

void GPUHandler::assertAllEqual(GLuint* ssboHandler, int length, int value)
{
    int numOutlier = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
    GLuint *ptr;
    ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (int i = 0; i < length; i++) {
        int val = *(((int*)ptr)+i);
        if (val != value) {
            Logger::instance().print(std::to_string(val) + " not equal to " + std::to_string(value), Logger::Mode::ERROR);
            numOutlier++;
        }
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    if (numOutlier > 0) {
        Logger::instance().print("Assertion error: Number of values that don't equal (" + std::to_string(value) + "): " + std::to_string(numOutlier) + "/" + std::to_string(length), Logger::Mode::ERROR);
        exit(-1);
    }
}

void GPUHandler::assertAllEqual(GLuint* ssboHandler, int length, uint value)
{
    int numOutlier = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
    GLuint *ptr;
    ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (int i = 0; i < length; i++) {
        uint val = *(((uint*)ptr)+i);
        if (val != value) {
            Logger::instance().print(std::to_string(val) + " not equal to " + std::to_string(value), Logger::Mode::ERROR);
            numOutlier++;
        }
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    if (numOutlier > 0) {
        Logger::instance().print("Assertion error: Number of values that don't equal (" + std::to_string(value) + "): " + std::to_string(numOutlier) + "/" + std::to_string(length), Logger::Mode::ERROR);
        exit(-1);
    }
}

void GPUHandler::assertCellContent(GLuint* cellHandler,GLuint* cellCntHandler, int length1, int length2)
{
    int* tempCellCnt = new int[length2];
    memset(tempCellCnt, 0, sizeof(int)*length2);

    /*
     * count cells
     */
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *cellHandler);
    GLuint *ptr;
    ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (int i = 0; i < length1; i++) {
        int cellIdx = *(((int*)ptr)+i);
        tempCellCnt[cellIdx]++;
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    int numOutlier = 0;

    /*
     * compare cell counts and count errors
     */
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *cellCntHandler);
    GLuint *ptr2;
    ptr2 = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
    for (int i = 0; i < length2; i++) {
        int elementCount = *(((int*)ptr2)+i);
        if (elementCount != tempCellCnt[i]) {
            numOutlier++;
        }
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    delete[] tempCellCnt;

    if (numOutlier++) {
        Logger::instance().print("Assertion error: Num of wrong cell counts: " + std::to_string(numOutlier) + "/" + std::to_string(length2), Logger::Mode::ERROR);
        exit(-1);
    }
}



void GPUHandler::deleteSSBO(GLuint* ssboHandler)
{
    glDeleteBuffers(1, ssboHandler);
}

