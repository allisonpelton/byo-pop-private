// Sexy::WinTwistApp (WinTwistApp.h).
#include "WinTwistApp.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "CustomBassMusicInterface.h"
#include "CustomResourceManager.h"
#include "Board.h"
#include "Buffer.h"
#include "Challenge.h"
#include "Credits.h"
#include "DDInterface.h"
#include "GPUDisplay.h"
#include "GPU2D.h"
#include "TwistD3D.h"
#include "ButtonWidget.h"
#include "EffectsManager.h"
#include "GameText.h"
#include "GoodTube.h"
#include "Gates.h"
#include "GraphicsRecorder.h"
#include "Hyperspace.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "LoaderScreen.h"
#include "Mandala.h"
#include "Menu.h"
#include "MenuButtonWidget.h"
#include "MenuPanels.h"
#include "TwistCheckbox.h"
#include "Profile.h"
#include "Ranks.h"
#include "Res.h"
#include "PlayButton.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include "TwistDialog.h"
#include "TwistUtil.h"
#include "UnderDialogWidget.h"
#include "VizManager.h"
#include "Widget.h"
#include "WidgetManager.h"
#include "X86Math.h"

#include <SDL.h>

#include <cstdlib>

using namespace Sexy;

WinTwistApp* Sexy::gApp = nullptr;
int Sexy::gGemWidth = 82;
int Sexy::gGemHeight = 82;

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

// TwistP3DListener (vtable 0x7f380c): a mesh's texture from the art's own folder, else NonResize's
class TwistP3DListener : public D3DObjectListener {
public:
	SharedImageRef LoadTexture(D3DObject* theObject, const std::string& theFrame, const std::string& theMesh,
		const std::string& theKey, const std::string& theFileName) override
	{
		// 0x761d62
		(void)theObject; (void)theFrame; (void)theMesh; (void)theKey;
		std::string aName = GetFileName(theFileName, true);
		SharedImageRef anImage = gApp->GetSharedImage(StrFormat("images/%d/tex/%s", gApp->mArtRes, aName.c_str()), "",
			nullptr);
		if ((Image*)anImage == nullptr)
			anImage = gApp->GetSharedImage(StrFormat("images/NonResize/tex/%s", aName.c_str()), "", nullptr);
		MemoryImage* anImageBits = (MemoryImage*)anImage;
		if (anImageBits != nullptr) {
			if (aName.find("nebula1") == std::string::npos)
				anImageBits->mD3DFlags = 8;
			anImageBits->mPurgeBits = true;
		}
		return anImage;
	}
};

// Two listeners, as the original has (0xe0b6e0 for the loading thread's models, 0xe0b6e8 for Init's)
TwistP3DListener gLoadingP3DListener;
TwistP3DListener gInitP3DListener;

}  // namespace

WinTwistApp::WinTwistApp()
{
	gApp = this;
	// port: the original makes them further on, after the board's objects; nothing reads them before
	MakeTrigTables();
	mProdName = "BejeweledTwist";
	mRegKey = "PopCap\\BejeweledTwist";
	mMusicVolume = 0.85;
	mSfxVolume = 0.85;

	// port: the machine as the original would see a current one (2D mode takes 600 again, Use2DMode)
	SDL_DisplayMode aMode;
	int aScreenHeight = SDL_GetDesktopDisplayMode(0, &aMode) == 0 ? aMode.h : 1080;
	int aMemoryMB = SDL_GetSystemRAM();
#ifdef __EMSCRIPTEN__
	// port: a browser gives SDL no memory size; navigator.deviceMemory is in gigabytes (at most 8), 4 when unknown
	aMemoryMB = EM_ASM_INT({ return (navigator.deviceMemory || 4) * 1024; });
#endif
	mArtRes = ChooseArtRes(mAllowArtRes1200, aMemoryMB, aScreenHeight, 256);
	mScale = (float)mArtRes / 1200.0f;

	mProfile = new Profile();
	delete mResourceManager;
	mResourceManager = new CustomResourceManager(this);
	gGraphicsRecorder = new GraphicsRecorder();
	// 0x7606de's neighbour: made here in the original too (0x7597cc), set up by Init
	mVizManager = new VizManager(this);

	// (2D mode, Use2DMode, is chosen by ReadFromRegistry once "Is3D" and the switches are known)
}

WinTwistApp::~WinTwistApp()
{
	if (mLoaderScreen != nullptr) {
		mWidgetManager->RemoveWidget(mLoaderScreen);
		delete mLoaderScreen;
	}
	if (mCredits != nullptr) {
		mWidgetManager->RemoveWidget(mCredits);
		delete mCredits;
	}
	if (mMainMenu != nullptr) {
		mWidgetManager->RemoveWidget(mMainMenu);
		delete mMainMenu;
	}
	if (mGates != nullptr) {
		mWidgetManager->RemoveWidget(mGates);
		delete mGates;
	}
	if (mWindowedButton != nullptr) {
		mWidgetManager->RemoveWidget(mWindowedButton);
		delete mWindowedButton;
	}
	if (mUnderDialog != nullptr) {
		// port: the original never deletes it
		mWidgetManager->RemoveWidget(mUnderDialog);
		delete mUnderDialog;
	}
	FreeGemImages();
	delete mProfile;
	delete mRanks;
	delete mChallengeInfo;
	if (mHyperspace != nullptr) {
		mWidgetManager->RemoveWidget(mHyperspace);
		delete mHyperspace;
	}
	if (mFXManager != nullptr) {
		mWidgetManager->RemoveWidget(mFXManager);
		delete mFXManager;
	}
	if (mVizManager != nullptr) {
		mWidgetManager->RemoveWidget(mVizManager);
		delete mVizManager;
	}
	// port: the original never deletes its models or its Direct3D interface's game side
	for (D3DObject* anObject : {mSidebarObject, mSpaceObject, mOuterSpaceObject, mStarsObject, mSaucerObject,
			mSaucerLightsObject, mPlanetRingsObject, mWarpConeObject})
		delete anObject;
	for (D3DObject* anObject : mCrystalObjects)
		delete anObject;
	for (D3DObject* anObject : mPlanetObjects)
		delete anObject;
	delete gD3DInterface;
	gD3DInterface = nullptr;
	delete gGraphicsRecorder;
	gGraphicsRecorder = nullptr;
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
	SetGemSize();
}

void WinTwistApp::SetGemSize()
{
	if (mArtRes == ART_RES_600) {
		gGemWidth = 64;
		gGemHeight = 64;
	} else if (mArtRes == ART_RES_768) {
		gGemWidth = 82;
		gGemHeight = 82;
	} else if (mArtRes == ART_RES_1200) {
		gGemWidth = 128;
		gGemHeight = gGemWidth;
	}
}

