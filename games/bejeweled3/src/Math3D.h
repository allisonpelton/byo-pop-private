// The game's own small 3D math (0x419510 to 0x419a50 and 0x428470 to 0x428920 (bejeweled3)): a vector, a 3 by 3 matrix, a
// transform and the perspective camera the main menu places its buttons with. They are not the framework's 2D Transform.
// The layout and the arithmetic are the same as Bejeweled Twist's (games/bejeweledtwist/src/Math3D.h), each function
// checked against this export.
#pragma once

namespace Sexy {

struct Vec3 {
	float x = 0;
	float y = 0;
	float z = 0;
};

// Nine floats, row by row; a new one is the identity
struct Mat3 {
	float m[9];

	Mat3();
	// 0x4197a0 (bejeweled3): a turn about z: the identity with cos, sin, -sin and cos at 0, 1, 3 and 4
	static Mat3 TurnZ(float theRadians);
};

// 0x419510 (bejeweled3): the matrix theSecond times theFirst (the first call argument is the right factor)
Mat3 Product(const Mat3& theFirst, const Mat3& theSecond);

// Position, rotation and scale (0x3c bytes)
struct Transform3D {
	Vec3 mPos;
	Mat3 mRot;
	Vec3 mScale{1.0f, 1.0f, 1.0f};
};

// 0x428600 (bejeweled3): the inner transform taken through the outer one: its place moved as a point of the outer, the
// rotation Product(theOuter.mRot, theInner.mRot) and the scales multiplied. The menu calls it with a new turn as the
// outer transform and a camera's own as the inner.
Transform3D Compose(const Transform3D& theOuter, const Transform3D& theInner);
// 0x428880 (bejeweled3): a point of the transform's own space in the world: scaled, turned and moved
Vec3 TransformPoint(const Transform3D& theTransform, const Vec3& thePoint);
// 0x4287e0 (bejeweled3): a point of the world in the transform's space: the point less the place, taken along the rows,
// divided by the scale
Vec3 WorldToCamera(const Transform3D& theCamera, const Vec3& thePoint);

// Sexy::Graphics3D::PerspectiveCamera (vtable 0x865ff8 (bejeweled3), 0x58 bytes, constructor 0x419910): its transform is
// at +4, then the planes and the factors that put a point of its space on the screen.
struct PerspectiveCamera {
	Transform3D mTransform;  // +0x04
	float mNear = 1.0f;      // +0x40
	float mFar = 10000.0f;   // +0x44
	float mScaleX = 0;       // +0x48: the cotangent of the half angle over the aspect
	float mScaleY = 0;       // +0x4c: the cotangent of the half angle
	float mDepthScale = 0;   // +0x50: far over (far - near)
	float mDepthOffset = 0;  // +0x54: that, negated, times near

	// 0x419970 (bejeweled3): theFovTimesAspect is 38.5 times the screen's aspect in the menu
	void SetFov(float theFovTimesAspect, float theAspect, float theNear, float theFar);
	// 0x428920 (bejeweled3) with 0x428660: the camera put at theEye and turned to face theTarget with theUp upwards (its
	// rows: the axis across, the axis up and the one back from the target); false, and the camera left as it was, when
	// the eye is within 0.001 of the target on each axis or looks within 0.999 of straight along the up axis
	bool LookAt(const Vec3& theEye, const Vec3& theTarget, const Vec3& theUp);
	// 0x419a50 (bejeweled3), slot 3: a point of the camera's space as x and y from 0 to 1 across the screen and its depth
	Vec3 Project(const Vec3& theCameraPoint) const;
	// 0x4287e0 (bejeweled3) then slot 3: a point of the world on the screen
	Vec3 ProjectWorld(const Vec3& thePoint) const { return Project(WorldToCamera(mTransform, thePoint)); }
};

}  // namespace Sexy
