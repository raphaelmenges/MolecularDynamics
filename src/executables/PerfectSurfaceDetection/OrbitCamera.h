#ifndef ORBIT_CAMERA_H
#define ORBIT_CAMERA_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class OrbitCamera
{
public:

    // Constructor (Degrees!)
    OrbitCamera(
        glm::vec3 center,
        GLfloat alpha,
        GLfloat beta,
        GLfloat radius,
        GLfloat minRadius,
        GLfloat maxRadius,
        GLfloat fov);

    // Destructor
    virtual ~OrbitCamera();

    // Update view and projection matrix (must be called at least once)
    void update(int viewportWidth, int viewportHeight);

    // Reset camera
    void reset(glm::vec3 center, GLfloat alpha, GLfloat beta, GLfloat radius);

    // Setter
    void setCenter(glm::vec3 center);
    void setAlpha(GLfloat alpha);
    void setBeta(GLfloat beta);
    void setRadius(GLfloat radius);

    // Getter
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
    glm::vec3 getPosition() const;
    glm::vec3 getCenter() const;
    GLfloat getAlpha() const;
    GLfloat getBeta() const;
    GLfloat getRadius() const;

protected:

    // Internal clamping
    void clampValues();
    void clampAlpha();
    void clampBeta();
    void clampRadius();

    // Members
    glm::vec3 mCenter;
    GLfloat mAlpha; // horizontal rotation
    GLfloat mBeta; // vertical rotation
    GLfloat mRadius;
    GLfloat mMinRadius;
    GLfloat mMaxRadius;
    GLfloat mFov;
    glm::vec3 mPosition;
    glm::mat4 mViewMatrix;
    glm::mat4 mProjectionMatrix;
};

#endif // ORBIT_CAMERA_H
