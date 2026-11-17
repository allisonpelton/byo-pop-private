// Sexy::PIEffect (PIEffect.h): the random draws and the loader of the new (.ppf) file format.
#include "PIEffect.h"

#include "PakInterface.h"
#include "X86Math.h"

#include <cstring>
#include <vector>

namespace Sexy {

namespace {

// The bytes of the file being read. The original's reader (GetData 0x565b40 (bejeweled3)) hands out the bytes in order and keeps a
// checksum of them; a read past the end ends the load here
class PPFReader {
public:
	const unsigned char* mData;
	size_t mSize;
	size_t mPosition;
	bool mFailed;

	PPFReader(const unsigned char* theData, size_t theSize) : mData(theData), mSize(theSize), mPosition(0), mFailed(false) {}

	// 0x565b40 (bejeweled3): the next theCount bytes, or null when the file ends first
	const unsigned char* GetData(size_t theCount)
	{
		if (mFailed || mPosition + theCount > mSize) {
			mFailed = true;
			return NULL;
		}
		const unsigned char* aBytes = mData + mPosition;
		mPosition += theCount;
		return aBytes;
	}

	void Skip(size_t theCount) { GetData(theCount); }

	int ReadByte()
	{
		const unsigned char* aBytes = GetData(1);
		return aBytes != NULL ? aBytes[0] : 0;
	}

	bool ReadBool() { return ReadByte() != 0; }

	int ReadShort()
	{
		const unsigned char* aBytes = GetData(2);
		return aBytes != NULL ? (short)(aBytes[0] | (aBytes[1] << 8)) : 0;
	}

	int ReadInt()
	{
		const unsigned char* aBytes = GetData(4);
		return aBytes != NULL ? (int)((unsigned int)aBytes[0] | ((unsigned int)aBytes[1] << 8) |
		                              ((unsigned int)aBytes[2] << 16) | ((unsigned int)aBytes[3] << 24)) : 0;
	}

	float ReadFloat()
	{
		int aBits = ReadInt();
		float aValue;
		std::memcpy(&aValue, &aBits, sizeof(aValue));
		return aValue;
	}

	// 0x567dc0 (bejeweled3): a byte for the length, then the characters
	std::string ReadString()
	{
		int aLength = ReadByte();
		const unsigned char* aBytes = GetData(aLength);
		if (aBytes == NULL)
			return std::string();
		return std::string((const char*)aBytes, aLength);
	}

	// 0x568710 (bejeweled3): a value's keyframes. A flags byte holds the key count (7 means a count follows), whether
	// the keys carry Bezier control points, whether the first key's time (0) and value (0, 1 or 2) are left out
	void ReadValue(PIValue& theValue)
	{
		int aFlags = ReadByte();
		int aCount = aFlags & 7;
		if (aCount == 7)
			aCount = ReadShort();
		bool aHasControls = aCount > 1 && ((aFlags >> 3) & 1) != 0;

		std::vector<PIPoint> aPoints;
		std::vector<PIPoint> aControls;
		std::vector<float> aTimes;
		theValue.mKeys.clear();
		theValue.mKeys.resize(aCount > 0 ? aCount : 0);

		for (int i = 0; i < aCount && !mFailed; i++) {
			float aTime = 0.0f;
			if ((aFlags & 0x10) == 0 || i != 0)
				aTime = ReadFloat();
			aTimes.push_back(aTime);

			float aValue;
			int aFirstValue = aFlags & 0x60;
			if (i == 0 && aFirstValue != 0)
				aValue = aFirstValue == 0x20 ? 0.0f : (aFirstValue == 0x40 ? 1.0f : 2.0f);
			else
				aValue = ReadFloat();

			double aX = (double)aTime;
			double aY = (double)aValue;
			PIPoint aPoint = {aX, aY};
			aPoints.push_back(aPoint);

			if (aHasControls) {
				float anInX = ReadFloat();
				float anInY = ReadFloat();
				if (i > 0) {
					PIPoint anIn = {(double)anInX + aX, (double)anInY + aY};
					aControls.push_back(anIn);
				}
				float anOutX = ReadFloat();
				float anOutY = ReadFloat();
				PIPoint anOut = {(double)anOutX + aX, (double)anOutY + aY};
				aControls.push_back(anOut);
			}

			if (i < (int)theValue.mKeys.size()) {
				theValue.mKeys[i].mTime = aTime;
				theValue.mKeys[i].mValue = aValue;
			}
		}

		// two keys of the same value make one
		if (!aHasControls && theValue.mKeys.size() == 2 && theValue.mKeys[1].mValue == theValue.mKeys[0].mValue)
			theValue.mKeys.pop_back();
		if (aCount > 1 && aHasControls)
			theValue.mBezier.Init(aPoints, aControls, aTimes, aCount);
	}

