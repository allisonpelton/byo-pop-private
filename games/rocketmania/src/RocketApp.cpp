// The application class of Rocket Mania Deluxe 1.0 (RocketManiaGame 0x42a0d0-0x42f730). See RocketApp.h and
// docs/GAME_NOTES.md.

#include "RocketApp.h"
#include "Screens.h"
#include "Dialogs.h"
#include "Board.h"
#include "Dialog.h"
#include "ButtonWidget.h"
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
	for (int i = 0; i < 8; i++)
		mHintFlags[i] = false;
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
bool RocketGameOver(RocketApp* theApp, const ScriptCommand& theCommand);

// port: the test scripts' "startgame" clicks the play link as soon as the loading screen shows it (the update at which
// the loading ends depends on the machine); it asks again at the next update until then
bool RocketApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	if (theCommand.mCommand == "autoplay")
		return RocketAutoplay(this, theCommand);
	if (theCommand.mCommand == "gameover")
		return RocketGameOver(this, theCommand);
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
		case DIALOG_OPTIONS:
		{
			// the options' DONE button: the screen mode and the cursors (the checkboxes act on nothing before)
			OptionsDialog* aDialog = (OptionsDialog*) GetDialog(DIALOG_OPTIONS);
			bool aWantWindowed = !aDialog->mFullscreenCheckbox->IsChecked();
			SwitchScreenMode(aWantWindowed, aDialog->m3DCheckbox->IsChecked());
			EnableCustomCursors(aDialog->mCursorsCheckbox->IsChecked());
			KillDialog(DIALOG_OPTIONS);
			break;
		}
		case DIALOG_RECORD_SCORE:
		{
			// the record dialog's OK: the name goes into the lists (which are under it) and is kept for the next time
			RecordScoreDialog* aDialog = (RecordScoreDialog*) GetDialog(DIALOG_RECORD_SCORE);
			HighScoreDisplayDialog* aHighScores = (HighScoreDisplayDialog*) GetDialog(DIALOG_HIGH_SCORES);
			if ((aDialog != NULL) && (aHighScores != NULL))
			{
				aHighScores->AddScore(aDialog->mEdit->mString);
				mLastScoreName = aDialog->mEdit->mString;
			}
			KillDialog(DIALOG_RECORD_SCORE);
			break;
		}
		default:
			// (the level dialogs: not translated yet)
			KillDialog(anId - 2000);
			break;
		}
	}
	else if ((anId >= 3000) && (anId < 4000))
		KillDialog(anId - 3000);
	// the buttons beside the board (0x42ed10's tail): 1 options, 2 quit, 3 pause
	if (theId == 1)
	{
		if (mBoard != NULL && mBoard->mState == 0)
			ShowOptions(false);
	}
	else if (theId == 2)
		QuitGame();
	else if (theId == 3)
		ShowPauseDialog();
}

// 0x42d4e0: every dialog made with DoDialog: the picture and button of the game's own, its fonts and colours, and the
// framework's default place (the callers move it)
Dialog* RocketApp::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
	const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	Dialog* aDialog = new Dialog2004(mImages[IMAGE_DIALOG], mImages[IMAGE_DIALOGBUTTON], theDialogId, isModal,
		theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
	aDialog->SetButtonFont(mFonts[FONT_VERDANA12_BOLD]);
	aDialog->SetHeaderFont(mFonts[FONT_VERDANA18_BOLD]);
	aDialog->SetLinesFont(mFonts[FONT_VERDANA12_BOLD]);
	aDialog->SetColor(0, mDialogColors[0]);
	aDialog->SetColor(1, mDialogColors[1]);
	aDialog->SetColor(3, mDialogColors[2]);
	aDialog->mContentInsets = Insets(0x24, 0x24, 0x24, 0x24);
	aDialog->Resize(0xce, 0x2a, 0x15c, aDialog->GetPreferredHeight(0x15c));
	return aDialog;
}

// 0x42d620: when the last dialog is gone the board gets the keyboard focus again
bool RocketApp::KillDialog(int theDialogId)
{
	bool aResult = SexyApp::KillDialog(theDialogId);
	if (aResult && GetDialogCount() == 0)
		mWidgetManager->SetFocus(mBoard);
	return aResult;
}

// 0x42f270 and 0x42f290: a modal dialog, and the window losing the focus, pause the board; closing the dialog and getting
// the focus back resume it
void RocketApp::ModalOpen()
{
	if (mBoard != NULL)
		mBoard->F421f30(true);
}

void RocketApp::ModalClose()
{
	if (mBoard != NULL)
		mBoard->F421f30(false);
}

void RocketApp::GotFocus()
{
	if (mBoard != NULL)
		mBoard->F421f30(false);
}

