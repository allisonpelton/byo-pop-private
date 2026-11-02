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

}  // namespace Sexy