void WinTwistApp::Init()
{
	SexyApp::Init();
	if (mShutdown)
		return;
	// 0x7653a1: unless partner.xml's MarketingMode is "LevelLock" (Steam's copy), +0x759 and +0x758 are set: a full
	// version whose Blitz is earned by play. With LevelLock +0x758 is the wrapper's check (0x73321a) and the mode
	// select opens Blitz at once for a registered copy; the port counts as registered. (The retail exe rejects Steam's
	// partner.xml by its signature; the user's Steam copy skips that check, so its game reads LevelLock.)
	mUnknown759 = GetString("MarketingMode", _S("")) != _S("LevelLock");
	LoadProperties("properties/config.xml", true, false);
	LoadResourceManifest();
	LoadGameTexts();
	// port: the original reads the scores before the properties; their defaults need the game's texts here
	mScale = (float)mArtRes / 1200.0f;  // 0x7652cc: the art size over 1200, again
	SetGemSize();
	LoadScores();
	LoadMusic();
	// 0x4a3c20: properties\default.xml, the texts of the high scores' stats page and others
	LoadProperties();
	mRanks = new Ranks();
	mChallengeInfo = new ChallengeInfo();  // 0x6dd087: loads properties\challenges.xml

	// The player who played last, or else the first one whose profile loads
	SexyString aLastName;
	mProfile->LoadUsers(&aLastName);
	if (!aLastName.empty() && !mProfile->Select(aLastName)) {
		aLastName = _S("");
		if (mProfile->SelectAny()) {
			aLastName = mProfile->mName;
			RegistryWriteString("LastProfileName", SexyStringToString(aLastName));
		}
	}
	// port: 0x764814 would then read PopCap's beta dump for the player (NOTES.md)

	// Without SexyCache's pictures the loader screen comes up, with its music
	mUseSexyCache = false;
	if (!mUseSexyCache)
		PlayMusic(202, true, false, 0, 0);
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

	mFXManager = new EffectsManager();
	mFXManager->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mFXManager);
	// The VizManager's place (the original loads the 3D models of space here too; with -record or -playback it seeds
	// both random number generators from the recording)
	mVizManager->mZOrder = -100;
	mVizManager->Resize(0, 0, mWidth, mHeight);
	mOuterSpaceObject = LoadD3DObject("outerspace.p3d", &gInitP3DListener);
	mSpaceObject = LoadD3DObject("space.p3d", &gInitP3DListener);
	mStarsObject = LoadD3DObject("stars.p3d", &gInitP3DListener);
	if (!mUseSexyCache) {
		ShowHyperspace();
		mLoaderScreen = new LoaderScreen(this);
		mLoaderScreen->Resize(0, 0, mWidth, mHeight);
		mLoaderScreen->mZOrder = 1000;
		mWidgetManager->AddWidget(mLoaderScreen);
	}
	// The layer behind the dialogs (the original makes its Direct3D textures here too, which 2D never uses)
	mUnderDialog = new UnderDialogWidget();
	mUnderDialog->Resize(0, 0, mWidth, mHeight);
	mUnderDialog->mZOrder = 0;
	mWidgetManager->AddWidget(mUnderDialog);
	mUnderDialog->SetVisible(false);
}

void WinTwistApp::PlayVoice(int theSoundId, bool theInterrupt)
{
	if (mUnmanagedSound != nullptr) {
		if (!theInterrupt)
			return;
		mUnmanagedSound->Stop();
		mUnmanagedSound->Release();
		mUnmanagedSound = nullptr;
	}
	mUnmanagedSound = PlaySoundEx(theSoundId, 0, 0.0f, 1.0f, false, false);
}

void WinTwistApp::PlaySample(int theSoundNum)
{
	if (mPlaySamples)
		PlaySoundEx(theSoundNum, 0, 0, 0, true, true);
}

void WinTwistApp::ShowHyperspace()
{
	if (mHyperspace == nullptr)
		mHyperspace = new Hyperspace();
	mHyperspace->Resize(0, 0, mWidth, mHeight);
	mHyperspace->mZOrder = mVizManager->mZOrder - 1;
	mHyperspace->SetTransition(0);
	mWidgetManager->AddWidget(mHyperspace);
}

void WinTwistApp::CreateGates()
{
	mGates = new Gates();
	mGates->mZOrder = mVizManager->mZOrder + 15;
	mGates->Resize(mWidth / 2 - mGates->mWidth / 2, mHeight / 2 - mGates->mHeight / 2, mGates->mWidth,
		mGates->mHeight);
	mGates->SetOpen(true, true);
}

void WinTwistApp::CreateMainMenu()
{
	mMainMenu = new Menu(this);
}

void WinTwistApp::Shutdown()
{
	if (mShutdown)
		return;
	// port: the original first ends a background thread (+0x8e0) and adds the session's minutes to the time played
	// (+0x75c; the "MaxTime" property chooses the clock); they come with the stats
	if (mUnknownF24 != nullptr) {
		if (mBoard != nullptr && mBoard->mUnknown2208 == mUnknownF24)
			mBoard->mUnknown2208 = nullptr;
		mWidgetManager->RemoveWidget(mUnknownF24);
		delete mUnknownF24;
		mUnknownF24 = nullptr;
	}
	// A game that may be saved goes now, which saves it (Board::RemovedFromManager)
	if (mBoard != nullptr && mBoard->CanSaveOnQuit()) {
		mWidgetManager->RemoveWidget(mBoard);
		delete mBoard;
		mBoard = nullptr;
	}
	mWidgetManager->RemoveWidget(mWindowedButton);
	mProfile->Save();
	SexyApp::Shutdown();
}

void WinTwistApp::MakeGemImages()
{
	SetGemSize();
	if (mGemImagesMade)
		return;
	mGemImagesMade = true;
	int aColor = 0;
	do {
		int aSize = 0;
		do {
			MemoryImage* anImage = new MemoryImage(this);
			anImage->Create(Ftol((double)gGemWidth * 1.5), Ftol((double)gGemHeight * 1.5));
			anImage->mPurgeBits = true;
			anImage->SetImageMode(true, true);
			anImage->mFilePath = "!prog";
			double aScale = 1.5 - ((double)aSize + 1.0) * 0.125;
			int aWidth = Ftol((double)gGemWidth * aScale) | 1;
			int aHeight = Ftol(aScale * (double)gGemHeight) | 1;
			Graphics g(anImage);
			Rect aRect(Ftol(((double)gGemWidth * 1.5 - aWidth) * 0.5), Ftol(((double)gGemHeight * 1.5 - aHeight) * 0.5),
				aWidth, aHeight);
			g.DrawImageCel(IMAGE_GEMS, aRect, aColor);
			mGemImages[aColor][aSize] = anImage;
			aSize++;
		} while (aSize < 7);
		MemoryImage* anImage = new MemoryImage(this);
		anImage->Create(IMAGE_BOMBGEMS->GetCelWidth() / 2, IMAGE_BOMBGEMS->GetCelHeight() / 2);
		anImage->SetImageMode(true, true);
		anImage->mFilePath = "!prog";
		Graphics g(anImage);
		Rect aRect(0, 0, IMAGE_BOMBGEMS->GetCelWidth() / 2, IMAGE_BOMBGEMS->GetCelHeight() / 2);
		g.DrawImageCel(IMAGE_BOMBGEMS, aRect, aColor);
		mBombImages[aColor] = anImage;
		mCompletedLoadingThreadTasks++;
		aColor++;
	} while (aColor < 7);
}

