#include "WinDMApp.h"
#include "Board.h"
#include "TitleScreen.h"
#include "HighScoreWidget.h"
#include "HighScoreDialog.h"
#include "OptionsDialog.h"
#include "RecordScoreDialog.h"
#include "GameConfig.h"
#include "ModIndicators.h"

#include "WidgetManager.h"
#include "ButtonWidget.h"
#include "Checkbox.h"
#include "EditWidget.h"
#include "Dialog.h"
#include "ImageFont.h"
#include "DDImage.h"
#include "MemoryImage.h"
#include "Graphics.h"
#include "SoundManager.h"
#include "SDLMusicInterface.h"
#include "PakInterface.h"
#include "testhooks/StateHash.h"
#include "testhooks/Autoplay.h"

#include <SDL.h>
#include <cstdio>
#include <cstdlib>
#include <sstream>

using namespace Sexy;

// 0x4d4d82
WinDMApp::WinDMApp()
{
	mShowInstructions = true;
	mImages = NULL;
	mGemShrinkImages = NULL;
	mTitleScreen = NULL;
	mBoard = NULL;
	mNoPopLink = false;

	mProdName = "Bejeweled";
	// port: the original's SexyAppBase ctor reads this from the exe's version resource (ProductVersion)
	mProductVersion = "1.87";
	mTitle = "Bejeweled Deluxe " + mProductVersion;
	mRegKey = "PopCap\\Bejeweled";

	mAlphaDigitsFont = NULL;
	mScoreFont = NULL;
	mHeaderFont = NULL;
	mMainFont = NULL;
	mHighScoreWidget = NULL;
	mOverFont = NULL;
	for (int i = 0; i < 6; i++)
	{
		mUnusedImagesA[i] = NULL;
		mUnusedImagesB[i] = NULL;
	}
	for (int i = 0; i < NUM_TIMERBAR_IMAGES; i++)
		mTimerBarImages[i] = NULL;

	mCustomCursorsEnabled = false;
	mIsDiamondMine = false;

	// port: the original leaves these to the zero-initialised global app object
	mNormalPointsBase = 0;
	mNormalPointsPerLevel = 0.0;
	mNormalMultBase = 0.0;
	mNormalMultPerLevel = 0.0;
	mTimedPointsBase = 0;
	mTimedPointsPerLevel = 0.0;
	mTimedMultBase = 0.0;
	mTimedMultPerLevel = 0.0;
	mTimeDrainBase = 0.0;
	mTimeDrainPerLevel = 0.0;
	mTimeDrainAccelBase = 0.0;
	mTimeDrainAccelPerLevel = 0.0;
	mNormalModeButton = NULL;
	mTimedModeButton = NULL;
	mTimedMode = false;

	// port: test hooks
	mTraceHashFile = NULL;
	mAutoplay = false;
	mAutoplaySeed = 0;
	mAutoplayTimed = false;
	mAutoplayCap = 200000;
	mAutoplayBot = NULL;
}

// 0x4d4b08
WinDMApp::~WinDMApp()
{
	if (mImages != NULL)
	{
		for (int i = 0; i < NUM_IMAGES; i++)
			delete mImages[i];
		delete [] mImages;
	}

	if (mGemShrinkImages != NULL)
	{
		for (int aColor = 0; aColor < Board::NUM_GEM_COLORS; aColor++)
		{
			for (int aFrame = 0; aFrame < NUM_SHRINK_FRAMES; aFrame++)
				delete mGemShrinkImages[aColor][aFrame];
			delete [] mGemShrinkImages[aColor]; // port: the original leaks the row arrays and the table
		}
		delete [] mGemShrinkImages;
	}

	for (std::list<ButtonWidget*>::iterator anItr = mButtonList.begin(); anItr != mButtonList.end(); ++anItr)
	{
		mWidgetManager->RemoveWidget(*anItr);
		delete *anItr;
	}

	for (int i = 0; i < 6; i++)
	{
		delete mUnusedImagesA[i];
		delete mUnusedImagesB[i];
	}

	for (int i = 0; i < NUM_TIMERBAR_IMAGES; i++)
		delete mTimerBarImages[i];

	if (mHighScoreWidget != NULL)
	{
		mWidgetManager->RemoveWidget(mHighScoreWidget);
		delete mHighScoreWidget;
	}

	delete mAlphaDigitsFont;
	delete mScoreFont; // port: the original never deletes the score font
	delete mHeaderFont;
	delete mMainFont;
	delete mOverFont;

	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		delete mBoard;
	}

	if (mTitleScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mTitleScreen);
		delete mTitleScreen;
	}

	// port: test hooks
	delete mAutoplayBot;
	if (mTraceHashFile != NULL)
		fclose(mTraceHashFile);
}

