// The application class of Rocket Mania Deluxe 1.0 (RocketManiaGame 0x42a0d0-0x42f730). See RocketApp.h and
// docs/GAME_NOTES.md.

#include "RocketApp.h"
#include "Screens.h"
#include "GameText.h"
#include "Compat2004.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "DDImage.h"
#include "MemoryImage.h"
#include "MusicInterface.h"
#include "SoundManager.h"
#include "WidgetManager.h"
#include "platform/VFS.h"

#include <cmath>
#include <cstdio>
#include <cstring>

using namespace Sexy;

RocketApp* Sexy::gApp = NULL;

// the fonts that Init creates (0x42b9b0) and the loading procedure's (0x42bdb0), with the original's slot
static const struct { int mFont; const char* mFile; } gInitFonts[] =
{
	{FONT_ARIAL8, "data\\Arial8.txt"},
	{FONT_ARIAL10_BOLD, "data\\Arial10Bold.txt"},
	{FONT_DOMO14_BOLD, "data\\DomoAregato14Bold.txt"}
};

static const struct { int mFont; const char* mFile; } gLoadFonts[] =
{
	{FONT_ARIAL8_BOLD, "data\\Arial8Bold.txt"},
	{FONT_ARIAL10, "data\\Arial10.txt"},
	{FONT_VERDANA12_BOLD, "data\\Verdana12Bold.txt"},
	{FONT_VERDANA18_BOLD, "data\\Verdana18Bold.txt"},
	{FONT_SHOWCARD22, "data\\ShowcardGothic22.txt"},
	{FONT_DOMO36_BOLD, "data\\DomoAregato36Bold.txt"},
	{FONT_DOMO22_BOLD, "data\\DomoAregato22Bold.txt"},
	{FONT_DOMO11_BOLD, "data\\DomoAregato11Bold.txt"},
	{FONT_MYRIAD11, "data\\MyriadCondensedWeb11.txt"}
};

// 0x42aec0
RocketApp::RocketApp()
{
	gApp = this;
	mProdName = "RocketMania";
	mProductVersion = "1.01";      // the exe's version resource says ProductVersion 1.01
	mTitle = "Rocket Mania Deluxe " + mProductVersion;
	mRegKey = "PopCap\\RocketMania";
	mWidth = GAME_WIDTH;
	mHeight = GAME_HEIGHT;

	mImages = NULL;
	for (int i = 0; i < NUM_FONTS; i++)
		mFonts[i] = NULL;
	mBubbleFlipH = NULL;
	mBubbleFlipV = NULL;
	// the dialogs' three colours (allocated in the ctor with FUN_004377f3)
	mDialogColors[0] = Color(0x1a, 0x5e, 0x49);
	mDialogColors[1] = Color(0x39, 0x79, 0x68);
	mDialogColors[2] = Color(0x49, 0x8c, 0x7a);
	mCheckChecked = NULL;
	mCheckUnchecked = NULL;
	memset(mPieces, 0, sizeof(mPieces));
	for (int i = 0; i < 7; i++)
		mHeadFrames[i] = NULL;
	mHeadRotate1 = NULL;
	mHeadRotate2 = NULL;
	for (int i = 0; i < 4; i++)
		mRocketHues[i] = NULL;
	mLoadingScreen = NULL;
	mGameSelector = NULL;
	mBoard = NULL;
	mNoPopLink = false;
	mIsRegistered = true;      // the Steam build plays as the registered game (properties/partner.xml: NoReg)
}

RocketApp::~RocketApp()
{
	if (mBoard != NULL)
		mWidgetManager->RemoveWidget((Widget*) mBoard);
	delete mBubbleFlipH;
	delete mBubbleFlipV;
	delete mCheckChecked;
	delete mCheckUnchecked;
	for (int r = 0; r < NUM_PIECE_ROWS; r++)
		for (int c = 0; c < NUM_PIECE_COLUMNS; c++)
			for (int v = 0; v < NUM_PIECE_VARIANTS; v++)
				delete mPieces[r][c][v];
	for (int i = 0; i < 7; i++)
		delete mHeadFrames[i];
	for (int i = 0; i < 4; i++)
		delete mRocketHues[i];
	if (mImages != NULL)
		delete[] mImages;
}

// 0x42b7d0: images\<name>
bool RocketApp::LoadImage(int theImageId)
{
	std::string aPath = std::string("images\\") + gImageNames[theImageId];
	mImages[theImageId] = GetImage(aPath);
	if (mImages[theImageId] == NULL)
	{
		Popup("Failed to load " + aPath);
		mLoadingFailed = true;
		return false;
	}
	return true;
}