void WinTwistApp::FreeGemImages()
{
	// port: the original also leaks the eight arrays that held the images (NOTES.md, "Deviations")
	if (!mGemImagesMade)
		return;
	for (int aColor = 0; aColor < 7; aColor++) {
		for (int aSize = 0; aSize < 7; aSize++) {
			delete mGemImages[aColor][aSize];
			mGemImages[aColor][aSize] = nullptr;
		}
	}
	for (int aColor = 0; aColor < 7; aColor++) {
		delete mBombImages[aColor];
		mBombImages[aColor] = nullptr;
	}
	mGemImagesMade = false;
}

void WinTwistApp::WriteToRegistry()
{
	SexyApp::WriteToRegistry();
	mProfile->SaveUsers();
	// port: LastScoreName (+0x7fc) comes with the high score's name entry
	if (mLoaded)
		RegistryWriteString("LastProfileName", SexyStringToString(mProfile->mName));
	// port: HighResolution (+0x818), AlwaysUpdate (+0x81a) and ArtRes (when the options changed it, +0xf35) keep
	// the art's size, which the port always chooses as 2D mode does; ModeSeconds0 to 3 (+0x7d4) come with the board
	RegistryWriteBoolean("CustomCursors", mCustomCursorsEnabled);
	RegistryWriteBoolean("ShowHints", mShowHints);
	RegistryWriteBoolean("DoViz", mDoViz);
	RegistryWriteInteger("VizSpeed", Ftol(mVizManager->mUnknown1B0 * 100.0));
}

void WinTwistApp::ReadFromRegistry()
{
	SexyApp::ReadFromRegistry();
	// port: the values the writer leaves out are left out here too
	RegistryReadBoolean("CustomCursors", &mCustomCursorsEnabled);
	RegistryReadBoolean("DoViz", &mDoViz);
	RegistryReadBoolean("ShowHints", &mShowHints);
	// The art's size, unless a switch chose it (+0xf34; the port has none of those switches)
	int anArtRes;
	if (RegistryReadInteger("ArtRes", &anArtRes))
		mArtRes = anArtRes;
	// "Is3D" off chooses 2D mode (Use2DMode; the port chooses it in MakeWindow, after the switches)
	// port: while 3D mode is being ported, a run without "Is3D" stays in 2D; the options' 3D box writes it true
	bool is3D = true;
	if (RegistryReadBoolean("Is3D", &is3D))
		mUse3D = mUse3DSwitch < 0 ? is3D : mUse3DSwitch != 0;
	EnableCustomCursors(mCustomCursorsEnabled);
	mProfile->mCustomCursors = mCustomCursorsEnabled;
}

bool WinTwistApp::Is3DAccelerationSupported()
{
	if (Is3DAccelerated())
		return true;
#ifdef __EMSCRIPTEN__
	return EM_ASM_INT({
		try {
			return document.createElement('canvas').getContext('webgl2') ? 1 : 0;
		} catch (e) {
			return 0;
		}
	}) != 0;
#else
	return true;
#endif
}

void WinTwistApp::MakeWindow()
{
	// port: 2D mode (Use2DMode) for "Is3D" off (0x762da8) and for a display without Direct3D (0x7606de), here once the
	// registry and the switches are read
	if (!mUse3D)
		Use2DMode();
	// The window's aspect is the screen's (SexyAppBase's constructor 0x766ce7, from GetSystemMetrics), kept between the
	// app's narrowest and widest (+0x600 and +0x608, by 0x503fd0 and 0x504010); 2D mode keeps 4:3 (Use2DMode)
	// port: here, since SDL knows no display before its video starts, which is started now with the framework's own
	// hints (SexyAppBase::MakeWindow); where the two limits are set was not found: the original on a 16:9 screen opens
	// at 16:10, so 16:10 is the widest, and 4:3 is taken as the narrowest
	if (mUse3D && !SDL_WasInit(SDL_INIT_VIDEO)) {
#ifndef __EMSCRIPTEN__
		SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
		SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
#endif
		SDL_InitSubSystem(SDL_INIT_VIDEO);
	}
	SDL_DisplayMode aMode;
	if (mUse3D && SDL_GetDesktopDisplayMode(0, &aMode) == 0 && aMode.w > 0 && aMode.h > 0) {
		int aWidth = aMode.w, aHeight = aMode.h;
		if (aWidth * 3 < aHeight * 4) {
			aWidth = 4;
			aHeight = 3;
		} else if (aWidth * 10 > aHeight * 16) {
			aWidth = 16;
			aHeight = 10;
		}
		mWindowAspect.Set(aWidth, aHeight);
	}
	// 0x75ef2a: the game is as high as its art (2D mode's is 600) and as wide as the window's aspect makes it
	mHeight = mArtRes;
	mWidth = (mWindowAspect.mNumerator * mArtRes) / mWindowAspect.mDenominator;
	// port: the window's own size and place (with Direct3D: at most 3/4 of the screen's width in height and 9/10 of
	// the work area's, and resizable) are the framework's
	SexyApp::MakeWindow();
}

void WinTwistApp::SwitchScreenMode(bool wantWindowed)
{
	SwitchScreenMode(wantWindowed, SexyAppBase::Is3DAccelerated());
}

DDInterface* WinTwistApp::CreateDDInterface()
{
	if (mUse3D)
		return new GPUDisplay(this, GPUPolicy::Twist());
	return SexyApp::CreateDDInterface();
}

uint32_t WinTwistApp::GetExtraWindowFlags()
{
	return mUse3D ? SDL_WINDOW_OPENGL : SexyApp::GetExtraWindowFlags();
}

void WinTwistApp::PostDDInterfaceInitHook()
{
	SexyApp::PostDDInterfaceInitHook();
	// port: the framework calls its display 3D-accelerated whenever SDL's renderer runs on the GPU (in a browser, and on
	// most desktops), but for Twist Is3DAccelerated means Direct3D and its 3D mode: a 2D run says no, as the original
	// does without Direct3D (Use2DMode); GPUDisplay says yes
	if (!mUse3D) {
		mDDInterface->mIs3D = false;
		return;
	}
	// The original's gD3DInterface, and its shader table (made at the resources' init, 0x73ef19)
	GPUDisplay* aDisplay = static_cast<GPUDisplay*>(mDDInterface);
	if (gD3DInterface == nullptr) {
		gD3DInterface = new TwistD3D(aDisplay->mDevice, aDisplay->mGPU2D);
		gD3DInterface->MakeShaders();
	}
}