// 0x4d43f9
void WinDMApp::Shutdown()
{
	if (!mShutdown)
	{
		// The deferred delete of the board saves the game a second time and counts it as played
		if ((mBoard != NULL) && (mBoard->IsGameInProgress()))
		{
			mBoard->SaveGame();
			mWidgetManager->RemoveWidget(mBoard);
			SafeDeleteWidget(mBoard);
			mBoard = NULL;
		}

		SexyApp::Shutdown();
	}
}

// port (web): the save Shutdown() would write now (see SexyAppBase::UpdateUserDataBackup)
bool WinDMApp::GetShutdownSave(std::string* theFileName, Buffer* theBuffer)
{
	if ((mBoard == NULL) || (!mBoard->IsGameInProgress()))
		return false;

	*theFileName = Board::GetSaveGameFileName();
	mBoard->SaveGameSnapshot(theBuffer);
	return true;
}

// port (web): Shutdown() writes the registry before the deferred delete of the board counts a game in
// progress as played (~Board) and MainLoopFinished() writes it again, so the last write has the count
void WinDMApp::WriteShutdownRegistry()
{
	bool countGame = (mBoard != NULL) && (mBoard->IsGameInProgress());
	if (countGame)
		mTimesPlayed++;
	WriteToRegistry();
	if (countGame)
		mTimesPlayed--;
}

// 0x4d4455
void WinDMApp::NewGame()
{
	if (mHighScoreWidget != NULL)
	{
		mWidgetManager->RemoveWidget(mHighScoreWidget);
		SafeDeleteWidget(mHighScoreWidget);
		mHighScoreWidget = NULL;
	}

	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
	}

	// port: pick up the tunables of the currently enabled mods
	gGameConfig.Load();
	ChooseBoardRng(); // port(variant): the new board's RNG source

	mBoard = new Board(this, mTimedMode);
	mBoard->Resize(0, 0, 640, 480);
	mWidgetManager->AddWidget(mBoard);
	mWidgetManager->BringToBack(mBoard);
	mWidgetManager->SetFocus(mBoard);

	if (mTimedMode)
	{
		mMusicInterface->FadeOut(MUSIC_NORMAL, true, 0.004);
		mMusicInterface->FadeIn(MUSIC_TIMED, -1, 0.002, false);
	}
	else
	{
		mMusicInterface->FadeOut(MUSIC_TIMED, true, 0.004);
		mMusicInterface->FadeIn(MUSIC_NORMAL, -1, 0.002, false);
	}
}

// port(variant): the RNG source of the board NewGame is about to create
// (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.2), for Board::InitVariant. A seed the player chose
// (GameConfig::mPendingSeed) comes first. Without one, rules and the seeds.perGame feature get a seed drawn
// from the global stream here, so it follows -seed. Any other game (the original rules, tunables-only mods
// included) draws from the global stream exactly as the original: nothing is drawn here.
void WinDMApp::ChooseBoardRng()
{
	gGameConfig.mNextBoardSeed = 0;
	gGameConfig.mNextBoardUserSeeded = false;
	if (gGameConfig.mPendingSeed != 0)
	{
		gGameConfig.mNextBoardSeed = gGameConfig.mPendingSeed;
		gGameConfig.mNextBoardUserSeeded = true;
	}
	else if (Board::ComputeVariantFlags(gGameConfig.mRules, gGameConfig.mFeatures).mOwnRng)
	{
		// Rand() is 0..2^31-1. MTRand plays seed 0 as 4357, so a drawn 0 is shown and kept as 4357.
		uint32_t aSeed = (uint32_t) Sexy::Rand();
		gGameConfig.mNextBoardSeed = (aSeed != 0) ? aSeed : 4357;
	}
}

// 0x4d4591
void WinDMApp::LoadingThreadCompleted()
{
}

// 0x4d4592
void WinDMApp::URLOpenSucceeded(const std::string& theURL)
{
	SexyApp::URLOpenSucceeded(theURL);
	KillDialog(DIALOG_OPENING_BROWSER);
}

// 0x4d45b1
MusicInterface* WinDMApp::CreateMusicInterface()
{
	return new SDLMusicInterface();
}

// 0x4d45e3
void WinDMApp::ShowRecordScoreDialog(const SexyString& theLines)
{
	RecordScoreDialog* aDialog = new RecordScoreDialog(this, theLines);
	aDialog->Resize(250, 32, 316, aDialog->GetPreferredHeight(316));
	AddDialog(DIALOG_RECORD_SCORE, aDialog);
}

// 0x4d4653
Dialog* WinDMApp::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	Dialog* aDialog = new Dialog(mImages[IMAGE_DIALOGBOX], mImages[IMAGE_BOXBUTTON], theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
	aDialog->SetButtonFont(mMainFont);
	aDialog->SetHeaderFont(mHeaderFont);
	aDialog->SetLinesFont(mMainFont);
	aDialog->Resize(250, 50, 316, aDialog->GetPreferredHeight(316));
	return aDialog;
}

