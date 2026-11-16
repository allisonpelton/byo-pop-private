// b3_pieffecttest: headless test of engines/prime's particle effect (PIEffect). Loads every .ppf file under
// assets/bejeweled3 (or the files and folders given), without images, checks that each file is read to its last byte,
// prints the counts of what each holds and a hash of every value in it, and ends with one summary line.
//
//   b3_pieffecttest [-v] [path ...]
//
// The update and draw of the effect are not translated yet, so no particles run here; the test draws the effect's
// random numbers with a fixed seed instead, to check the generator.
#include "PIEffect.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <SDL.h>  // main becomes SDL_main, which the platform's SDL2main calls

namespace fs = std::filesystem;
using namespace Sexy;

namespace {

// FNV-1a over the bytes of what is added
class Hasher {
public:
	uint64_t mHash;
	Hasher() : mHash(1469598103934665603ULL) {}

	void Add(const void* theBytes, size_t theCount)
	{
		const unsigned char* aBytes = (const unsigned char*)theBytes;
		for (size_t i = 0; i < theCount; i++) {
			mHash ^= aBytes[i];
			mHash *= 1099511628211ULL;
		}
	}
	void AddInt(int theValue) { Add(&theValue, sizeof(theValue)); }
	void AddFloat(float theValue) { Add(&theValue, sizeof(theValue)); }
	void AddDouble(double theValue) { Add(&theValue, sizeof(theValue)); }
	void AddString(const std::string& theValue)
	{
		AddInt((int)theValue.size());
		Add(theValue.data(), theValue.size());
	}

	void AddValue(const PIValue& theValue)
	{
		AddInt((int)theValue.mKeys.size());
		for (size_t i = 0; i < theValue.mKeys.size(); i++) {
			AddFloat(theValue.mKeys[i].mTime);
			AddFloat(theValue.mKeys[i].mValue);
		}
		AddInt(theValue.mBezier.mPointCount);
	}