void WinTwistApp::SwitchScreenMode(bool wantWindowed, bool is3d, bool force)
{
	SexyApp::SwitchScreenMode(wantWindowed, is3d, false);
	(void)force;
	if (mWindowedButton != nullptr)
		mWindowedButton->SetVisible(!mIsWindowed);
	if (mMainMenu != nullptr && mMainMenu->mOptionsPanel != nullptr)
		mMainMenu->mOptionsPanel->mFullscreenCheckbox->mChecked = !wantWindowed;
	if (mOptionsDialog != nullptr)
		static_cast<OptionsPanel*>(mOptionsDialog)->mFullscreenCheckbox->mChecked = !wantWindowed;
	// The original then clears the window's resize-corner flag (Direct3D only) and calls its slot 12 (0x75ef1b),
	// which does nothing more without that flag than the framework's
	PlaceWindowedButton();
}

void WinTwistApp::FinishOptionsDialog(bool theResolution)
{
	SDL_Log("FinishOptionsDialog()");
	OptionsPanel* aDialog = static_cast<OptionsPanel*>(mOptionsDialog);
	if (aDialog == nullptr && mMainMenu != nullptr) {
		mMainMenu->mOptionsPanel->ApplySettings(theResolution);
		return;
	}
	aDialog->ApplySettings(theResolution);
	if (mBoard == nullptr) {
		mWidgetManager->RemoveWidget(aDialog);
		SafeDeleteWidget(aDialog);
		if (mMainMenu != nullptr)
			mWidgetManager->SetFocus(mMainMenu);
	} else {
		// it zooms out over the game, which has the keys again
		aDialog->mZooming = true;
		aDialog->mZoom.SetCurve(GTS(0x7d8c7c));
		mWidgetManager->SetFocus(mBoard);
		mWidgetManager->RemoveBaseModal(mOptionsDialog);
	}
	mOptionsDialog = nullptr;
}

void WinTwistApp::ShowOptionsDialog()
{
	// 0x75f6fc
	if (mOptionsDialog != nullptr)
		return;
	float aScale = (float)((double)mArtRes / 1200.0);
	OptionsPanel* aPanel = new OptionsPanel(this);
	mOptionsDialog = aPanel;
	int aWidth = GetImageById(aPanel->mUnknownB0)->mWidth;
	int aHeight = GetImageById(aPanel->mUnknownB0)->mHeight;
	int aY = Ftol((double)GetImageOffsetY(aPanel->mUnknownB0) * (double)aScale);
	aPanel->Resize(mWidth / 2 - aWidth / 2, aY, aWidth, aHeight);
	aPanel->mZOrder = mBoard == nullptr ? 10 : mBoard->mZOrder + 1;
	if (aPanel->mZOrder < 11)
		aPanel->mZOrder = 10;
	aPanel->mFlushOverlays = true;
	mWidgetManager->AddWidget(aPanel);
	mWidgetManager->AddBaseModal(aPanel);
	mWidgetManager->SetFocus(aPanel);
}

void WinTwistApp::ShowCredits()
{
	mCredits = new Credits();
	mCredits->mZOrder = 1000;
	mWidgetManager->AddWidget(mCredits);
	// port: as in the original, nothing takes the base modal away again; taking the credits out of the widget manager
	// only clears it (the stale entry is dropped by the next RemoveBaseModal)
	mWidgetManager->AddBaseModal(mCredits);
}

void WinTwistApp::ShowHelpDialog()
{
	// 0x75f82d
	if (mUnknownF44 != nullptr)
		return;
	HelpPanel* aPanel = new HelpPanel(this);
	mUnknownF44 = aPanel;
	int aWidth = Ftol((double)mScale * 1000.0);
	int aY = Ftol((double)mScale * 100.0);
	aPanel->Resize(mWidth / 2 - aWidth / 2, aY, aWidth, GetImageById(aPanel->mUnknownB0)->mHeight);
	aPanel->mZOrder = mBoard == nullptr ? 10 : mBoard->mZOrder + 1;
	if (aPanel->mZOrder < 11)
		aPanel->mZOrder = 10;
	aPanel->mFlushOverlays = true;
	mWidgetManager->AddWidget(aPanel);
	mWidgetManager->AddBaseModal(aPanel);
	mWidgetManager->SetFocus(aPanel);
}

void WinTwistApp::PlaceWindowedButton()
{
	if (mWindowedButton == nullptr)
		return;
	mWindowedButton->Resize(mWidth - mWindowedButton->mNormalRect.mWidth - 5, 5,
		IMAGE_WINDOWEDBUTTON->GetCelWidth(), IMAGE_WINDOWEDBUTTON->GetCelHeight());
	mWindowedButton->SetVisible(!mIsWindowed && !mForceFullscreen);
}

MenuButtonWidget* WinTwistApp::MakeMenuButton(Image* theImage, Image* theCover, int theId, int theX, int theY,
	const SexyString& theLabel, const SexyString& theToolTip)
{
	MenuButtonWidget* aButton = new MenuButtonWidget(theId, theImage, this);
	aButton->mButtonImage = theImage;
	aButton->mOverImage = theImage;
	aButton->mDownImage = theImage;
	aButton->mDisabledImage = theImage;
	aButton->mCover = theCover;
	int aWidth = theImage->GetCelWidth();
	int aHeight = theImage->GetCelHeight();
	if (theImage->mNumRows * theImage->mNumCols < 2) {
		aButton->mNormalRect = Rect(0, 0, theImage->mWidth, theImage->mHeight);
	} else {
		aButton->mNormalRect = theImage->GetCelRect(0);
		aButton->mOverRect = theImage->GetCelRect(1);
		aButton->mDownRect = theImage->GetCelRect(2);
		aButton->mDisabledRect = theImage->GetCelRect(theImage->mNumRows * theImage->mNumCols == 4 ? 3 : 0);
		aButton->mOverAlphaFadeInSpeed = 0.05000000074505806;
		aButton->mOverAlphaSpeed = 0.05000000074505806;
	}
	aButton->mToolTip = theToolTip;
	aButton->mLabel = theLabel;
	aButton->mZOrder = 0;
	aButton->mX = theX;
	aButton->mY = theY;
	aButton->mWidth = aWidth;
	aButton->mHeight = aHeight;
	aButton->SetVisible(true);
	return aButton;
}

