// The helpers of Camera3D.h (moved out of ChallengeMenu.cpp unchanged).
#include "Camera3D.h"

#include <cmath>

namespace Sexy {

// 0x4c0840: a vector made unit (left alone when it has no length)
Vec3 NormalizedVec(const Vec3& v)
{
	float aLength = (float)std::sqrt((double)(v.y * v.y + v.x * v.x) + (double)(v.z * v.z));
	if (aLength == 0.0f)
		return v;
	float anInverse = (float)(1.0 / (double)aLength);
	return Vec3{v.x * anInverse, v.y * anInverse, v.z * anInverse};
}

Vec3 CrossVec(const Vec3& a, const Vec3& b)  // 0x4c06d0: a crossed with b
{
	return Vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

float DotVec(const Vec3& a, const Vec3& b)  // 0x4c06a0
{
	return (float)((double)(b.z * a.z) + (double)(b.x * a.x) + (double)(b.y * a.y));
}

// 0x674035 (0x67411e puts the place first): the transform turned to look from its place away from theTarget (its third row
// is the way from the target to the place), theUp above it; left alone when the way is all but nothing or all but along
// theUp. True when it turned.
// 0x673f7f: a zero vector copied into a camera, inline here
bool LookFrom(Transform3D& thePlace, const Vec3& theTarget, const Vec3& theUp)
{
	Vec3 aWay{thePlace.mPos.x - theTarget.x, thePlace.mPos.y - theTarget.y, thePlace.mPos.z - theTarget.z};  // 0x673bb6
	// 0x673f7f: the way is all but nothing
	if (std::fabs(aWay.x) <= 0.0010000000474974513 && std::fabs(aWay.y) <= 0.0010000000474974513 &&
		std::fabs(aWay.z) <= 0.0010000000474974513)
		return false;
	Vec3 aDirection = NormalizedVec(aWay);
	float aDot = DotVec(theUp, aDirection);
	if (std::fabs(aDot) > 0.9989999999525025)  // 0x7d7a60
		return false;
	thePlace.mRot.m[6] = aDirection.x;
	thePlace.mRot.m[7] = aDirection.y;
	thePlace.mRot.m[8] = aDirection.z;
	Vec3 aRight = NormalizedVec(CrossVec(theUp, aDirection));
	thePlace.mRot.m[0] = aRight.x;
	thePlace.mRot.m[1] = aRight.y;
	thePlace.mRot.m[2] = aRight.z;
	Vec3 aTop = NormalizedVec(CrossVec(aDirection, aRight));
	thePlace.mRot.m[3] = aTop.x;
	thePlace.mRot.m[4] = aTop.y;
	thePlace.mRot.m[5] = aTop.z;
	return true;
}

}  // namespace Sexy
