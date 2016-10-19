//============================================================================
// Distributed under the MIT License. Author: Adrian Derstroff
//============================================================================

#ifndef OPENGL_FRAMEWORK_ASSERTDATA_H
#define OPENGL_FRAMEWORK_ASSERTDATA_H

#include "GPUHandler.h"
#include "glm/glm.hpp"

class AssertData {
public:
    static void assertAtomsAreInBounds(GLuint* ssboHandler, int length, glm::vec3 min, glm::vec3 max)
    {
        int numOutlier = 0;

        glm::vec4* atomPositions = GPUHandler::getDataFromSSBO<glm::vec4>(ssboHandler, length);
        for (int i = 0; i < length; i++) {
            float x = atomPositions[i].x;
            float y = atomPositions[i].y;
            float z = atomPositions[i].z;
            bool xInBounds = x >= min.x && x <= max.x;
            bool yInBounds = y >= min.y && y <= max.y;
            bool zInBounds = z >= min.z && z <= max.z;
            if (!(xInBounds && yInBounds && zInBounds)) {
                numOutlier++;
            }
        }

        if (numOutlier > 0) {
            Logger::instance().print("Assertion error: Some atoms are outside the grid!", Logger::Mode::ERROR);
            exit(-1);
        }

        delete[] atomPositions;
    }

    template<class T>
    static void assertSum(GLuint* ssboHandler, int length, T sum)
    {
        T testSum = 0;
        T* data = GPUHandler::getDataFromSSBO<T>(ssboHandler, length);
        for (int i = 0; i < length; i++) {
            testSum += data[i];
        }

        if (testSum != sum) {
            Logger::instance().print("Assertion error: " + std::to_string(testSum) + " does not match expected " + std::to_string(sum), Logger::Mode::ERROR);
            exit(-1);
        }

        delete[] data;
    }

    template<class T>
    static void assertBelowLimit(GLuint* ssboHandler, int length, T limit)
    {
        int numOutlier = 0;
        T* data = GPUHandler::getDataFromSSBO<T>(ssboHandler, length);
        for (int i = 0; i < length; i++) {
            if (data[i] >= limit) {
                numOutlier++;
            }
        }

        if (numOutlier > 0) {
            Logger::instance().print("Assertion error: Number of values not below limit (" + std::to_string(limit) + "): " + std::to_string(numOutlier) + "/" + std::to_string(length), Logger::Mode::ERROR);
            exit(-1);
        }

        delete[] data;
    }

    template<class T>
    static void assertAboveLimit(GLuint* ssboHandler, int length, T limit)
    {
        int numOutlier = 0;
        T* data = GPUHandler::getDataFromSSBO<T>(ssboHandler, length);
        for (int i = 0; i < length; i++) {
            if (data[i] <= limit) {
                numOutlier++;
            }
        }

        if (numOutlier > 0) {
            Logger::instance().print("Assertion error: Number of values not above limit (" + std::to_string(limit) + "): " + std::to_string(numOutlier) + "/" + std::to_string(length), Logger::Mode::ERROR);
            exit(-1);
        }

        delete[] data;
    }

    template<class T>
    static void assertAboveEqLimit(GLuint* ssboHandler, int length, T limit){
        int numOutlier = 0;
        T* data = GPUHandler::getDataFromSSBO<T>(ssboHandler, length);
        for (int i = 0; i < length; i++) {
            if (data[i] < limit) {
                numOutlier++;
            }
        }

        if (numOutlier > 0) {
            Logger::instance().print("Assertion error: Number of values not above or equal to limit (" + std::to_string(limit) + "): " + std::to_string(numOutlier) + "/" + std::to_string(length), Logger::Mode::ERROR);
            exit(-1);
        }

        delete[] data;
    }

    template<class T>
    static void assertAllEqual(GLuint* ssboHandler, int length, T value)
    {
        int numOutlier = 0;
        T* data = GPUHandler::getDataFromSSBO<T>(ssboHandler, length);
        for (int i = 0; i < length; i++) {
            if (data[i] != value) {
                numOutlier++;
            }
        }

        if (numOutlier > 0) {
            Logger::instance().print("Assertion error: Number of values that don't equal (" + std::to_string(value) + "): " + std::to_string(numOutlier) + "/" + std::to_string(length), Logger::Mode::ERROR);
            exit(-1);
        }

        delete[] data;
    }

    template<class T>
    static void assertArraysEqual(GLuint* ssboHandler1, GLuint* ssboHandler2, int length)
    {
        int numOutlier = 0;
        T* tempData1 = GPUHandler::getDataFromSSBO<T>(ssboHandler1, length);
        T* tempData2 = GPUHandler::getDataFromSSBO<T>(ssboHandler2, length);
        for (int i = 0; i < length; i++) {
            if (tempData1[i] != tempData2[i]) {
                numOutlier++;
            }
        }

        if (numOutlier > 0) {
            Logger::instance().print("Assertion error: Number of values that don't match: " + std::to_string(numOutlier) + "/" + std::to_string(length), Logger::Mode::ERROR);
            exit(-1);
        }

        delete[] tempData1;
        delete[] tempData2;
    }

    template<class T>
    static void assertDataMonotInc(GLuint* ssboHandler, int length, T startValue)
    {
        int numOutlier = 0;
        int lastValue = startValue;
        T* data = GPUHandler::getDataFromSSBO<T>(ssboHandler, length);
        for (int i = 0; i < length; i++) {
            if (data[i] < lastValue) {
                numOutlier++;
            }
            lastValue = data[i];
        }

        if (numOutlier > 0) {
            Logger::instance().print("Assertion error: Number of values that are not monotonially increasing: " + std::to_string(numOutlier) + "/" + std::to_string(length), Logger::Mode::ERROR);
            exit(-1);
        }

        delete[] data;
    }

    static void assertCellContent(GLuint* cellHandler,GLuint* cellCntHandler, int length1, int length2)
    {
        int numOutlier = 0;

        int* tempCellCnt = new int[length2];
        memset(tempCellCnt, 0, sizeof(int)*length2);

        int* cellIdxs = GPUHandler::getDataFromSSBO<int>(cellHandler, length1);
        for (int i = 0; i < length1; i++) {
            tempCellCnt[cellIdxs[i]]++;
        }

        int* cellCounts = GPUHandler::getDataFromSSBO<int>(cellCntHandler, length2);
        for (int i = 0; i < length2; i++) {
            if (cellCounts[i] != tempCellCnt[i]) {
                numOutlier++;
            }
        }

        if (numOutlier++) {
            Logger::instance().print("Assertion error: Num of wrong cell counts: " + std::to_string(numOutlier) + "/" + std::to_string(length2), Logger::Mode::ERROR);
            exit(-1);
        }

        delete[] tempCellCnt;
        delete[] cellIdxs;
        delete[] cellCounts;
    }
};


#endif //OPENGL_FRAMEWORK_ASSERTDATA_H