void RocketApp::LostFocus()
{
	if (mBoard != NULL)
		mBoard->F421f30(true);
}

// 0x42e390: "Are you sure you want to quit the game?" with YES and NO, in the middle of the window
void RocketApp::ShowQuitDialog()
{
	Dialog* aDialog = DoDialog(DIALOG_QUIT, true, GT(0x004e463c), GT(0x004e7c5c), "", Dialog::BUTTONS_YES_NO);
	if (aDialog != NULL)
		aDialog->Resize(mWidth / 2 - aDialog->mWidth / 2, mHeight / 2 - aDialog->mHeight / 2, aDialog->mWidth, aDialog->mHeight);
}

// 0x42dcc0: the high scores over the whole window; theOpenedFrom is 0 after a game, 1 from the menu, 2 in a game and 3 from
// the options
void RocketApp::ShowHighScores(int theOpenedFrom)
{
	HighScoreDisplayDialog* aDialog = new HighScoreDisplayDialog(this, theOpenedFrom);
	aDialog->SetColor(0, Color(0xff, 0xff, 0xff));
	aDialog->SetColor(1, Color(0, 0xff, 0));
	aDialog->SetColor(3, Color(0, 0xff, 0));
	aDialog->mContentInsets = Insets(0x24, 0x24, 0x24, 0x24);
	aDialog->Resize(0, 0, mWidth, mHeight);
	AddDialog(DIALOG_HIGH_SCORES, aDialog);
	// (the talking head: unless opened in a game it goes to rest (0x4165f0), and in every case it is brought in front of the
	// dialog (0x42fdf3); the head is not ported yet)
}

// 0x42df40: the dialog for the name of a score, in the middle of the window, with a fanfare
void RocketApp::ShowRecordScore(const SexyString& theMessage)
{
	RecordScoreDialog* aDialog = new RecordScoreDialog(this, theMessage);
	int aHeight = aDialog->GetPreferredHeight(0x13c);
	aDialog->Resize(mWidth / 2 - 0x9e, mHeight / 2 - aHeight / 2, 0x13c, aHeight);
	AddDialog(DIALOG_RECORD_SCORE, aDialog);
	PlaySample(SOUND_TUBETRICKS_STING_ONE_V01);
}

// 0x42de20: the options dialog, in the middle of the screen; from the menu the talking head steps aside, in a game the
// menu music plays
void RocketApp::ShowOptions(bool theFromMenu)
{
	OptionsDialog* aDialog = new OptionsDialog(this, theFromMenu);
	aDialog->SetColor(0, mDialogColors[0]);
	aDialog->SetColor(1, mDialogColors[1]);
	aDialog->SetColor(3, mDialogColors[2]);
	int aHeight = aDialog->GetPreferredHeight(0x184);
	aDialog->Resize(0x7e, 0xf0 - aHeight / 2, 0x184, aHeight);
	AddDialog(DIALOG_OPTIONS, aDialog);
	if (theFromMenu)
	{
		// (0x4165f0: the talking head's move is not ported yet)
	}
	else
		mMusicInterface->FadeIn(0, -1, 0.2, false);
}

// 0x42d760: the credits, near the top of the screen
void RocketApp::ShowCredits()
{
	CreditsDialog* aDialog = new CreditsDialog(this);
	aDialog->SetColor(0, mDialogColors[0]);
	aDialog->SetColor(1, mDialogColors[1]);
	aDialog->SetColor(3, mDialogColors[0]);
	aDialog->Resize(0xc4, 0x10, 0x184, aDialog->GetPreferredHeight(0x184));
	AddDialog(DIALOG_CREDITS, aDialog);
}

// 0x42e330: a dialog in the middle of the window
void RocketApp::CenterDialog(Dialog* theDialog)
{
	if (theDialog != NULL)
		theDialog->Resize(mWidth / 2 - theDialog->mWidth / 2, mHeight / 2 - theDialog->mHeight / 2, theDialog->mWidth,
			theDialog->mHeight);
}

// 0x42f2b0: the framework's values, then the last name typed for a high score and the hints
void RocketApp::WriteToRegistry()
{
	SexyApp::WriteToRegistry();
	if (mLastScoreName.length() != 0)
		RegistryWriteString("LastScoreName", mLastScoreName);
	RegistryWriteBoolean("HintsDisabled", mNoHints);
	for (int i = 0; i < 8; i++)
	{
		char aName[200];
		snprintf(aName, sizeof(aName), "HintsDisabledFlag%d", i);
		RegistryWriteBoolean(aName, mHintFlags[i]);
	}
}

