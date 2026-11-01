// Sexy::WinTwistApp (WinTwistApp.h).
#include "WinTwistApp.h"

#include "CustomResourceManager.h"
#include "GameText.h"
#include "Res.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "Widget.h"
#include "WidgetManager.h"

#include <SDL.h>

using namespace Sexy;

WinTwistApp* Sexy::gApp = nullptr;

namespace {

// The art size the constructor picks (0x766ce7): the largest when the machine has the memory, the screen height and
// the video memory for it, 768 for a smaller machine, 600 for the smallest. The original also lets compat.cfg
// (GetAppDefaultArtRes) decide; the port has no compat.cfg.
int ChooseArtRes(bool theAllow1200, int theTotalPhysMB, int theScreenHeight, int theVidMemMB)
{
	if (theAllow1200 && theTotalPhysMB >= 768 && theScreenHeight >= 1200 && theVidMemMB >= 92)
		return WinTwistApp::ART_RES_1200;
	if (theTotalPhysMB > 255 && theScreenHeight > 599 && theVidMemMB > 59)
		return WinTwistApp::ART_RES_768;
	return WinTwistApp::ART_RES_600;
}

}  // namespace

WinTwistApp::WinTwistApp()
{
	gApp = this;
	mProdName = "BejeweledTwist";
	mRegKey = "PopCap\\BejeweledTwist";
	mMusicVolume = 0.85;
	mSfxVolume = 0.85;

	// port: the machine as the original would see a current one: the art size is decided again by Use2DMode
	SDL_DisplayMode aMode;
	int aScreenHeight = SDL_GetDesktopDisplayMode(0, &aMode) == 0 ? aMode.h : 1080;
	mArtRes = ChooseArtRes(mAllowArtRes1200, SDL_GetSystemRAM(), aScreenHeight, 256);
	mScale = (float)mArtRes / 1200.0f;

	delete mResourceManager;
	mResourceManager = new CustomResourceManager(this);

	// port: the framework draws in software, so the game runs as the original does without Direct3D (0x7606de)
	Use2DMode();
}

WinTwistApp::~WinTwistApp()
{
	if (mLoaderScreen != nullptr) {
		mWidgetManager->RemoveWidget(mLoaderScreen);
		delete mLoaderScreen;
	}
	gApp = nullptr;
}

void WinTwistApp::Use2DMode()
{
	if (mArtRes != ART_RES_600) {
		mArtRes = ART_RES_600;
		mScale = 0.5f;
	}
	mWidth = 800;
	mHeight = 600;
	mWindowAspect.Set(4, 3);
}

void WinTwistApp::Init()
{
	SexyApp::Init();
	if (mShutdown)
		return;
	LoadProperties("properties/config.xml", true, false);
	LoadResourceManifest();
	LoadGameTexts();

	// Without SexyCache's pictures the loader screen comes up, with its music (0x75fc23(0xca))
	mUseSexyCache = false;
	if (!mUseSexyCache && !mInitialLoadDone)
		mResourceManager->LoadResources("LoadScreen");
	mInitialLoadDone = true;
	mResourceManager->LoadResources("LoadScreenNonRelease");
	if (!ExtractLoadScreenNonReleaseResources(mResourceManager))
		ShowResourceError(true);
	if (!Is3DAccelerated()) {
		mResourceManager->LoadResources("2DOnly");
		if (!Extract2DOnlyResources(mResourceManager))
			ShowResourceError(true);
	}
	SetCursorImage(0, IMAGE_CURSOR_POINTER);
	SetCursorImage(1, IMAGE_CURSOR_HAND);
	SetCursorImage(2, IMAGE_CURSOR_DRAGGING);
}

bool WinTwistApp::LoadTubeResources()
{
	mResourceManager->LoadResources("Tube");
	bool anOk = ExtractTubeResources(mResourceManager);
	if (anOk)
		mTubeLoaded = true;
	else
		ShowResourceError(true);
	return anOk;
}