// 0x4d46fc
void WinDMApp::ModalOpen()
{
	if (mBoard != NULL)
		mBoard->Pause(mTimedMode);
}

// 0x4d46fc
void WinDMApp::LostFocus()
{
	if (mBoard != NULL)
		mBoard->Pause(mTimedMode);
}

// 0x4d4716
void WinDMApp::ModalClose()
{
	if (mBoard != NULL)
		mBoard->Unpause(mTimedMode);
}

// 0x4d4716
void WinDMApp::GotFocus()
{
	if (mBoard != NULL)
		mBoard->Unpause(mTimedMode);
}

// 0x4d4730
void WinDMApp::SetTimedMode(bool isTimed, bool startNewGame)
{
	mTimedMode = isTimed;

	if (isTimed)
	{
		mNormalModeButton->mButtonImage = NULL;
		mTimedModeButton->mButtonImage = mImages[IMAGE_OVER_TIMETRIAL_SELECTED];
	}
	else
	{
		mNormalModeButton->mButtonImage = mImages[IMAGE_OVER_NORMAL_SELECTED];
		mTimedModeButton->mButtonImage = NULL;
	}

	mNormalModeButton->MarkDirty();
	mTimedModeButton->MarkDirty();

	if (startNewGame)
		NewGame();
}

// 0x4d47b1
void WinDMApp::ShowHighScoreWidget()
{
	// The board is only removed: mBoard keeps it until the next NewGame deletes it
	mWidgetManager->RemoveWidget(mBoard);

	mHighScoreWidget = new HighScoreWidget(this);
	mHighScoreWidget->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mHighScoreWidget);
	mWidgetManager->BringToBack(mHighScoreWidget);
}

// 0x4d483e
void WinDMApp::ShowOptionsDialog()
{
	OptionsDialog* aDialog = new OptionsDialog(this);
	aDialog->Resize(234, 50, 348, aDialog->GetPreferredHeight(348));
	AddDialog(DIALOG_OPTIONS, aDialog);
}

// 0x4d48a9
void WinDMApp::ButtonPress(int theId)
{
	PlaySample(SOUND_MENUCLICK);
}

// 0x4d48b6
void WinDMApp::ShowHighScoreDialog()
{
	HighScoreDialog* aDialog = new HighScoreDialog(this);
	aDialog->Resize(194, 50, 428, aDialog->GetPreferredHeight(428));
	AddDialog(DIALOG_HIGH_SCORES, aDialog);
}

// port: -trace (SexyAppBase test hooks). Prints the grid's gem colours, the score and the valid swaps
// (as click coordinates) each time a board settles into STATE_PLAYING, for writing -script runs.
// port(variant): a board with its own RNG also prints its seed.
static void TraceBoard(WinDMApp* theApp)
{
	static Board* sLastBoard = NULL;
	static int sLastState = -1;

	Board* aBoard = theApp->mBoard;
	int aState = (aBoard != NULL) ? aBoard->mState : -1;
	bool settled = (aState == STATE_PLAYING) && ((aBoard != sLastBoard) || (sLastState != STATE_PLAYING));
	sLastBoard = aBoard;
	sLastState = aState;
	if (!settled)
		return;

	printf("TRACE update %d: %s level %d score %d board tick %d\n", theApp->mUpdateCount, aBoard->mTimedMode ? "timed" : "normal",
		aBoard->mDisplayLevel, aBoard->mScore, aBoard->mUpdateCnt);
	for (int aRow = 0; aRow < aBoard->mRows; aRow++)
	{
		printf("TRACE   row %d:", aRow);
		for (int aCol = 0; aCol < aBoard->mCols; aCol++)
			printf(" %d", (aBoard->mBoard[aRow][aCol] != NULL) ? aBoard->mBoard[aRow][aCol]->mColor : -1);
		printf("\n");
	}
	// port(variant): "user" for a seed the player chose; after the rows, which keep their place under the header
	if (aBoard->mOwnRng != NULL)
		printf("TRACE   seed %u %s\n", (unsigned) aBoard->mGameSeed, aBoard->mUserSeeded ? "user" : "pergame");
	int aMove[4];
	for (int i = 0; aBoard->FindMove(aMove, i, true, true); i++)
		printf("TRACE   move %d: (%d,%d)->(%d,%d) click %d %d then %d %d\n", i, aMove[0], aMove[1], aMove[2], aMove[3],
			aBoard->GetColX(aMove[0]) + 24, aBoard->GetRowY(aMove[1]) + 24, aBoard->GetColX(aMove[2]) + 24, aBoard->GetRowY(aMove[3]) + 24);
	fflush(stdout);
}