	void AddValue2D(const PIValue2D& theValue)
	{
		AddInt((int)theValue.mKeys.size());
		for (size_t i = 0; i < theValue.mKeys.size(); i++) {
			AddFloat(theValue.mKeys[i].mTime);
			AddDouble(theValue.mKeys[i].mX);
			AddDouble(theValue.mKeys[i].mY);
		}
		AddInt(theValue.mBezier.mPointCount);
	}
};

// A hash of the definition's numbers: every value's keyframes, the flags and the names
uint64_t HashDefinition(const PIEffect& theEffect)
{
	Hasher aHasher;
	const PIEffectDef& aDef = theEffect.mDef;
	for (size_t i = 0; i < aDef.mTextures.size(); i++) {
		aHasher.AddString(aDef.mTextures[i].mName);
		aHasher.AddString(aDef.mTextures[i].mImageId);
		aHasher.AddInt(aDef.mTextures[i].mFrameCount);
		aHasher.AddInt(aDef.mTextures[i].mColumnCount);
	}
	for (size_t i = 0; i < aDef.mEmitters.size(); i++) {
		const PIEmitter& anEmitter = aDef.mEmitters[i];
		aHasher.AddString(anEmitter.mName);
		aHasher.AddValue(anEmitter.mLife);
		aHasher.AddValue(anEmitter.mNumber);
		aHasher.AddValue(anEmitter.mVelocity);
		aHasher.AddValue(anEmitter.mAngle);
		aHasher.AddValue(anEmitter.mScaleOverLife);
		for (size_t j = 0; j < anEmitter.mParticleDefs.size(); j++) {
			const PIParticleDef& aParticle = anEmitter.mParticleDefs[j];
			aHasher.AddString(aParticle.mName);
			aHasher.AddInt(aParticle.mTextureIndex);
			aHasher.AddValue(aParticle.mLife);
			aHasher.AddValue(aParticle.mNumber);
			aHasher.AddValue(aParticle.mSizeX);
			aHasher.AddValue(aParticle.mVisibility);
			aHasher.AddValue(aParticle.mAngle);
			aHasher.AddInt((int)aParticle.mColorPoints.size());
			for (size_t k = 0; k < aParticle.mColorPoints.size(); k++) {
				aHasher.AddInt((int)aParticle.mColorPoints[k].mColor);
				aHasher.AddFloat(aParticle.mColorPoints[k].mTime);
			}
			aHasher.AddInt((int)aParticle.mAlphaPoints.size());
			aHasher.AddDouble(aParticle.mOriginX);
			aHasher.AddDouble(aParticle.mOriginY);
		}
	}
	for (size_t i = 0; i < aDef.mLayers.size(); i++) {
		const PILayerDef& aLayer = aDef.mLayers[i];
		aHasher.AddString(aLayer.mName);
		aHasher.AddValue2D(aLayer.mPosition);
		aHasher.AddValue(aLayer.mAngle);
		aHasher.AddDouble(aLayer.mOriginX);
		aHasher.AddDouble(aLayer.mOriginY);
		for (size_t j = 0; j < aLayer.mEmitterInstances.size(); j++) {
			const PIEmitterInstanceDef& anInstance = aLayer.mEmitterInstances[j];
			aHasher.AddString(anInstance.mName);
			aHasher.AddInt(anInstance.mEmitterIndex);
			aHasher.AddInt(anInstance.mShape);
			aHasher.AddInt(anInstance.mShapeCount);
			aHasher.AddValue2D(anInstance.mPosition);
			aHasher.AddValue(anInstance.mScale);
			aHasher.AddValue(anInstance.mAlpha);
			aHasher.AddValue(anInstance.mRotation);
			aHasher.AddInt(anInstance.mColor.mRed);
			aHasher.AddInt(anInstance.mColor.mGreen);
			aHasher.AddInt(anInstance.mColor.mBlue);
			aHasher.AddInt((int)anInstance.mPoints.size());
			aHasher.AddInt((int)anInstance.mFreeEmitterIndices.size());
		}
		aHasher.AddInt((int)aLayer.mDeflectors.size());
		aHasher.AddInt((int)aLayer.mBlockers.size());
		aHasher.AddInt((int)aLayer.mForces.size());
	}
	aHasher.AddInt(theEffect.mWidth);
	aHasher.AddInt(theEffect.mHeight);
	aHasher.AddInt(theEffect.mFramerate);
	aHasher.AddInt(theEffect.mFirstFrameNum);
	aHasher.AddInt(theEffect.mLastFrameNum);
	return aHasher.mHash;
}

// A hash of the values the effect's curves give over their spans, so the curve code is part of the check
uint64_t HashCurves(PIEffect& theEffect)
{
	Hasher aHasher;
	PIEffectDef& aDef = theEffect.mDef;
	for (size_t i = 0; i < aDef.mEmitters.size(); i++) {
		PIEmitter& anEmitter = aDef.mEmitters[i];
		PIValue* aValues[4] = {&anEmitter.mLife, &anEmitter.mNumber, &anEmitter.mVelocity, &anEmitter.mScaleOverLife};
		for (int j = 0; j < 4; j++) {
			for (int aStep = 0; aStep <= 20; aStep++)
				aHasher.AddFloat(aValues[j]->GetValueAt((float)aStep * 0.05f * (float)(theEffect.mLastFrameNum + 1)));
		}
		for (size_t j = 0; j < anEmitter.mParticleDefs.size(); j++) {
			PIParticleDef& aParticle = anEmitter.mParticleDefs[j];
			for (int aStep = 0; aStep <= 20; aStep++) {
				float aTime = (float)aStep * 0.05f;
				aHasher.AddFloat(aParticle.mSizeXOverLife.GetValueAt(aTime));
				aHasher.AddFloat(aParticle.mVisibility.GetValueAt(aTime, 1.0f));
			}
		}
	}
	for (size_t i = 0; i < aDef.mLayers.size(); i++) {
		PILayerDef& aLayer = aDef.mLayers[i];
		for (int aStep = 0; aStep <= 20; aStep++) {
			float aTime = (float)aStep * 0.05f * (float)(theEffect.mLastFrameNum + 1);
			PIPoint aPosition = aLayer.mPosition.GetValueAt(aTime);
			aHasher.AddDouble(aPosition.mX);
			aHasher.AddDouble(aPosition.mY);
			PIPoint aVelocity = aLayer.mPosition.GetVelocityAt(aTime);
			aHasher.AddDouble(aVelocity.mX);
			aHasher.AddDouble(aVelocity.mY);
			aHasher.AddFloat(aLayer.mAngle.GetValueAt(aTime));
		}
	}
	return aHasher.mHash;
}

void CollectFiles(const fs::path& thePath, std::vector<fs::path>& theFiles)
{
	std::error_code anError;
	if (fs::is_directory(thePath, anError)) {
		for (fs::recursive_directory_iterator anIterator(thePath, anError), anEnd; anIterator != anEnd; anIterator.increment(anError)) {
			std::string anExtension = anIterator->path().extension().string();
			for (size_t i = 0; i < anExtension.size(); i++) {
				if (anExtension[i] >= 'A' && anExtension[i] <= 'Z')
					anExtension[i] = (char)(anExtension[i] - 'A' + 'a');
			}
			if (anIterator->is_regular_file() && anExtension == ".ppf")
				theFiles.push_back(anIterator->path());
		}
	}
	else if (fs::is_regular_file(thePath, anError)) {
		theFiles.push_back(thePath);
	}
}

}   // namespace

