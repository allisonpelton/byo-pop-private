#include "WinDMApp.h"
#include "Board.h"
#include "BoardTrace.h"
#include "TitleScreen.h"
#include "HighScoreWidget.h"
#include "HighScoreDialog.h"
#include "OptionsDialog.h"
#include "RecordScoreDialog.h"
#ifdef __EMSCRIPTEN__
#include "SiteProfiles.h"
#endif
#include "GameConfig.h"
#include "ModIndicators.h"
#include "LiveSplitObserver.h"
#include "UndoOrb.h"
#include "GemSheet.h"

#include "WidgetManager.h"
#include "ButtonWidget.h"
#include "Checkbox.h"
#include "EditWidget.h"
#include "Dialog.h"
#include "DialogButton.h"
#include "ImageFont.h"
#include "DDImage.h"
#include "DDInterface.h"
#include "MemoryImage.h"
#include "Graphics.h"
#include "SoundManager.h"
#include "SDLMusicInterface.h"
#include "PakInterface.h"
#include "ReplayRecorder.h"
#include "testhooks/StateHash.h"
#include "testhooks/Autoplay.h"
#include "testhooks/CodecCheck.h"
#include "testhooks/AudioCapture.h"
#include "testhooks/MemReport.h"
#include "WatchController.h"
#include "ReplayOverlay.h"
#include "GameStatsPage.h"
#include "HyperlinkWidget.h"
#include "ExtrasDialog.h"
#include "ReplayBrowserDialog.h"
#include "art/CodeArt.h"
#include "platform/ModManager.h"

#include <SDL.h>
#include <algorithm>
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
	mGemShrinkColors = 0; // port(variant)
	mMissingSounds = 0; // port(variant)
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
	mCodecCheck = NULL;
	mAudioCapture = NULL;
	mWatch = NULL; // port(variant): watching a replay (-replay, WatchController.h)
	mNextValuesBoard = 0; // port(mods)
	mReplayQuit = false; // port(variant)
	mReplayOverlay = NULL; // port(variant)
	mReplayStatsLink = NULL; // port(variant)
	mReplayStatsPage = NULL; // port(variant)
	mPendingPlaySeed = 0; // port(variant)
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
		for (int aColor = 0; aColor < mGemShrinkColors; aColor++) // port(variant): the colours made
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
	delete mCodecCheck;
	delete mAudioCapture;
	if (mReplayOverlay != NULL) // port(variant)
	{
		mWidgetManager->RemoveWidget(mReplayOverlay);
		delete mReplayOverlay;
	}
	delete mWatch; // port(variant)
	LiveSplitObserver::AppDestroyed(this); // port(variant): the LiveSplit block's last write: not active, no board
}

