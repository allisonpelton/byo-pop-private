// port: b3_pamtest loads Bejeweled 3's animations (the .pam files, versions 4 and 5) with engines/prime's PopAnim,
// headless:
//     build/native/b3_pamtest.exe [-gamedir=<folder>] [-artres=600|768|1200] [-v] [-play] [-draw]
// It finds every .pam under the game folder (assets/bejeweled3 by default), loads each as the resource manager's
// DoLoadPopAnim does (a PopAnim with no listener, its image scale and draw scale at the art size over the manifest's
// 1200, then LoadAnimation) and prints each failure with the player's error. -play also plays each animation (its
// main sprite or, in a file without one, each of its sprites by name) for a few hundred updates; -draw also draws
// every few updates onto a MemoryImage. The last line is "files N, loaded L, failed F, hash H": the hash covers the
// frame counts, the transforms, the image names and, with -play, the frame the animation is at after each update and
// with -draw the pixels drawn. The output names the game's files: it is for reading and goes into no file of the
// repository.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include <SDL.h>  // main becomes SDL_main, which the platform's SDL2main calls

#include "DDInterface.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "PIEffect.h"
#include "PopAnim.h"
#include "SDLSoundManager.h"
#include "SexyAppBase.h"
#include "SWTri.h"
#include "platform/VFS.h"
#include "platform/openjpeg/JP2Decoder.h"

using namespace Sexy;

namespace {

std::string gGameDir = "assets/bejeweled3";
bool gVerbose = false;
bool gPlay = false;
bool gDraw = false;
int gArtRes = 1200;
const int kBaseArtRes = 1200;  // the manifest's own
const int kPlayUpdates = 300;
const int kDrawEvery = 25;
const int kCanvasWidth = 640;
const int kCanvasHeight = 480;
long long gLitPixels = 0;  // pixels drawn that are not the black the canvas starts as

unsigned int HashInt(unsigned int theHash, int theValue)
{
	return (theHash ^ (unsigned int)theValue) * 16777619u;  // FNV-1a over whole values
}

unsigned int HashFloat(unsigned int theHash, float theValue)
{
	int aBits;
	memcpy(&aBits, &theValue, sizeof(aBits));
	return HashInt(theHash, aBits);
}

unsigned int HashString(unsigned int theHash, const std::string& theText)
{
	for (size_t i = 0; i < theText.size(); i++)
		theHash = HashInt(theHash, (unsigned char)theText[i]);
	return HashInt(theHash, (int)theText.size());
}

unsigned int HashMatrix(unsigned int theHash, const SexyMatrix3& theMatrix)
{
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++)
			theHash = HashFloat(theHash, theMatrix.m[i][j]);
	}
	return theHash;
}

// The game's app finds a picture named "file|ID" through its resource manager; here the file is all there is
class TestApp : public SexyAppBase {
public:
	SharedImageRef GetSharedImage(const std::string& theFileName, const std::string& theVariant = "",
	                              bool* isNew = NULL) override
	{
		size_t aBar = theFileName.find('|');
		return SexyAppBase::GetSharedImage(aBar == std::string::npos ? theFileName : theFileName.substr(0, aBar),
		                                   theVariant, isNew);
	}
};

// Counts what the animations tell the game
class TestListener : public PopAnimListener {
public:
	int mStops = 0;
	int mCommands = 0;
	int mSamples = 0;
	int mEffectCommands = 0;   // "addparticleeffect" commands the animation was told of

	void PopAnimStopped(int) override { mStops++; }
	bool PopAnimCommand(int, PASpriteInst*, const std::string& theCommand, const std::string&) override
	{
		mCommands++;
		if (_stricmp(theCommand.c_str(), "addparticleeffect") == 0)
			mEffectCommands++;
		return false;
	}
	void PopAnimPlaySample(int, const std::string&, double, double) override { mSamples++; }
};

TestListener gListener;

unsigned int HashSprite(unsigned int theHash, const PASpriteDef& theDef)
{
	theHash = HashString(theHash, theDef.mName);
	theHash = HashInt(HashInt(HashInt(theHash, (int)theDef.mFrames.size()), theDef.mWorkStart), theDef.mWorkLength);
	theHash = HashFloat(theHash, theDef.mFrameRate);
	theHash = HashInt(theHash, (int)theDef.mLabels.size());
	for (std::map<std::string, int, StringLessNoCase>::const_iterator anItr = theDef.mLabels.begin();
	     anItr != theDef.mLabels.end(); ++anItr)
		theHash = HashInt(HashString(theHash, anItr->first), anItr->second);
	for (size_t f = 0; f < theDef.mFrames.size(); f++) {
		const PAFrame& aFrame = theDef.mFrames[f];
		theHash = HashInt(HashInt(theHash, aFrame.mHasStop ? 1 : 0), (int)aFrame.mObjectPosVector.size());
		for (size_t o = 0; o < aFrame.mObjectPosVector.size(); o++) {
			const PAObjectPos& anObject = aFrame.mObjectPosVector[o];
			theHash = HashInt(HashInt(HashInt(theHash, anObject.mObjectNum), anObject.mResNum),
			                  (anObject.mIsSprite ? 1 : 0) | (anObject.mAdditive ? 2 : 0));
			theHash = HashInt(HashInt(theHash, anObject.mPreloadFrames), anObject.mImageFrame);
			theHash = HashMatrix(HashFloat(theHash, anObject.mTimeScale), anObject.mTransform);
			theHash = HashInt(HashInt(HashInt(HashInt(theHash, anObject.mColor.mRed), anObject.mColor.mGreen),
			                          anObject.mColor.mBlue), anObject.mColor.mAlpha);
			if (anObject.mHasSrcRect)
				theHash = HashInt(HashInt(HashInt(HashInt(theHash, anObject.mSrcRect.mX), anObject.mSrcRect.mY),
				                          anObject.mSrcRect.mWidth), anObject.mSrcRect.mHeight);
		}
		theHash = HashInt(theHash, (int)aFrame.mCommands.size());
		for (size_t c = 0; c < aFrame.mCommands.size(); c++)
			theHash = HashString(HashString(theHash, aFrame.mCommands[c].mCommand), aFrame.mCommands[c].mParam);
	}
	return theHash;
}

