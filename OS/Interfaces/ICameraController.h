#pragma once
#include "../Core/Config.h"
#include "IOperatingSystem.h"
#include "../Math/MathTypes.h"

class CameraMatrix
{
public:
	CameraMatrix();
	CameraMatrix(const CameraMatrix& mat);
	inline const CameraMatrix& operator= (const CameraMatrix& mat);

	inline const CameraMatrix operator* (const Matrix4& mat) const;
	inline const CameraMatrix operator* (const CameraMatrix& mat) const;

    // Returns the camera matrix or the left eye matrix on VR platforms.
    mat4 getPrimaryMatrix() const;

    // Applies offsets to the projection matrices (useful when needing to jitter the camera for techniques like TAA)
    void applyProjectionSampleOffset(float xOffset, float yOffset);

    static inline const CameraMatrix inverse(const CameraMatrix& mat);
    static inline const CameraMatrix transpose(const CameraMatrix& mat);
    static inline const CameraMatrix perspective(float fovxRadians, float aspectInverse, float zNear, float zFar);
    static inline const CameraMatrix perspectiveReverseZ(float fovxRadians, float aspectInverse, float zNear, float zFar);
    static inline const CameraMatrix orthographic(float left, float right, float bottom, float top, float zNear, float zFar);
    static inline const CameraMatrix identity();
    static inline void extractFrustumClipPlanes(const CameraMatrix& vp, Vector4& rcp, Vector4& lcp, Vector4& tcp, Vector4& bcp, Vector4& fcp, Vector4& ncp, bool const normalizePlanes);

private:
    union
    {
        mat4 mCamera;
        mat4 mLeftEye;
    };
};

inline const CameraMatrix& CameraMatrix::operator= (const CameraMatrix& mat)
{
    mLeftEye = mat.mLeftEye;
    return *this;
}

inline const CameraMatrix CameraMatrix::operator* (const Matrix4& mat) const
{
    CameraMatrix result;
    result.mLeftEye = mLeftEye * mat;
    return result;
}

inline const CameraMatrix CameraMatrix::operator* (const CameraMatrix& mat) const
{
    CameraMatrix result;
    result.mLeftEye = mLeftEye * mat.mLeftEye;
    return result;
}

inline const CameraMatrix CameraMatrix::inverse(const CameraMatrix& mat)
{
    CameraMatrix result;
    result.mLeftEye = ::inverse(mat.mLeftEye);
    return result;
}

inline const CameraMatrix CameraMatrix::transpose(const CameraMatrix& mat)
{
    CameraMatrix result;
    result.mLeftEye = ::transpose(mat.mLeftEye);
    return result;
}

inline const CameraMatrix CameraMatrix::perspective(float fovxRadians, float aspectInverse, float zNear, float zFar)
{
    CameraMatrix result;
    result.mCamera = mat4::perspective(fovxRadians, aspectInverse, zNear, zFar);
    return result;
}

inline const CameraMatrix CameraMatrix::perspectiveReverseZ(float fovxRadians, float aspectInverse, float zNear, float zFar)
{
    CameraMatrix result;
    result.mCamera = mat4::perspectiveReverseZ(fovxRadians, aspectInverse, zNear, zFar);
    return result;
}

inline const CameraMatrix CameraMatrix::orthographic(float left, float right, float bottom, float top, float zNear, float zFar)
{
    CameraMatrix result;
    result.mCamera = mat4::orthographic(left, right, bottom, top, zNear, zFar);
    return result;
}

inline const CameraMatrix CameraMatrix::identity()
{
    CameraMatrix result;
    result.mLeftEye = mat4::identity();
    return result;
}

inline void CameraMatrix::extractFrustumClipPlanes(const CameraMatrix& vp, Vector4& rcp, Vector4& lcp, Vector4& tcp, Vector4& bcp, Vector4& fcp, Vector4& ncp, bool const normalizePlanes)
{
    mat4::extractFrustumClipPlanes(vp.mCamera, rcp, lcp, tcp, bcp, fcp, ncp, normalizePlanes);
}