	// 0x5681a0 (bejeweled3): a position's keyframes: time (an int), x and y, and the Bezier control points when the
	// leading byte says the keys have them
	void ReadValue2D(PIValue2D& theValue)
	{
		int aCount = ReadShort();
		bool aHasControls = false;
		if (aCount > 1)
			aHasControls = ReadBool();

		std::vector<PIPoint> aPoints;
		std::vector<PIPoint> aControls;
		std::vector<float> aTimes;
		for (int i = 0; i < aCount && !mFailed; i++) {
			float aTime = (float)ReadInt();
			aTimes.push_back(aTime);

			double aX = (double)ReadFloat();
			double aY = (double)ReadFloat();
			PIPoint aPoint = {aX, aY};
			aPoints.push_back(aPoint);

			if (aHasControls) {
				float anInX = ReadFloat();
				float anInY = ReadFloat();
				if (i > 0) {
					PIPoint anIn = {(double)anInX + aX, (double)anInY + aY};
					aControls.push_back(anIn);
				}
				float anOutX = ReadFloat();
				float anOutY = ReadFloat();
				PIPoint anOut = {(double)anOutX + aX, (double)anOutY + aY};
				aControls.push_back(anOut);
			}

			PIValue2D::KeyFrame aKey = {aTime, aX, aY};
			theValue.mKeys.push_back(aKey);
		}
		if (aCount > 1 && aHasControls)
			theValue.mBezier.Init(aPoints, aControls, aTimes, aCount);
	}

	// 0x5685e0 (bejeweled3): a position's keyframes without control points: time (an int), x and y
	void ReadEPoint(PIValue2D& theValue)
	{
		int aCount = ReadShort();
		for (int i = 0; i < aCount && !mFailed; i++) {
			float aTime = (float)ReadInt();
			double aX = (double)ReadFloat();
			double aY = (double)ReadFloat();
			PIValue2D::KeyFrame aKey = {aTime, aX, aY};
			theValue.mKeys.push_back(aKey);
		}
	}

