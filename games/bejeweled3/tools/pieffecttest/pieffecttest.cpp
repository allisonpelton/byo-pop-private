// b3_pieffecttest: headless test of engines/prime's particle effect (PIEffect). Loads every .ppf file under
// assets/bejeweled3 (or the files and folders given), without images, checks that each file is read to its last byte,
// prints the counts of what each holds and a hash of every value in it, and ends with one summary line.
//
//   b3_pieffecttest [-v] [path ...]
//
// It then plays each effect with a fixed seed (a cached state first, for the files that have one, which must read to
// its last byte), hashing the particle counts every update and the particles' places every few. Then it plays each
// effect again with pictures of its own making (the game's images are not needed) and draws it every few updates onto a
// software image, hashing the pixels (and, for every other file, with a color that is not white).
#include "DDImage.h"
#include "DDInterface.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "PIEffect.h"
#include "SDLSoundManager.h"
#include "SWTri.h"
#include "SexyAppBase.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
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

// the particles of one group, in order: where each is, how old, and which of the definition's it is
void HashGroup(Hasher& theHasher, const PIParticleGroup& theGroup)
{
	theHasher.Add(&theGroup.mCount, sizeof(theGroup.mCount));
	for (const PIParticleInstance* aParticle = theGroup.mHead; aParticle != NULL; aParticle = aParticle->mNext) {
		theHasher.Add(&aParticle->mPositionX, sizeof(double));
		theHasher.Add(&aParticle->mPositionY, sizeof(double));
		theHasher.Add(&aParticle->mAge, sizeof(float));
		theHasher.Add(&aParticle->mAngle, sizeof(float));
		theHasher.Add(&aParticle->mIndex, sizeof(int));
		theHasher.Add(&aParticle->mTransform.m02, sizeof(float));
		theHasher.Add(&aParticle->mTransform.m12, sizeof(float));
		const PIFreeEmitterInstance* aFreeEmitter = dynamic_cast<const PIFreeEmitterInstance*>(aParticle);
		if (aFreeEmitter != NULL && theGroup.mIsFreeEmitterList)
			HashGroup(theHasher, aFreeEmitter->mParticles);
	}
}

// every particle the effect has now
void HashParticles(Hasher& theHasher, PIEffect& theEffect)
{
	for (size_t i = 0; i < theEffect.mLayers.size(); i++) {
		PILayer* aLayer = theEffect.mLayers[i];
		for (size_t j = 0; j < aLayer->mEmitterInstances.size(); j++) {
			HashGroup(theHasher, aLayer->mEmitterInstances[j]->mFreeEmitters);
			HashGroup(theHasher, aLayer->mEmitterInstances[j]->mParticles);
		}
	}
}

// an effect whose textures are made here: a picture with a different color at every pixel, so a wrong cell, turn or
// color shows in the hash
class DrawnEffect : public PIEffect {
public:
	SharedImageRef LoadTextureImage(const std::string& theImageName) override
	{
		static std::map<std::string, SharedImage*> sPictures;   // kept for the whole run
		SharedImage*& aShared = sPictures[theImageName];
		if (aShared == NULL) {
			DDImage* aPicture = new DDImage(gSexyAppBase->mDDInterface);
			aPicture->Create(kPictureSize, kPictureSize);
			unsigned long* aBits = aPicture->GetBits();
			for (int y = 0; y < kPictureSize; y++) {
				for (int x = 0; x < kPictureSize; x++) {
					unsigned long anAlpha = (unsigned long)(((x - kPictureSize / 2) * (x - kPictureSize / 2) +
					                                         (y - kPictureSize / 2) * (y - kPictureSize / 2)) < 1000 ? 255 : 90);
					aBits[y * kPictureSize + x] = (anAlpha << 24) | ((unsigned long)(x * 4) << 16) |
					                              ((unsigned long)(y * 4) << 8) | (unsigned long)((x ^ y) * 4 & 0xff);
				}
			}
			aPicture->BitsChanged();
			aShared = new SharedImage();
			aShared->mImage = aPicture;
		}
		return SharedImageRef(aShared);
	}
	static const int kPictureSize = 64;
};

