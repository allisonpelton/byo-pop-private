// Sexy::PIEffect (PIEffect.h): LoadCachedState, which puts the effect into the state its file ends with (the particles
// and emitters as they are some frames in), so that the first Update does not have to run those frames.
#include "PIEffect.h"

#include "MTRand.h"

#include <cstring>

namespace Sexy {

namespace {

const int kMatrixBytes = 0x24;   // a SexyMatrix3 is nine floats

// The bytes of the cached state. The exe reads them with its Buffer class (0x54fe40 (bejeweled3) to 0x54ffc0 (bejeweled3)): all
// values little endian, a double as two words
class StateReader {
public:
	const unsigned char* mData;
	size_t mSize;
	size_t mPosition;
	bool mFailed;

	StateReader(const unsigned char* theData, size_t theSize) : mData(theData), mSize(theSize), mPosition(0), mFailed(false) {}

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

	// 0x54fe40 (bejeweled3)
	bool ReadBool()
	{
		const unsigned char* aBytes = GetData(1);
		return aBytes != NULL && aBytes[0] != 0;
	}

	// 0x54fe50 (bejeweled3)
	int ReadShort()
	{
		const unsigned char* aBytes = GetData(2);
		return aBytes != NULL ? (short)(aBytes[0] | (aBytes[1] << 8)) : 0;
	}

	// 0x54fe80 (bejeweled3)
	int ReadInt()
	{
		const unsigned char* aBytes = GetData(4);
		return aBytes != NULL ? (int)((unsigned int)aBytes[0] | ((unsigned int)aBytes[1] << 8) |
		                              ((unsigned int)aBytes[2] << 16) | ((unsigned int)aBytes[3] << 24)) : 0;
	}

	// 0x54fec0 (bejeweled3)
	float ReadFloat()
	{
		int aBits = ReadInt();
		float aValue;
		std::memcpy(&aValue, &aBits, sizeof(aValue));
		return aValue;
	}

	// 0x54ff00 (bejeweled3)
	double ReadDouble()
	{
		unsigned int aLow = (unsigned int)ReadInt();
		unsigned int aHigh = (unsigned int)ReadInt();
		unsigned long long aBits = ((unsigned long long)aHigh << 32) | aLow;
		double aValue;
		std::memcpy(&aValue, &aBits, sizeof(aValue));
		return aValue;
	}

	// 0x550030 (bejeweled3)
	PIPoint ReadPoint()
	{
		PIPoint aPoint;
		aPoint.mX = ReadDouble();
		aPoint.mY = ReadDouble();
		return aPoint;
	}