// 0x42f4b0
void RocketApp::ReadFromRegistry()
{
	SexyApp::ReadFromRegistry();
	std::string aName;
	if (RegistryReadString("LastScoreName", &aName))
		mLastScoreName = aName;
	bool aFlag;
	if (RegistryReadBoolean("HintsDisabled", &aFlag))
		mNoHints = aFlag;
	for (int i = 0; i < 8; i++)
	{
		char aKey[200];
		snprintf(aKey, sizeof(aKey), "HintsDisabledFlag%d", i);
		if (RegistryReadBoolean(aKey, &aFlag))
			mHintFlags[i] = aFlag;
	}
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
	RemoveBoardButtons();
	if (mBoard != NULL)
	{
		// a game in progress is saved (0x41b420: unless it is finished) before the board goes
		if (mBoard->mState != 2)
			mBoard->Save();
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

// 0x41d380: the mode's saved game is deleted (a new game of the mode, or the loaded one once it is in the board)
bool RocketApp::DeleteSavedGame(int theMode)
{
	return Platform::RemoveUserFile(SavePath(theMode));
}

// 0x42f730 (slot 31, Shutdown): a game in progress is saved on the way out
void RocketApp::ShutdownHook()
{
	if (mBoard != NULL && mBoard->mState != 2)
		mBoard->Save();
	SexyApp::ShutdownHook();
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

// 0x42ea30: a game begins (or is continued): the board replaces the one in progress.
void RocketApp::StartBoard(bool theContinuing)
{
	KillDialog(3);
	for (int aSong = 0; aSong < 4; aSong++)
		mMusicInterface->FadeOut(aSong, true, 0.005);
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
	AddBoardButtons();
	if (!theContinuing)
	{
		DeleteSavedGame(mMode);
		KillDialog(1);
	}
}

// 0x42d1a0: the three buttons on the left of the screen. Each shows a picture over the column's own while the pointer is on
// it (the first image) and while it is pressed (the second); the label is part of the column's picture.
void RocketApp::AddBoardButtons()
{
	static const struct { int mId; int mY; int mOver; int mDown; } sButtons[3] =
	{
		{ 3, 0xf5, 49, 50 },    // pause
		{ 1, 0xd2, 40, 41 },    // options
		{ 2, 0x1b8, 38, 39 }    // quit
	};
	for (int i = 0; i < 3; i++)
	{
		ButtonWidget* aButton = new ButtonWidget(sButtons[i].mId, this);
		aButton->Resize(10, sButtons[i].mY, 0x6b, 0x23);
		aButton->mOverImage = mImages[sButtons[i].mOver];
		aButton->mDownImage = mImages[sButtons[i].mDown];
		aButton->mDoFinger = true;
		aButton->mMouseInsets = Insets(1, 1, 3, 3);
		mWidgetManager->AddWidget(aButton);
		mBoardButtons.push_back(aButton);
	}
}

// 0x42d270: the buttons are disabled while the board is paused
void RocketApp::DisableBoardButtons(bool theDisabled)
{
	for (std::list<ButtonWidget*>::iterator anItr = mBoardButtons.begin(); anItr != mBoardButtons.end(); ++anItr)
		(*anItr)->SetDisabled(theDisabled);
}

void RocketApp::RemoveBoardButtons()
{
	for (std::list<ButtonWidget*>::iterator anItr = mBoardButtons.begin(); anItr != mBoardButtons.end(); ++anItr)
	{
		mWidgetManager->RemoveWidget(*anItr);
		SafeDeleteWidget(*anItr);
	}
	mBoardButtons.clear();
}

// 0x42d820: the pause button: a modal dialog with one button; the board under a modal dialog is not updated, so the game
// stands still until the button kills the dialog (the generic path of ButtonDepress)
void RocketApp::ShowPauseDialog()
{
	if (mBoard == NULL || mBoard->mState != 0)
		return;
	Dialog* aDialog = DoDialog(8, true, GT(0x004e50bc), GT(0x004e7bd8), GT(0x004e7c18), Dialog::BUTTONS_FOOTER);
	if (aDialog != NULL)
	{
		int aHeight = aDialog->GetPreferredHeight(0x15c);
			aDialog->Resize(0x92, 0xf0 - aHeight / 2, 0x15c, aHeight);
	}
}

// 0x42e530: the quit button: asks, in a game in progress, whether to go back to the menu (the game is saved); in a finished
// game goes back at once
void RocketApp::QuitGame()
{
	if (mBoard == NULL)
		return;
	if (mBoard->mState != 2)
	{
		if (mBoard->mState != 0)
			return;
		Dialog* aDialog = DoDialog(0, true, GT(0x004e7c84), GT(0x004e7c98), "", Dialog::BUTTONS_YES_NO);
		if (aDialog != NULL)
		{
			int aHeight = aDialog->GetPreferredHeight(0x15c);
			aDialog->Resize(0x92, 0xf0 - aHeight / 2, 0x15c, aHeight);
		}
		return;
	}
	ShowGameSelector();
}