unsigned int HashLoaded(unsigned int theHash, const PopAnim& theAnim)
{
	theHash = HashInt(HashInt(theHash, theAnim.mVersion), theAnim.mFrameRate);
	theHash = HashInt(HashInt(HashInt(HashInt(theHash, theAnim.mPamX), theAnim.mPamY), theAnim.mPamWidth),
	                  theAnim.mPamHeight);
	theHash = HashInt(theHash, (int)theAnim.mImages.size());
	for (size_t i = 0; i < theAnim.mImages.size(); i++) {
		const PAImage& anEntry = theAnim.mImages[i];
		theHash = HashInt(HashInt(HashString(theHash, anEntry.mName), (int)anEntry.mImages.size()),
		                  anEntry.mAdditive ? 1 : 0);
		theHash = HashInt(HashInt(HashInt(HashInt(theHash, anEntry.mNumCols), anEntry.mNumRows), anEntry.mGridWidth),
		                  anEntry.mGridHeight);
		theHash = HashMatrix(theHash, anEntry.mTransform);
	}
	theHash = HashInt(theHash, (int)theAnim.mData->mSprites.size());
	for (size_t i = 0; i < theAnim.mData->mSprites.size(); i++)
		theHash = HashSprite(theHash, theAnim.mData->mSprites[i]);
	if (theAnim.mData->mMain != NULL)
		theHash = HashSprite(HashInt(theHash, 1), *theAnim.mData->mMain);
	return theHash;
}

long long gEffectParticles = 0, gEffectArea = 0;
long long gEffectLayerUpdates = 0;   // over every update, the particle effects the animations had on their sprites

int CountEffects(const PASpriteInst* theInst)
{
	int aCount = (int)theInst->mEffects.size();
	for (size_t i = 0; i < theInst->mEffects.size(); i++) {
		gEffectParticles += theInst->mEffects[i].mEffect->mParticleCount;
		gEffectArea += theInst->mEffects[i].mEffect->mDrawArea;
	}
	for (size_t i = 0; i < theInst->mSlots.size(); i++) {
		if (theInst->mSlots[i].mChild != NULL)
			aCount += CountEffects(theInst->mSlots[i].mChild);
	}
	return aCount;
}

unsigned int HashInstance(unsigned int theHash, const PASpriteInst* theInst)
{
	theHash = HashInt(HashFloat(theHash, theInst->mFrameNum), theInst->mRepeatCount);
	theHash = HashMatrix(theHash, theInst->mTransform);
	for (size_t i = 0; i < theInst->mSlots.size(); i++) {
		if (theInst->mSlots[i].mChild != NULL)
			theHash = HashInstance(theHash, theInst->mSlots[i].mChild);
	}
	return theHash;
}

unsigned int HashCanvas(unsigned int theHash, MemoryImage* theCanvas)
{
	const unsigned int* aBits = (const unsigned int*)theCanvas->GetBits();
	int aCount = theCanvas->mWidth * theCanvas->mHeight;
	for (int i = 0; i < aCount; i++) {
		theHash = (theHash ^ aBits[i]) * 16777619u;
		if ((aBits[i] & 0xffffff) != 0)
			gLitPixels++;
	}
	return theHash;
}

// Plays the animation from the start (its main sprite, or the named one), checking that nothing crashes
unsigned int Play(PopAnim* theAnim, const std::string& theSprite, bool theByName, MemoryImage* theCanvas,
                  unsigned int theHash)
{
	bool aStarted = theByName ? theAnim->PlayLabel(theSprite, true) : theAnim->PlayFrame(0, true);
	theHash = HashInt(theHash, aStarted ? 1 : 0);
	if (!aStarted)
		return theHash;
	Graphics* g = theCanvas != NULL ? new Graphics(theCanvas) : NULL;
	for (int anUpdate = 0; anUpdate < kPlayUpdates; anUpdate++) {
		theAnim->Update();
		theHash = HashInstance(theHash, theAnim->mMainInst);
		theHash = HashInt(theHash, theAnim->mPlaying ? 1 : 0);
		gEffectLayerUpdates += CountEffects(theAnim->mMainInst);
		if (theCanvas != NULL && anUpdate % kDrawEvery == 0) {
			g->SetColor(Color(0, 0, 0, 255));
			g->FillRect(0, 0, theCanvas->mWidth, theCanvas->mHeight);
			g->PushState();
			g->Translate(theCanvas->mWidth / 2, theCanvas->mHeight / 2);
			theAnim->Draw(g);
			g->PopState();
			theHash = HashCanvas(theHash, theCanvas);
		}
	}
	delete g;
	return theHash;
}

}  // namespace

