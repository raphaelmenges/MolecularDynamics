//#include "ShaderTools/Renderer.h"
////#include "visibilityextractiondemo.h"

//void printProperties()
//{
//    // Compute Shader Props
//    int groupMax_x;
//    int groupMax_y;
//    int groupMax_z;
//    int maxInvocations;

//    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &groupMax_x);
//    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &groupMax_y);
//    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &groupMax_z);
//    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxInvocations);

//    std::cout << "Max work group size as (x,y,z): (" << groupMax_x << "," << groupMax_y << "," << groupMax_z << ")" << std::endl;
//    std::cout << "Max work group invocations: " << maxInvocations << std::endl;

//    // Texture Props
//    int maxTexture;
//    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexture);
//    std::cout << "Max texture size: " << maxTexture << std::endl;

//    int maxTexture3D;
//    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &maxTexture3D);
//    std::cout << "Max 3D texture size: " << maxTexture3D << std::endl;

////    GLuint tex3d;
////    glGenTextures(1, &tex3d);
////    glBindTexture(GL_TEXTURE_3D, tex3d);
////    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
////    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
////    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, maxTexture3D, maxTexture3D, 64, 0, GL_RGBA, GL_FLOAT, 0);

//    int err = glGetError();
//    printf("%d\n", err);


////    int maxAtomicCountersFrag;
////    glGetIntegerv(GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS, &maxAtomicCountersFrag);
////    std::cout << "GL_MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS: " << maxAtomicCountersFrag << std::endl;
//}


//int main(int argc, char *argv[]) {
//    printProperties();

//    //VisibilityExtractionDemo demo;
//    //demo.init();
//    //demo.run();

//}


