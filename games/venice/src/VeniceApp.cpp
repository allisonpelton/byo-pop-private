// The game's app and its widget (0x43fa60-0x441650): the partner's settings, the logos and loading screen, the 250 updates a second
// and the drawing of the scenes.

#include "VeniceApp.h"

#include "Graphics.h"
#include "ImageRecord.h"
#include "KeyCodes.h"
#include "Level.h"
#include "Profiles.h"
#include "SceneLoader.h"
#include "SceneTag.h"
#include "VeniceGlobals.h"
#include "VeniceInput.h"
#include "VeniceMath.h"
#include "VeniceSound.h"
#include "WidgetManager.h"

#include <SDL.h>
#include <cstdint>
#include <cstring>
#include <string>

using namespace Sexy;

VeniceApp* gApp = nullptr;

static const char* DefaultProductName = "Venice Deluxe";
static const char* DefaultRegistryKey = "Popcap Games\\Venice Deluxe";
static const char* PartnerFileName = "properties\\partner.xml";

// the game's own time unit: it updates every 4 milliseconds
static const int UpdatesPerSecond = 250;
static const int UpdateMilliseconds = 1000 / UpdatesPerSecond;
// the most updates the game catches up on after a delay, and the time (in updates) behind which it gives up on the rest
static const int MaxCatchUpUpdates = 20;
static const int GiveUpBehindUpdates = 100;

// the logos stay on the screen this long each (the next one starts while the last one's last part still shows)
static const int LogoSpacingMilliseconds = 2000;
static const int LogoFirstWaitMilliseconds = 1000;
static const float LogoFadeMilliseconds = 500.0f;
static const float LogoHoldMilliseconds = 2000.0f;

// the loading bar's place on the loading screen and the loaded percent that fills it (the config counts up to 99)
static const float LoadingBarX = 399.0f;
static const float LoadingBarY = 437.0f;
static const int LoadingBarFullPercent = 99;
static const int LoadingBarClipBottom = 600;
// how long a slice of the loading runs before the screen is drawn again
static const unsigned LoadSliceMilliseconds = 10;
// the load states: partner logos (-2 before they are made, -1 while they show), the loading screen's picture (0), the bar's (1),
// the config's tags (2 to 99, the percent the tags say)
static const int LoadStateLogosToMake = -2;
static const int LoadStateLogosShowing = -1;
static const int LoadStateLoadingScreen = 0;
static const int LoadStateLoadingBar = 1;
static const int LoadStateLoading = 2;

static int sTickBase = 0; // 0xa2a1ac: the update count the game has run up to (0 until the first update sets it)
static int sPreviousBatch = 1; // 0x5b51c8: how many updates the last batch ran

// 0x4407c0
void ResetTickBase()
{
	sTickBase = 0;
}

// 0x43fa60: the partner's file gives the product's name, its registry key and whether the game starts in a window
VeniceApp::VeniceApp()
{
	mProductVersion = "1.0";
	std::string aProductName = DefaultProductName;
	std::string aRegistryKey = DefaultRegistryKey;

	std::string aPartnerText;
	ReadGameFile(PartnerFileName, &aPartnerText);
	SceneTag aTag;
	const char* aText = ReadTag(aPartnerText.c_str(), &aTag);
	while (aTag.mName != "/Properties" && !(aTag.mName.empty() && *aText == '\0')) {
		if (aTag.Has("id")) {
			std::string aContent(aTag.mContent == nullptr ? "" : aTag.mContent, aTag.mContentLength);
			if (strcmp(aTag.GetString("id"), "DefaultWindowed") == 0)
				mIsWindowed = strncmp(aContent.c_str(), "true", aTag.mContentLength) == 0;
			else if (strcmp(aTag.GetString("id"), "ProdName") == 0)
				aProductName = aContent;
			else if (strcmp(aTag.GetString("id"), "RegistryKey") == 0)
				aRegistryKey = aContent;
			// the file's PartnerName is read and never used
		}
		aText = ReadTag(aText, &aTag);
	}

	mTitle = aProductName + " " + mProductVersion;
	mProdName = aProductName;
	mRegKey = aRegistryKey;
	mWidth = (int)gScreenWidth;
	mHeight = (int)gScreenHeight;

	// a desktop no bigger than the game's screen makes the game full screen only (the web page does not ask: the page's
	// size is not the desktop's)
#ifndef __EMSCRIPTEN__
	SDL_InitSubSystem(SDL_INIT_VIDEO);
	SDL_DisplayMode aDesktopMode;
	if (SDL_GetDesktopDisplayMode(0, &aDesktopMode) == 0) {
		mDesktopWidth = aDesktopMode.w;
		mDesktopHeight = aDesktopMode.h;
		if ((float)mDesktopWidth <= gScreenWidth || (float)mDesktopHeight <= gScreenHeight)
			mForceFullscreen = true;
	}
#endif
	// the game plays its own sounds and music; the framework's player is not used
	mNoSoundNeeded = true;
	// the framework's loading thread has nothing to load: it is over before it starts
	mLoadingThreadCompleted = true;
	gApp = this;
	RegistryReadInteger("music_volume" + mProductVersion, &gMusicVolume);
	RegistryReadInteger("sound_volume" + mProductVersion, &gSoundVolume);
	SetCursor(CURSOR_NONE);
}