// port: the test's entry point
int main(int argc, char** argv)
{
	bool aVerbose = false;
	std::vector<fs::path> aFiles;
	for (int i = 1; i < argc; i++) {
		if (std::strcmp(argv[i], "-v") == 0)
			aVerbose = true;
		else
			CollectFiles(argv[i], aFiles);
	}
	if (aFiles.empty())
		CollectFiles("assets/bejeweled3", aFiles);

	int aLoaded = 0;
	int aFailed = 0;
	int anInexact = 0;
	int aCached = 0;
	size_t anEmitterCount = 0;
	size_t aLayerCount = 0;
	size_t anInstanceCount = 0;
	size_t aParticleDefCount = 0;
	Hasher aTotalHash;

	for (size_t aFileIndex = 0; aFileIndex < aFiles.size(); aFileIndex++) {
		std::ifstream aStream(aFiles[aFileIndex], std::ios::binary);
		std::vector<unsigned char> aData((std::istreambuf_iterator<char>(aStream)), std::istreambuf_iterator<char>());

		PIEffect anEffect;
		anEffect.mRand.SRand(1234UL);
		if (!anEffect.Load(aData.data(), aData.size(), aFiles[aFileIndex].string())) {
			aFailed++;
			std::printf("FAIL %s: %s (read %zu of %zu)\n", aFiles[aFileIndex].string().c_str(), anEffect.mError.c_str(),
			            anEffect.mBytesRead, aData.size());
			continue;
		}
		aLoaded++;
		if (anEffect.mBytesRead != aData.size()) {
			anInexact++;
			std::printf("INEXACT %s: read %zu of %zu bytes\n", aFiles[aFileIndex].string().c_str(), anEffect.mBytesRead,
			            aData.size());
		}
		if (anEffect.mCachedStateSize > 0)
			aCached++;

		size_t anInstances = 0;
		size_t aParticleDefs = 0;
		for (size_t i = 0; i < anEffect.mDef.mLayers.size(); i++)
			anInstances += anEffect.mDef.mLayers[i].mEmitterInstances.size();
		for (size_t i = 0; i < anEffect.mDef.mEmitters.size(); i++)
			aParticleDefs += anEffect.mDef.mEmitters[i].mParticleDefs.size();
		anEmitterCount += anEffect.mDef.mEmitters.size();
		aLayerCount += anEffect.mDef.mLayers.size();
		anInstanceCount += anInstances;
		aParticleDefCount += aParticleDefs;

		uint64_t aDefinitionHash = HashDefinition(anEffect);
		uint64_t aCurveHash = HashCurves(anEffect);
		float aDraws[4] = {anEffect.GetRandFloat(), anEffect.GetRandFloatU(), anEffect.GetRandSign(), anEffect.GetVariationScalar()};
		aTotalHash.Add(&aDefinitionHash, sizeof(aDefinitionHash));
		aTotalHash.Add(&aCurveHash, sizeof(aCurveHash));
		aTotalHash.Add(aDraws, sizeof(aDraws));

		if (aVerbose) {
			std::printf("%s: v%d %zu/%zu bytes, %zu textures, %zu emitters (%zu particle defs), %zu layers (%zu emitter instances), "
			            "%dx%d, %d fps, frames %d..%d, hash %016llx curves %016llx\n",
			            aFiles[aFileIndex].string().c_str(), anEffect.mVersion, anEffect.mBytesRead, aData.size(),
			            anEffect.mDef.mTextures.size(), anEffect.mDef.mEmitters.size(), aParticleDefs, anEffect.mDef.mLayers.size(),
			            anInstances, anEffect.mWidth, anEffect.mHeight, anEffect.mFramerate, anEffect.mFirstFrameNum,
			            anEffect.mLastFrameNum, (unsigned long long)aDefinitionHash, (unsigned long long)aCurveHash);
		}
	}

	std::printf("pieffecttest: %zu files, %d loaded, %d failed, %d not read exactly, %d with a cached state; %zu emitters, "
	            "%zu particle defs, %zu layers, %zu emitter instances; hash %016llx\n",
	            aFiles.size(), aLoaded, aFailed, anInexact, aCached, anEmitterCount, aParticleDefCount, aLayerCount,
	            anInstanceCount, (unsigned long long)aTotalHash.mHash);
	return aFailed == 0 && anInexact == 0 ? 0 : 1;
}