// 0x4d43f9
void WinDMApp::Shutdown()
{
	if (mAudioCapture != NULL)
		mAudioCapture->Finish(); // port: test hook (releases the mixer's lock before the audio device closes)
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

// port (web): the save Shutdown() would write now (see SexyAppBase::UpdateUserDataBackup).
// port(variant): and its sidecar, ending in SAVE(snapshot) for a recording, taken at the same tick, so a
// backup restored after a crash is consistent (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.5)
void WinDMApp::GetShutdownSaves(std::vector<ShutdownSave>* theSaves)
{
	if ((mBoard == NULL) || (!mBoard->IsGameInProgress()))
		return;

	Buffer aBuffer;
	mBoard->SaveGameSnapshot(&aBuffer);
	ShutdownSave aSave;
	aSave.mFileName = mBoard->GetSaveGameFileName(); // port(mods): the board's own name
	aSave.mData.assign((const char*) aBuffer.GetDataPtr(), (size_t) aBuffer.GetDataLen());
	theSaves->push_back(aSave);

	ShutdownSave aSidecar;
	aSidecar.mData = mBoard->SidecarSnapshot(aSave.mData);
	if (!aSidecar.mData.empty())
	{
		aSidecar.mFileName = mBoard->GetSidecarFileName();
		theSaves->push_back(aSidecar);
	}
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
	if ((mWatch != NULL) && mWatch->OnBoardEnd(false)) return; // port(variant): a replay's board after New Game: its watch ends (WatchController.h)
	if (mHighScoreWidget != NULL)
	{
		mWidgetManager->RemoveWidget(mHighScoreWidget);
		SafeDeleteWidget(mHighScoreWidget);
		mHighScoreWidget = NULL;
	}

	bool canCommit = CanCommitNextGameValues(); // port(mods): asked while the outgoing board is still the board
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
	}

	// port: pick up the tunables of the currently enabled mods; port(mods): first the values held for the next
	// fresh game, unless a game would be stranded
	gGameConfig.Load(canCommit);
	ChooseBoardRng(); // port(variant): the new board's RNG source

	EnsureGemColors(gGameConfig.mGemColors); // port(variant)
	mBoard = new Board(this, mTimedMode);
	if (gGameConfig.mFeatures.mReplays) // port(variant): record the game (features.replays, ReplayRecorder.h)
		mBoard->mRecorder = new ReplayRecorder(this, mBoard); // port(variant)
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

// port(variant): more gem colours (games/bejeweled/docs/CODE_MODS_DESIGN.md 4.2). NewGame calls it with the rules of
// the board it makes, and a replay with its own rules (WatchController::CreateBoard). The rows the sheet lacks are
// generated (GemSheet.h) with the looks the enabled mods' imageOps give, the default look for the others.
void WinDMApp::EnsureGemColors(int theColors)
{
	if ((mImages == NULL) || (mImages[IMAGE_GEMSHEET] == NULL) || (mGemShrinkImages == NULL))
		return;
	theColors = std::min(theColors, (int) Board::MAX_GEM_COLORS);
	MemoryImage* aSheet = (MemoryImage*) mImages[IMAGE_GEMSHEET];
	if (GemSheet::Rows(aSheet) < theColors)
	{
		GemSheet::Extend(aSheet, theColors, GemSheet::ModLooks(Platform::ModManager::Get().GetImageOps(), NULL));
		SDL_Log("Gem sheet: %d colour rows", GemSheet::Rows(aSheet));
	}
	while (mGemShrinkColors < std::min(theColors, GemSheet::Rows(aSheet)))
		MakeGemShrinkImages(mGemShrinkColors);
}

// The 49x49 images of a matched gem of colour theColor shrinking (port: the loop body of Init's 0x4d6919)
void WinDMApp::MakeGemShrinkImages(int theColor)
{
	mGemShrinkImages[theColor] = new MemoryImage*[NUM_SHRINK_FRAMES];
	for (int aFrame = 0; aFrame < NUM_SHRINK_FRAMES; aFrame++)
	{
		MemoryImage* anImage = new MemoryImage(this);
		anImage->Create(49, 49);

		// Odd sizes 43, 35, 29, 21, 15, 7 (with 53-bit doubles), centred in the 49x49 cell
		int aSize = (int) ((1.0 - (aFrame + 1) / 7.0) * 49.0) | 1;
		int anOffset = (49 - aSize) / 2;

		Graphics g(anImage);
		g.DrawImage(mImages[IMAGE_GEMSHEET], Rect(anOffset, anOffset, aSize, aSize), Rect(2, theColor * 104, 49, 49));
		mGemShrinkImages[theColor][aFrame] = anImage;
	}
	mGemShrinkColors = std::max(mGemShrinkColors, theColor + 1);
}

// 0x4d4591
// port(lean): the loaded images at 1 byte a pixel where they have 256 colours or fewer (features.lean.images).
// Palletize is lossless: it keeps an exact colour table or leaves the image as it is. The textures upload from the
// table, and an image that is read or drawn into later gets its pixels back (MemoryImage::GetBits).
static void CompactImages(SexyAppBase* theApp)
{
	for (MemoryImage* anImage : theApp->mMemoryImageSet)
	{
		DDImage* aDDImage = dynamic_cast<DDImage*>(anImage);
		if ((aDDImage == NULL) || !aDDImage->mIsScreen)
			anImage->Palletize();
	}
}

void WinDMApp::LoadingThreadCompleted()
{
	if (gGameConfig.mFeatures.mLean && gGameConfig.mFeatures.mLeanImages)
		CompactImages(this);
}

// port(lean): at most features.lean.frameRate frames a second once loaded; the game updates 100 times a second
// whatever the frame rate, so only the drawing thins out
bool WinDMApp::DrawDirtyStuff()
{
	bool aDrew = SexyApp::DrawDirtyStuff();
	if (aDrew && mLoaded && gGameConfig.mFeatures.mLean && (gGameConfig.mFeatures.mLeanFrameRate > 0))
		mNextDrawTick = mLastDrawTick + 1000 / gGameConfig.mFeatures.mLeanFrameRate;
	return aDrew;
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
	if ((mWatch != NULL) && mWatch->OnBoardEnd(true)) return; // port(variant): a replay's game over: its watch ends, no high-score screen
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

// port: -trace (SexyAppBase test hooks): the board's settles (BoardTrace.h, which the headless verifier
// prints too)
static void TraceBoard(WinDMApp* theApp)
{
	static BoardTracer sTracer;

	Board* aBoard = theApp->mBoard;
	if ((theApp->mWatch != NULL) && theApp->mWatch->IsEndedBoard(aBoard)) // port(variant): a watched replay's last board settles no more
		aBoard = NULL;
	sTracer.AfterUpdate(aBoard, theApp->mUpdateCount);
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
	if (mWatch != NULL) mWatch->BeforeFrame(); // port(variant): a replay's events and its board's update (WatchController.h)
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
	if ((!mCodecCheckFileName.empty()) && (mCodecCheck == NULL))
		mCodecCheck = new CodecCheck(this, mCodecCheckFileName);
	if (mCodecCheck != NULL)
		mCodecCheck->AfterUpdate();
	if ((!mAudioCaptureFileName.empty()) && (mAudioCapture == NULL))
		mAudioCapture = new AudioCapture(mAudioCaptureFileName);
	if (mAudioCapture != NULL)
		mAudioCapture->Update();
	if (LiveSplitObserver::Wanted()) LiveSplitObserver::AfterUpdate(this); // port(variant): fills the LiveSplit block (features.livesplit, desktop)
	ApplyNextGameValues(); // port(mods)
}

// port(mods): values held for the next fresh game (mod-settings.json "next") may apply only when no game would be
// stranded (docs/MOD_MENU_DESIGN.md 6.2): not while a replay is watched, not while the board is in progress (NewGame's
// deferred delete saves it, under its own name) and not while a save exists under the current key
bool WinDMApp::CanCommitNextGameValues()
{
	if ((mWatch != NULL) && mWatch->IsWatching())
		return false;
	if ((mBoard != NULL) && mBoard->IsGameInProgress())
		return false;
	return !FileExists(Board::ScopedSaveGameFileName());
}

// port(mods): values held for the next fresh game also reach a board nobody has played yet, the one
// TitleScreenIsFinished makes or the one left after "Continue? No" (review A9 of docs/MOD_MENU_DESIGN.md): that board
// is made again with them. Only while values are held, so no oracle run gets here. Never with a dialog or the mod
// menu open (the menu's no-Rand scope), the title or game-over screen up, or during a shutdown; once per board, so a
// commit that cannot happen (a newer mod-settings.json) is not tried again every frame.
void WinDMApp::ApplyNextGameValues()
{
	Platform::ModManager& aMods = Platform::ModManager::Get();
	if ((!aMods.HasNextGameValues()) || (mBoard == NULL) || (mTitleScreen != NULL) || (mHighScoreWidget != NULL) ||
		IsModMenuOpen() || (!mDialogMap.empty()) || mShutdown)
		return;
	if ((!mBoard->mIsNewGame) || (mBoard->mState == STATE_GAME_OVER) || (mBoard->mState == STATE_GAME_OVER_DONE) ||
		(mBoard->mBoardSerial == mNextValuesBoard) || (!CanCommitNextGameValues()))
		return;
	mNextValuesBoard = mBoard->mBoardSerial;
	if (aMods.CommitNextGameValues(true))
	{
		SDL_Log("mods: the values held for the next game apply: the untouched board is made again");
		NewGame();
	}
}

// port(mods): a game in progress is one the shutdown would save (Board::IsGameInProgress; a replay's board never
// is): the menu's Restart asks first
bool WinDMApp::HasGameInProgress()
{
	return (mBoard != NULL) && mBoard->IsGameInProgress();
}

// port(mods): not while the high-score name is being entered: the score would be lost
bool WinDMApp::CanRestartNow(SexyString* theReason)
{
	if (GetDialog(DIALOG_RECORD_SCORE) != NULL)
	{
		if (theReason != NULL)
			*theReason = GetString("MODMENU_RESTART_RECORD_SCORE", _S("Enter your name for the high score first."));
		return false;
	}
	return SexyApp::CanRestartNow(theReason);
}

// port(mods): the framework's (game files, user dir, mods, render scale), -replaydir as given (without it the
// folder follows the user dir again), and -gameseed while that seed still waits for its game
// (Board::UsePendingSeed). Not -replay: a restart does not watch again.
void WinDMApp::GetRestartArgs(std::vector<std::string>* theArgs)
{
	SexyApp::GetRestartArgs(theArgs);
	for (const std::string& anArg : mCmdLineArgs)
	{
		if ((anArg.compare(0, 11, "-replaydir=") == 0) || (anArg.compare(0, 12, "--replaydir=") == 0))
			theArgs->push_back(anArg);
	}
	if (gGameConfig.mPendingSeed != 0)
		theArgs->push_back(StrFormat("-gameseed=%u", (unsigned) gGameConfig.mPendingSeed));
}

// port(mods): what the enabled mods' rules resolved to at the last GameConfig::Load, per mod: a mod's "rules" or
// "features" ignored and a mod dropped from the merged set are errors; overridden, unknown and clamped values are
// warnings (the same messages as mods-status.json and -dumprules)
void WinDMApp::GetModMessages(std::vector<ModMessage>* theMessages)
{
	const RulesStatus& aStatus = gGameConfig.mStatus;
	for (const RulesMessage& aMessage : aStatus.mErrors)
		theMessages->push_back({aMessage.mModId, aMessage.mText, true});
	for (const RulesMessage& aMessage : aStatus.mDropped)
		theMessages->push_back({aMessage.mModId, aMessage.mText, true});
	for (const RulesMessage& aMessage : aStatus.mWarnings)
		theMessages->push_back({aMessage.mModId, aMessage.mText, false});
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
	else if (theParamName == "-audiocapture")
		mAudioCaptureFileName = theParamValue;
	else if (theParamName == "-codeccheck")
		mCodecCheckFileName = theParamValue;
	else if ((theParamName == "-replay") && (mWatch == NULL)) // port(variant): watch a replay (WatchController.h)
		mWatch = new WatchController(this, theParamValue);
	else if (theParamName == "-replayquit")
		mReplayQuit = true;
	// port(variant): -livesplitlog=<file>: one line per LiveSplit block write (LiveSplitObserver.h)
	else if (theParamName == "-livesplitlog")
		LiveSplitObserver::SetLogFile(theParamValue);
	else
		SexyApp::HandleCmdLineParam(theParamName, theParamValue);
}

// theName with the extension of theOther ("snaps/s1.dat", "x\\savegame.bjr" -> "snaps/s1.bjr")
static std::string WithExtensionOf(const std::string& theName, const std::string& theOther)
{
	size_t aDot = theName.find_last_of('.');
	size_t aSlash = theName.find_last_of("/\\");
	std::string aStem = ((aDot != std::string::npos) && ((aSlash == std::string::npos) || (aDot > aSlash))) ? theName.substr(0, aDot) : theName;
	size_t anOtherDot = theOther.find_last_of('.');
	return aStem + ((anOtherDot != std::string::npos) ? theOther.substr(anOtherDot) : std::string());
}

// port: script command "save <file>" (see WinDMApp.h). Silent when it works, so a script with save
// lines logs exactly what the same script without them logs.
bool WinDMApp::HandleScriptCommand(const ScriptCommand& theCommand)
{
	// port: the standard test commands (testhooks/TestCommands.cpp)
	if (HandleTestCommand(theCommand))
		return true;
	// port(variant): gemsheet <file.png>, the gem sheet as loaded and grown (stage 8), its HD pixels to <file>@<n>x.png
	if ((theCommand.mCommand == "gemsheet") && (mImages != NULL) && (mImages[IMAGE_GEMSHEET] != NULL))
	{
		if (!GemSheet::WritePngs((MemoryImage*) mImages[IMAGE_GEMSHEET], theCommand.mArgs))
			SDL_Log("script: line %d: gemsheet: cannot write %s", theCommand.mLine, theCommand.mArgs.c_str());
		return true;
	}
	// port: memreport <file>, what the game holds in memory by owner (testhooks/MemReport.h)
	if (theCommand.mCommand == "memreport")
	{
		if (!WriteMemReport(this, theCommand.mArgs))
			SDL_Log("script: line %d: memreport: cannot write %s", theCommand.mLine, theCommand.mArgs.c_str());
		return true;
	}
	if (theCommand.mCommand != "save")
		return false;

	std::vector<ShutdownSave> aSaves;
	if (!theCommand.mArgs.empty())
		GetShutdownSaves(&aSaves);
	if (theCommand.mArgs.empty())
		SDL_Log("script: line %d: save needs <file>", theCommand.mLine);
	else if (aSaves.empty())
		SDL_Log("script: line %d: save: no game in progress, %s not written", theCommand.mLine, theCommand.mArgs.c_str());
	for (size_t i = 0; i < aSaves.size(); i++)
	{
		// The save itself to <file>; port(variant): its sidecar next to it
		std::string aName = (i == 0) ? theCommand.mArgs : WithExtensionOf(theCommand.mArgs, aSaves[i].mFileName);
		const std::string& aData = aSaves[i].mData;
		FILE* aFile = fopen(aName.c_str(), "wb");
		bool success = (aFile != NULL) && (fwrite(aData.data(), 1, aData.size(), aFile) == aData.size());
		if ((aFile != NULL) && (fclose(aFile) != 0))
			success = false;
		if (!success)
			SDL_Log("script: line %d: save: cannot write %s", theCommand.mLine, aName.c_str());
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
		if ((mDialogMap.size() == 0) && (mReplayOverlay != NULL)) // port(variant): the watch's keys (ReplayOverlay.h)
			mWidgetManager->SetFocus(mReplayOverlay);
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
	SexyString aText = (mBoard != NULL) ? mBoard->InstructionsText() : GetString("INSTRUCTIONDIALOG"); // port(variant): the rules' own text (BoardVariant.cpp)
	Dialog* aDialog = NewDialog(DIALOG_INSTRUCTIONS, false, "", aText, "", Dialog::BUTTONS_NONE);
	aDialog->mContentInsets = Insets(16, 24, 16, 24);
	aDialog->Resize(201, 38, 418, aDialog->GetPreferredHeight(418));
	AddDialog(DIALOG_INSTRUCTIONS, aDialog);
}

// 0x4d5ac1
void WinDMApp::ShowFullInstructionsDialog()
{
	SexyString aText = GetString("FULL_INSTRUCTIONS2");
	if (mBoard != NULL) aText = mBoard->FullInstructionsText(aText); // port(variant): the rules' own first paragraph (BoardVariant.cpp)
	Dialog* aDialog = NewDialog(DIALOG_FULL_INSTRUCTIONS, true, GetString("FULL_INSTRUCTIONS1"), aText, GetString("DONE"), Dialog::BUTTONS_FOOTER);
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
	// main() has applied the enabled mods; read their gameplay tunables. port(mods): values held for the next fresh
	// game apply now unless a saved game waits for "Continue?" (CanCommitNextGameValues)
	gGameConfig.Load(CanCommitNextGameValues());

	LoadProperties();
	EnableCodeArt(); // port(variant)

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

// port(variant): code-drawn art (art/CodeArt.h). The labels drawn into the images are strings, so a mod can reword them.
void WinDMApp::EnableCodeArt()
{
	if (!gGameConfig.mFeatures.mCodeArt)
		return;
	CodeArt::Settings aSettings = CodeArt::SettingsFor(gGameConfig.mFeatures, gGameConfig.mGemColors);
	// drawn at the window's render scale, and at least 2, so a window made larger later still looks sharp
	int aRenderScale = (mDDInterface != NULL) ? mDDInterface->mRenderScale : 2;
	aSettings.mScale = std::max(2, std::min(aSettings.mScale, aRenderScale));
	aSettings.mText = [this](const std::string& theId, const std::string& theDefault) { return GetString(theId, theDefault); };
	CodeArt::Enable(aSettings);
}

// DoUpdateCheck (0x4d68da, UpdateCheckDialog 18 + HTTP version check) is not ported: the port is always up to date

// 0x4d6a40
void WinDMApp::ButtonDepress(int theId)
{
	if ((mWatch != NULL) && mWatch->ButtonDepress(theId)) return; // port(variant): New Game and the mode buttons end a watch
	if ((theId == BUTTON_REPLAY_STATS) && (mReplayStatsPage == NULL) && (mBoard != NULL) && (mBoard->mStats != NULL)) // port(variant)
	{
		// The watched board's Stats page over the board window and the dialog; a click on it closes it
		mReplayStatsPage = new GameStatsPage(this, [this]()
		{
			mWidgetManager->RemoveWidget(mReplayStatsPage);
			SafeDeleteWidget(mReplayStatsPage);
			mReplayStatsPage = NULL;
		});
		mReplayStatsPage->Resize(GameStatsPage::PAGE_X, GameStatsPage::PAGE_Y, GameStatsPage::PAGE_WIDTH, GameStatsPage::PAGE_HEIGHT);
		mWidgetManager->AddWidget(mReplayStatsPage);
		mWidgetManager->BringToFront(mReplayStatsPage);
		return;
	}
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
#ifdef __EMSCRIPTEN__
				SiteScoresChanged(this);
#endif
				KillDialog(DIALOG_RECORD_SCORE);
			}
			break;

		case DIALOG_CONFIRM_LOAD:
			KillDialog(DIALOG_CONFIRM_LOAD);
			mBoard->LoadGame();
			mBoard->DeleteSaveGame();
			break;

		// port(variant): stage 13
		case DIALOG_CONFIRM_WATCH:
			KillDialog(DIALOG_CONFIRM_WATCH);
			StartWatch(mPendingWatch);
			break;

		case DIALOG_REPLAY_END:
			{
				// Watch again: the same file from its start (StartWatch deletes the controller that holds the name)
				std::string aPath = mWatch->GetFileName();
				StartWatch(aPath);
			}
			break;

		case DIALOG_CONFIRM_SEED:
			KillDialog(DIALOG_CONFIRM_SEED);
			StartSeedGame(mPendingPlaySeed);
			break;

		case DIALOG_CONFIRM_DELETE:
			{
				KillDialog(DIALOG_CONFIRM_DELETE);
				ReplayBrowserDialog* aBrowser = (ReplayBrowserDialog*) GetDialog(DIALOG_REPLAYS);
				if (aBrowser != NULL)
					aBrowser->DeleteSelected();
			}
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
		else if (aDialogId == DIALOG_REPLAY_END) // port(variant): Back
			EndWatch();
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

	case BUTTON_UNDO: // port(variant)
		mBoard->RequestUndo();
		break;
	}
}

// port(variant): a replay from the browser or the game-over screen (WinDMApp.h). A game in progress is left only after
// asking; ~Board saves it, as on a mode switch, and the end of the watch offers it again.
void WinDMApp::WatchReplay(const std::string& thePath, bool theFromDialogs)
{
	std::string aPath = thePath; // the browser that holds it may close first
	if (theFromDialogs)
		CloseExtras();
	if (HasGameInProgress())
	{
		mPendingWatch = aPath;
		Dialog* aDialog = DoDialog(DIALOG_CONFIRM_WATCH, true, GetString("REPLAY_CONFIRM_WATCH1", _S("WATCH REPLAY")),
			GetString("REPLAY_CONFIRM_WATCH2", _S("Leave this game to watch the replay? Your game is saved, and offered again when you are done watching.")),
			"", Dialog::BUTTONS_YES_NO);
		aDialog->Resize(234, 50, 348, aDialog->GetPreferredHeight(348));
		return;
	}
	StartWatch(aPath);
}

// port(variant): a new controller in place of the last one (stopped first if it still watched; its ended board
// is replaced by the new watch's, or by the player's game if this replay cannot be played)
void WinDMApp::StartWatch(const std::string& thePath)
{
	std::string aPath = thePath; // it may be the name the old controller holds
	RemoveReplayStats();
	KillDialog(DIALOG_REPLAY_END);
	bool wasPlayback = (mBoard != NULL) && mBoard->mPlaybackOnly;
	if (mWatch != NULL)
	{
		if (mWatch->IsWatching())
			mWatch->Stop();
		delete mWatch;
	}
	mWatch = new WatchController(this, aPath);
	if (mWatch->Start())
		return;
	RemoveReplayOverlay();
	if (wasPlayback)
	{
		NewGame();
		ConfirmLoadGame();
	}
	// "REPLAY <file>: cannot play: <why>": the why
	std::string aWhy = mWatch->GetResult();
	size_t aColon = aWhy.find(": cannot play: ");
	if (aColon != std::string::npos)
		aWhy = aWhy.substr(aColon + 15);
	Dialog* aDialog = DoDialog(DIALOG_REPLAY_ERROR, true, GetString("REPLAY_ERROR1", _S("REPLAY")),
		GetString("REPLAY_ERROR2", _S("This replay cannot be played here: ")) + aWhy, GetString("DIALOG_BUTTON_OK", _S("OK")), Dialog::BUTTONS_FOOTER);
	aDialog->Resize(234, 50, 348, aDialog->GetPreferredHeight(348));
}

// port(variant): back to the player's game, as the side-bar buttons ended a watch in stage 3b
void WinDMApp::EndWatch()
{
	RemoveReplayStats();
	KillDialog(DIALOG_REPLAY_END);
	RemoveReplayOverlay();
	if ((mWatch == NULL) || !mWatch->IsWatching())
		return;
	mWatch->Stop();
	NewGame();
	ConfirmLoadGame();
}

// port(variant): the overlay over each playback board, on top of the side bar's widgets, with the focus (a modal
// dialog keeps both)
void WinDMApp::OnWatchBoard()
{
	if (mReplayOverlay == NULL)
	{
		mReplayOverlay = new ReplayOverlay(this);
		mWidgetManager->AddWidget(mReplayOverlay);
	}
	if (mWidgetManager->mBaseModalWidget == NULL)
	{
		mWidgetManager->BringToFront(mReplayOverlay);
		mWidgetManager->SetFocus(mReplayOverlay);
	}
}

// port(variant): REPLAY END over the replay's last board: Watch again or Back
void WinDMApp::OnWatchEnded()
{
	SexyString aLines;
	if (mWatch->IsEndOk() && (mBoard != NULL))
		aLines = GetString("REPLAY_END_SCORE", _S("Score: ")) + StrFormat(_S("%d"), mBoard->mScore) + GetString("REPLAY_END_LEVEL", _S(", level ")) +
			StrFormat(_S("%d"), mBoard->mLevel + 1);
	else
		aLines = GetString("REPLAY_END_CORRUPT", _S("This replay no longer plays the same in this build, so it stops here."));
	// A board that counted (features.gameStats) gets a STATS link on a line of its own above the buttons: a child of the
	// dialog, so it moves with it and takes clicks while the dialog is modal
	bool hasStats = mWatch->IsEndOk() && (mBoard != NULL) && (mBoard->mStats != NULL);
	if (hasStats)
		aLines += _S("\n ");
	Dialog* aDialog = DoDialog(DIALOG_REPLAY_END, true, GetString("REPLAY_END", _S("REPLAY END")), aLines, "", Dialog::BUTTONS_YES_NO);
	aDialog->mYesButton->mLabel = GetString("REPLAY_WATCH_AGAIN", _S("Watch again"));
	aDialog->mNoButton->mLabel = GetString("REPLAY_BACK", _S("Back"));
	aDialog->Resize(234, 50, 348, aDialog->GetPreferredHeight(348));
	if (hasStats)
	{
		mReplayStatsLink = new HyperlinkWidget(BUTTON_REPLAY_STATS, this);
		mReplayStatsLink->mLabel = GetString("REPLAY_STATS", _S("STATS"));
		mReplayStatsLink->SetFont(aDialog->mLinesFont);
		mReplayStatsLink->mColor = aDialog->GetColor(Dialog::COLOR_LINES);
		mReplayStatsLink->mOverColor = Color(255, 255, 255);
		int aWidth = aDialog->mLinesFont->StringWidth(mReplayStatsLink->mLabel) + 8;
		int aHeight = aDialog->mLinesFont->GetHeight() + 4;
		int aButtonsTop = aDialog->mYesButton->mY - aDialog->mY;
		mReplayStatsLink->Resize((aDialog->mWidth - aWidth) / 2, aButtonsTop - aHeight - 6, aWidth, aHeight);
		aDialog->AddWidget(mReplayStatsLink);
	}
}

void WinDMApp::RemoveReplayStats()
{
	if (mReplayStatsPage != NULL)
	{
		mWidgetManager->RemoveWidget(mReplayStatsPage);
		SafeDeleteWidget(mReplayStatsPage);
		mReplayStatsPage = NULL;
	}
	if (mReplayStatsLink != NULL)
	{
		if (mReplayStatsLink->mParent != NULL)
			mReplayStatsLink->mParent->RemoveWidget(mReplayStatsLink);
		SafeDeleteWidget(mReplayStatsLink);
		mReplayStatsLink = NULL;
	}
}

void WinDMApp::RemoveReplayOverlay()
{
	if (mReplayOverlay == NULL)
		return;
	mWidgetManager->RemoveWidget(mReplayOverlay);
	SafeDeleteWidget(mReplayOverlay);
	mReplayOverlay = NULL;
}

// port(variant): Extras has something to show (8): the seed field or line, replays (the leaderboard implies them);
// not LiveSplit alone
bool WinDMApp::HasExtras()
{
	const GameFeatures& aFeatures = gGameConfig.mFeatures;
	return aFeatures.mSeedEntry || aFeatures.mSeedPerGame || aFeatures.mReplays || aFeatures.mLeaderboard;
}

void WinDMApp::ShowExtrasDialog()
{
	ExtrasDialog* aDialog = new ExtrasDialog(this);
	aDialog->Resize(234, 50, 348, aDialog->GetPreferredHeight(348));
	AddDialog(DIALOG_EXTRAS, aDialog);
}

void WinDMApp::ShowReplayBrowser()
{
	ReplayBrowserDialog* aDialog = new ReplayBrowserDialog(this);
	aDialog->Resize((mWidth - ReplayBrowserDialog::WIDTH) / 2, 30, ReplayBrowserDialog::WIDTH,
		aDialog->GetPreferredHeight(ReplayBrowserDialog::WIDTH));
	AddDialog(DIALOG_REPLAYS, aDialog);
}

void WinDMApp::CloseExtras()
{
	KillDialog(DIALOG_REPLAYS);
	KillDialog(DIALOG_EXTRAS);
	if (GetDialog(DIALOG_OPTIONS) != NULL)
		ButtonDepress(2000 + DIALOG_OPTIONS); // Done: its checkboxes apply
}

// port(variant): Extras' Play this seed (4.10): a new game on theSeed now. A game in progress is abandoned as New
// Game abandons it, after asking; an untouched board or the game-over screen needs no asking, and a watch ends
// first (then the saved game is offered, as after any watch; a resumed save leaves the seed for the next new game).
void WinDMApp::PlaySeed(uint32_t theSeed)
{
	bool isWatching = (mWatch != NULL) && mWatch->IsWatching();
	if (!isWatching && (mBoard != NULL) && (mBoard->mState != STATE_GAME_OVER_DONE) && !mBoard->mIsNewGame)
	{
		mPendingPlaySeed = theSeed;
		Dialog* aDialog = DoDialog(DIALOG_CONFIRM_SEED, true, GetString("CONFIRM_SEED1", _S("PLAY SEED")),
			GetString("CONFIRM_SEED2", _S("Abandon this game and start a new one on seed ")) + StrFormat(_S("%u?"), (unsigned) theSeed), "",
			Dialog::BUTTONS_YES_NO);
		aDialog->Resize(234, 50, 348, aDialog->GetPreferredHeight(348));
		return;
	}
	StartSeedGame(theSeed);
}

void WinDMApp::StartSeedGame(uint32_t theSeed)
{
	CloseExtras();
	gGameConfig.mPendingSeed = theSeed;
	if ((mWatch != NULL) && mWatch->IsWatching())
		EndWatch();
	else if ((mBoard->mState == STATE_GAME_OVER_DONE) || mBoard->mIsNewGame)
		NewGame();
	else
		mBoard->RequestNewGame(); // New Game's shake and fall, then NewGame
}

void WinDMApp::ConfirmDeleteReplay()
{
	Dialog* aDialog = DoDialog(DIALOG_CONFIRM_DELETE, true, GetString("REPLAY_DELETE1", _S("DELETE REPLAY")),
		GetString("REPLAY_DELETE2", _S("Delete this replay? It cannot be brought back.")), "", Dialog::BUTTONS_YES_NO);
	aDialog->Resize(234, 50, 348, aDialog->GetPreferredHeight(348));
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
	if (gGameConfig.mFeatures.mLean && gGameConfig.mFeatures.mLeanImages)
	{
		// port(lean): only the title screen draws its picture
		delete mImages[IMAGE_TITLESCREEN];
		mImages[IMAGE_TITLESCREEN] = NULL;
	}

	MakeButton(BUTTON_NEW_GAME, Rect(18, 142, 156, 41), -1, IMAGE_OVER_NEWGAME_PUSHED);
	mNormalModeButton = MakeButton(BUTTON_NORMAL_MODE, Rect(18, 185, 78, 31), -1, IMAGE_OVER_NORMAL_PUSHED);
	mTimedModeButton = MakeButton(BUTTON_TIMED_MODE, Rect(96, 185, 78, 31), -1, IMAGE_OVER_TIMETRIAL_PUSHED);
	MakeButton(BUTTON_OPTIONS, Rect(18, 241, 156, 41), -1, IMAGE_OVER_OPTIONS_PUSHED);
	MakeButton(BUTTON_QUIT, Rect(18, 405, 156, 40), -1, IMAGE_OVER_QUIT_PUSHED);
	ButtonWidget* aHintButton = MakeButton(BUTTON_HINT, Rect(64, 303, 53, 73), IMAGE_OVER_HINT, IMAGE_OVER_HINT);
	aHintButton->mMouseInsets = Insets(-16, -8, -16, -8);
#ifndef BEJ_NO_VARIANTS
	// port(variant): the red orb left of the timer bar, the undo button of a board with undo (UndoOrb.h); it draws
	// nothing and takes no mouse on any other board
	UndoOrb* anUndoOrb = new UndoOrb(this, BUTTON_UNDO);
	mWidgetManager->AddWidget(anUndoOrb);
	mButtonList.push_back(anUndoOrb);
#endif

	// SetTimedMode already starts a game; the second NewGame replaces that board (both consume Rand())
	if ((mWatch != NULL) && mWatch->Start()) return; // port(variant): -replay=<file>: the replay instead of the two new games
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
				if (!CodeArt::IsEnabled())
				{
					Popup(std::string("Failed to load sounds\\") + gSoundNames[i]);
					mLoadingFailed = true;
					return;
				}
				mMissingSounds++; // port(variant): code art plays on without it (features.codeArt)
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

	// port(variant): room for every colour the rules allow; the original 7 now, the others when a board needs them
	// (EnsureGemColors), so the loading bar counts as before
	mGemShrinkImages = new MemoryImage**[Board::MAX_GEM_COLORS];
	for (int aColor = 0; aColor < Board::NUM_GEM_COLORS; aColor++)
	{
		MakeGemShrinkImages(aColor);
		mCompletedLoadingThreadTasks++;
	}

	PrecacheAdditive((MemoryImage*) mImages[IMAGE_BLACKHOLE]);
	PrecacheAlpha((MemoryImage*) mImages[IMAGE_HOLEMASK]);
	mCompletedLoadingThreadTasks++;
}