// 0x46db9c: a new image holding the part theRect of theImage
MemoryImage* RocketApp::CopyImage(Image* theImage, const Rect& theRect)
{
	MemoryImage* aCopy = new MemoryImage(this);
	aCopy->Create(theRect.mWidth, theRect.mHeight);
	Graphics g(aCopy);
	g.DrawImage(theImage, -theRect.mX, -theRect.mY);
	return aCopy;
}

// 0x42b9b0
void RocketApp::Init()
{
	SexyApp::Init();

	// the 2003 SexyApp::Init seeds the C library generator with the tick count; -seed=N makes a run repeatable
	CrtSRand(mHasSeedParam ? (unsigned int) mRandSeed : (unsigned int) GetTickCount());
	LoadGameTexts();

	mNoPopLink = GetBoolean("NoPopLink", false);

	mImages = new Image*[NUM_IMAGES];
	for (int i = 0; i < NUM_IMAGES; i++)
		mImages[i] = NULL;
	// the cursors and the loading screen's pictures are loaded here, the rest in the loading procedure
	for (int i = 0; i < 7; i++)
	{
		if (!LoadImage(i))
		{
			Shutdown();
			return;
		}
	}
	for (size_t i = 0; i < sizeof(gInitFonts) / sizeof(gInitFonts[0]); i++)
		mFonts[gInitFonts[i].mFont] = new ImageFont(this, gInitFonts[i].mFile);

	mLoadingScreen = new LoadingScreen(this);
	mLoadingScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mLoadingScreen);

	SetCursorImage(0, mImages[IMAGE_CURSOR_POINTER]);
	SetCursorImage(1, mImages[IMAGE_CURSOR_HAND]);
	SetCursorImage(2, mImages[IMAGE_CURSOR_DRAGGING]);
	SetCursorImage(3, mImages[IMAGE_CURSOR_TEXT]);
}

// 0x42bdb0: the music, the sounds, the other fonts and images, and the pictures made from them
void RocketApp::LoadingThreadProc()
{
	// the music: the in-game and the menu piece are joined from their segments first (when the game folder has no
	// cached_*.wav of them), then the four files are loaded
	AssembleMusic("music\\Tubetricks In-Game One v02", 'I');
	AssembleMusic("music\\Tubetricks Menu One v02", 'C');
	for (int i = 0; i < NUM_MUSIC; i++)
		mMusicInterface->LoadMusic(i, gMusicNames[i]);
	mMusicInterface->FadeIn(SONG_MENU, -1, 0.002, false);

	mNumLoadingThreadTasks = 0x150;
	if (!mSoundManager->Initialized())
		mCompletedLoadingThreadTasks += NUM_SOUNDS;
	else
	{
		for (int i = 0; i < NUM_SOUNDS; i++)
		{
			if (!mSoundManager->LoadSound(i, std::string("sounds\\") + gSoundNames[i]))
			{
				Popup(std::string("Failed to load sounds\\") + gSoundNames[i]);
				mLoadingFailed = true;
				return;
			}
			mCompletedLoadingThreadTasks++;
		}
	}

	for (size_t i = 0; i < sizeof(gLoadFonts) / sizeof(gLoadFonts[0]); i++)
	{
		mFonts[gLoadFonts[i].mFont] = new ImageFont(this, gLoadFonts[i].mFile);
		if (gLoadFonts[i].mFont == FONT_SHOWCARD22)
			mFonts[FONT_SHOWCARD22]->mAscent = 0x1e;
		mCompletedLoadingThreadTasks++;
	}

	// the other images, after the first seven (the loop ends at the sounds' names, entry 114)
	for (int i = 7; i < NUM_IMAGES; i++)
	{
		if (!LoadImage(i))
			return;
		mCompletedLoadingThreadTasks++;
	}

	GenerateImages();
}

// 0x42eb50
void RocketApp::ButtonPress(int theId)
{
	PlaySample(SOUND_UI_LITTLE_BEEP_05B);
}

// 0x42d470
void RocketApp::LoadingScreenDone()
{
	// (the original also starts the trial clock here in its minutes mode: not ported, the game is registered)
	mWidgetManager->RemoveWidget(mLoadingScreen);
	SafeDeleteWidget(mLoadingScreen);
	mLoadingScreen = NULL;
	ShowGameSelector();
}

// 0x42e010 (not translated yet)
void RocketApp::ShowGameSelector()
{
}