// port: -tracehash (see WinDMApp.h): "<update> <board state> <logic hash> <full hash>" after every update
static void TraceHash(WinDMApp* theApp)
{
	if (theApp->mTraceHashFile == NULL)
	{
		theApp->mTraceHashFile = fopen(theApp->mTraceHashFileName.c_str(), "wb");
		if (theApp->mTraceHashFile == NULL)
		{
			SDL_Log("tracehash: cannot write %s", theApp->mTraceHashFileName.c_str());
			theApp->mTraceHashFileName.clear();
			return;
		}
	}

	uint64_t aLogicHash;
	uint64_t aFullHash;
	HashGameState(theApp, &aLogicHash, &aFullHash);
	fprintf(theApp->mTraceHashFile, "%d %d %016llx %016llx\n", theApp->mUpdateCount, (theApp->mBoard != NULL) ? theApp->mBoard->mState : -1,
		(unsigned long long) aLogicHash, (unsigned long long) aFullHash);
}

// 0x4d4921
void WinDMApp::UpdateFrames()
{
	SexyApp::UpdateFrames();

	if (mTraceMode)
		TraceBoard(this);

	// port: test hooks (see WinDMApp.h)
	if (!mTraceHashFileName.empty())
		TraceHash(this);
	if ((mAutoplay) && (mAutoplayBot == NULL))
		mAutoplayBot = new AutoplayBot(this, mAutoplaySeed, mAutoplayTimed, mAutoplayCap, mAutoplayLogFileName, mAutoplayShotFileName);
	if (mAutoplayBot != NULL)
		mAutoplayBot->Update();
}

// port: the game's command-line test hooks (see WinDMApp.h); everything else goes to the framework
void WinDMApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-tracehash")
		mTraceHashFileName = theParamValue;
	else if (theParamName == "-autoplay")
	{
		mAutoplay = true;
		mAutoplaySeed = strtoul(theParamValue.c_str(), NULL, 10);
		// the bot's input only, as in a -script run (from the start: focus changes would pause the game)
		mScriptedInput = true;
	}
	else if (theParamName == "-autoplaytimed")
		mAutoplayTimed = true;
	else if (theParamName == "-autoplaycap")
		mAutoplayCap = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaylog")
		mAutoplayLogFileName = theParamValue;
	else if (theParamName == "-autoplayshot")
		mAutoplayShotFileName = theParamValue;
	else
		SexyApp::HandleCmdLineParam(theParamName, theParamValue);
}

// port: script command "save <file>" (see WinDMApp.h). Silent when it works, so a script with save
// lines logs exactly what the same script without them logs.
bool WinDMApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	if (theCommand.mCommand != "save")
		return false;

	std::string aSaveName;
	Buffer aBuffer;
	if (theCommand.mArgs.empty())
		SDL_Log("script: line %d: save needs <file>", theCommand.mLine);
	else if (!GetShutdownSave(&aSaveName, &aBuffer))
		SDL_Log("script: line %d: save: no game in progress, %s not written", theCommand.mLine, theCommand.mArgs.c_str());
	else
	{
		FILE* aFile = fopen(theCommand.mArgs.c_str(), "wb");
		bool success = (aFile != NULL) && (fwrite(aBuffer.GetDataPtr(), 1, aBuffer.GetDataLen(), aFile) == (size_t) aBuffer.GetDataLen());
		if ((aFile != NULL) && (fclose(aFile) != 0))
			success = false;
		if (!success)
			SDL_Log("script: line %d: save: cannot write %s", theCommand.mLine, theCommand.mArgs.c_str());
	}
	return true;
}

// 0x4d4926
void WinDMApp::SetMusicVolume(double theVolume)
{
	SexyApp::SetMusicVolume(theVolume);
}

// 0x4d4a0e
bool WinDMApp::KillDialog(int theDialogId)
{
	if (SexyApp::KillDialog(theDialogId))
	{
		if (mDialogMap.size() == 0)
			mWidgetManager->SetFocus(mBoard);
		return true;
	}

	return false;
}

// 0x4d4ad9
std::string WinDMApp::GetGameSEHInfo()
{
	return SexyApp::GetGameSEHInfo();
}

// 0x4d4e97
void WinDMApp::URLOpenFailed(const std::string& theURL)
{
	SexyApp::URLOpenFailed(theURL);
	KillDialog(DIALOG_OPENING_BROWSER);

	SexyString aLines = GetString("URL_OPEN_ERROR1") + theURL + GetString("URL_OPEN_ERROR2");
	CopyToClipboard(theURL);

	Dialog* aDialog = NewDialog(DIALOG_URL_FAILED, true, GetString("OPEN_BROWSER"), aLines, GetString("DIALOG_BUTTON_OK"), Dialog::BUTTONS_FOOTER);
	aDialog->Resize(100, 50, 440, aDialog->GetPreferredHeight(440));
	AddDialog(DIALOG_URL_FAILED, aDialog);
}

