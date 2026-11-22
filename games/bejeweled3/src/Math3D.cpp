// Sexy::Vec3, Mat3, Transform3D and PerspectiveCamera (Math3D.h).
#include "Math3D.h"

#include <cmath>

using namespace Sexy;

namespace {

// 0x428470 (bejeweled3): the vector made one long (left as it is when it has no length)
Vec3 Normalized(const Vec3& theVector)
{
	float aLength = (float)std::sqrt((double)(theVector.y * theVector.y + theVector.x * theVector.x) +
		(double)(theVector.z * theVector.z));
	if (aLength == 0.0f)
		return theVector;
	float anInverse = (float)(1.0 / (double)aLength);
	return Vec3{theVector.x * anInverse, theVector.y * anInverse, theVector.z * anInverse};
}

// The cross product a by b, written inline in 0x428660 (bejeweled3)
Vec3 Cross(const Vec3& a, const Vec3& b)
{
	return Vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

}  // namespace

// port: the identity
Mat3::Mat3()
{
	for (int i = 0; i < 9; i++)
		m[i] = 0.0f;
	m[0] = 1.0f;
	m[4] = 1.0f;
	m[8] = 1.0f;
}

// 0x4197a0 (bejeweled3)
Mat3 Mat3::TurnZ(float theRadians)
{
	float aSin = std::sin(theRadians);
	float aCos = std::cos(theRadians);
	Mat3 aTurn;
	aTurn.m[0] = aCos;
	aTurn.m[1] = aSin;
	aTurn.m[3] = -aSin;
	aTurn.m[4] = aCos;
	return aTurn;
}

// 0x419510 (bejeweled3)
Mat3 Sexy::Product(const Mat3& theFirst, const Mat3& theSecond)
{
	Mat3 aProduct;
	for (int aRow = 0; aRow < 3; aRow++) {
		for (int aColumn = 0; aColumn < 3; aColumn++) {
			double aSum = 0.0;
			for (int k = 0; k < 3; k++)
				aSum += (double)theSecond.m[aRow * 3 + k] * (double)theFirst.m[k * 3 + aColumn];
			aProduct.m[aRow * 3 + aColumn] = (float)aSum;
		}
	}
	return aProduct;
}

// 0x428880 (bejeweled3)
Vec3 Sexy::TransformPoint(const Transform3D& theTransform, const Vec3& thePoint)
{
	const float* m = theTransform.mRot.m;
	double aX = (double)theTransform.mScale.x * thePoint.x;
	double aY = (double)theTransform.mScale.y * thePoint.y;
	double aZ = (double)theTransform.mScale.z * thePoint.z;
	return Vec3{(float)(theTransform.mPos.x + (aY * m[3] + aX * m[0] + aZ * m[6])),
		(float)(theTransform.mPos.y + (aY * m[4] + aX * m[1] + aZ * m[7])),
		(float)(theTransform.mPos.z + (aX * m[2] + aY * m[5] + aZ * m[8]))};
}

// 0x428600 (bejeweled3)
Transform3D Sexy::Compose(const Transform3D& theOuter, const Transform3D& theInner)
{
	Transform3D aResult;
	aResult.mScale = Vec3{theOuter.mScale.x * theInner.mScale.x, theOuter.mScale.y * theInner.mScale.y,
		theOuter.mScale.z * theInner.mScale.z};
	aResult.mRot = Product(theOuter.mRot, theInner.mRot);
	aResult.mPos = TransformPoint(theOuter, theInner.mPos);
	return aResult;
}

// 0x4287e0 (bejeweled3)
Vec3 Sexy::WorldToCamera(const Transform3D& theCamera, const Vec3& thePoint)
{
	const float* m = theCamera.mRot.m;
	double aX = (double)thePoint.x - theCamera.mPos.x;
	double aY = (double)thePoint.y - theCamera.mPos.y;
	double aZ = (double)thePoint.z - theCamera.mPos.z;
	return Vec3{(float)((aX * m[0] + aY * m[1] + aZ * m[2]) / theCamera.mScale.x),
		(float)((aZ * m[5] + aX * m[3] + aY * m[4]) / theCamera.mScale.y),
		(float)((aX * m[6] + aY * m[7] + aZ * m[8]) / theCamera.mScale.z)};
}

// 0x419970 (bejeweled3)
void PerspectiveCamera::SetFov(float theFovTimesAspect, float theAspect, float theNear, float theFar)
{
	float anAngle = (float)((double)theFovTimesAspect * 0.5 * 3.1415927410125732 / 180.0);
	anAngle = (float)((double)anAngle / (double)theAspect);
	float aCosine = (float)std::cos((double)anAngle);
	float aSine = (float)std::sin((double)anAngle);
	mScaleY = (float)((double)aCosine / (double)aSine);
	mScaleX = (float)((double)mScaleY / (double)theAspect);
	float aDepthScale = (float)((double)theFar / ((double)theFar - (double)theNear));
	mDepthScale = aDepthScale;
	mDepthOffset = (float)(-(double)aDepthScale * (double)theNear);
	mNear = theNear;
	mFar = theFar;
}

// 0x419a50 (bejeweled3)
Vec3 PerspectiveCamera::Project(const Vec3& theCameraPoint) const
{
	Vec3 aPoint;
	float aNegZ = -theCameraPoint.z;
	aPoint.z = (float)(((double)mDepthOffset + (double)aNegZ * mDepthScale) / (double)mFar);
	aPoint.x = (float)((double)(float)(((double)mScaleX * theCameraPoint.x) / (double)aNegZ) * 0.5 + 0.5);
	aPoint.y = (float)((double)(float)(((double)mScaleY * theCameraPoint.y) / (double)aNegZ) * -0.5 + 0.5);
	return aPoint;
}

// 0x428920 and 0x428660 (bejeweled3)
bool PerspectiveCamera::LookAt(const Vec3& theEye, const Vec3& theTarget, const Vec3& theUp)
{
	Vec3 aBack{theEye.x - theTarget.x, theEye.y - theTarget.y, theEye.z - theTarget.z};
	const float aTolerance = 0.0010000000474974513f;  // 0x7d43a0
	if (std::fabs(aBack.x) <= aTolerance && std::fabs(aBack.y) <= aTolerance && std::fabs(aBack.z) <= aTolerance)
		return false;
	Vec3 aBackUnit = Normalized(aBack);
	float anAlong = (float)((double)theUp.y * aBackUnit.y + (double)theUp.x * aBackUnit.x + (double)theUp.z * aBackUnit.z);
	if (!(std::fabs(anAlong) <= 0.9989999999524)) // 0x8811f0
		return false;
	Vec3 aAcross = Normalized(Cross(theUp, aBackUnit));
	Vec3 anUp = Normalized(Cross(aBackUnit, aAcross));
	mTransform.mPos = theEye;
	float* m = mTransform.mRot.m;
	m[0] = aAcross.x;
	m[1] = aAcross.y;
	m[2] = aAcross.z;
	m[3] = anUp.x;
	m[4] = anUp.y;
	m[5] = anUp.z;
	m[6] = aBackUnit.x;
	m[7] = aBackUnit.y;
	m[8] = aBackUnit.z;
	return true;
}
