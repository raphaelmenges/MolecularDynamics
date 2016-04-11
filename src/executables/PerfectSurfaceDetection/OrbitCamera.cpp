#include "OrbitCamera.h"

const GLfloat CAMERA_BETA_BIAS = 0.0001f;

OrbitCamera::OrbitCamera(glm::vec3 center, GLfloat alpha, GLfloat beta, GLfloat radius, GLfloat minRadius, GLfloat maxRadius)
{
	mCenter = center;
	mAlpha = alpha;
	mBeta = beta;
	mRadius = radius;
	mMinRadius = minRadius;
	mMaxRadius = maxRadius;
	mPosition = glm::vec3(0, 0, 0);
	clampValues();
}

OrbitCamera::~OrbitCamera()
{
	// Nothing to do
}

void OrbitCamera::update()
{
	// Calculate current position
	mPosition.x = mRadius * glm::sin(glm::radians(mBeta)) * glm::cos(glm::radians(mAlpha));
	mPosition.y = mRadius * glm::cos(glm::radians(mBeta));
	mPosition.z = mRadius * glm::sin(glm::radians(mBeta)) * glm::sin(glm::radians(mAlpha));
	mPosition += mCenter;

	// Calculate view matrix using simple up vector
	mViewMatrix= glm::lookAt(mPosition, mCenter, glm::vec3(0.0f, 1.0f, 0.0f));
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