// 0x440040
VeniceApp::~VeniceApp()
{
	mWidgetManager->RemoveWidget(mWidget);
	delete mWidget;
	mWidget = nullptr;
	gApp = nullptr;
}

// 0x440190: the profiles are read once the window is up
void VeniceApp::Init()
{
	SexyApp::Init();
	LoadProfiles();
}

// 0x4402d0: the one widget fills the window; the game's updates run on the clock, so the framework updates it every 4 ms
void VeniceApp::InitHook()
{
	SexyApp::InitHook();
	if (mShutdown)
		return;
	mWidget = new VeniceWidget();
	mWidget->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mWidget);
	mWidgetManager->SetFocus(mWidget);
	mAutoStartLoadingThread = false;
	mFrameTime = UpdateMilliseconds;
}

// 0x43ffa0: the scenes go first, then the pictures and the sounds they used
void VeniceApp::ShutdownHook()
{
	gScenes.Clear();
	gImageRecords.Clear();
	gSounds.Clear();
}

// the volumes are the only settings the game keeps in the registry (0x440040 writes them as the app ends)
void VeniceApp::WriteToRegistry()
{
	SexyApp::WriteToRegistry();
	RegistryWriteInteger("music_volume" + mProductVersion, gMusicVolume);
	RegistryWriteInteger("sound_volume" + mProductVersion, gSoundVolume);
}

// 0x440410
void VeniceApp::LostFocus()
{
	SexyApp::LostFocus();
	gHasFocus = false;
	SetCursor(CURSOR_POINTER);
	PauseSounds();
	gFocusFrames = 2;
}

// 0x440450: the click that gives the focus back is not the player's click, unless a main menu is waiting for it
void VeniceApp::GotFocus()
{
	SexyApp::GotFocus();
	gHasFocus = true;
	SetCursor(CURSOR_NONE);
	ResumeSounds();
	ResetTickBase();
	gIgnoreNextPress = true;
	if (gLoadState > LoadStateLoadingDone && !gScenes.IsEmpty()) {
		for (SceneObject* aScene = gScenes.mHead; aScene != nullptr; aScene = aScene->mNext) {
			if (strstr(aScene->mName, "main_menu") != nullptr)
				gIgnoreNextPress = false;
		}
	}
	gFocusFrames = 0;
}

// 0x4415e0: the window is asked to close (the last scene is gone)
static void RequestClose()
{
	if (gLoadState > LoadStateLogosShowing) {
		SDL_Event aQuit = {};
		aQuit.type = SDL_QUIT;
		SDL_PushEvent(&aQuit);
	}
}

// 0x436d80: one update of every scene, top one last; a scene that says it is done is removed, and with none left the game ends
static void UpdateScenes()
{
	bool aMore = gScenes.Begin();
	while (aMore) {
		if (!gScenes.mCursor->Update())
			gScenes.RemoveCursor();
		aMore = gScenes.Next();
	}
	if (gScenes.mCursor == nullptr)
		RequestClose();
}

// 0x436de0
static void DrawScenes()
{
	bool aMore = gScenes.Begin();
	while (aMore) {
		gScenes.mCursor->Draw();
		aMore = gScenes.Next();
	}
}

// 0x440650: the updates the clock is behind by, run one after the other (a long delay is not made up for)
void VeniceWidget::Update()
{
	MarkDirty();
	int aLoadState = gLoadState;
	if (aLoadState >= LoadStateLoadingScreen && aLoadState <= LoadStateLoadingDone) {
		// the data loads while the loading screen draws; the original does it on a thread
		if (aLoadState >= LoadStateLoading && ContinueLoadConfig(LoadSliceMilliseconds))
			gLoadState = LoadStateLoadingDone;
		return;
	}

	int aTickCount = (int)(gApp->GetClockTime() / UpdateMilliseconds);
	if (sTickBase == 0)
		sTickBase = aTickCount;
	int aUpdatesBehind = aTickCount - sTickBase;
	if (aUpdatesBehind > GiveUpBehindUpdates) {
		aUpdatesBehind = sPreviousBatch < MaxCatchUpUpdates + 1 ? sPreviousBatch : MaxCatchUpUpdates;
		sTickBase = aTickCount - aUpdatesBehind;
	}
	if (aUpdatesBehind == 0)
		return;
	for (int anUpdate = 0; anUpdate < aUpdatesBehind; anUpdate++) {
		// without the focus (once the logos are over), and while the focus counts down, time passes and nothing updates
		if ((gHasFocus || aLoadState <= LoadStateLogosShowing) && gFocusFrames != 1)
			UpdateScenes();
		sTickBase++;
	}
	sPreviousBatch = aUpdatesBehind;
}