void WinTwistApp::StartMainMenu()
{
	// With -record the original starts recording here (app +0xc42)
	if (mUnknownF85) {
		// A classic game at once, as TWISTBOT: 0x760492(1, true, false) comes with the board
		mProfile->Create(GT(0x7da118));
		SDL_Log("WinTwistApp::StartMainMenu: starting a game at once is not ported yet");
		return;
	}
	CreateGates();
	CreateMainMenu();
	if (mResumeGame) {
		// The saved game (app +0x934's mode) is resumed, or the challenges come up: comes with the board
		SDL_Log("WinTwistApp::StartMainMenu: resuming a game is not ported yet");
	}
	// The registration widget (0x73b403, 0x98 bytes, app +0x924, z order 99) comes with the registration code
	if (mWindowedButton == nullptr) {
		mWindowedButton = new ButtonWidget(0x2a, this);
		mWindowedButton->mZOrder = 99;
		mWidgetManager->AddWidget(mWindowedButton);
		mWindowedButton->mButtonImage = IMAGE_WINDOWEDBUTTON;
		mWindowedButton->mNormalRect = IMAGE_WINDOWEDBUTTON->GetCelRect(0);
		mWindowedButton->mOverRect = IMAGE_WINDOWEDBUTTON->GetCelRect(1);
		mWindowedButton->mDownRect = IMAGE_WINDOWEDBUTTON->GetCelRect(2);
	}
	PlaceWindowedButton();
	MakeGemImages();
	if (!mResumeGame)
		ShowMainMenu(false, false, 0);
	mGates->Update();
	mMainMenu->Update();
	ClearUpdateBacklog(false);
}

void WinTwistApp::ShowMainMenu(bool theEndStats, bool theChallenges, float theDelay)
{
	// With a board the original first takes it away (or, while the board still loads, tells it to come back here):
	// comes with the board
	ShowHyperspace();
	if (mGates == nullptr)
		CreateGates();
	if (mMainMenu == nullptr)
		CreateMainMenu();
	mMainMenu->ShowMain(true);
	if (mGates->mParent == nullptr)
		mWidgetManager->AddWidget(mGates);
	if (mFXManager->mParent == nullptr) {
		mFXManager->mZOrder = -1;
		mWidgetManager->AddWidget(mFXManager);
	}
	mMainMenu->mStartDelay = theDelay;
	if (!theChallenges) {
		mMainMenu->mZOrder = mGates->mZOrder - 1;
		if (!mWidgetManager->HasWidget(mMainMenu))
			mWidgetManager->AddWidget(mMainMenu);
		else
			mMainMenu->AddedToManager(mWidgetManager);
		if (theEndStats) {
			mMainMenu->ShowEndStats();
			mGates->SetOpen(true, true);
		} else if (mLoaderScreen == nullptr) {
			mGates->SetOpen(false, false);
		}
	} else {
		mMainMenu->ShowChallenges(true);
		mGates->SetOpen(true, true);
	}
	mWidgetManager->SetFocus(mMainMenu);
	AddHighScore();  // 0x764f9c
	mUnknownF85 = false;
	if (mWidgetManager->mFocusWidget == nullptr)
		mWidgetManager->SetFocus(mMainMenu);
}

PlayButton* WinTwistApp::GetMainMenuPlayButton()
{
	return mMainMenu != nullptr ? mMainMenu->mPlayButton : nullptr;
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
		mSidebarObject = LoadD3DObject("sidebar.p3d", &gLoadingP3DListener);
		mSaucerObject = LoadD3DObject("saucer.p3d", &gLoadingP3DListener);
		mSaucerLightsObject = LoadD3DObject("saucerlights.p3d", &gLoadingP3DListener);
		mPlanetRingsObject = LoadD3DObject("planetrings.p3d", &gLoadingP3DListener);
		mWarpConeObject = LoadD3DObject("warpcone.p3d", &gLoadingP3DListener);
		for (int i = 0; i < 2; i++)
			mPlanetObjects[i] = LoadD3DObject(StrFormat("planet%i.p3d", i + 1), &gLoadingP3DListener);
		for (int i = 0; i < 7; i++)
			mCrystalObjects[i] = LoadD3DObject(StrFormat("crystal%i.p3d", i + 1), &gLoadingP3DListener);
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
	// The credits are gone (Credits::Update): out of the widgets and deleted
	if (mCredits != nullptr && mCredits->mClicked) {
		mWidgetManager->RemoveWidget(mCredits);
		SafeDeleteWidget(mCredits);
		mCredits = nullptr;
	}
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
	if (mLoaderScreen != nullptr && mLoaderScreen->mFinished) {
		mWidgetManager->RemoveWidget(mLoaderScreen);
		SafeDeleteWidget(mLoaderScreen);
		mLoaderScreen = nullptr;
		mResourceManager->DeleteResources("LoadScreen");
	}
	// The board's game is over: back to the main menu, with the game's stats when it ended (not when it was left)
	if (mBoard != nullptr && mBoard->mUnknown1FF5) {
		bool anEnded = mBoard->mUnknown36B0;
		Hyperspace* aSpace = mBoard->mHyperspace;
		if (aSpace != nullptr) {
			mHyperspace = aSpace;
			aSpace->mState = 0;
			aSpace->mFlag155 = false;
			aSpace->mIs3D = false;
			aSpace->mFlag158 = false;
			mBoard->mHyperspace = nullptr;
		}
		DeleteBoard();
		if (mLastGameMode != 2)
			ShowMainMenu(anEnded, false, 0);
		mGates->Update();
		mMainMenu->Update();
		mWidgetManager->SetFocus(mMainMenu);
	}
	// The track waiting for the music to fade out: song 0's volume tells, whichever song is playing
	if (mPendingMusic != -1 && mTwistMusic != nullptr) {
		auto it = mTwistMusic->mSongs.find(0);
		if (it != mTwistMusic->mSongs.end() && it->second.mVolume == 0.0) {
			if (mPendingMusicDelay < 1) {
				it->second.mVolume = 1.0;
				if (mTwistMusic->mCurTrack == 305)
					mTwistMusic->ResetChannels();
				mTwistMusic->PlayTrack(mPendingMusic, false);
				// With the board, track 305 then sets the turn-based challenge's channels and tempo (0x6b2fda)
				mPendingMusic = -1;
			} else {
				mPendingMusicDelay--;
			}
		}
	}
	// The screen darkens behind the dialogs and the options, and while the board is paused for the player to see
	if (mDialogMap.size() == 0 && mUnknownF44 == nullptr && mOptionsDialog == nullptr &&
		(mBoard == nullptr || mBoard->mUnknown1574 == 0)) {
		float aDim = (float)(mDialogDim - 0.03);
		if (aDim < 0.0f)
			aDim = 0.0f;
		mDialogDim = aDim;
		if (aDim == 0.0f && mUnderDialog->mVisible)
			mUnderDialog->SetVisible(false);
	} else if (mDialogDim < 1.0f) {
		float aDim = (float)(mDialogDim + 0.04);
		if (aDim > 1.0f)
			aDim = 1.0f;
		mDialogDim = aDim;
		if (!mUnderDialog->mVisible) {
			if (mDialogMap.size() != 0) {
				mWidgetManager->PutBehind(mUnderDialog, mDialogList.back());
			} else if (mOptionsDialog != nullptr || mUnknownF44 != nullptr) {
				mWidgetManager->PutBehind(mUnderDialog, mOptionsDialog != nullptr ? mOptionsDialog : mUnknownF44);
			}
			if (mFXManager != nullptr)
				mWidgetManager->PutBehind(mFXManager, mUnderDialog);
			mUnderDialog->SetVisible(true);
			mUnderDialog->mUpdateCnt = 100;
		} else {
			mUnderDialog->MarkDirty();
		}
	}
}

