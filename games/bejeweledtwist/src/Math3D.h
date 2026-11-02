// The game's own small 3D math (0x4bd450-0x4c1670, 0x673fd0, 0x680589, 0x68fa0b, 0x6dda5c): a vector, a 3 by 3 matrix
// and a transform. The challenge menu's camera is made from them (ChallengeMenu.h); they are not the framework's 2D
// Transform (SexyMatrix.h).
#pragma once

namespace Sexy {

struct Vec3 {
	float x = 0;
	float y = 0;
	float z = 0;
};

// Nine floats, row by row (the rows are the axes of a rotation); a new one is the identity (0x4bd450)
struct Mat3 {
	float m[9];

	Mat3();
	Mat3 operator*(const Mat3& theOther) const;  // 0x4c0a00: this times theOther, row by row
	void RotateZ(float theRadians);              // 0x673fd0: this = this * a turn about z
	void RotateX(float theRadians);              // 0x680589
	void RotateY(float theRadians);              // 0x68fa0b
	Mat3 Transposed() const;                     // 0x6dda5c: the rows become the columns
	Vec3 Row(int theRow) const { return Vec3{m[theRow * 3], m[theRow * 3 + 1], m[theRow * 3 + 2]}; }
};

// 0x4c1670: the matrix a turned towards the matrix b by theT (0 to 1) through quaternions (0x4c0d30, 0x4c10f0,
// 0x4c0f00): the shortest arc, or, with theLinear, a straight blend of the quaternions made unit again
Mat3 SlerpMat3(const Mat3& a, const Mat3& b, float theT, bool theLinear);

// Position, rotation and scale (0x3c bytes); a new one is the identity (0x4bd4c0)
struct Transform3D {
	Vec3 mPos;
	Mat3 mRot;
	Vec3 mScale{1.0f, 1.0f, 1.0f};
};

// 0x4c0c80: the vector times the matrix, the matrix's rows weighted by x, y and z (0x673fa4 does it in place)
Vec3 TimesMat3(const Vec3& theVector, const Mat3& theMatrix);

// 0x68b369: a point of the transform's own space in the world: scaled by its scale, turned by its rotation
// (0x4c0c80) and moved by its place (0x68b6b6 does it in place)
Vec3 TransformPoint(const Transform3D& theTransform, const Vec3& thePoint);

// 0x6dd14b: a point of the world in the camera's space: the point less the camera's place, turned by the rows of the
// camera's rotation (its axes) and divided by its scale
Vec3 WorldToCamera(const Transform3D& theCamera, const Vec3& thePoint);

// A perspective camera (0x58 bytes, vtable 0x7d7a6c; ctor 0x67417d, whose field of view is 0x673c7f): the camera's
// transform at +0x04, then the planes and the factors that put a point of the camera's space on the screen
struct PerspectiveCamera {
	Transform3D mTransform;  // +0x04: the camera (the menu copies gMenuCamera into it, 0x673c10)
	float mNear = 0;         // +0x40
	float mFar = 0;          // +0x44
	float mScaleX = 0;       // +0x48: the cotangent of the half angle over the aspect
	float mScaleY = 0;       // +0x4c: the cotangent of the half angle
	float mDepthScale = 0;   // +0x50: far over (far - near)
	float mDepthOffset = 0;  // +0x54: that, negated, times near

	// 0x67417d: theFovTimesAspect is 45 times theAspect in the menu (so the half angle is 22.5 degrees)
	PerspectiveCamera(float theFovTimesAspect, float theAspect, float theNear, float theFar);
	// 0x674208: a point of the camera's space as x and y from 0 to 1 across the screen and its depth (negative behind)
	Vec3 Project(const Vec3& theCameraPoint) const;
	// 0x6dd19b: a point of the world on the screen
	Vec3 ProjectWorld(const Vec3& thePoint) const { return Project(WorldToCamera(mTransform, thePoint)); }
};

}  // namespace Sexy