// 0x4d5077
bool WinDMApp::OpenURL(const std::string& theURL, bool shutdownOnOpen)
{
	Dialog* aDialog = NewDialog(DIALOG_OPENING_BROWSER, true, "", GetString("OPENING_BROWSER"), "", Dialog::BUTTONS_NONE);
	aDialog->mContentInsets = Insets(16, 26, 16, 16);
	aDialog->Resize(180, 140, 280, 70);
	AddDialog(DIALOG_OPENING_BROWSER, aDialog);

	return SexyApp::OpenURL(theURL, shutdownOnOpen);
}

// 0x4d5174
void WinDMApp::WriteToRegistry()
{
	SexyApp::WriteToRegistry();
	RegistryWriteString("LastScoreName", mLastScoreName);
}

// 0x4d51bc
void WinDMApp::ReadFromRegistry()
{
	SexyApp::ReadFromRegistry();
	mRegisterLink = "http://www.popcap.com/register.php?theGame=bejeweled&referid=" + mReferId;
	RegistryReadString("LastScoreName", &mLastScoreName);
	RegistryReadBoolean("NoPopLink", &mNoPopLink);
}

// 0x4d5272
void WinDMApp::ShowRegisterDialog()
{
	// The unregistered branch (RegisterDialog, id 5) is not ported: the port is always registered
	Dialog* aDialog = DoDialog(DIALOG_ALREADY_REGISTERED, true, "Already Registered", "You have already registered Bejeweled!", "OK", Dialog::BUTTONS_FOOTER);
	aDialog->Move(aDialog->mX, aDialog->mY + 24);
}

// ShowRegisterToRecordScoreDialog (0x4d5486, dialog 7) is not ported: only unregistered copies show it

// 0x4d550d
void WinDMApp::ConfirmLoadGame()
{
	if (mBoard->SaveGameExists())
	{
		Dialog* aDialog = DoDialog(DIALOG_CONFIRM_LOAD, true, GetString("CONFIRM_LOAD_DIALOG1"), GetString("CONFIRM_LOAD_DIALOG2"), "", Dialog::BUTTONS_YES_NO);
		aDialog->Resize(234, 50, 348, aDialog->GetPreferredHeight(348));
	}
}

// 0x4d5610
void WinDMApp::ConfirmNewGame()
{
	if (mBoard->mState == STATE_GAME_OVER_DONE)
		NewGame();
	else
		DoDialog(DIALOG_NEW_GAME, true, GetString("NEWGAMEDIALOG1"), GetString("NEWGAMEDIALOG2"), "", Dialog::BUTTONS_YES_NO);
}

// 0x4d56ec
void WinDMApp::ConfirmNormalMode()
{
	if (mBoard->mState == STATE_GAME_OVER_DONE)
		SetTimedMode(false, true);
	else
		DoDialog(DIALOG_CONFIRM_NORMAL_MODE, true, GetString("CONFIRMNORMALMODE1"), GetString("CONFIRMNORMALMODE2"), "", Dialog::BUTTONS_YES_NO);
}

// 0x4d57cd
void WinDMApp::ConfirmTimedMode()
{
	if (mBoard->mState == STATE_GAME_OVER_DONE)
		SetTimedMode(true, true);
	else
		DoDialog(DIALOG_CONFIRM_TIMED_MODE, true, GetString("CONFIRMTIMEDMODE1"), GetString("CONFIRMTIMEDMODE2"), "", Dialog::BUTTONS_YES_NO);
}

// 0x4d58af
void WinDMApp::ConfirmQuit()
{
	SexyString aLines = GetString("CONFIRMQUIT1");
	if ((mBoard != NULL) && (mBoard->IsGameInProgress()))
		aLines += GetString("CONFIRMQUIT2");

	DoDialog(DIALOG_QUIT, true, GetString("QUIT"), aLines, "", Dialog::BUTTONS_YES_NO);
}

// 0x4d59cc
void WinDMApp::ShowInstructionsDialog()
{
	Dialog* aDialog = NewDialog(DIALOG_INSTRUCTIONS, false, "", GetString("INSTRUCTIONDIALOG"), "", Dialog::BUTTONS_NONE);
	aDialog->mContentInsets = Insets(16, 24, 16, 24);
	aDialog->Resize(201, 38, 418, aDialog->GetPreferredHeight(418));
	AddDialog(DIALOG_INSTRUCTIONS, aDialog);
}