int main(int argc, char** argv)
{
	for (int i = 1; i < argc; i++) {
		if (strncmp(argv[i], "-gamedir=", 9) == 0)
			gGameDir = argv[i] + 9;
		else if (strncmp(argv[i], "-artres=", 8) == 0)
			gArtRes = atoi(argv[i] + 8);
		else if (strcmp(argv[i], "-v") == 0)
			gVerbose = true;
		else if (strcmp(argv[i], "-play") == 0)
			gPlay = true;
		else if (strcmp(argv[i], "-draw") == 0)
			gPlay = gDraw = true;
		else {
			printf("unknown argument: %s\n", argv[i]);
			return 2;
		}
	}
	if (gArtRes != 600 && gArtRes != 768 && gArtRes != 1200) {
		printf("-artres is 600, 768 or 1200\n");
		return 2;
	}

	SDL_setenv("SDL_AUDIODRIVER", "dummy", 0);
	Platform::VFS::Get().SetBaseDir(gGameDir);
	Platform::RegisterOpenJPEGImages();
	SexyAppBase* anApp = new TestApp();
	anApp->mDDInterface = new DDInterface(anApp);
	anApp->mSoundManager = new SDLSoundManager();
	SWTri_AddAllDrawTriFuncs();

	// every .pam under the game folder, in a fixed order
	std::vector<std::string> aFiles;
	std::error_code anError;
	for (std::filesystem::recursive_directory_iterator anItr(gGameDir, anError), anEnd; !anError && anItr != anEnd;
	     anItr.increment(anError)) {
		if (!anItr->is_regular_file(anError) || anItr->path().extension() != ".pam")
			continue;
		aFiles.push_back(std::filesystem::relative(anItr->path(), gGameDir, anError).generic_string());
	}
	std::sort(aFiles.begin(), aFiles.end());

	MemoryImage* aCanvas = NULL;
	if (gDraw) {
		aCanvas = new MemoryImage();
		aCanvas->Create(kCanvasWidth, kCanvasHeight);
	}

	int aLoaded = 0, aFailed = 0;
	unsigned int aHash = 2166136261u;
	for (size_t i = 0; i < aFiles.size(); i++) {
		PopAnim* anAnim = new PopAnim(0, &gListener);
		anAnim->mImageScale = (float)gArtRes / kBaseArtRes;
		anAnim->mDrawScale = (float)gArtRes / kBaseArtRes;
		anAnim->mWidth = kCanvasWidth;
		anAnim->mHeight = kCanvasHeight;
		anAnim->LoadAnimation(aFiles[i]);
		if (!anAnim->mLoaded) {
			aFailed++;
			printf("FAIL %s: PopAnim loading error: %s\n", aFiles[i].c_str(), anAnim->GetError().c_str());
			delete anAnim;
			continue;
		}
		aLoaded++;
		aHash = HashLoaded(HashString(aHash, aFiles[i]), *anAnim);
		if (gVerbose)
			printf("%s: version %d, %d images, %d sprites%s\n", aFiles[i].c_str(), anAnim->mVersion,
			       (int)anAnim->mImages.size(), (int)anAnim->mData->mSprites.size(),
			       anAnim->mData->mMain != NULL ? ", a main sprite" : "");
		if (gPlay) {
			if (anAnim->mData->mMain != NULL) {
				aHash = Play(anAnim, "", false, aCanvas, aHash);
			}
			else {
				std::vector<std::string> aNames;
				for (size_t s = 0; s < anAnim->mData->mSprites.size(); s++)
					aNames.push_back(anAnim->mData->mSprites[s].mName);
				for (size_t s = 0; s < aNames.size(); s++)
					aHash = Play(anAnim, aNames[s], true, aCanvas, HashString(aHash, aNames[s]));
			}
		}
		delete anAnim;
	}
	printf("files %d, loaded %d, failed %d, hash %08x (stops %d, commands %d, samples %d, lit pixels %lld)\n",
	       (int)aFiles.size(), aLoaded, aFailed, aHash, gListener.mStops, gListener.mCommands, gListener.mSamples,
	       gLitPixels);
	printf("effects: %d commands, %lld effect layers over the updates, %lld particles, %lld drawn area\n", gListener.mEffectCommands,
	       gEffectLayerUpdates, gEffectParticles, gEffectArea);
	fflush(stdout);
	_exit(aFailed == 0 ? 0 : 1);
}
