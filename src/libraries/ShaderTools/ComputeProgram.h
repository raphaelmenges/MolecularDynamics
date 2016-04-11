// Authors: Gerrit Lochmann, Raphael Menges

// TODO
// - shader progam is never deleted ?!

#ifndef COMPUTE_PROGRAM_H
#define COMPUTE_PROGRAM_H

#include "VertexArrayObject.h"
#include "FrameBufferObject.h"
#include "AssetTools/Texture.h"

class ComputeProgram
{
    public:

        // Constructor
        ComputeProgram(ShaderProgram* shaderProgram);

        // Dispatch shader compute program
        ComputeProgram* run(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);

        // Get shader program
        ShaderProgram* getShaderProgram();

        // Set custom compute program
        ComputeProgram* setShaderProgram(ShaderProgram* shaderProgram);

        // Update single uniform value
        template <class T>
        ComputeProgram* update(std::string name, T value) {
            shaderProgram->update(name, value);
            return this;
        }

        // Bind texture by id
        ComputeProgram* texture(std::string name, GLuint textureID);
        ComputeProgram* texture(std::string name, GLuint textureID, GLuint samplerHandle);

        // Bind texture by object
        ComputeProgram* texture(std::string name, Texture* texture);
        ComputeProgram* texture(std::string name, Texture* texture, GLuint samplerHandle);

    private:

        ShaderProgram* shaderProgram;
};

#endif // COMPUTE_PROGRAM_H