// 0x4d5ac1
void WinDMApp::ShowFullInstructionsDialog()
{
	Dialog* aDialog = NewDialog(DIALOG_FULL_INSTRUCTIONS, true, GetString("FULL_INSTRUCTIONS1"), GetString("FULL_INSTRUCTIONS2"), GetString("DONE"), Dialog::BUTTONS_FOOTER);
	aDialog->mTextAlign = -1;
	aDialog->Resize(32, 8, mWidth - 64, aDialog->GetPreferredHeight(mWidth - 64));
	AddDialog(DIALOG_FULL_INSTRUCTIONS, aDialog);
}

// 0x4d5bef
void WinDMApp::ShowColorDepthDialog()
{
	DoDialog(DIALOG_FULL_INSTRUCTIONS, true, GetString("COLOR_DEPTH_EXPLANATION1"), GetString("COLOR_DEPTH_EXPLANATION2"), GetString("DIALOG_BUTTON_OK"), Dialog::BUTTONS_FOOTER);
}

// ShowRegisterNagDialog (0x4d5cdf, dialog 15) is not ported: Board shows it only to unregistered copies

// 0x4d62b8
bool WinDMApp::LoadAppImage(int theImageId)
{
	mImages[theImageId] = GetImage(std::string("images\\") + gImageNames[theImageId], true);
	if (mImages[theImageId] == NULL)
	{
		Popup(std::string("Failed to load images\\") + gImageNames[theImageId]);
		mLoadingFailed = true;
		return false;
	}

	((DDImage*) mImages[theImageId])->GenerateDDSurface();
	return true;
}

// 0x4d6399
void WinDMApp::PreDisplayHook()
{
	mIsDiamondMine = GetBoolean("IsDiamondMine", false);
	if (mIsDiamondMine)
		mTitle = GetString("Title", "Diamond Mine") + " " + mProductVersion;
}

// 0x4d649e
void WinDMApp::Init()
{
	SexyApp::Init();

	// port: no Vista AppData migration of userdata (the port's user files live in the VFS user dir).
	// main() has applied the enabled mods; read their gameplay tunables.
	gGameConfig.Load();

	LoadProperties();

	DIALOG_YES_STRING = GetString("DIALOG_BUTTON_YES");
	DIALOG_NO_STRING = GetString("DIALOG_BUTTON_NO");
	DIALOG_OK_STRING = GetString("DIALOG_BUTTON_OK");
	DIALOG_CANCEL_STRING = GetString("DIALOG_BUTTON_CANCEL");

	mImages = new Image*[NUM_IMAGES];
	for (int i = 0; i < NUM_IMAGES; i++)
		mImages[i] = NULL;

	for (int i = 0; i < NUM_TITLE_IMAGES; i++)
	{
		if (!LoadAppImage(i))
		{
			Shutdown();
			return;
		}
	}

	mHeaderFont = new ImageFont(this, gFontFiles[FONT_HEADER]);
	mMainFont = new ImageFont(this, gFontFiles[FONT_MAIN]);

	mTitleScreen = new TitleScreen(this);
	mTitleScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mTitleScreen);

	for (int i = 0; i < NUM_MUSIC; i++)
		mMusicInterface->LoadMusic(i, gMusicFiles[i]);
	mMusicInterface->PlayMusic(MUSIC_TITLE, 0, false);

	SetCursorImage(CURSOR_POINTER, mImages[IMAGE_CURSOR]);
	SetCursorImage(CURSOR_HAND, mImages[IMAGE_POINTER]);
	SetCursorImage(CURSOR_DRAGGING, mImages[IMAGE_OVERHAND]);
	SetCursorImage(CURSOR_TEXT, mImages[IMAGE_TYPER]);
}

// DoUpdateCheck (0x4d68da, UpdateCheckDialog 18 + HTTP version check) is not ported: the port is always up to date

