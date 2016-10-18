//
// Created by ubundrian on 12.08.16.
//

#ifndef OPENGL_FRAMEWORK_GPUHANDLER_H
#define OPENGL_FRAMEWORK_GPUHANDLER_H

#include <vector>
#include <cstring>
#include <GL/glew.h>
#include <Utils/Logger.h>

#include "NeighborhoodSearchDefines.h"
#include "../../executables/NeighborSearchTest/SimpleAtom.h"

class GPUHandler {
public:
    /*
     * initialize ssbo of specified size
     */
    template<class T>
    static void initSSBO(GLuint* ssboHandler, int length){
        glGenBuffers(1, ssboHandler);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(T)*length, NULL, GL_DYNAMIC_COPY);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            Logger::instance().print("Error while initializing SSBO: " + std::to_string(err), Logger::Mode::ERROR);
        }
    }

    /*
     * fill every block of the ssbo with the provided value
     */
    template<class T>
    static void fillSSBO(GLuint* ssboHandler, int length, T value){
        T* values = new T[length];
        std::fill_n(values, length, value);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
        GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
        memcpy(p, values, sizeof(T)*length);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            Logger::instance().print("Error while filling SSBO with ints: " + std::to_string(err), Logger::Mode::ERROR);
        }

        delete[] values;
    }

    /*
     * transfer data from or to the ssbo
     */
    template<class T>
    static void copyDataToSSBO(GLuint* targetHandler, T* data, int length){
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, *targetHandler);
        GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
        memcpy(p, &data, sizeof(T)*length);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }
    template<class T>
    static T* getDataFromSSBO(GLuint* ssboHandler, int length)
    {
        T* data = new T[length];
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
        GLuint *ptr;
        ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        for (int i = 0; i < length; i++) {
            data[i] = *(((T*)ptr)+i);
        }
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        return data;
    }

    /*
     * prints the specified number of entries of the ssbo
     */
    template<class T>
    static void printSSBOData(GLuint* ssboHandler, int length){
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssboHandler);
        GLuint *ptr;
        ptr = (GLuint*) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        Logger::instance().print("SSBO:"); Logger::instance().tabIn();
        for (int i = 0; i < length; i++) {
            T val = *(((T*)ptr)+i);
            Logger::instance().print(std::to_string(i) + ": " + std::to_string(val));
        }
        Logger::instance().tabOut();
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    /*
     * deleting ssbo
     */
    static void deleteSSBO(GLuint* ssboHandler)
    {
        glDeleteBuffers(1, ssboHandler);
    }
};


#endif //OPENGL_FRAMEWORK_GPUHANDLER_H
