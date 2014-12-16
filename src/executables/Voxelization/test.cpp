#include "ShaderTools/DefaultRenderLoop.h"
#include "ShaderTools/RenderPass.h"
#include "ShaderTools/VertexArrayObjects/Cube.h"
#include "ShaderTools/VertexArrayObjects/Quad.h"

#include "SlicemapUtilities.h"

#define PI 3.14159265359f

/*TODO
 * - fix bugs
 * - enable slicemapping shader to use 128 slices
 * - enable slicemapoverlay shader to use 128 slices
 */

// DESCRIPTION
/*
 * This program creates a 128-slices slicemap from an object encoded in a RGBA texture of a 32bit unsigned integer format
 * For simplicity, the voxelization camera and the first person camera use the same matrices and the slicemap is overlayed additively onto the scene.
 */

// SHADER PROGRAMS
auto sp = new ShaderProgram({"/Voxelization/simpleVertex.vert", "/Voxelization/simpleColoring.frag"});
auto slicemappingShader = new ShaderProgram({"/Voxelization/simpleVertex.vert", "/Voxelization/sliceMapMultipleTargets.frag"});
auto projectSlicemap	= new ShaderProgram({"/Voxelization/screenfill.vert", "/Voxelization/sliceMapOverlay.frag"});

// OBJECTS
auto cube = new Cube();
auto quad = new Quad();

// RENDERPASSES
auto pass = new RenderPass( cube, sp, width, height );	// render cube
auto slicemappingPass = new SlicemapRenderPass( cube, slicemappingShader, width, height ); // render slice map
auto projectSlicemapPass = new RenderPass( quad, projectSlicemap);	// project slice map onto rendered scene

GLuint bitmask = createRGBA32UIBitMask();

// GLOBAL VARIABLES
float near = 0.1f;  // clipping planes
float far  = 6.0f;  // of voxelization camera

float red = 0.25f;   // object colors
float green = 0.25f;
float blue = 0.25f;

bool enabled = true;
float backgroundTransparency = 0.5f;

int numSlicemaps = 1;

glm::mat4 model = glm::mat4(1.0f);
glm::mat4 view = glm::lookAt(glm::vec3(0.0f,0.0f,3.0f), glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f,1.0f,0.0f) );
glm::mat4 projection = glm::perspective(60.0f * PI / 180.0f, (float) width/height, near, far );