// 0x4d6a40
void WinDMApp::ButtonDepress(int theId)
{
	if ((theId % 10000 >= 2000) && (theId % 10000 < 3000))
	{
		// YES / OK / footer of dialog theId - 2000
		int aDialogId = theId - 2000;
		switch (aDialogId)
		{
		case DIALOG_NEW_GAME:
			// The board shakes and falls, then calls NewGame (no high-score screen)
			mBoard->RequestNewGame();
			KillDialog(DIALOG_NEW_GAME);
			break;

		case DIALOG_QUIT:
			Shutdown();
			KillDialog(DIALOG_QUIT);
			break;

		case DIALOG_OPTIONS:
			{
				OptionsDialog* anOptionsDialog = (OptionsDialog*) GetDialog(DIALOG_OPTIONS);
				bool wantWindowed = !anOptionsDialog->mFullscreenCheckbox->IsChecked();
				if (mIsWindowed != wantWindowed)
					SwitchScreenMode(wantWindowed);
				EnableCustomCursors(anOptionsDialog->mCustomCursorsCheckbox->IsChecked());
				KillDialog(DIALOG_OPTIONS);
			}
			break;

		case DIALOG_CONFIRM_NORMAL_MODE:
			KillDialog(DIALOG_CONFIRM_NORMAL_MODE);
			SetTimedMode(false, true);
			break;

		case DIALOG_CONFIRM_TIMED_MODE:
			KillDialog(DIALOG_CONFIRM_TIMED_MODE);
			SetTimedMode(true, true);
			break;

		// DIALOG_REGISTER (Validate and register), DIALOG_REGISTER_TO_RECORD and DIALOG_REGISTER_NAG
		// (both ShowRegisterDialog) are unregistered-only and not ported

		case DIALOG_RECORD_SCORE:
			{
				RecordScoreDialog* aRecordDialog = (RecordScoreDialog*) GetDialog(DIALOG_RECORD_SCORE);
				mHighScoreWidget->AddHighScore(aRecordDialog->mNameWidget->mString);
				mLastScoreName = aRecordDialog->mNameWidget->mString;
				KillDialog(DIALOG_RECORD_SCORE);
			}
			break;

		case DIALOG_CONFIRM_LOAD:
			KillDialog(DIALOG_CONFIRM_LOAD);
			mBoard->LoadGame();
			mBoard->DeleteSaveGame();
			break;

		// DIALOG_CHECK_UPDATES (DoUpdateCheck) and the UpdateCheckDialog results DIALOG_UPDATE_AVAILABLE
		// (OpenUpdateURL) / DIALOG_UP_TO_DATE are not ported

		default:
			KillDialog(aDialogId);
			break;
		}
	}
	else if ((theId % 10000 >= 3000) && (theId % 10000 < 4000))
	{
		// NO / CANCEL of dialog theId - 3000 (DIALOG_UPDATE_AVAILABLE's "No" is not ported)
		int aDialogId = theId - 3000;
		if (aDialogId == DIALOG_CONFIRM_LOAD)
		{
			KillDialog(DIALOG_CONFIRM_LOAD);
			mBoard->DeleteSaveGame();
		}
		else
			KillDialog(aDialogId);
	}

	switch (theId)
	{
	case BUTTON_NEW_GAME:
		ConfirmNewGame();
		break;

	case BUTTON_NORMAL_MODE:
		// Same mode acts as New Game; switching before the first scoring move needs no confirmation
		if (!mTimedMode)
			ConfirmNewGame();
		else if (mBoard->mIsNewGame)
			SetTimedMode(false, true);
		else
			ConfirmNormalMode();
		break;

	case BUTTON_TIMED_MODE:
		if (mTimedMode)
			ConfirmNewGame();
		else if (mBoard->mIsNewGame)
			SetTimedMode(true, true);
		else
			ConfirmTimedMode();
		break;

	case BUTTON_OPTIONS:
		ShowOptionsDialog();
		break;

	case BUTTON_QUIT:
		ConfirmQuit();
		break;

	case BUTTON_HINT:
		mBoard->ShowHint();
		break;
	}
}

// 0x4d6e71
ButtonWidget* WinDMApp::MakeButton(int theId, const Rect& theRect, int theOverImageId, int theDownImageId)
{
	// No mButtonImage: the board background shows the button art, the widget only adds hover/pressed images
	ButtonWidget* aButton = new ButtonWidget(theId, this);
	aButton->Resize(theRect);
	aButton->mDoFinger = true;
	if (theOverImageId != -1)
		aButton->mOverImage = mImages[theOverImageId];
	aButton->mDownImage = mImages[theDownImageId];
	aButton->mMouseInsets = Insets(1, 1, 3, 3);
	mWidgetManager->AddWidget(aButton);
	mButtonList.push_back(aButton);
	return aButton;
}

// 0x4d6f28
void WinDMApp::TitleScreenIsFinished()
{
	mMusicInterface->FadeOut(MUSIC_TITLE, true, 0.004);

	mWidgetManager->RemoveWidget(mTitleScreen);
	SafeDeleteWidget(mTitleScreen);
	mTitleScreen = NULL;

	MakeButton(BUTTON_NEW_GAME, Rect(18, 142, 156, 41), -1, IMAGE_OVER_NEWGAME_PUSHED);
	mNormalModeButton = MakeButton(BUTTON_NORMAL_MODE, Rect(18, 185, 78, 31), -1, IMAGE_OVER_NORMAL_PUSHED);
	mTimedModeButton = MakeButton(BUTTON_TIMED_MODE, Rect(96, 185, 78, 31), -1, IMAGE_OVER_TIMETRIAL_PUSHED);
	MakeButton(BUTTON_OPTIONS, Rect(18, 241, 156, 41), -1, IMAGE_OVER_OPTIONS_PUSHED);
	MakeButton(BUTTON_QUIT, Rect(18, 405, 156, 40), -1, IMAGE_OVER_QUIT_PUSHED);
	ButtonWidget* aHintButton = MakeButton(BUTTON_HINT, Rect(64, 303, 53, 73), IMAGE_OVER_HINT, IMAGE_OVER_HINT);
	aHintButton->mMouseInsets = Insets(-16, -8, -16, -8);

	// SetTimedMode already starts a game; the second NewGame replaces that board (both consume Rand())
	SetTimedMode(false, true);
	NewGame();
	ConfirmLoadGame();
}

