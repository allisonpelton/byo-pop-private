// ThunderballApp.cpp: Peggle's application class.

#include "ThunderballApp.h"

#include "BlendedImage.h"
#include "MusicInterface.h"
#include "Res.h"
#include "ResourceManager.h"

using namespace Sexy;

ThunderballApp* Sexy::gThunderballApp = nullptr;
bool Sexy::gColorblind = false;
BlendedImage* Sexy::gPegImages[NUM_PEG_IMAGES] = {};
BlendedImage* Sexy::gPegShadowImage = nullptr;

// the slots of the two pieces of music the app loads; the other music is loaded with its level (0x42ca30)
static const int MUSIC_ODE_TO_JOY = 0;
static const int MUSIC_OPENING = 2;

// 0x426f10
ThunderballApp::ThunderballApp()
{
	gThunderballApp = this;
	mProdName = "Peggle";
	mTitle = "Peggle"; // the window's title (the original's is "Peggle Deluxe"; the user's rule of 2026-11-18 keeps Deluxe off the titles)
	mRegKey = "PopCap\\Peggle";
	mWidth = 800;
	mHeight = 600;
}

ThunderballApp::~ThunderballApp()
{
	for (int i = 0; i < NUM_FEVER_STAR_COLORS; i++)
		for (int j = 0; j < NUM_FEVER_STAR_SIZES; j++)
			delete mFeverStars[i][j];
	for (int i = 0; i < NUM_BRICK_IMAGES; i++)
		delete mBrickImages[i];
	for (int i = 0; i < NUM_PEG_IMAGES; i++)
	{
		delete gPegImages[i];
		gPegImages[i] = nullptr;
	}
	delete gPegShadowImage;
	gPegShadowImage = nullptr;
}

// 0x42ca30
void ThunderballApp::Init()
{
	SexyApp::Init();
	LoadResourceManifest();
	mResourceManager->LoadResources("Init");
	if (!ExtractInitResources(mResourceManager))
		ShowResourceError(true);
	mResourceManager->LoadResources("Load");
	if (!ExtractLoadResources(mResourceManager))
		ShowResourceError(true);

	// the music of the menus and the pictures of the mouse pointer
	mMusicInterface->LoadMusic(MUSIC_ODE_TO_JOY, "music/odetojoy.ogg");
	mMusicInterface->LoadMusic(MUSIC_OPENING, "music/opening.ogg");
	EnableCustomCursors(true);
	SetCursorImage(CURSOR_POINTER, IMAGE_CURSOR_POINTER);
	SetCursorImage(CURSOR_HAND, IMAGE_CURSOR_HAND);
	SetCursorImage(CURSOR_DRAGGING, IMAGE_CURSOR_DRAGGING);
	SetCursorImage(CURSOR_TEXT, IMAGE_CURSOR_TEXT);

	// the game's data
	mCharacterMgr.Load();
	mHighScoreMgr.Load();
	mProfileMgr.Load();
	mStageMgr.Load(nullptr);
	mStatsMgr.Load(nullptr);
	mCheatStatsMgr.Load("LevelEditor/stats_cheat.dat");
	mTrophyMgr.Load(nullptr);

	// the player who played last (the registry's "CurUser"), or any
	if (mCurrentPlayer == nullptr)
	{
		std::string aName;
		if (RegistryReadString("CurUser", &aName))
			mCurrentPlayer = mProfileMgr.GetProfile(aName);
	}
	if (mCurrentPlayer == nullptr)
		mCurrentPlayer = mProfileMgr.GetAnyProfile();
	if (IsScreenSaver() && mCurrentPlayer == nullptr)
		return; // (the original shows "No User Profile" and quits)
	if (mStatsMgr.mName.empty() && mCurrentPlayer != nullptr)
		mStatsMgr.mName = mCurrentPlayer->mName;
	mLevelMusicPending = true; // the loading screen loads it
	ShowLoadingScreen();
}
