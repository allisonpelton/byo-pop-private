// Sexy::Vec3, Mat3 and Transform3D (Math3D.h).
#include "Math3D.h"

#include <cmath>

using namespace Sexy;

namespace {

// The quaternion (x, y, z, then w) that 0x4c0d30, 0x4c0f00 and 0x4c10f0 work on
struct Quat {
	float x, y, z, w;
};

// 0x4c0840: the vector made one long (as it is when it has no length)
Vec3 Normalized(const Vec3& v)
{
	float aLength = (float)std::sqrt((double)(v.y * v.y + v.x * v.x) + (double)(v.z * v.z));
	if (aLength == 0.0f)
		return v;
	float anInverse = (float)(1.0 / (double)aLength);
	return Vec3{v.x * anInverse, v.y * anInverse, v.z * anInverse};
}

Vec3 Cross(const Vec3& a, const Vec3& b)
{
	return Vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// 0x4c0b40: the matrix's rows made unit and square to each other, the axis of the first row from the other two
Mat3 Orthonormalized(const Mat3& theMatrix)
{
	Mat3 aMatrix = theMatrix;
	Vec3 r1 = aMatrix.Row(1);
	Vec3 r2 = aMatrix.Row(2);
	Vec3 r0 = Normalized(Cross(r1, r2));
	aMatrix.m[0] = r0.x;
	aMatrix.m[1] = r0.y;
	aMatrix.m[2] = r0.z;
	r1 = Normalized(Cross(r2, r0));
	aMatrix.m[3] = r1.x;
	aMatrix.m[4] = r1.y;
	aMatrix.m[5] = r1.z;
	r2 = Normalized(Cross(r0, r1));
	aMatrix.m[6] = r2.x;
	aMatrix.m[7] = r2.y;
	aMatrix.m[8] = r2.z;
	return aMatrix;
}

// 0x4c0d30: the quaternion of a rotation matrix
Quat QuatFromMat3(const Mat3& theMatrix)
{
	Mat3 aMatrix = Orthonormalized(theMatrix);
	Quat aQuat = {0, 0, 0, 0};
	float aTrace = (float)((double)aMatrix.m[0] + (double)aMatrix.m[4] + (double)aMatrix.m[8]);
	if (0.0f < aTrace) {
		float aRoot = (float)std::sqrt((double)aTrace + 1.0);
		aQuat.w = (float)((double)aRoot * 0.5);
		float aScale = (float)(0.5 / (double)aRoot);
		aQuat.x = (aMatrix.m[5] - aMatrix.m[7]) * aScale;
		aQuat.y = (aMatrix.m[6] - aMatrix.m[2]) * aScale;
		aQuat.z = (aMatrix.m[1] - aMatrix.m[3]) * aScale;
		return aQuat;
	}
	// the largest of the diagonal picks the axis i; j and k follow it round (0x8595d8 holds 1, 2, 0)
	static const int aNext[3] = {1, 2, 0};
	int i = aMatrix.m[0] < aMatrix.m[4] ? 1 : 0;
	if (aMatrix.m[i * 3 + i] < aMatrix.m[8])
		i = 2;
	int j = aNext[i];
	int k = aNext[j];
	const float* R[3] = {&aMatrix.m[0], &aMatrix.m[3], &aMatrix.m[6]};
	float aRoot = (float)std::sqrt((double)(R[i][i] - (R[k][k] + R[j][j])) + 1.0);
	float v[3] = {0, 0, 0};
	v[i] = (float)((double)aRoot * 0.5);
	float aScale = aRoot;
	if (aScale != 0.0f)
		aScale = (float)(0.5 / (double)aScale);
	aQuat.w = (R[j][k] - R[k][j]) * aScale;
	v[j] = (R[j][i] + R[i][j]) * aScale;
	v[k] = (R[i][k] + R[k][i]) * aScale;
	aQuat.x = v[0];
	aQuat.y = v[1];
	aQuat.z = v[2];
	return aQuat;
}

// 0x4c0f00: the rotation matrix of a unit quaternion
Mat3 Mat3FromQuat(const Quat& q)
{
	Mat3 aMatrix;
	float x = q.x;
	float y = q.y;
	float z = q.z;
	float aZ2 = (float)((double)z * 2.0);
	float aW2 = (float)((double)q.w * 2.0);
	float aXX2 = (float)((double)x * 2.0 * (double)x);
	float aYY2 = (float)((double)y * 2.0 * (double)y);
	float aXY2 = (float)((double)x * (double)y * 2.0);
	aMatrix.m[0] = (float)(1.0 - ((double)aYY2 + (double)aZ2 * (double)z));
	aMatrix.m[1] = (float)((double)aW2 * (double)z + (double)aXY2);
	aMatrix.m[3] = (float)((double)aXY2 - (double)aW2 * (double)z);
	aMatrix.m[2] = (float)((double)aZ2 * (double)x - (double)aW2 * (double)y);
	aMatrix.m[4] = (float)(1.0 - ((double)aZ2 * (double)z + (double)aXX2));
	aMatrix.m[6] = (float)((double)aZ2 * (double)x + (double)aW2 * (double)y);
	aMatrix.m[5] = (float)((double)aZ2 * (double)y + (double)aW2 * (double)x);
	aMatrix.m[7] = (float)((double)aZ2 * (double)y - (double)aW2 * (double)x);
	aMatrix.m[8] = (float)(1.0 - ((double)aYY2 + (double)aXX2));
	return aMatrix;
}

// 0x4c1590: the quaternion made one long (0x4c14b0 divides by the length)
Quat NormalizedQuat(const Quat& q)
{
	float aLength = (float)std::sqrt((double)q.x * q.x + (double)q.y * q.y + (double)q.z * q.z + (double)q.w * q.w);
	if (aLength == 0.0f)
		return q;
	float anInverse = (float)(1.0 / (double)aLength);
	return Quat{q.x * anInverse, q.y * anInverse, q.z * anInverse, (float)((double)q.w / (double)aLength)};
}

// 0x4c10f0: the quaternion a turned towards b by theT, the shortest way round
Quat SlerpQuat(const Quat& theFrom, const Quat& theTo, float theT, bool theLinear)
{
	const float anEpsilon = 9.999999974752427e-07f;
	// a and b alike: a
	if (std::fabs(theFrom.w - theTo.w) <= anEpsilon && std::fabs(theFrom.x - theTo.x) <= anEpsilon && std::fabs(theFrom.y - theTo.y) <= anEpsilon &&
		std::fabs(theFrom.z - theTo.z) <= anEpsilon)
		return theFrom;
	if (theT <= 0.0f)
		theT = 0.0f;
	else if (1.0f <= theT)
		theT = 1.0f;
	float anInvT = (float)(1.0 - (double)theT);
	Quat q2 = theTo;
	float aDot = (float)((double)(theTo.y * theFrom.y) + (double)(theFrom.x * theTo.x) + (double)(theTo.z * theFrom.z));
	aDot = (float)((double)aDot + (double)(theFrom.w * theTo.w));
	if (aDot < 0.0f) {
		aDot = -aDot;
		q2 = Quat{-theTo.x, -theTo.y, -theTo.z, -theTo.w};
	}
	if (0.0010000000474974513 < 1.0 - (double)aDot && !theLinear) {
		float anAngle = (float)std::acos((double)aDot);
		if (0.0010000000474974513f <= anAngle && 0.0010000000474974513 <= 3.1415927410125732 - (double)anAngle &&
			std::isfinite((double)anAngle)) {
			float anInverse = (float)(1.0 / (double)std::sin(anAngle));
			float aFromScale = (float)((double)std::sin((float)((double)anAngle * (double)anInvT)) * (double)anInverse);
			float aToScale = (float)((double)std::sin((float)((double)anAngle * (double)theT)) * (double)anInverse);
			float aToX = q2.x * aToScale;
			float aToY = q2.y * aToScale;
			float aToZ = q2.z * aToScale;
			Quat r;
			r.x = (float)((double)(theFrom.x * aFromScale) + (double)aToX);
			r.y = (float)((double)(theFrom.y * aFromScale) + (double)aToY);
			r.z = (float)((double)(theFrom.z * aFromScale) + (double)aToZ);
			r.w = (float)((double)q2.w * (double)aToScale + (double)aFromScale * (double)theFrom.w);
			return NormalizedQuat(r);
		}
	}
	// a straight blend
	float aToX = q2.x * theT;
	float aToY = q2.y * theT;
	float aToZ = q2.z * theT;
	float aFromX = theFrom.x * anInvT;
	float aFromY = theFrom.y * anInvT;
	float aFromZ = theFrom.z * anInvT;
	Quat r;
	r.x = (float)((double)aFromX + (double)aToX);
	r.y = (float)((double)aFromY + (double)aToY);
	r.z = (float)((double)aFromZ + (double)aToZ);
	r.w = (float)((double)q2.w * (double)theT + (double)theFrom.w * (double)anInvT);
	return NormalizedQuat(r);
}

// The turn of 0x673fd0, 0x680589 and 0x68fa0b: the identity with these four entries (a cosine, a sine, minus the sine
// and the cosine again) changed
Mat3 Turn(float theRadians, int theCos1, int theSin, int theMinusSin, int theCos2)
{
	float aSin = std::sin(theRadians);
	float aCos = std::cos(theRadians);
	Mat3 aTurn;
	aTurn.m[theCos1] = aCos;
	aTurn.m[theSin] = aSin;
	aTurn.m[theMinusSin] = -aSin;
	aTurn.m[theCos2] = aCos;
	return aTurn;
}

}  // namespace

// port: the identity matrix
Mat3::Mat3()
{
	for (int i = 0; i < 9; i++)
		m[i] = 0.0f;
	m[0] = 1.0f;
	m[4] = 1.0f;
	m[8] = 1.0f;
}

// port: the product of two 3x3 matrices
Mat3 Mat3::operator*(const Mat3& theOther) const
{
	Mat3 aProduct;
	for (int aRow = 0; aRow < 3; aRow++) {
		for (int aColumn = 0; aColumn < 3; aColumn++) {
			double aSum = 0.0;
			for (int k = 0; k < 3; k++)
				aSum += (double)m[aRow * 3 + k] * (double)theOther.m[k * 3 + aColumn];
			aProduct.m[aRow * 3 + aColumn] = (float)aSum;
		}
	}
	return aProduct;
}

void Mat3::RotateZ(float theRadians)
{
	*this = *this * Turn(theRadians, 0, 1, 3, 4);
}

void Mat3::RotateX(float theRadians)
{
	*this = *this * Turn(theRadians, 4, 5, 7, 8);
}

void Mat3::RotateY(float theRadians)
{
	*this = *this * Turn(theRadians, 0, 6, 2, 8);
}

Mat3 Mat3::Transposed() const
{
	Mat3 aMatrix;
	for (int aRow = 0; aRow < 3; aRow++) {
		for (int aColumn = 0; aColumn < 3; aColumn++)
			aMatrix.m[aRow * 3 + aColumn] = m[aColumn * 3 + aRow];
	}
	return aMatrix;
}

Mat3 Sexy::SlerpMat3(const Mat3& a, const Mat3& b, float theT, bool theLinear)
{
	Quat aToQuat = QuatFromMat3(b);
	Quat aFromQuat = QuatFromMat3(a);
	return Mat3FromQuat(SlerpQuat(aFromQuat, aToQuat, theT, theLinear));
}

namespace {

// 0x4c06a0: the dot product, the y terms first
float Dot(const Vec3& a, const Vec3& b)
{
	return (float)(((double)a.y * (double)b.y + (double)a.x * (double)b.x) + (double)a.z * (double)b.z);
}

}  // namespace

Vec3 Sexy::TimesMat3(const Vec3& theVector, const Mat3& theMatrix)
{
	const float* m = theMatrix.m;
	return Vec3{
		(float)(((double)m[3] * (double)theVector.y + (double)m[0] * (double)theVector.x) + (double)m[6] * (double)theVector.z),
		(float)(((double)m[1] * (double)theVector.x + (double)m[4] * (double)theVector.y) + (double)m[7] * (double)theVector.z),
		(float)(((double)m[2] * (double)theVector.x + (double)m[5] * (double)theVector.y) + (double)m[8] * (double)theVector.z)};
}

// 0x68b31c: component-wise vector multiply, inline here
Vec3 Sexy::TransformPoint(const Transform3D& theTransform, const Vec3& thePoint)
{
	// 0x68b31c: scaled, each axis by its own
	Vec3 aScaled{(float)((double)theTransform.mScale.x * (double)thePoint.x), (float)((double)theTransform.mScale.y * (double)thePoint.y),
		(float)((double)theTransform.mScale.z * (double)thePoint.z)};
	Vec3 aTurned = TimesMat3(aScaled, theTransform.mRot);
	// 0x4c0760
	return Vec3{(float)((double)theTransform.mPos.x + (double)aTurned.x), (float)((double)theTransform.mPos.y + (double)aTurned.y),
		(float)((double)theTransform.mPos.z + (double)aTurned.z)};
}

// 0x6dd0bd: component-wise vector divide, inline here
// 0x6dd107: a vector read from a transform, inline here
// 0x6dda14: a vector built from three transform columns, inline here
Vec3 Sexy::WorldToCamera(const Transform3D& theCamera, const Vec3& thePoint)
{
	Vec3 aDelta{(float)((double)thePoint.x - (double)theCamera.mPos.x), (float)((double)thePoint.y - (double)theCamera.mPos.y),
		(float)((double)thePoint.z - (double)theCamera.mPos.z)};
	// 0x6dd107: the dot products with the three rows
	Vec3 aTurned{Dot(aDelta, theCamera.mRot.Row(0)), Dot(aDelta, theCamera.mRot.Row(1)), Dot(aDelta, theCamera.mRot.Row(2))};
	// 0x6dd0bd
	return Vec3{(float)((double)aTurned.x / (double)theCamera.mScale.x), (float)((double)aTurned.y / (double)theCamera.mScale.y),
		(float)((double)aTurned.z / (double)theCamera.mScale.z)};
}

PerspectiveCamera::PerspectiveCamera(float theFovTimesAspect, float theAspect, float theNear, float theFar)
{
	SetFov(theFovTimesAspect, theAspect, theNear, theFar);
}

void PerspectiveCamera::SetFov(float theFovTimesAspect, float theAspect, float theNear, float theFar)
{
	// 0x673c7f
	float anAngle = (float)((double)theFovTimesAspect * 0.5);
	anAngle = (float)((double)anAngle * 3.1415927410125732 / 180.0);
	anAngle = (float)((double)anAngle / (double)theAspect);
	double aCosine = (double)std::cos(anAngle);
	double aSine = (double)std::sin(anAngle);
	float aCotangent = (float)(aCosine / aSine);
	mScaleY = aCotangent;
	mScaleX = (float)((double)aCotangent / (double)theAspect);
	// 0x673ce6: fsubrp is far less near (Intel's FSUBRP E1), so the scale is far over (far - near)
	float aDepthScale = (float)((double)theFar / ((double)theFar - (double)theNear));
	mDepthScale = aDepthScale;
	mDepthOffset = (float)(-(double)aDepthScale * (double)theNear);
	mNear = theNear;
	mFar = theFar;
}

Vec3 PerspectiveCamera::Project(const Vec3& theCameraPoint) const
{
	Vec3 aPoint;
	float aNegZ = -theCameraPoint.z;
	aPoint.z = (float)(((double)mDepthScale * (double)aNegZ + (double)mDepthOffset) / (double)mFar);
	float aX = (float)(((double)mScaleX * (double)theCameraPoint.x) / (double)aNegZ);
	aPoint.x = (float)((double)aX * 0.5 + 0.5);
	float aY = (float)(((double)mScaleY * (double)theCameraPoint.y) / (double)aNegZ);
	aPoint.y = (float)(0.5 + (double)aY * -0.5);
	return aPoint;
}