	// 0x54ffc0 (bejeweled3)
	SexyMatrix3 ReadMatrix()
	{
		SexyMatrix3 aMatrix;
		aMatrix.LoadIdentity();
		aMatrix.m00 = ReadFloat();
		aMatrix.m01 = ReadFloat();
		aMatrix.m02 = ReadFloat();
		aMatrix.m10 = ReadFloat();
		aMatrix.m11 = ReadFloat();
		aMatrix.m12 = ReadFloat();
		aMatrix.m20 = ReadFloat();
		aMatrix.m21 = ReadFloat();
		aMatrix.m22 = ReadFloat();
		return aMatrix;
	}
};

// 0x569d60 (bejeweled3)
void ReadDefInstance(StateReader& theReader, PIParticleDefInstance& theDefInstance)
{
	theDefInstance.mSpawnAccumulator = theReader.ReadFloat();
	theDefInstance.mNumberVariation = theReader.ReadFloat();
	theDefInstance.mParticleCount = theReader.ReadInt();
	theDefInstance.mUpdateCount = theReader.ReadInt();
}

// 0x569d90 (bejeweled3): one particle (the layer's deflectors make it keep a random value for them)
void ReadParticle(StateReader& theReader, MTRand& theRand, bool theLayerHasDeflectors, PIParticleInstance* theParticle)
{
	PIParticleDef* aParticleDef = theParticle->mParticleDef;
	theParticle->mAge = theReader.ReadFloat();
	theParticle->mLife = theReader.ReadFloat();
	theParticle->mLifeRatio = theReader.ReadFloat();
	theParticle->mScale = theReader.ReadFloat();
	PIPoint aPosition = theReader.ReadPoint();
	theParticle->mPositionX = aPosition.mX;
	theParticle->mPositionY = aPosition.mY;
	PIPoint aVelocity = theReader.ReadPoint();
	theParticle->mVelocityX = aVelocity.mX;
	theParticle->mVelocityY = aVelocity.mY;
	PIPoint anOrigin = theReader.ReadPoint();
	theParticle->mOriginX = anOrigin.mX;
	theParticle->mOriginY = anOrigin.mY;
	if (aParticleDef != NULL && aParticleDef->mAttachToEmitter) {
		PIPoint aSpawnOffset = theReader.ReadPoint();
		theParticle->mSpawnOffsetX = aSpawnOffset.mX;
		theParticle->mSpawnOffsetY = aSpawnOffset.mY;
		theParticle->mEmitterRotation = theReader.ReadFloat();
	}
	theParticle->mAngle = theReader.ReadFloat();

	// the nine variations: the file gives only those that are not zero
	float* aVariations[9] = {&theParticle->mLifeVariation, &theParticle->mSizeXVariation, &theParticle->mSizeYVariation,
	                         &theParticle->mVelocityVariation, &theParticle->mWeightVariation, &theParticle->mSpinVariation,
	                         &theParticle->mMotionRandomnessVariation, &theParticle->mBounceVariation,
	                         &theParticle->mFreeEmitterNumberVariation};
	int aPresent = theReader.ReadShort();
	for (int i = 0; i < 9; i++)
		*aVariations[i] = (aPresent & (1 << i)) != 0 ? theReader.ReadFloat() : 0.0f;

	theParticle->mSizeX = theReader.ReadFloat();
	theParticle->mSizeY = theReader.ReadFloat();
	if (aParticleDef != NULL && aParticleDef->mRandomColor)
		theParticle->mColorOffset = theReader.ReadFloat();
	if (aParticleDef != NULL && aParticleDef->mRandomStartFrame)
		theParticle->mRandomFrame = theReader.ReadShort();
	if (theLayerHasDeflectors)
		theParticle->mRandomValue = theReader.ReadFloat();
	// (the exe draws the random frame anew after reading it, which moves the generator on)
	if (aParticleDef != NULL && aParticleDef->mRandomStartFrame)
		theParticle->mRandomFrame = (int)(theRand.NextNoAssert() & 0x7fff);
	else
		theParticle->mRandomFrame = 0;
}

}

// 0x56e4f0 (bejeweled3): the exe's function also loads a state a game saved (the path SaveState 0x56eed0 (bejeweled3) writes, which is
// not translated); this is the path the cached state of the file takes
bool PIEffect::LoadCachedState()
{
	if (!mError.empty())
		return false;
	ResetAnim();

	StateReader aReader(mCachedState.data(), mCachedState.size());
	int aSize = aReader.ReadInt();
	size_t anEnd = aReader.mPosition + (size_t)aSize;
	int aFlag = aReader.ReadShort();
	mFrameNum = aReader.ReadFloat();
	if (aFlag == 0) {
		aReader.ReadBool();
		aReader.Skip(kMatrixBytes);
		aReader.Skip(kMatrixBytes);
	}

	if (mFrameNum <= 0.0f) {
		aReader.mPosition = anEnd;
		return !aReader.mFailed;
	}

	for (size_t aLayerIndex = 0; aLayerIndex < mDef.mLayers.size(); aLayerIndex++) {
		PILayerDef& aLayerDef = mDef.mLayers[aLayerIndex];
		PILayer* aLayer = mLayers[aLayerIndex];
		bool aLayerHasDeflectors = !aLayerDef.mDeflectors.empty();
		for (size_t anInstanceIndex = 0; anInstanceIndex < aLayerDef.mEmitterInstances.size(); anInstanceIndex++) {
			PIEmitterInstanceDef& anInstanceDef = aLayerDef.mEmitterInstances[anInstanceIndex];
			PIEmitterInstance* anInstance = aLayer->mEmitterInstances[anInstanceIndex];
			if (aReader.ReadBool())
				anInstance->mTransform = aReader.ReadMatrix();
			anInstance->mWasInRange = aReader.ReadBool();
			anInstance->mCanEmit = aReader.ReadBool();
			if (anInstanceDef.mEmitterIndex < 0 || anInstanceDef.mEmitterIndex >= (int)mDef.mEmitters.size())
				return false;
			PIEmitter* anEmitter = &mDef.mEmitters[anInstanceDef.mEmitterIndex];

			for (size_t i = 0; i < anEmitter->mParticleDefs.size(); i++)
				ReadDefInstance(aReader, anInstance->mDefInstances[i]);
			for (size_t i = 0; i < anInstanceDef.mFreeEmitterIndices.size(); i++)
				ReadDefInstance(aReader, anInstance->mFreeDefInstances[i]);

			// the free emitters, each with its own particles
			int aFreeEmitterCount = aReader.ReadInt();
			for (int aFreeIndex = 0; aFreeIndex < aFreeEmitterCount; aFreeIndex++) {
				int aKind = aReader.ReadShort();
				if (aReader.mFailed || aKind < 0 || aKind >= (int)anInstanceDef.mFreeEmitterIndices.size())
					return false;
				int anEmitterIndex = anInstanceDef.mFreeEmitterIndices[aKind];
				if (anEmitterIndex < 0 || anEmitterIndex >= (int)mDef.mEmitters.size())
					return false;
				PIFreeEmitterInstance* aFreeEmitter = new PIFreeEmitterInstance();
				aFreeEmitter->mEmitter = &mDef.mEmitters[anEmitterIndex];
				aFreeEmitter->mFreeEmitter = NULL;
				aFreeEmitter->mParticleDef = NULL;
				aFreeEmitter->mIndex = aFreeIndex;
				ReadParticle(aReader, mRand, aLayerHasDeflectors, aFreeEmitter);
				PIEmitter* aFreeEmitterDef = aFreeEmitter->mEmitter;
				aFreeEmitter->mDefInstances.assign(aFreeEmitterDef->mParticleDefs.size(), PIParticleDefInstance());
				for (size_t i = 0; i < aFreeEmitterDef->mParticleDefs.size(); i++)
					ReadDefInstance(aReader, aFreeEmitter->mDefInstances[i]);
				anInstance->mFreeEmitters.Append(aFreeEmitter);

				int aParticleCount = aReader.ReadInt();
				for (int aParticleIndex = 0; aParticleIndex < aParticleCount; aParticleIndex++) {
					PIParticleInstance* aParticle = new PIParticleInstance();
					aParticle->mEmitter = aFreeEmitter->mEmitter;
					aParticle->mFreeEmitter = aFreeEmitter;
					int aDefIndex = aReader.ReadShort();
					if (aReader.mFailed || aDefIndex < 0 || aDefIndex >= (int)aFreeEmitterDef->mParticleDefs.size()) {
						delete aParticle;
						return false;
					}
					aParticle->mParticleDef = &aFreeEmitterDef->mParticleDefs[aDefIndex];
					aParticle->mIndex = aParticleIndex;
					ReadParticle(aReader, mRand, aLayerHasDeflectors, aParticle);
					CalcParticleTransform(aLayer, anInstance, aParticle->mEmitter, aParticle->mParticleDef,
					                      &aFreeEmitter->mParticles, aParticle);
					aFreeEmitter->mParticles.Append(aParticle);
				}
			}

			// the instance's own particles
			int aParticleCount = aReader.ReadInt();
			for (int aParticleIndex = 0; aParticleIndex < aParticleCount; aParticleIndex++) {
				PIParticleInstance* aParticle = new PIParticleInstance();
				aParticle->mEmitter = anEmitter;
				aParticle->mFreeEmitter = NULL;
				int aDefIndex = aReader.ReadShort();
				if (aReader.mFailed || aDefIndex < 0 || aDefIndex >= (int)anEmitter->mParticleDefs.size()) {
					delete aParticle;
					return false;
				}
				aParticle->mParticleDef = &anEmitter->mParticleDefs[aDefIndex];
				aParticle->mIndex = aParticleIndex;
				ReadParticle(aReader, mRand, aLayerHasDeflectors, aParticle);
				CalcParticleTransform(aLayer, anInstance, anEmitter, aParticle->mParticleDef, &anInstance->mParticles,
				                      aParticle);
				anInstance->mParticles.Append(aParticle);
			}
		}
	}
	return !aReader.mFailed && aReader.mPosition == anEnd;
}

}