const int kPlayUpdates = 300;     // how many updates each effect is played for
const int kHashEvery = 10;        // the particles' places are hashed this often
const int kCanvasSize = 512;      // the software image the draw pass draws onto

}   // namespace

// port: the test's entry point
int main(int argc, char** argv)
{
	std::setvbuf(stdout, NULL, _IONBF, 0);
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
	int aBadState = 0;
	size_t aBlockerCount = 0;
	size_t aShownDeflectors = 0;
	size_t aShownForces = 0;
	long long aUpdateCount = 0;
	int aPeakParticles = 0;
	Hasher aTotalHash;
	Hasher aPlayHash;
	int aDuplicates = 0, aCopyDiffers = 0;   // duplicates played, and the ones that played differently from the original
	Hasher aDrawHash;
	int aDrawFailed = 0;
	int aDrawCount = 0;
	long long aLitPixels = 0;
	int aAreaFrames = 0;

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
		for (size_t i = 0; i < anEffect.mDef.mLayers.size(); i++) {
			const PILayerDef& aLayerDef = anEffect.mDef.mLayers[i];
			aBlockerCount += aLayerDef.mBlockers.size();
			for (size_t j = 0; j < aLayerDef.mDeflectors.size(); j++)
				aShownDeflectors += aLayerDef.mDeflectors[j].mShown ? 1 : 0;
			for (size_t j = 0; j < aLayerDef.mForces.size(); j++)
				aShownForces += aLayerDef.mForces[j].mShown ? 1 : 0;
		}
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

		if (!anEffect.mCachedState.empty() && !anEffect.LoadCachedState()) {
			aBadState++;
			std::printf("BADSTATE %s: the cached state does not read to its end\n", aFiles[aFileIndex].string().c_str());
		}
		anEffect.ResetAnim();
		anEffect.mSeeds.assign(1, 1234);
		int aFilePeak = 0;
		// a duplicate (what an animation plays) is played beside it and must do the same (an effect with a cached
		// state is left out: the duplicate does not take that)
		PIEffect* aCopy = anEffect.mCachedState.empty() ? anEffect.Duplicate() : NULL;
		Hasher aCopyOriginalHash, aCopyHash;
		for (int anUpdate = 0; anUpdate < kPlayUpdates; anUpdate++) {
			anEffect.Update();
			aUpdateCount++;
			if (aCopy != NULL) {
				aCopy->Update();
				if (anUpdate % kHashEvery == kHashEvery - 1) {
					HashParticles(aCopyOriginalHash, anEffect);
					HashParticles(aCopyHash, *aCopy);
				}
				if (aCopy->mParticleCount != anEffect.mParticleCount || aCopy->mEmitterCount != anEffect.mEmitterCount)
					aCopyDiffers++;
			}
			int aCounts[2] = {anEffect.mParticleCount, anEffect.mEmitterCount};
			aPlayHash.Add(aCounts, sizeof(aCounts));
			if (anEffect.mParticleCount > aFilePeak)
				aFilePeak = anEffect.mParticleCount;
			if (anUpdate % kHashEvery == kHashEvery - 1)
				HashParticles(aPlayHash, anEffect);
		}
		if (aFilePeak > aPeakParticles)
			aPeakParticles = aFilePeak;
		if (aCopy != NULL) {
			aDuplicates++;
			if (aCopyOriginalHash.mHash != aCopyHash.mHash)
				aCopyDiffers++;
			delete aCopy;
		}

		if (aVerbose) {
			std::printf("%s: v%d %zu/%zu bytes, %zu textures, %zu emitters (%zu particle defs), %zu layers (%zu emitter instances), "
			            "%dx%d, %d fps, frames %d..%d, hash %016llx curves %016llx, at most %d particles\n",
			            aFiles[aFileIndex].string().c_str(), anEffect.mVersion, anEffect.mBytesRead, aData.size(),
			            anEffect.mDef.mTextures.size(), anEffect.mDef.mEmitters.size(), aParticleDefs, anEffect.mDef.mLayers.size(),
			            anInstances, anEffect.mWidth, anEffect.mHeight, anEffect.mFramerate, anEffect.mFirstFrameNum,
			            anEffect.mLastFrameNum, (unsigned long long)aDefinitionHash, (unsigned long long)aCurveHash, aFilePeak);
		}
	}

	// the draw pass: each effect again, with pictures made here, drawn every few updates onto a software image (which
	// needs an app, so it is made only now: the first pass must find no images)
	SDL_setenv("SDL_AUDIODRIVER", "dummy", 0);
	SexyAppBase* anApp = new SexyAppBase();
	anApp->mDDInterface = new DDInterface(anApp);
	anApp->mSoundManager = new SDLSoundManager();
	SWTri_AddAllDrawTriFuncs();   // the software drawer of a turned picture
	MemoryImage aCanvas;
	aCanvas.Create(kCanvasSize, kCanvasSize);
	for (size_t aFileIndex = 0; aFileIndex < aFiles.size(); aFileIndex++) {
		std::ifstream aStream(aFiles[aFileIndex], std::ios::binary);
		std::vector<unsigned char> aData((std::istreambuf_iterator<char>(aStream)), std::istreambuf_iterator<char>());
		if (aVerbose)
			std::printf("drawing %s\n", aFiles[aFileIndex].string().c_str());
		{
			DrawnEffect aDrawn;
			aDrawn.mRand.SRand(1234UL);
			if (!aDrawn.Load(aData.data(), aData.size(), aFiles[aFileIndex].string())) {
				aDrawFailed++;
				continue;
			}
			if (!aDrawn.mCachedState.empty())
				aDrawn.LoadCachedState();
			aDrawn.ResetAnim();
			aDrawn.mSeeds.assign(1, 1234);
			aDrawn.mDrawTransform.m02 = (float)(kCanvasSize / 2);
			aDrawn.mDrawTransform.m12 = (float)(kCanvasSize / 2);
			if (aFileIndex % 2 == 1)
				aDrawn.mColor = Color(200, 255, 128, 220);
			for (int anUpdate = 0; anUpdate < kPlayUpdates; anUpdate++) {
				aDrawn.Update();
				if (anUpdate % kHashEvery != kHashEvery - 1)
					continue;
				unsigned long* aBits = aCanvas.GetBits();
				std::memset(aBits, 0, sizeof(unsigned long) * kCanvasSize * kCanvasSize);
				aCanvas.BitsChanged();
				Graphics aGraphics(&aCanvas);
				aDrawn.Draw(&aGraphics);
				aBits = aCanvas.GetBits();
				for (int i = 0; i < kCanvasSize * kCanvasSize; i++) {
					unsigned int aPixel = (unsigned int)aBits[i];
					aDrawHash.Add(&aPixel, sizeof(aPixel));
					if ((aPixel & 0xffffff) != 0)
						aLitPixels++;
				}
				aDrawHash.AddInt(aDrawn.mDrawArea);
				if (aDrawn.mDrawArea > 0)
					aAreaFrames++;
				aDrawCount++;
			}
		}

	}

	std::printf("pieffecttest: %zu files, %d loaded, %d failed, %d not read exactly, %d with a cached state; %zu emitters, "
	            "%zu particle defs, %zu layers, %zu emitter instances; hash %016llx\n",
	            aFiles.size(), aLoaded, aFailed, anInexact, aCached, anEmitterCount, aParticleDefCount, aLayerCount,
	            anInstanceCount, (unsigned long long)aTotalHash.mHash);
	std::printf("layers hold %zu blockers, %zu deflectors and %zu forces with their flag set\n", aBlockerCount, aShownDeflectors,
	            aShownForces);
	std::printf("played: %lld updates, %d cached states not read to their end, at most %d particles; hash %016llx\n",
	            aUpdateCount, aBadState, aPeakParticles, (unsigned long long)aPlayHash.mHash);
	std::printf("duplicates: %d played beside their originals, %d differed\n", aDuplicates, aCopyDiffers);
	std::printf("drawn: %d frames, %d effects not loaded with pictures, %lld lit pixels, %d with an area; hash %016llx\n", aDrawCount, aDrawFailed,
	            aLitPixels, aAreaFrames, (unsigned long long)aDrawHash.mHash);
	return aFailed == 0 && anInexact == 0 && aBadState == 0 && aDrawFailed == 0 && aCopyDiffers == 0 ? 0 : 1;
}
