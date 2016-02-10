/**
 * @file   		ComputeProgram.h
 * @author 		Gerrit Lochmann
 * @date   		@todo
 * @copyright	@todo
 *
 * @brief  		Manages a render pass
 *
 * The ComputeProgram class manages a single render pass.
 */

#ifndef COMPUTE_PROGRAM_H
#define COMPUTE_PROGRAM_H

#include "VertexArrayObject.h"
#include "FrameBufferObject.h"
#include "AssetTools/Texture.h"

class ComputeProgram
{
    public:
        ComputeProgram(ShaderProgram* shaderProgram);

		/**
		 * @brief Executes the whole render pass
         * @return The ComputeProgram instance
		 */
        ComputeProgram* run(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);

        ShaderProgram* getShaderProgram();
        ComputeProgram* setShaderProgram(ShaderProgram* shaderProgram);

		template <class T>
        ComputeProgram* update(std::string name, T value) {
			shaderProgram->update(name, value);
			return this;
        }

        ComputeProgram* texture(std::string name, GLuint textureID);
        ComputeProgram* texture(std::string name, GLuint textureID, GLuint samplerHandle);

        ComputeProgram* texture(std::string name, Texture* texture);
        ComputeProgram* texture(std::string name, Texture* texture, GLuint samplerHandle);

	// private:
		// Shader program to use within a render pass
        ShaderProgram* shaderProgram;
};

#endif // COMPUTE_PROGRAM_H