// 0x4d82d2
void WinDMApp::LoadingThreadProc()
{
	mNumLoadingThreadTasks = NUM_LOADING_TASKS;

	// port: the original reads data\diff.txt with std::fstream from the game folder; the port reads it
	// through the VFS so mods can replace it. Same operator>> sequence, no error checks.
	std::string aDiffText;
	PFILE* aDiffFile = p_fopen("data\\diff.txt", "r");
	if (aDiffFile != NULL)
	{
		char aBuffer[1024];
		size_t aCount;
		while ((aCount = p_fread(aBuffer, 1, sizeof(aBuffer), aDiffFile)) > 0)
			aDiffText.append(aBuffer, aCount);
		p_fclose(aDiffFile);
	}
	std::istringstream aDiffStream(aDiffText);
	aDiffStream >> mNormalPointsBase >> mNormalPointsPerLevel >> mNormalMultBase >> mNormalMultPerLevel
		>> mTimedPointsBase >> mTimedPointsPerLevel >> mTimedMultBase >> mTimedMultPerLevel
		>> mTimeDrainBase >> mTimeDrainPerLevel >> mTimeDrainAccelBase >> mTimeDrainAccelPerLevel;

	for (int i = NUM_TITLE_IMAGES; i < NUM_IMAGES; i++)
	{
		if (!LoadAppImage(i))
			return;
		mCompletedLoadingThreadTasks++;
	}

	if (mSoundManager->Initialized())
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
	else
		mCompletedLoadingThreadTasks += NUM_SOUNDS;

	mAlphaDigitsFont = new ImageFont(mImages[IMAGE_ALPHADIGITS], gFontFiles[FONT_ALPHADIGITS]);
	mScoreFont = new ImageFont(mImages[IMAGE_SCOREFONT], gFontFiles[FONT_SCOREFONT]);
	mOverFont = new ImageFont(mImages[IMAGE_OVERFONT], gFontFiles[FONT_OVERFONT]);
	if (gGameConfig.mAnyModEnabled)
		AddModsCrossbarToG(mOverFont);  // port: a sign that mods are in use
	mCompletedLoadingThreadTasks += 2;

	// Timer bar hues 0, 3, 7, 11 .. 85 (red to green), then white
	for (int i = 0; i < NUM_TIMERBAR_IMAGES - 1; i++)
		mTimerBarImages[i] = CreateColorizedImage(mImages[IMAGE_TIMERBAR], Color((int) HSLToRGB(85 * i / 23, 255, 128)));
	mTimerBarImages[NUM_TIMERBAR_IMAGES - 1] = CreateColorizedImage(mImages[IMAGE_TIMERBAR], Color(255, 255, 255));

	mGemShrinkImages = new MemoryImage**[Board::NUM_GEM_COLORS];
	for (int aColor = 0; aColor < Board::NUM_GEM_COLORS; aColor++)
	{
		mGemShrinkImages[aColor] = new MemoryImage*[NUM_SHRINK_FRAMES];
		for (int aFrame = 0; aFrame < NUM_SHRINK_FRAMES; aFrame++)
		{
			MemoryImage* anImage = new MemoryImage(this);
			anImage->Create(49, 49);

			// Odd sizes 43, 35, 29, 21, 15, 7 (with 53-bit doubles), centred in the 49x49 cell
			int aSize = (int) ((1.0 - (aFrame + 1) / 7.0) * 49.0) | 1;
			int anOffset = (49 - aSize) / 2;

			Graphics g(anImage);
			g.DrawImage(mImages[IMAGE_GEMSHEET], Rect(anOffset, anOffset, aSize, aSize), Rect(2, aColor * 104, 49, 49));
			mGemShrinkImages[aColor][aFrame] = anImage;
		}
		mCompletedLoadingThreadTasks++;
	}

	PrecacheAdditive((MemoryImage*) mImages[IMAGE_BLACKHOLE]);
	PrecacheAlpha((MemoryImage*) mImages[IMAGE_HOLEMASK]);
	mCompletedLoadingThreadTasks++;
}