	// A list of points: for each, 8 bytes that nothing reads and a position's keyframes (as the deflector and
	// blocker lists and the emitter instance's points are all stored)
	void ReadPointList(std::vector<PIValue2D>& thePoints)
	{
		int aCount = ReadShort();
		for (int i = 0; i < aCount && !mFailed; i++) {
			Skip(8);
			PIValue2D aPoint;
			ReadEPoint(aPoint);
			thePoints.push_back(aPoint);
		}
	}
};

// 0x568d80 (bejeweled3): one particle definition of an emitter
void ReadParticleDef(PPFReader& theReader, PIParticleDef& theDef)
{
	theReader.Skip(64);
	theDef.mDrawInSecondPass = theReader.ReadBool();
	theDef.mSingleParticle = theReader.ReadBool();
	theDef.mSecondPassOnly = theReader.ReadBool();
	theDef.mAttachToEmitter = theReader.ReadBool();
	theDef.mAttachRatio = theReader.ReadFloat();
	theDef.mFlipX = theReader.ReadBool();
	theDef.mFlipY = theReader.ReadBool();
	theDef.mRandomStartFrame = theReader.ReadBool();
	theDef.mColorCycles = theReader.ReadInt();
	theDef.mAlphaCycles = theReader.ReadInt();
	theReader.ReadBool();   // +0xa74: nothing reads it
	theDef.mName = theReader.ReadString();
	theDef.mAlignToMotion = theReader.ReadBool();
	theDef.mRandomStartAngle = theReader.ReadBool();
	theDef.mAlignToEmitterAngle = theReader.ReadBool();
	theDef.mStartAngleDegrees = theReader.ReadInt();
	theDef.mAlignOffsetDegrees = theReader.ReadInt();
	theDef.mFrameOffset = theReader.ReadInt();
	theDef.mColorFromLifeRatio = theReader.ReadBool();
	theReader.Skip(4);
	theDef.mTextureIndex = theReader.ReadInt();

	int aColorCount = theReader.ReadShort();
	for (int i = 0; i < aColorCount && !theReader.mFailed; i++) {
		int aRed = theReader.ReadByte();
		int aGreen = theReader.ReadByte();
		int aBlue = theReader.ReadByte();
		PIParticleDef::ColorPoint aPoint;
		aPoint.mColor = 0xFF000000u | ((unsigned int)aRed << 16) | ((unsigned int)aGreen << 8) | (unsigned int)aBlue;
		aPoint.mTime = theReader.ReadFloat();
		theDef.mColorPoints.push_back(aPoint);
	}
	int anAlphaCount = theReader.ReadShort();
	for (int i = 0; i < anAlphaCount && !theReader.mFailed; i++) {
		PIParticleDef::AlphaPoint aPoint;
		aPoint.mAlpha = theReader.ReadByte();
		aPoint.mTime = theReader.ReadFloat();
		theDef.mAlphaPoints.push_back(aPoint);
	}

	PIValue* aValues[23] = {
		&theDef.mLife, &theDef.mNumber, &theDef.mSizeX, &theDef.mVelocity, &theDef.mWeight, &theDef.mSpin,
		&theDef.mMotionRandomness, &theDef.mBounce, &theDef.mLifeVariation, &theDef.mNumberVariation,
		&theDef.mSizeXVariation, &theDef.mVelocityVariation, &theDef.mWeightVariation, &theDef.mSpinVariation,
		&theDef.mMotionRandomnessVariation, &theDef.mBounceVariation, &theDef.mSizeXOverLife,
		&theDef.mVelocityOverLife, &theDef.mWeightOverLife, &theDef.mSpinOverLife, &theDef.mMotionRandomnessOverLife,
		&theDef.mBounceOverLife, &theDef.mVisibility};
	for (int i = 0; i < 23; i++)
		theReader.ReadValue(*aValues[i]);

	theDef.mOriginX = (double)theReader.ReadFloat();
	theDef.mOriginY = (double)theReader.ReadFloat();
	theReader.Skip(8);
	theDef.mSizeYFollowsX = theReader.ReadBool();
	theReader.ReadValue(theDef.mSizeY);
	theReader.ReadValue(theDef.mSizeYVariation);
	theReader.ReadValue(theDef.mSizeYOverLife);
	theDef.mRandomAngleRange = theReader.ReadInt();
	theDef.mRandomAngleBase = theReader.ReadInt();
	theDef.mUseInstanceColor = theReader.ReadBool();
	theDef.mInheritParentTint = theReader.ReadBool();
	theDef.mAngleFromEmitter = theReader.ReadBool();
	theReader.ReadValue(theDef.mAngle);
	theReader.ReadValue(theDef.mAngleVariation);
	theReader.Skip(4);
	PIValue aDiscarded;
	theReader.ReadValue(aDiscarded);
	theDef.mColorStepped = theReader.ReadBool();
	theDef.mTintWithLayerColor = theReader.ReadBool();
	theDef.mColorByIndex = theReader.ReadBool();
	theDef.mColorIndexDivisor = theReader.ReadInt();
	theDef.mUseInstanceAlpha = theReader.ReadBool();
}

// 0x568d80 (bejeweled3): an emitter
void ReadEmitter(PPFReader& theReader, PIEmitter& theEmitter)
{
	theReader.Skip(4);
	theEmitter.mName = theReader.ReadString();
	theEmitter.mIsSuperEmitter = theReader.ReadBool();
	theReader.Skip(4);
	theEmitter.mOldestInFront = theReader.ReadBool();

	int aDefCount = theReader.ReadShort();
	for (int i = 0; i < aDefCount && !theReader.mFailed; i++) {
		PIParticleDef aDef;
		ReadParticleDef(theReader, aDef);
		if (theEmitter.mOldestInFront)
			theEmitter.mParticleDefs.insert(theEmitter.mParticleDefs.begin(), aDef);
		else
			theEmitter.mParticleDefs.push_back(aDef);
	}

	theReader.Skip(4);
	PIValue* aValues[42] = {
		&theEmitter.mLife, &theEmitter.mNumber, &theEmitter.mVelocity, &theEmitter.mWeight, &theEmitter.mSpin,
		&theEmitter.mMotionRandomness, &theEmitter.mBounce, &theEmitter.mValueA, &theEmitter.mLifeScale,
		&theEmitter.mNumberScale, &theEmitter.mSizeXScale, &theEmitter.mSizeYScale, &theEmitter.mVelocityScale,
		&theEmitter.mWeightScale, &theEmitter.mSpinScale, &theEmitter.mMotionRandomnessScale,
		&theEmitter.mBounceScale, &theEmitter.mScale, &theEmitter.mAlphaFactor, &theEmitter.mValueB,
		&theEmitter.mTintFactor, &theEmitter.mAngle, &theEmitter.mAngleVariation, &theEmitter.mLifeVariation,
		&theEmitter.mValueC, &theEmitter.mSizeXVariation, &theEmitter.mSizeYVariation, &theEmitter.mVelocityVariation,
		&theEmitter.mWeightVariation, &theEmitter.mSpinVariation, &theEmitter.mMotionRandomnessVariation,
		&theEmitter.mBounceVariation, &theEmitter.mValueD, &theEmitter.mNumberOverLife, &theEmitter.mSizeOverLife,
		&theEmitter.mValueE, &theEmitter.mVelocityOverLife, &theEmitter.mWeightOverLife, &theEmitter.mSpinOverLife,
		&theEmitter.mMotionRandomnessOverLife, &theEmitter.mBounceOverLife, &theEmitter.mScaleOverLife};
	for (int i = 0; i < 42; i++)
		theReader.ReadValue(*aValues[i]);
	theEmitter.mHasLife = !theEmitter.mLife.mKeys.empty();
	theReader.Skip(8);
}

// 0x56aac0 (bejeweled3): an emitter instance of a layer
void ReadEmitterInstance(PPFReader& theReader, PIEmitterInstanceDef& theInstance)
{
	theReader.Skip(56);
	theInstance.mStartOffset = theReader.ReadInt();
	theReader.Skip(4);
	theInstance.mName = theReader.ReadString();
	theInstance.mShape = theReader.ReadInt();
	theReader.Skip(8);
	bool aCircleFlag = theReader.ReadBool();
	if (aCircleFlag && theInstance.mShape == 2)
		theInstance.mShape = 4;
	theInstance.mFlagA = theReader.ReadBool();
	theInstance.mFlagB = theReader.ReadBool();

	// the colour: three ints, of which only the low byte is used
	int aRed = theReader.ReadByte();
	theReader.Skip(3);
	int aGreen = theReader.ReadByte();
	theReader.Skip(3);
	int aBlue = theReader.ReadByte();
	theReader.Skip(3);
	theInstance.mColor = Color(aRed, aGreen, aBlue, 255);

	theReader.Skip(4);
	theInstance.mShapeCount = theReader.ReadInt();
	theInstance.mEmitterIndex = theReader.ReadInt();
	theReader.ReadValue2D(theInstance.mPosition);
	theReader.ReadPointList(theInstance.mPoints);

	PIValue* aValues[17] = {
		&theInstance.mLifeScale, &theInstance.mNumberScale, &theInstance.mSizeXScale, &theInstance.mVelocityScale,
		&theInstance.mWeightScale, &theInstance.mSpinScale, &theInstance.mMotionRandomnessScale,
		&theInstance.mBounceScale, &theInstance.mScale, &theInstance.mTint, &theInstance.mAlpha, &theInstance.mAngle,
		&theInstance.mAngleVariation, &theInstance.mValueG, &theInstance.mRotation, &theInstance.mShapeWidth,
		&theInstance.mShapeHeight};
	for (int i = 0; i < 17; i++)
		theReader.ReadValue(*aValues[i]);

	theInstance.mShapeRows = theReader.ReadInt();
	theReader.Skip(4);
	theReader.ReadValue(theInstance.mSizeYScale);
	theReader.Skip(4);
	theReader.ReadValue(theInstance.mValueH);

	int aStringCount = theReader.ReadShort();
	for (int i = 0; i < aStringCount && !theReader.mFailed; i++)
		theReader.ReadString();

	bool aHasMask = theReader.ReadBool();
	std::string aMaskName = theReader.ReadString();
	if (aHasMask)
		theInstance.mMaskImageName = aMaskName;

	theReader.Skip(8);
	theInstance.mFlagC = theReader.ReadBool();
	theReader.Skip(8);
	theInstance.mHasFreeEmitters = theReader.ReadBool();

	int aFreeCount = theReader.ReadShort();
	for (int i = 0; i < aFreeCount && !theReader.mFailed; i++)
		theInstance.mFreeEmitterIndices.push_back(theReader.ReadShort());
	theReader.Skip(12);
}

void ReadDeflector(PPFReader& theReader, PIDeflector& theDeflector)
{
	theDeflector.mName = theReader.ReadString();
	theDeflector.mBounce = theReader.ReadFloat();
	theDeflector.mHits = theReader.ReadFloat();
	theDeflector.mAllowedDirection = theReader.ReadFloat();
	theDeflector.mFlagA = theReader.ReadBool();
	theReader.ReadValue2D(theDeflector.mPosition);
	theReader.ReadPointList(theDeflector.mPoints);
	theReader.ReadValue(theDeflector.mActive);
	theReader.ReadValue(theDeflector.mAngle);
}

void ReadBlocker(PPFReader& theReader, PIBlocker& theBlocker)
{
	theBlocker.mName = theReader.ReadString();
	theReader.Skip(20);
	theReader.ReadValue2D(theBlocker.mPosition);
	theReader.ReadPointList(theBlocker.mPoints);
	theReader.ReadValue(theBlocker.mActive);
	theReader.ReadValue(theBlocker.mScale);
}

void ReadForce(PPFReader& theReader, PIForce& theForce)
{
	theForce.mName = theReader.ReadString();
	theForce.mFlagA = theReader.ReadBool();
	theReader.ReadValue2D(theForce.mPosition);
	for (int i = 0; i < 7; i++)
		theReader.ReadValue(theForce.mCurves[i]);
}

}   // namespace

PIEffect::PIEffect()
	: mWidth(0), mHeight(0), mBackgroundColor(0, 0, 0, 255), mFramerate(0), mFirstFrameNum(0), mLastFrameNum(0),
	  mVersion(0), mBytesRead(0), mCachedStateSize(0)
{
}

PIEffect::~PIEffect()
{
}

// 0x5679b0 (bejeweled3): without a game around there are no images to find
bool PIEffect::LoadTextureImage(PITexture&)
{
	return true;
}

// 0x56aac0 (bejeweled3)
void PIEffect::LoadEffect(const std::string& theFileName)
{
	PFILE* aStream = p_fopen(theFileName.c_str(), "rb");
	if (aStream == nullptr) {
		mError = "Unable to open the file";
		return;
	}
	p_fseek(aStream, 0, SEEK_END);
	int aSize = p_ftell(aStream);
	p_fseek(aStream, 0, SEEK_SET);
	std::vector<unsigned char> aData(aSize > 0 ? aSize : 0);
	size_t aRead = aData.empty() ? 0 : p_fread(aData.data(), 1, aSize, aStream);
	p_fclose(aStream);
	aData.resize(aRead);
	Load(aData.data(), aData.size(), theFileName);
}

// 0x56aac0 (bejeweled3)
bool PIEffect::Load(const unsigned char* theData, size_t theSize, const std::string& theFileName)
{
	mError.clear();
	mDef = PIEffectDef();
	mBytesRead = 0;
	mCachedStateSize = 0;

	// the ".ppf" test is a case-blind search of the name for ".ppf"
	std::string aLowerName = theFileName;
	for (size_t i = 0; i < aLowerName.size(); i++) {
		if (aLowerName[i] >= 'A' && aLowerName[i] <= 'Z')
			aLowerName[i] = (char)(aLowerName[i] - 'A' + 'a');
	}
	if (aLowerName.find(".ppf") == std::string::npos) {
		mError = "Only .ppf files are read";
		return false;
	}

	PPFReader aReader(theData, theSize);
	std::string aHeader = aReader.ReadString();
	mVersion = aReader.ReadInt();
	if (mVersion < 0) {
		mError = "PPF version too old";
		return false;
	}
	mHeaderText = aReader.ReadString();

	int aTextureCount = aReader.ReadShort();
	for (int i = 0; i < aTextureCount && !aReader.mFailed; i++) {
		PITexture aTexture;
		aTexture.mName = aReader.ReadString();
		aTexture.mFrameCount = aReader.ReadShort();
		aTexture.mColumnCount = aReader.ReadShort();
		aTexture.mHasMargin = aReader.ReadBool();
		aTexture.mImageId = aReader.ReadString();
		if (!aReader.mFailed && !LoadTextureImage(aTexture)) {
			mError = "Unable to load image: " + aTexture.mImageId;
			return false;
		}
		mDef.mTextures.push_back(aTexture);
	}

	int anEmitterCount = aReader.ReadShort();
	for (int i = 0; i < anEmitterCount && !aReader.mFailed; i++) {
		PIEmitter anEmitter;
		ReadEmitter(aReader, anEmitter);
		mDef.mEmitters.push_back(anEmitter);
	}

	int aLayerCount = aReader.ReadShort();
	for (int i = 0; i < aLayerCount && !aReader.mFailed; i++) {
		PILayerDef aLayer;
		aLayer.mName = aReader.ReadString();

		int anInstanceCount = aReader.ReadShort();
		for (int j = 0; j < anInstanceCount && !aReader.mFailed; j++) {
			PIEmitterInstanceDef anInstance;
			ReadEmitterInstance(aReader, anInstance);
			aLayer.mEmitterInstances.push_back(anInstance);
		}

		int aDeflectorCount = aReader.ReadShort();
		for (int j = 0; j < aDeflectorCount && !aReader.mFailed; j++) {
			PIDeflector aDeflector;
			ReadDeflector(aReader, aDeflector);
			aLayer.mDeflectors.push_back(aDeflector);
		}

		int aBlockerCount = aReader.ReadShort();
		for (int j = 0; j < aBlockerCount && !aReader.mFailed; j++) {
			PIBlocker aBlocker;
			ReadBlocker(aReader, aBlocker);
			aLayer.mBlockers.push_back(aBlocker);
		}

		aReader.ReadValue2D(aLayer.mPosition);
		PIPoint anOrigin = aLayer.mPosition.GetValueAt(0.0f);
		aLayer.mOriginX = anOrigin.mX;
		aLayer.mOriginY = anOrigin.mY;
		aReader.ReadValue(aLayer.mAngle);
		aReader.ReadString();
		aReader.Skip(32);
		int aStringCount = aReader.ReadShort();
		for (int j = 0; j < aStringCount && !aReader.mFailed; j++)
			aReader.ReadString();
		aReader.Skip(36);

		int aForceCount = aReader.ReadShort();
		for (int j = 0; j < aForceCount && !aReader.mFailed; j++) {
			PIForce aForce;
			ReadForce(aReader, aForce);
			aLayer.mForces.push_back(aForce);
		}
		aReader.Skip(28);
		mDef.mLayers.push_back(aLayer);
	}

	// the tail: the picture's colour, size and frame range
	int aRed = aReader.ReadByte();
	aReader.Skip(3);
	int aGreen = aReader.ReadByte();
	aReader.Skip(3);
	int aBlue = aReader.ReadByte();
	aReader.Skip(3);
	mBackgroundColor = Color(aRed, aGreen, aBlue, 255);
	aReader.Skip(8);
	mFramerate = aReader.ReadShort();
	aReader.Skip(6);
	mWidth = aReader.ReadInt();
	mHeight = aReader.ReadInt();
	aReader.Skip(20);
	mFirstFrameNum = aReader.ReadInt();
	mLastFrameNum = aReader.ReadInt();
	aReader.ReadString();
	aReader.Skip(5);
	if (mVersion >= 1)
		mCachedStateSize = aReader.ReadInt();
	if (mCachedStateSize > 0)
		aReader.Skip(mCachedStateSize);

	mBytesRead = aReader.mPosition;
	if (aReader.mFailed) {
		mError = "The file ends too early";
		return false;
	}
	return true;
}

// 0x567d00 (bejeweled3)
float PIEffect::GetRandFloat()
{
	unsigned int aRemainder = (unsigned int)mRand.NextNoAssert() % 20000000u;
	return (float)((double)(int)aRemainder / 10000000.0 - 1.0);
}

// 0x567d40 (bejeweled3)
float PIEffect::GetRandFloatU()
{
	unsigned int aRemainder = (unsigned int)mRand.NextNoAssert() % 10000000u;
	return (float)((double)(int)aRemainder / 10000000.0);
}

// 0x567d70 (bejeweled3)
float PIEffect::GetRandSign()
{
	return (mRand.NextNoAssert() & 1) == 0 ? 1.0f : -1.0f;
}

// 0x567d90 (bejeweled3)
float PIEffect::GetVariationScalar()
{
	double aFirst = (double)GetRandFloat();
	double aSecond = (double)GetRandFloat();
	return (float)(aSecond * aFirst);
}

// port: not translated yet (see games/bejeweled3/docs/NOTES.md, "Particles")
// 0x573290 (bejeweled3)
void PIEffect::Update()
{
}

// port: not translated yet (see games/bejeweled3/docs/NOTES.md, "Particles")
// 0x575650 (bejeweled3)
void PIEffect::Draw(Graphics* /*g*/)
{
}

// port: not translated yet (see games/bejeweled3/docs/NOTES.md, "Particles")
// 0x56f730 (bejeweled3)
void PIEffect::ResetAnim()
{
}

// port: not translated yet (see games/bejeweled3/docs/NOTES.md, "Particles")
// 0x575790 (bejeweled3)
bool PIEffect::IsActive()
{
	return false;
}

}