void testZeros(GLuint texture, GLenum format = GL_RGBA)
{
	glActiveTexture( GL_TEXTURE10 );
	glBindTexture(GL_TEXTURE_2D, texture);
	int width_, height_;
	glGetTexLevelParameteriv(GL_TEXTURE_2D,0, GL_TEXTURE_WIDTH, &width_);
	glGetTexLevelParameteriv(GL_TEXTURE_2D,0, GL_TEXTURE_HEIGHT, &height_);
//	std::cout<<"2D texture: "<< texture <<" width: "<< width_ <<", height: "<< height_ << std::endl;

	unsigned int *data = (unsigned int*)malloc( sizeof(unsigned int) * height_ * width_ * 4);
	glGetTexImage(GL_TEXTURE_2D, 0, format, GL_UNSIGNED_INT, data);

	bool notzero = false;
	for( unsigned int i = 0; i < width_ * height_ * 4 ; i++ )
	{
		if ( data[i] > 0 )
		{
			std::cout<<"i[" << i / 4 << ", " << i % 4 << "] : " << data[i] << std::endl;
			notzero = true;
			break;
		}
	}
	if ( !notzero )
	{
		std::cout << "2D texture: " << texture <<" is all zeros..." << std::endl;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture( GL_TEXTURE0 );

	delete data;
}

void testZeros1D(GLuint texture)
{
	glActiveTexture( GL_TEXTURE11 );
	glBindTexture(GL_TEXTURE_1D, texture);
	int width_ = 0;
	glGetTexLevelParameteriv(GL_TEXTURE_1D, 0, GL_TEXTURE_WIDTH, &width_);
//	std::cout<<"1D texture: " << texture << " width: "<< width_ << std::endl;

	unsigned int *data = (unsigned int*)malloc(sizeof(unsigned int) * width_ * 4);
	glGetTexImage(GL_TEXTURE_1D, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, data);

	bool notzero = false;
	for( unsigned int i = 0; i < width_*4; i++ )
	{
		if ( data[i] > 0 )
		{
			std::cout<<"i[" << i / 4 << ", " << i % 4 <<"] : " << data[i] << std::endl;
			notzero = true;
		}
	}
	if ( !notzero )
	{
		std::cout << "1D texture: " << texture <<" is all zeros..." << std::endl;
	}

	delete data;

	glBindTexture(GL_TEXTURE_1D, 0);
	glActiveTexture( GL_TEXTURE0 );
}

bool once = false;

void testError()
{
    GLenum error = glGetError();
	switch (error) {
	case GL_NO_ERROR:
		std::cout << "Status : everything's fine up until now" << std::endl;
		break;
	default:
		std::cout << "ERROR-Status ";
		switch(error)
		{
		   case GL_INVALID_ENUM:
		    	std::cout<<"--- INVALID_ENUM"<< std::endl;
		    break;
		    case GL_INVALID_VALUE:
		    	std::cout<<"--- INVALID_VALUE"<< std::endl;
		    break;
		    case GL_INVALID_OPERATION:
		    	std::cout<<"--- INVALID_OPERATION"<< std::endl;
		    break;
		    case GL_INVALID_FRAMEBUFFER_OPERATION:
		    	std::cout<<"--- INVALID_FRAMEBUFFER_OPERATION"<< std::endl;
		    break;
		    case GL_OUT_OF_MEMORY:
		    	std::cout<<"--- OUT_OF_MEMORY"<< std::endl;
		    break;
		    case GL_STACK_UNDERFLOW:
		    	std::cout<<"--- STACK_UNDERFLOW"<< std::endl;
		    break;
		    case GL_STACK_OVERFLOW:
		    	std::cout<<"--- STACK_OVERFLOW"<< std::endl;
		    break;
		    default:
		    	std::cout<<"--- whaaaaa...??????????????"<<std::endl;
		}
		break;
	}
}

void testFramebuffer(GLenum target)
{
	const char* framebuffername;
	switch(target)
	{
		case GL_DRAW_FRAMEBUFFER:
			framebuffername = "DRAW_FRAMEBUFFER";
		break;
		case GL_READ_FRAMEBUFFER:
			framebuffername = "READ_FRAMEBUFFER";
		break;
		case GL_FRAMEBUFFER:
			framebuffername = "FRAMEBUFFER";
		break;
	}

	GLenum error = glCheckFramebufferStatus(target);
	switch(error)
	{
	case GL_FRAMEBUFFER_COMPLETE:
		std::cout << "FB Status : "<< framebuffername<< " is fine up until now" << std::endl;
		break;
	default:
		std::cout<<"ERROR-FB Status : " << framebuffername << " ";
		switch(error)
		{
			case GL_FRAMEBUFFER_UNDEFINED:
				std::cout<<"--- FRAMEBUFFER_UNDEFINED" << std::endl;
			break;
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
				std::cout<<"--- FRAMEBUFFER_INCOMPLETE_ATTACHMENT" << std::endl;
			break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
				std::cout<<"--- FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT" << std::endl;
			break;
			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
				std::cout<<"--- FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER" << std::endl;
			break;
			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
				std::cout<<"--- FRAMEBUFFER_INCOMPLETE_READ_BUFFER" << std::endl;
			break;
			case GL_FRAMEBUFFER_UNSUPPORTED:
				std::cout<<"--- FRAMEBUFFER_UNSUPPORTED" << std::endl;
			break;
			case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
				std::cout<<"--- FRAMEBUFFER_INCOMPLETE_MULTISAMPLE" << std::endl;
			break;
			case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
				std::cout<<"--- FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS" << std::endl;
			break;
			default:
				std::cout<<"--- whaaaaa... ??????????" << std::endl;
		}
	}
}

int main(int argc, char *argv[]) {
    sp -> printUniformInfo();
    sp -> printInputInfo();
    sp -> printOutputInfo();

    slicemappingShader->printUniformInfo();
    slicemappingShader->printInputInfo();
    slicemappingShader->printOutputInfo();

    projectSlicemap->printUniformInfo();
    projectSlicemap->printInputInfo();
    projectSlicemap->printOutputInfo();

	glEnable(GL_TEXTURE_1D);

//    //////////////////////////////////////////////////////////////////////////
//	  // DEBUG TEST: bitmask contains values | WORKS
//    testZeros1D(bitmask);
//    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // DEBUG TEST: possibility to clear framebuffer texture to arbitrary color manually | ---- FAILS ---
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	testFramebuffer(GL_FRAMEBUFFER);
//	glBindFramebuffer(GL_FRAMEBUFFER, pass->frameBufferObject->getFrameBufferObjectHandle());
	glBindFramebuffer(GL_FRAMEBUFFER, slicemappingPass->frameBufferObject->getFrameBufferObjectHandle());
//	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, slicemappingPass->frameBufferObject->getFrameBufferObjectHandle());
    testError();
	testFramebuffer(GL_FRAMEBUFFER);
    GLuint in[4] = {0, 1, 2, 3};
    glClearBufferuiv(GL_COLOR, 0, in);
	testError();
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    testError();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, slicemappingPass->frameBufferObject->getFrameBufferObjectHandle());
    testError();
	testFramebuffer(GL_READ_FRAMEBUFFER);
    unsigned int out[4] = {1337, 1337, 1337, 1337};
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    testError();
    glReadPixels(0, 0, 1, 1, GL_RGBA_INTEGER, GL_UNSIGNED_INT, &out);
    testError();
    printf("IN  %d %d %d %d\n", in [0], in [1], in [2], in [3]);
    printf("OUT %d %d %d %d\n", out[0], out[1], out[2], out[3]);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    //////////////////////////////////////////////////////////////////////////

//    //////////////////////////////////////////////////////////////////////////
//    // DEBUG TEST: possibility to upload values manually| WORKS
//    glBindTexture(GL_TEXTURE_2D, slicemappingPass->frameBufferObject->get("slice0_127") );
//    unsigned int *data = (unsigned int*)malloc(sizeof(unsigned int) * width * height * 4);
//    for ( unsigned int i = 0; i < width*height*4; i++ )
//    {
//    	data[i] = 65536u;	// arbitrary value
//    }
//    glTexSubImage2D(GL_TEXTURE_2D, 0,0,0,width,height,GL_RGBA_INTEGER,GL_UNSIGNED_INT, data);
//    glBindTexture(GL_TEXTURE_2D, 0);
//    testZeros(slicemappingPass->frameBufferObject->get("slice0_127"), GL_RGBA_INTEGER);
//    delete data;
//    //////////////////////////////////////////////////////////////////////////
//
//    //////////////////////////////////////////////////////////////////////////
//    // DEBUG TEST: framebuffer is aware that texture is of unsigned integer type | WORKS
//    slicemappingPass->frameBufferObject->bind(); // identify as unsigned int
////    pass->frameBufferObject->bind();		     // identify as float
//    int param;
//    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, &param );
//    if ( param == GL_UNSIGNED_INT )
//    {
//    	std::cout<<"framebuffer attachment type : UNSIGNED_INT"<<std::endl;
//    }
//    else
//    {
//    	if ( param == GL_FLOAT )
//    	    {
//    	    	std::cout<<"framebuffer attachment type : FLOAT"<<std::endl;
//    	    }
//    	else
//    	{
//    		std::cout<<"framebuffer attachment type : UNKNOWN..." << std::endl;
//    	}
//    }
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
//    //////////////////////////////////////////////////////////////////////////

    renderLoop([]{
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {view = glm::translate(glm::vec3(0.0f,0.0f,0.01f)) * view;}
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {view = glm::translate(glm::vec3(0.0f,0.0f,-0.01f)) * view;}
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {view = glm::translate(glm::vec3(-0.01f, 0.0f,0.0f)) * view;}
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {view = glm::translate(glm::vec3(0.01f,0.0f,0.0f)) * view;}
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {glfwDestroyWindow(window); exit(-1);}
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {enabled = !enabled;}
		if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS) {backgroundTransparency += 0.15;}
		if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS) {backgroundTransparency -= 0.15;}

		if ( !once )
		{

			//TODO clear slice map to 0

			glDisable(GL_DEPTH_TEST);
			glEnable(GL_LOGIC_OP);
			glLogicOp(GL_OR);

			// bind bitmask to image unit 0
			glBindImageTexture(0,	// image unit binding
					bitmask,		// texture
					0,				// texture level
					GL_FALSE,		// layered
					0,				// layer
					GL_READ_ONLY,	// access
					GL_RGBA32UI		// format
			);

			// render slicemap
			slicemappingPass
			->update("model", model)
			->update("view", view)
			->update("projection", projection)
			->update("near", near)
			->update("far", far)
//			->update("numSlicemaps", numSlicemaps)
			->run();

			glEnable(GL_DEPTH_TEST);
			glDisable(GL_LOGIC_OP);
			glLogicOp(GL_COPY);

//			once = true;

			//DEBUGGING
		    testZeros(slicemappingPass->frameBufferObject->get("slice0_127"), GL_RGBA_INTEGER);
		}

		// render scene
        pass
        -> clear(0, 0, 0, 0)
        -> update("model", model)
        -> update("view", view)
        -> update("projection", projection)
        -> update("red", red)
        -> update("green", green)
        -> update("blue", blue)
        -> update("alpha", 1.0f)
        -> run();

//		testZeros(pass->get("fragmentColor"));

        // bind slicemap to image unit 0
        glUseProgram( projectSlicemap->getProgramHandle() );

        glBindImageTexture(1,           // image unit binding
		slicemappingPass->get("slice0_127"), // texture
		0,								// texture level
		GL_FALSE,                       // layered
		0,                              // layer
		GL_READ_ONLY,                   // access
		GL_RGBA32UI                     // format
		);

		projectSlicemapPass
        ->clear(0,0,0,0)
        ->texture("baseTexture", pass->get("fragmentColor"))
        ->update("backgroundTransparency", backgroundTransparency)
        ->run();

		glBindImageTexture(1,0,0,GL_FALSE,0,GL_READ_WRITE,GL_RGBA32UI);
		glBindImageTexture(0,0,0,GL_FALSE,0,GL_READ_WRITE,GL_RGBA32UI);

    });
}