// 0x4407d0: a scene that shows a picture: it waits (nothing is drawn), fades the picture in over half a second at the middle of
// the screen and keeps it there for two and a half seconds. The first is drawn over black.
static void MakeLogoScene(int theStartMilliseconds, const char* theImageName)
{
	Level* aScene = new Level();
	gScenes.Append(aScene);
	aScene->SetImage(theImageName, 0, 0, 0, 0);
	aScene->AddStage((float)theStartMilliseconds + (float)LogoFirstWaitMilliseconds, 0, 0, 0, nullptr);
	aScene->AddStage(LogoFadeMilliseconds, 0, 0, 1, nullptr);
	aScene->SetAttribute(AttributeX, gScreenWidth / 2.0f, gScreenWidth / 2.0f);
	aScene->SetAttribute(AttributeY, gScreenHeight / 2.0f, gScreenHeight / 2.0f);
	aScene->SetAttribute(AttributeAlpha, 0.0f, 1.0f);
	aScene->AddStage(LogoHoldMilliseconds, 0, 0, 1, nullptr);
	aScene->AddStage(LogoFadeMilliseconds, 0, 0, 1, nullptr);
	aScene->mDimmed = theStartMilliseconds == 0;
}

// 0x440930: PopCap's logo, the partner's logos (the pictures the partner file names) and the loading screen's picture, one after
// the other
static void MakeLogoScenes()
{
	if (gLoadState != LoadStateLogosToMake)
		return;
	int aStartMilliseconds = 0;
	MakeLogoScene(aStartMilliseconds, "gfx\\popcap.jpg");
	aStartMilliseconds += LogoSpacingMilliseconds;

	std::string aPartnerText;
	ReadGameFile(PartnerFileName, &aPartnerText);
	SceneTag aTag;
	const char* aText = ReadTag(aPartnerText.c_str(), &aTag);
	while (aTag.mName != "/Properties" && !(aTag.mName.empty() && *aText == '\0')) {
		if (aTag.Has("id") && strcmp(aTag.GetString("id"), "PartnerImage") == 0) {
			std::string anImageName(aTag.mContent == nullptr ? "" : aTag.mContent, aTag.mContentLength);
			MakeLogoScene(aStartMilliseconds, anImageName.c_str());
			aStartMilliseconds += LogoSpacingMilliseconds;
		}
		aText = ReadTag(aText, &aTag);
	}
	MakeLogoScene(aStartMilliseconds, "gfx\\loading.jpg");
	gLoadState = LoadStateLogosShowing;
}