void WinTwistApp::LoadingThreadProc()
{
	mNumLoadingThreadTasks = mResourceManager->GetNumResources("Sounds") + mResourceManager->GetNumResources("LoadingThread") +
		mResourceManager->GetNumResources("NonResize") + mResourceManager->GetNumResources("InitialMainMenu");
	if (mLoadMode != 2) {
		while (mResourceManager->LoadNextResource()) {
			if (mShutdown)
				return;
			mCompletedLoadingThreadTasks++;
		}
		if (mResourceManager->HadError() || !LoadTubeResources()) {
			ShowResourceError(false);
			mLoadingFailed = true;
			return;
		}
		mResourceManager->StartLoadResources("NonResize");
		while (mResourceManager->LoadNextResource()) {
			if (mShutdown)
				return;
			mCompletedLoadingThreadTasks++;
		}
		mResourceManager->StartLoadResources("LoadingThread");
		while (mResourceManager->LoadNextResource()) {
			if (mShutdown)
				return;
			mCompletedLoadingThreadTasks++;
		}
		if (mResourceManager->HadError()) {
			ShowResourceError(false);
			mLoadingFailed = true;
			return;
		}
		// The original loads its 3D models here (sidebar, saucer, planets, crystals): 3D mode only
	}
	if (mLoadMode != 1) {
		mResourceManager->StartLoadResources("Sounds");
		while (mResourceManager->LoadNextResource())
			mCompletedLoadingThreadTasks++;
		if (mResourceManager->HadError()) {
			ShowResourceError(false);
			mLoadingFailed = true;
			return;
		}
	}
	if ((mLoadMode == 2 || (ExtractLoadingThreadResources(mResourceManager) && ExtractNonResizeResources(mResourceManager))) &&
		(mLoadMode == 1 || ExtractSoundsResources(mResourceManager))) {
		SDL_Log("Loading completed: %d out of %d", mCompletedLoadingThreadTasks, mNumLoadingThreadTasks);
		mLoadingThreadDone = true;
		return;
	}
	ShowResourceError(false);
	mLoadingFailed = true;
}

SoundInstance* WinTwistApp::PlaySoundEx(int theSoundId, int thePan, float thePitch, float theVolume, bool theTrack,
	bool theScaleVolume)
{
	// The board's pause and its pitch changes (board +0x210c, +0x1380, +0x1c8c) come with the board
	if ((!mLoadingThreadCompleted && mUseSexyCache) || mSoundManager == nullptr || !mHasFocus || mSoundsOff)
		return nullptr;
	if (theVolume == 0.0f)
		theVolume = 1.0f;
	SoundInstance* anInstance = mSoundManager->GetSoundInstance(theSoundId);
	if (anInstance == nullptr)
		return nullptr;
	if (thePitch != 0.0f)
		anInstance->AdjustPitch(thePitch);
	if (thePan != 0)
		anInstance->SetPan(thePan);
	if (theTrack)
		mTrackedSounds.push_back({anInstance, (double)theVolume});
	if (theScaleVolume)
		theVolume = theVolume * (float)mSfxVolumeScale;
	anInstance->SetVolume(theVolume);
	anInstance->Play(false, false);
	return anInstance;
}

void WinTwistApp::UpdateFrames()
{
	SexyApp::UpdateFrames();
	// The tracked sounds: the ended ones are released, the others follow mSfxVolumeScale
	for (size_t i = 0; i < mTrackedSounds.size(); i++) {
		TrackedSound& aSound = mTrackedSounds[i];
		if (!aSound.mInstance->IsPlaying()) {
			aSound.mInstance->Release();
			if (aSound.mInstance == mUnmanagedSound)
				mUnmanagedSound = nullptr;
			mTrackedSounds.erase(mTrackedSounds.begin() + i);
			i--;
		} else if (aSound.mInstance != mUnmanagedSound) {
			double aVolume = mSfxVolumeScale * aSound.mVolume;
			if (aSound.mInstance->GetVolume() != aVolume)
				aSound.mInstance->SetVolume(aVolume);
		}
	}
}