int WinTwistApp::ScoreList(int theGameMode)
{
	int aList = 0;
	if (theGameMode != 1)
		aList = (theGameMode == 3) * 2 - 1;
	return aList;
}

int WinTwistApp::CheckForHighScore(int theGameMode, SexyString theName, int theLevel, int theScore)
{
	int aPlace = -1;
	unsigned int aList = (unsigned int)ScoreList(theGameMode);
	if (aList < 2) {
		HighScore* aScore = &mHighScores[aList * 10 + 9];
		int i = 9;
		do {
			int aNext = i;
			if (theScore <= aScore->mScore)
				break;
			aScore--;
			i = aNext - 1;
			aPlace = aNext;
		} while (-1 < i);
		if (mMainMenu == nullptr)
			CreateMainMenu();
		// (and a line for the debugger with the mode, place, name, level and score)
		mMainMenu->SetNewHighScore(aPlace, theName, theLevel, theScore);
	}
	mUnknownF68 = aPlace;
	return aPlace;
}

void WinTwistApp::AddHighScore()
{
	int aList = ScoreList(mLastGameMode);
	int aPlace = mUnknownF68;
	if (-1 < aPlace && aPlace < 10 && -1 < aList) {
		int i = 8;
		if (aPlace < 9) {
			HighScore* aScore = &mHighScores[aList * 10 + 9];
			do {
				*aScore = *(aScore - 1);
				i--;
				aScore--;
			} while (aPlace <= i);
		}
		HighScore& aNew = mHighScores[aList * 10 + aPlace];
		aNew.mLevel = mUnknownF70;
		aNew.mName = mProfile->mName;
		aNew.mScore = mUnknownF6C;
		SaveScores();
	}
}

void WinTwistApp::LoadScores()
{
	Buffer aBuffer;  // the original also holds the app's critical section while it reads
	mScoresLoaded = false;
	std::string aPath = GetAppDataFolder() + "users\\scores.dat";
	if (ReadBufferFromFile(aPath, &aBuffer, false) && aBuffer.ReadLong() == 9) {
		for (int i = 0; i < 20; i++) {
			mHighScores[i].mName = WStringToSexyString(aBuffer.ReadUTF8String());
			mHighScores[i].mLevel = aBuffer.ReadLong();
			mHighScores[i].mScore = aBuffer.ReadLong();
		}
	} else {
		SetDefaultScores();
		SaveScores();
	}
	mScoresLoaded = true;
}

std::string WinTwistApp::GetSaveName(int theMode, int theChallenge)
{
	std::string aSuffix;
	if (theMode == 0) {
		aSuffix = "_casual";
	} else if (theMode == 1) {
		aSuffix = "_classic";
	} else if (theMode == 2) {
		// "_challenge_" and the id of category theChallenge (0x6d9783; past the end an empty id)
		aSuffix = "_challenge_" + mChallengeInfo->GetCategoryId(theChallenge);
	} else if (theMode == 3) {
		aSuffix = "_blitz";
	} else if (theMode == 4) {
		aSuffix = "_zen";
	} else if (theMode == 5) {
		aSuffix = "_puzzle";
	}
	return Profile::GetUserFolder(mProfile->mName) + "\\savegame" + aSuffix + ".dat";
}

void WinTwistApp::DeleteBoard()
{
	if (mBoard != nullptr) {
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = nullptr;
	}
}

void WinTwistApp::NewGame(int theMode, bool thePlayPressed, int theChallenge)
{
	mPlayPressed = thePlayPressed;
	if (mMainMenu != nullptr && mMainMenu->mHighScoresPanel != nullptr)
		mMainMenu->mHighScoresPanel->ResetNewRow();
	if (mBoard != nullptr) {
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
	}
	FadeOutMusic();
	mVizManager->SetVisible(true);
	mWidgetManager->AddWidget(mVizManager);
	mVizManager->mSpeed = 1.0;
	mVizManager->mPaused = false;
	for (UnknownFA0& aSlot : mUnknownFA0) {
		aSlot.mUnknown20 = -1;
		aSlot.mUnknown24 = -1;
		aSlot.mUnknown28 = 0;
		aSlot.mBuffer.Clear();
	}
	mBoard = new Board(this, theMode, 0, theChallenge);
	mBoard->StartGame();
	mBoard->mZOrder = mVizManager->mZOrder + 10;
	mWidgetManager->AddWidget(mBoard);
	mBoard->Resize(0, 0, mWidth, mHeight);
	if (mHyperspace == nullptr) {
		mBoard->mGoodTube->mVisible = true;
	} else if (!mUnknownF85) {
		mWidgetManager->RemoveWidget(mHyperspace);
		mBoard->TakeHyperspace(mHyperspace);
		mHyperspace = nullptr;
	} else {
		mVizManager->SetVisible(false);
	}
	mBoard->StartLevel();
}

bool WinTwistApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	if (theCommand.mCommand == "newgame") {
		NewGame(std::atoi(theCommand.mArgs.c_str()), true, 0);
		return true;
	}
	// "loadgame <mode>" goes on with the mode's saved game, as the main menu's buttons do when one exists
	if (theCommand.mCommand == "loadgame") {
		NewGame(std::atoi(theCommand.mArgs.c_str()), false, 0);
		return true;
	}
	// "autoplayoff" ends -autoplay's moves, so a script can go on with its own clicks
	if (theCommand.mCommand == "autoplayoff") {
		mAutoPlayMode = -1;
		return true;
	}
	return false;
}

void WinTwistApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	// port: -3d turns 3D mode on while it is being ported; the original's -nod3d9 and -noacceleration keep 2D
	if (theParamName == "-3d") {
		mUse3D = true;
		mUse3DSwitch = 1;
		return;
	}
	if (theParamName == "-nod3d9" || theParamName == "-noacceleration") {
		mUse3D = false;
		mUse3DSwitch = 0;
		return;
	}
	static const char* const kTestSwitches[] = {"-widescreen", "-4x3", "-artres600", "-artres768", "-artres1200",
		"-skipmenu", "-record", "-playback"};
	for (const char* aSwitch : kTestSwitches) {
		if (theParamName == aSwitch) {
			// port: the original's screen size, art size and seed recording switches are not ported
			NotPorted("WinTwistApp::HandleCmdLineParam: a test switch");
			return;
		}
	}
	if (theParamName != "-autoplay") {
		SexyApp::HandleCmdLineParam(theParamName, theParamValue);
		return;
	}
	mAutoPlayMode = 1;
	if (theParamValue.empty())
		return;
	if (theParamValue == "classic")
		mAutoPlayMode = 1;
	if (theParamValue == "zen")
		mAutoPlayMode = 4;
	if (theParamValue == "blitz")
		mAutoPlayMode = 3;
}

void WinTwistApp::DeleteSave(int theMode, int theChallenge)
{
	if (theMode != 5 && theMode != 4)
		EraseFile(GetSaveName(theMode, theChallenge));
}

void WinTwistApp::ShowModeSelect(int theMode)
{
	Mandala* aMandala = new Mandala(theMode);
	int aWidth = Ftol((double)mScale * 1920.0);
	int aHeight = Ftol((double)mScale * 1200.0);
	aMandala->Resize(mWidth / 2 - aWidth / 2, mHeight / 2 - aHeight / 2, aWidth, aHeight);
	mWidgetManager->AddWidget(aMandala);
	mUnknownF24 = aMandala;
	mWidgetManager->PutBehind(aMandala, mMainMenu);
}

void WinTwistApp::OpenModeSelect()
{
	// (the original asks the profile about the modes 1 to 5 first and drops the answers)
	for (int i = 1; i < 6; i++)
		mProfile->IsModeUnlocked(i);
	ShowModeSelect(0);
}

bool WinTwistApp::SaveExists(int theMode, int theChallenge)
{
	if (theMode == 3)
		return false;
	return FileExists(GetSaveName(theMode, theChallenge));
}

void WinTwistApp::SaveScores()
{
	Buffer aBuffer;
	aBuffer.WriteLong(9);
	for (int i = 0; i < 20; i++) {
		aBuffer.WriteUTF8String(SexyStringToWString(mHighScores[i].mName));
		aBuffer.WriteLong(mHighScores[i].mLevel);
		aBuffer.WriteLong(mHighScores[i].mScore);
	}
	WriteBufferToFile(GetAppDataFolder() + "users\\scores.dat", &aBuffer);
}

void WinTwistApp::SetDefaultScores()
{
	// Both lists take the same ten levels; the names and scores run on through the two
	int i = 0;
	for (int aList = 0; aList < 2; aList++) {
		for (int aRow = 0; aRow < 10; aRow++, i++) {
			mHighScores[i].mLevel = GameInt(GTB(0x850ea0, 40) + aRow * 4);
			mHighScores[i].mName = GT(GameInt(GTP(0x850e50, 20) + i * 4));
			mHighScores[i].mScore = GameInt(GTB(0x850e00, 80) + i * 4);
		}
	}
}

Dialog* WinTwistApp::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
	const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	TwistDialog* aDialog = new TwistDialog(IMAGE_SIZEABLEDIALOG, IMAGE_SIZEABLEBUTTON, theDialogId, isModal,
		theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
	aDialog->mTwistListener = this;
	aDialog->PopUp();
	return aDialog;
}

Dialog* WinTwistApp::DoDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
	const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	Dialog* anOldDialog = GetDialog(theDialogId);
	if (anOldDialog != nullptr) {
		KillDialog(theDialogId);
		anOldDialog->mParent->RemoveWidget(anOldDialog);
		SafeDeleteWidget(anOldDialog);
		// port: the original leaves the old dialog among the app's dialogs, where AddDialog would find it deleted;
		// no dialog of the game is ever asked for twice
		mDialogMap.erase(theDialogId);
		mDialogList.remove(anOldDialog);
	}
	TwistDialog* aDialog = static_cast<TwistDialog*>(
		NewDialog(theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode));
	double aHalf = mWidth * 0.5;
	int aPreferred = aDialog->GetPreferredWidth();
	aDialog->mWidth = (int)((double)aPreferred <= aHalf ? aHalf : (double)aPreferred);
	int aHeight = aDialog->GetPreferredHeight(aDialog->mWidth);
	aDialog->mHeight = aHeight;
	if (mOptionsDialog != nullptr)
		aDialog->mZOrder = mOptionsDialog->mZOrder + 1;
	aDialog->mX = mWidth / 2 - aDialog->mWidth / 2;
	aDialog->mY = (int)((double)(mHeight / 2) - aHeight * (double)0.55f);
	AddDialog(theDialogId, aDialog);
	return aDialog;
}

bool WinTwistApp::KillDialog(int theDialogId)
{
	Dialog* aDialog = GetDialog(theDialogId);
	if (aDialog != nullptr)
		static_cast<TwistDialog*>(aDialog)->Close();
	return aDialog != nullptr;
}

void WinTwistApp::ButtonDepress(int theId)
{
	if (theId == 0x2a) {
		// the button at the top right in full screen
		SwitchScreenMode(true);
	}
	// (0x2b, the registration widget's button, comes with the registration code)
	PlaySample(SOUND_CLICK);
}

void WinTwistApp::DialogButtonDepress(int theDialogId, int theButtonId)
{
	switch (theDialogId) {
	case 3:
		FinishOptionsDialog(true);
		return;
	case 4: {
		// The options' question: the game's options dialog's, or else the main menu's panel's
		OptionsPanel* aPanel = static_cast<OptionsPanel*>(mOptionsDialog);
		if (aPanel == nullptr)
			aPanel = mMainMenu->mOptionsPanel;
		aPanel->Answer(theButtonId != Dialog::ID_CANCEL);
		break;
	}
	case 0xb:
		// The restart question: on OK the game restarts, by its mode; blitz's clock goes on (0x6af2be)
		if (theButtonId == 1000) {
			if (mBoard->mGameMode == 2)
				mBoard->RestartChallenge();
			else if (mBoard->mGameMode == 3)
				mBoard->RestartBlitz();
		}
		if (mBoard->mGameMode == 3)
			mBoard->Unpause(true);
		DialogThirdButtonDepress(0xb);
		return;
	case 0x28:
		// The original looks the dialog up and drops it; unless Cancel, the profile's +0x1b0 is cleared and the
		// board's tip closed
		GetDialog(0x28);
		if (theButtonId != Dialog::ID_CANCEL) {
			mProfile->mUnknown1B0 = false;
			mProfile->Save();
			mBoard->CloseTip();
		}
		KillDialog(0x28);
		return;
	case 0x2d:
		return;
	}
	KillDialog(theDialogId);
}

