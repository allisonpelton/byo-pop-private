// The application class of Rocket Mania Deluxe 1.0 (RocketManiaGame 0x42a0d0-0x42f730). See RocketApp.h and
// docs/GAME_NOTES.md.

#include "RocketApp.h"
#include "Screens.h"
#include "Board.h"
#include "Dialog.h"
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
#include "platform/UserFiles.h"

#include <SDL.h>

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
	mMode = 0;
	mDifficulty = 0;
	mNoPopLink = false;
	mNoHints = false;
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

// testhooks/Autoplay.cpp: plays a game with random clicks
bool RocketAutoplay(RocketApp* theApp, const ScriptCommand& theCommand);

// port: the test scripts' "startgame" clicks the play link as soon as the loading screen shows it (the update at which
// the loading ends depends on the machine); it asks again at the next update until then
bool RocketApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	if (theCommand.mCommand == "autoplay")
		return RocketAutoplay(this, theCommand);
	if (theCommand.mCommand != "startgame")
		return SexyApp::HandleScriptCommand(theCommand);
	if ((mLoadingScreen != NULL) && !mShutdown)
	{
		if (!mLoadingScreen->mLinkShown)
			AddScriptCommand(mUpdateCount + 1, theCommand.mCommand, theCommand.mArgs);
		else
		{
			int aX = mLoadingScreen->mPlayLink->mX + mLoadingScreen->mPlayLink->mWidth / 2;
			int aY = mLoadingScreen->mPlayLink->mY + mLoadingScreen->mPlayLink->mHeight / 2;
			mWidgetManager->MouseMove(aX, aY);
			mWidgetManager->MouseDown(aX, aY, 1);
			mWidgetManager->MouseUp(aX, aY, 1);
		}
	}
	return true;
}

// 0x42ed10: the dialogs' buttons arrive as 2000 + dialog id (yes, ok) and 3000 + dialog id (no, cancel)
void RocketApp::ButtonDepress(int theId)
{
	int anId = theId % 10000;
	if ((anId >= 2000) && (anId < 3000))
	{
		switch (anId - 2000)
		{
		case DIALOG_GAME_OVER:
			KillDialog(DIALOG_GAME_OVER);
			ShowGameSelector();
			break;
		case DIALOG_QUIT:
			KillDialog(DIALOG_QUIT);
			Shutdown();
			break;
		case DIALOG_HINT:
			// (the original's hint dialog starts the game that waited for it: not translated yet)
			KillDialog(DIALOG_HINT);
			break;
		case DIALOG_DIFFICULTY:
			KillDialog(DIALOG_DIFFICULTY);
			break;
		default:
			// (the options, high score, record and level dialogs: not translated yet)
			KillDialog(anId - 2000);
			break;
		}
	}
	else if ((anId >= 3000) && (anId < 4000))
		KillDialog(anId - 3000);
}

// 0x42e390: "Are you sure you want to quit the game?" with YES and NO, in the middle of the window
void RocketApp::ShowQuitDialog()
{
	Dialog* aDialog = DoDialog(DIALOG_QUIT, true, GT(0x004e463c), GT(0x004e7c5c), "", Dialog::BUTTONS_YES_NO);
	if (aDialog != NULL)
		aDialog->Resize(mWidth / 2 - aDialog->mWidth / 2, mHeight / 2 - aDialog->mHeight / 2, aDialog->mWidth, aDialog->mHeight);
}

// 0x42dcc0 (not translated yet)
void RocketApp::ShowHighScores(bool theFromMenu)
{
	(void) theFromMenu;
	SDL_Log("ShowHighScores: not translated yet");
}

// 0x42de20 (not translated yet)
void RocketApp::ShowOptions(bool theFromMenu)
{
	(void) theFromMenu;
	SDL_Log("ShowOptions: not translated yet");
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

// 0x42e010: the menu replaces whatever was on screen (the game in progress is closed first)
void RocketApp::ShowGameSelector()
{
	KillDialog(3);
	if (mBoard != NULL)
	{
		// (closing the game in progress: saving it unless it is over, the Board is not translated yet)
		mWidgetManager->RemoveWidget((Widget*) mBoard);
		mBoard = NULL;
	}
	if (mGameSelector != NULL)
	{
		SafeDeleteWidget(mGameSelector);
		mWidgetManager->RemoveWidget(mGameSelector);
	}

	mGameSelector = new GameSelector(this);
	mGameSelector->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mGameSelector);
	mWidgetManager->BringToBack(mGameSelector);
	mWidgetManager->SetFocus(mGameSelector);

	for (int i = 1; i < 4; i++)
		mMusicInterface->StopMusic(i);
	if (!mMusicInterface->IsPlaying(SONG_MENU))
		mMusicInterface->PlayMusic(SONG_MENU, 0, false);
	mMusicInterface->FadeIn(SONG_MENU, -1, 0.2, false);
}

