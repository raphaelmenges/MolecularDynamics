#include "OrbitCamera.h"
#include <glm/gtc/matrix_transform.hpp>

const GLfloat CAMERA_BETA_BIAS = 0.0001f;

OrbitCamera::OrbitCamera(
    glm::vec3 center,
    GLfloat alpha,
    GLfloat beta,
    GLfloat radius,
    GLfloat minRadius,
    GLfloat maxRadius,
    GLfloat fov,
    GLfloat orthoScale)
{
    mCenter = center;
    mAlpha = alpha;
    mBeta = beta;
    mRadius = radius;
    mMinRadius = minRadius;
    mMaxRadius = maxRadius;
    mFov = fov;
    mOrthoScale = orthoScale;
    mPosition = glm::vec3(0, 0, 0);
    clampValues();
}

OrbitCamera::~OrbitCamera()
{
    // Nothing to do
}

void OrbitCamera::update(int viewportWidth, int viewportHeight, bool perspective)
{
    // Calculate view and projection matrix
    if(perspective)
    {
        // Perspective
        mPosition.x = mRadius * glm::sin(glm::radians(mBeta)) * glm::cos(glm::radians(mAlpha));
        mPosition.y = mRadius * glm::cos(glm::radians(mBeta));
        mPosition.z = mRadius * glm::sin(glm::radians(mBeta)) * glm::sin(glm::radians(mAlpha));
        mPosition += mCenter;

        // View matrix
        mViewMatrix= glm::lookAt(mPosition, mCenter, glm::vec3(0.0f, 1.0f, 0.0f));

        // Projection matrix
        mProjectionMatrix = glm::perspective(
            glm::radians(mFov),
            (GLfloat)viewportWidth / (GLfloat)viewportHeight,
            0.1f,
            mMaxRadius * 2.f);
    }
    else
    {
        // Orthographic (max radius is used to calculate camera point)
        mPosition.x = mMaxRadius * glm::sin(glm::radians(mBeta)) * glm::cos(glm::radians(mAlpha));
        mPosition.y = mMaxRadius * glm::cos(glm::radians(mBeta));
        mPosition.z = mMaxRadius * glm::sin(glm::radians(mBeta)) * glm::sin(glm::radians(mAlpha));
        mPosition += mCenter;

        // View matrix
        mViewMatrix= glm::lookAt(mPosition, mCenter, glm::vec3(0.0f, 1.0f, 0.0f));

        // Projection matrix
        GLfloat halfWidth = ((GLfloat) viewportWidth) * mOrthoScale;
        GLfloat halfHeight = ((GLfloat) viewportHeight) * mOrthoScale;
        GLfloat zoom = glm::max(0.0000001f, (mRadius - mMinRadius) / (mMaxRadius - mMinRadius));
        mProjectionMatrix = glm::ortho(
            zoom * -halfWidth,
            zoom * halfWidth,
            zoom * -halfHeight,
            zoom * halfHeight,
            0.1f,
            mMaxRadius * 2.f);
    }
}

void OrbitCamera::reset(glm::vec3 center, GLfloat alpha, GLfloat beta, GLfloat radius)
{
    mCenter = center;
    mAlpha = alpha;
    mBeta = beta;
    mRadius = radius;
}

void OrbitCamera::setCenter(glm::vec3 center)
{
    mCenter = center;
}

void OrbitCamera::setAlpha(GLfloat alpha)
{
    mAlpha = alpha;
    clampAlpha();
}

void OrbitCamera::setBeta(GLfloat beta)
{
    mBeta = beta;
    clampBeta();
}

void OrbitCamera::setRadius(GLfloat radius)
{
    mRadius = radius;
    clampRadius();
}

glm::mat4 OrbitCamera::getViewMatrix() const
{
    return mViewMatrix;
}

glm::mat4 OrbitCamera::getProjectionMatrix() const
{
    return mProjectionMatrix;
}

glm::vec3 OrbitCamera::getPosition() const
{
    return mPosition;
}

glm::vec3 OrbitCamera::getCenter() const
{
    return mCenter;
}

GLfloat OrbitCamera::getAlpha() const
{
    return mAlpha;
}

GLfloat OrbitCamera::getBeta() const
{
    return mBeta;
}

GLfloat OrbitCamera::getRadius() const
{
    return mRadius;
}

glm::vec3 OrbitCamera::getDirection() const
{
    return glm::normalize(mCenter - mPosition);
}

void OrbitCamera::clampValues()
{
    // Horizontal rotation
    clampAlpha();

    // Vertical rotation
    clampBeta();

    // Zoom/Radius
    clampRadius();
}

void OrbitCamera::clampAlpha()
{
    mAlpha = fmodf(mAlpha, 2 * glm::degrees(glm::pi<GLfloat>()));
    if (mAlpha < 0)
    {
        mAlpha = 2 * glm::degrees(glm::pi<GLfloat>()) + mAlpha;
    }
}

void OrbitCamera::clampBeta()
{
    mBeta = glm::clamp(mBeta, CAMERA_BETA_BIAS, glm::degrees(glm::pi<GLfloat>()) - CAMERA_BETA_BIAS);
}

void OrbitCamera::clampRadius()
{
    mRadius = glm::clamp(mRadius, mMinRadius, mMaxRadius);
}