void WinTwistApp::DialogThirdButtonDepress(int theDialogId)
{
	KillDialog(theDialogId);
}

MusicInterface* WinTwistApp::CreateMusicInterface()
{
	// port: the original keeps its one CustomBassMusicInterface in a static
	if (mNoSoundNeeded)
		return new MusicInterface;
	mTwistMusic = new CustomBassMusicInterface();
	return mTwistMusic;
}

void WinTwistApp::LoadMusic()
{
	LoadProperties("properties/music.xml", true, false);
	if (mTwistMusic == nullptr || !mTwistMusic->LoadMusic(0, "music/bejewel twist 06.10.mo3"))
		return;
	// The game's tunes, as many as have an offset
	mNumIngameTracks = 0;
	int i = 0;
	std::string aName = StrFormat("MUSIC_INGAME%d_OFFSET", 0);
	while (true) {
		int anOffset = GetInteger(aName, -1);
		CustomBassMusicInterface::Track* aTrack;
		if (anOffset == -1 || (aTrack = mTwistMusic->AddTrack(0, i + 100, 0)) == nullptr)
			break;
		aTrack->mOffset = anOffset;
		aName = StrFormat("MUSIC_INGAME%d_TEMPO_NORMAL", i);
		aTrack->mTempo[0] = GetInteger(aName, 0);
		aName = StrFormat("MUSIC_INGAME%d_TEMPO_DANGER", i);
		aTrack->mTempo[1] = GetInteger(aName, 0);
		// The first four channels play only in danger
		for (int aChannel = 0; aChannel < 4; aChannel++)
			if (aChannel < (int)aTrack->mChannels.size())
				aTrack->mChannels[aChannel].mTarget[0] = 0;
		mNumIngameTracks++;
		i++;
		aName = StrFormat("MUSIC_INGAME%d_OFFSET", i);
	}
	struct { int mTrackId; const char* mTempo; const char* mOffset; } aTracks[] = {
		{200, "MUSICTEMPO_WIN", "MUSICOFFSET_WIN"},
		{201, "MUSICTEMPO_LOSE", "MUSICOFFSET_LOSE"},
		{0, "MUSICTEMPO_MAIN_MENU", "MUSICOFFSET_MAIN_MENU"},
		{202, "MUSICTEMPO_LOADING", "MUSICOFFSET_LOADING"},
		{203, "MUSICTEMPO_WHEELINTRO", "MUSICOFFSET_WHEELINTRO"},
		{204, "MUSICTEMPO_WHEELSCORE", "MUSICOFFSET_WHEELSCORE"},
		{205, "MUSICTEMPO_TROPHYROOM", "MUSICOFFSET_TROPHYROOM"},
	};
	for (auto& anEntry : aTracks) {
		CustomBassMusicInterface::Track* aTrack =
			mTwistMusic->AddTrack(0, anEntry.mTrackId, GetInteger(anEntry.mTempo));
		if (aTrack != nullptr)
			aTrack->mOffset = GetInteger(anEntry.mOffset);
	}
	// Blitz's song: its tracks (0x7f3cfc) and their offsets (0x850ec8)
	mTwistMusic->LoadMusic(1, "music/BejeweledTwist24.mo3");
	static const int kBlitzTracks[] = {300, 301, 302, 303, 304, 305, 306, 307, 308, -1};
	static const char* const kBlitzOffsets[] = {"MUSIC_OFFSET_BLITZ", "MUSIC_OFFSET_BLITZWIN_MODERATE",
		"MUSIC_OFFSET_BLITZWIN_GOOD", "MUSIC_OFFSET_BLITZWIN_EXCELLENT", "MUSIC_OFFSET_CHALLENGE_MENU",
		"MUSIC_OFFSET_CHALLENGE_TURNBASED", "MUSIC_OFFSET_CHALLENGE_UNTIMED", "MUSIC_OFFSET_MEGA_FRUIT_BONUS",
		"MUSIC_OFFSET_SUPERSPEED_BLITZ"};
	i = 0;
	do {
		CustomBassMusicInterface::Track* aTrack =
			mTwistMusic->AddTrack(1, kBlitzTracks[i], GetInteger("MUSICTEMPO_BLITZ"));
		if (aTrack != nullptr)
			aTrack->mOffset = GetInteger(kBlitzOffsets[i]);
		i++;
	} while (kBlitzTracks[i] > -1);
}

void WinTwistApp::PlayMusic(int theTrackId, bool noLoop, bool theFade, int theExtra1, int theExtra2)
{
	if (mTwistMusic == nullptr || mPendingMusic == theTrackId || mTwistMusic->mCurTrack == theTrackId)
		return;
	if (!theFade || mTwistMusic->mCurTrack == -1) {
		if (mTwistMusic->mCurTrack == 305)
			mTwistMusic->ResetChannels(-1);
		mTwistMusic->PlayTrack(theTrackId, noLoop);
		mPendingMusic = -1;
	} else {
		FadeOutMusic();
		mPendingMusic = theTrackId;
	}
}

int WinTwistApp::GetCurrentMusic()
{
	if (mMusicInterface != nullptr)
		return mTwistMusic != nullptr ? mTwistMusic->mCurTrack : -1;
	return 0;
}

void WinTwistApp::FadeOutMusic()
{
	mMusicInterface->FadeOutAll(true, 0.004);
}

void WinTwistApp::PlaySoundAt(int theSoundId, int theX, float thePitch, float theVolume)
{
	int aHalf = mWidth / 2;
	PlaySoundEx(theSoundId, Ftol((double)((float)(theX - aHalf) / (float)aHalf) * 800.0), thePitch, theVolume, true,
		true);
}

void WinTwistApp::Mute(bool autoMute)
{
	SexyAppBase::Mute(autoMute);
	if (gBoard != nullptr && gBoard->mUnknown267C != nullptr) {
		// port: the original reads the music interface and the track without a check; the port has none without
		// sound
		if (mTwistMusic != nullptr) {
			CustomBassMusicInterface::Track* aTrack = mTwistMusic->FindTrack(0x133);
			if (aTrack != nullptr)
				mTwistMusic->PauseMusic(aTrack->mSongId);
		}
	}
}

void WinTwistApp::Unmute(bool autoMute)
{
	SexyAppBase::Unmute(autoMute);
	if (gBoard != nullptr && gBoard->mUnknown267C != nullptr) {
		// (the same)
		if (mTwistMusic != nullptr) {
			mTwistMusic->SetVolume(mSfxVolume < mMusicVolume ? mMusicVolume : mSfxVolume);
			CustomBassMusicInterface::Track* aTrack = mTwistMusic->FindTrack(0x133);
			if (aTrack != nullptr)
				mTwistMusic->ResumeMusic(aTrack->mSongId);
		}
	}
}