// app+0x554 to +0x594: the texts that name the modes and difficulties (the addresses of the 1.0 image)
static const unsigned int gModeNames[3] = {0x004e79ec, 0x004e79e4, 0x004e79d8};
static const unsigned int gModeTitles[3] = {0x004e79c4, 0x004e79ac, 0x004e7994};
static const unsigned int gHighScoreTitles[3] = {0x004e7980, 0x004e796c, 0x004e7954};
static const unsigned int gDifficultyNames[4] = {0, 0x004e794c, 0x004e793c, 0x004e792c};
static const unsigned int gDifficultyNamesUpper[4] = {0, 0x004e7944, 0x004e7934, 0x004e7924};

const SexyString& RocketApp::ModeName(int theMode) { return GT(gModeNames[theMode]); }
const SexyString& RocketApp::ModeTitle(int theMode) { return GT(gModeTitles[theMode]); }
const SexyString& RocketApp::HighScoreTitle(int theMode) { return GT(gHighScoreTitles[theMode]); }
const SexyString& RocketApp::DifficultyName(int theDifficulty) { return GT(gDifficultyNames[theDifficulty]); }
const SexyString& RocketApp::DifficultyNameUpper(int theDifficulty) { return GT(gDifficultyNamesUpper[theDifficulty]); }

// 0x42f700: the original keeps "Players//savea.dat" (classic), "saveb" (arcade) and "savec" (strategy)
std::string RocketApp::SavePath(int theMode)
{
	if (theMode == 1)
		return "Players/savea.dat";
	return (theMode == 0) ? "Players/saveb.dat" : "Players/savec.dat";
}

static unsigned int ReadLE32(const std::string& theData, size_t theOffset)
{
	return (unsigned int) (unsigned char) theData[theOffset] | ((unsigned int) (unsigned char) theData[theOffset + 1] << 8) |
		((unsigned int) (unsigned char) theData[theOffset + 2] << 16) | ((unsigned int) (unsigned char) theData[theOffset + 3] << 24);
}

// 0x4062b0: the file exists and starts with the save magic
bool RocketApp::HasSavedGame(int theMode)
{
	std::string aData;
	if (!Platform::ReadHostFile(Platform::VFS::Get().GetUserDir() + "/" + SavePath(theMode), &aData))
		return false;
	return (aData.size() >= 4) && (ReadLE32(aData, 0) == 0xFADA0002);
}

// 0x4036f0: the header of a saved game, as the continue dialog shows it: magic, one value that is not used, the
// difficulty, the score and the level index
bool RocketApp::ReadSavedGame(int theMode, int* theDifficulty, int* theScore, int* theLevel)
{
	std::string aData;
	if (!Platform::ReadHostFile(Platform::VFS::Get().GetUserDir() + "/" + SavePath(theMode), &aData))
		return false;
	if ((aData.size() < 20) || (ReadLE32(aData, 0) != 0xFADA0002))
		return false;
	*theDifficulty = (int) ReadLE32(aData, 8);
	*theScore = (int) ReadLE32(aData, 12);
	*theLevel = (int) ReadLE32(aData, 16);
	return true;
}

// 0x42d670 (not translated yet): no mode explanation opens
bool RocketApp::ShowModeHint(int theHint, bool theForce)
{
	(void) theHint;
	(void) theForce;
	return false;
}

// 0x42ea30: a game begins (or is continued): the board replaces the one in progress. Left out: the fade of the four
// songs (app+0x2f4), the three buttons on the left of the screen (0x42d1a0, which
// the Board's ButtonDepress answers) and the deletion of this mode's saved game when it is a new one (0x41d380).
void RocketApp::StartBoard(bool theContinuing)
{
	KillDialog(3);
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = NULL;
	}
	// 0x42e190: the menu is taken away (and deleted) when a game begins
	if (mGameSelector != NULL)
	{
		mWidgetManager->RemoveWidget(mGameSelector);
		SafeDeleteWidget(mGameSelector);
		mGameSelector = NULL;
	}
	SDL_Log("StartBoard: mode %d difficulty %d", mMode, mDifficulty);
	mBoard = new Board(this, theContinuing);
	mBoard->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mBoard);
	mWidgetManager->BringToFront(mBoard);
	mWidgetManager->SetFocus(mBoard);
	if (!theContinuing)
		KillDialog(1);
}
