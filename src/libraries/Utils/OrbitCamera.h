// Author: Raphael Menges
// Simple orbit camera. Supports perspective and orthographic projection.

#ifndef ORBIT_CAMERA_H
#define ORBIT_CAMERA_H

#include <GL/glew.h>
#include <glm/glm.hpp>

class OrbitCamera
{
public:

    // Constructor (Degrees!)
    // Orthographic projection takes max radius as position for camera
    OrbitCamera(
            glm::vec3 center,
            GLfloat alpha,
            GLfloat beta,
            GLfloat radius,
            GLfloat minRadius,
            GLfloat maxRadius,
            GLfloat fov,
            GLfloat orthoScale);

    // Destructor
    virtual ~OrbitCamera();

    // Update view and projection matrix (must be called at least once)
    void update(GLint viewportWidth, GLint viewportHeight, GLboolean perspective);

    // Reset camera
    void reset(glm::vec3 center, GLfloat alpha, GLfloat beta, GLfloat radius);

    // Setter
    void setCenter(glm::vec3 center);
    void setAlpha(GLfloat alpha); // degrees
    void setBeta(GLfloat beta); // degrees
    void setRadius(GLfloat radius);

    // Getter
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
    glm::vec3 getPosition() const;
    glm::vec3 getCenter() const;
    GLfloat getAlpha() const;
    GLfloat getBeta() const;
    GLfloat getRadius() const;
    glm::vec3 getDirection() const; // normalized
    glm::vec3 getPositionAtPixel(int x, int y) const; // orthographic camera, only

private:

    // Internal clamping
    void clampValues();
    void clampAlpha();
    void clampBeta();
    void clampRadius();

    // Members
    glm::vec3 mCenter;
    GLfloat mAlpha; // horizontal rotation; [0,360[ degree
    GLfloat mBeta; // vertical rotation; [0,180[ degree
    GLfloat mRadius;
    GLfloat mMinRadius;
    GLfloat mMaxRadius;
    GLfloat mFov;
    GLfloat mOrthoScale;
    glm::vec3 mPosition;
    glm::mat4 mViewMatrix;
    glm::mat4 mProjectionMatrix;
    GLint mViewportWidth;
    GLint mViewportHeight;
    GLfloat mOrthoZoom;
};

#endif // ORBIT_CAMERA_H
