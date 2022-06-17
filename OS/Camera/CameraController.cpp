#define _USE_MATH_DEFINES
#include "../Interfaces/ICameraController.h"
#include "../Interfaces/ILog.h"

// Include this file as last include in all cpp files allocating memory
#include "../Interfaces/IMemory.h"

CameraMatrix::CameraMatrix()
{}

CameraMatrix::CameraMatrix(const CameraMatrix& mat)
{
    mLeftEye = mat.mLeftEye;
}

mat4 CameraMatrix::getPrimaryMatrix() const
{
	return mCamera;
}

void CameraMatrix::applyProjectionSampleOffset(float xOffset, float yOffset)
{
	mLeftEye[2][0] += xOffset;
	mLeftEye[2][1] += yOffset;
}