// 0x440e60
void VeniceWidget::Draw(Graphics* g)
{
	gGraphics = g;

	// 0x440b30: once the logos are all done the loading screen takes over
	if (gLoadState == LoadStateLogosShowing && gScenes.mCursor == nullptr)
		gLoadState = LoadStateLoadingScreen;

	MakeLogoScenes();

	// 0x440b90: the loading screen's picture and the bar's
	if (gLoadState == LoadStateLoadingScreen) {
		mLoadingScreen.SetImage("gfx\\loading.jpg", 0, 0, 0, 0);
		if (mLoadingScreen.mImage != nullptr) {
			mLoadingScreen.mWorld[AttributeX] = gScreenWidth / 2.0f;
			mLoadingScreen.mWorld[AttributeY] = gScreenHeight / 2.0f;
		}
		mLoadingBar.SetImage("gfx\\loadbar.png", 0, 0, 0, 0);
		if (mLoadingBar.mImage != nullptr) {
			mLoadingBar.mWorld[AttributeX] = LoadingBarX;
			mLoadingBar.mWorld[AttributeY] = LoadingBarY;
		}
		gLoadState = LoadStateLoadingBar;
	}

	// 0x440c80: the loading starts (the original starts a thread here): the sound goes up, the random numbers start from the
	// clock (or the seed a test gave) and the config's tags are read in slices by the updates
	if (gLoadState == LoadStateLoadingBar) {
		gLoadState = LoadStateLoading;
		SeedRandom(gApp->mHasSeedParam ? (uint32_t)gApp->mRandSeed : (uint32_t)gApp->GetClockTime());
		BeginLoadConfig();
	}

	// 0x440d10: the loading screen, and the bar clipped to the percent that is loaded
	int aLoadState = gLoadState;
	if (aLoadState > LoadStateLogosShowing && aLoadState <= LoadStateLoadingDone) {
		mLoadingScreen.DrawSelf();
		if (mLoadingBar.mImage != nullptr) {
			int aBarWidth = (int)mLoadingBar.mImage->mFrameWidth;
			int aBarLeft = (int)mLoadingBar.mWorld[AttributeX] - aBarWidth / 2;
			int aLoadedWidth = (aBarWidth * aLoadState) / LoadingBarFullPercent;
			mLoadingBar.mClipLeft = aBarLeft;
			mLoadingBar.mClipTop = 0;
			mLoadingBar.mClipRight = aBarLeft + aLoadedWidth;
			mLoadingBar.mClipBottom = LoadingBarClipBottom;
			gGraphics->SetClipRect(aBarLeft, 0, aLoadedWidth, LoadingBarClipBottom);
			mLoadingBar.DrawSelf();
			if (aLoadState == LoadStateLoadingDone) {
				gLoadState = LoadStateRunning;
				gFocusFrames = 0;
			}
		}
	}

	// the original returns here, before drawing the scenes, while it is loading or for the frame the focus returns, but only
	// until it has drawn the scenes once, which its first frame does
	DrawScenes();

	// 0x440f30: while the window has no focus a sign says the game is paused, except over the main menu
	if (gLoadState > LoadStateLoadingDone)
		gFocusFrames--;
	bool aMainMenuShows = false;
	for (SceneObject* aScene = gScenes.mHead; aScene != nullptr; aScene = aScene->mNext) {
		if (strstr(aScene->mName, "main_menu") != nullptr)
			aMainMenuShows = true;
	}
	if (gLoadState > LoadStateLoadingDone && !gHasFocus) {
		if (!mPausedOverlayMade) {
			mPausedOverlayMade = true;
			mPausedOverlay.AddStage(1000.0f, 1, 1, 1, nullptr);
			mPausedOverlay.SetImage("gfx\\gamepaused.png", 0, 0, 0, 0);
			mPausedOverlay.SetAttribute(AttributeX, gScreenWidth / 2.0f, gScreenWidth / 2.0f);
			mPausedOverlay.SetAttribute(AttributeY, gScreenHeight / 2.0f - 1.0f, gScreenHeight / 2.0f - 1.0f);
		}
		if (!aMainMenuShows) {
			mPausedOverlay.Update();
			mPausedOverlay.Draw();
		}
	}
}

// 0x4411f0, 0x441210
void VeniceWidget::MouseEnter()
{
	gMouseInside = true;
}

void VeniceWidget::MouseLeave()
{
	gMouseInside = false;
}

// 0x441190, 0x4411c0
void VeniceWidget::MouseMove(int theX, int theY)
{
	gMouseX = theX;
	gMouseY = theY;
}

void VeniceWidget::MouseDrag(int theX, int theY)
{
	gMouseX = theX;
	gMouseY = theY;
}

// 0x441310: a click count of -1 is the right button and 3 the middle one, which the game does not read
void VeniceWidget::MouseDown(int theX, int theY, int theClickCount)
{
	if (theClickCount < 1 || theClickCount == 3) {
		if (theClickCount < 0) {
			PushKeyRing(KeyRingMouseRight);
			if (gKeyStates[KeyStateRightButton] == KeyStateNone)
				gKeyStates[KeyStateRightButton] = KeyStateNew;
		}
	} else {
		PushKeyRing(KeyRingMouseLeft);
		if (gKeyStates[KeyStateLeftButton] == KeyStateNone)
			gKeyStates[KeyStateLeftButton] = KeyStateNew;
	}
	Widget::MouseDown(theX, theY, theClickCount);
}

// 0x4413e0
void VeniceWidget::MouseUp(int theX, int theY, int theClickCount)
{
	if (theClickCount < 1 || theClickCount == 3) {
		if (theClickCount < 0)
			gKeyStates[KeyStateRightButton] = KeyStateNone;
	} else
		gKeyStates[KeyStateLeftButton] = KeyStateNone;
	Widget::MouseUp(theX, theY, theClickCount);
}

// 0x441230: the wheel counts as a press of the up or down key
void VeniceWidget::MouseWheel(int theDelta)
{
	AdvanceKeyRing();
	if (theDelta > 0)
		gKeyStates[KEYCODE_UP] = KeyStateNew;
	else if (theDelta < 0)
		gKeyStates[KEYCODE_DOWN] = KeyStateNew;
}

// 0x441430
void VeniceWidget::KeyDown(KeyCode theKey)
{
	PushKeyRing((int)theKey);
	if (gKeyStates[theKey & 0xff] == KeyStateNone)
		gKeyStates[theKey & 0xff] = KeyStateNew;
}

// 0x4414a0
void VeniceWidget::KeyUp(KeyCode theKey)
{
	// port: the framework's key codes are not all below 256
	if ((unsigned)theKey < (unsigned)KeyStateCount)
		gKeyStates[theKey] = KeyStateNone;
}
