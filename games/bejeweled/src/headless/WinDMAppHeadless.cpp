// port(variant): the verifier's WinDMApp (games/bejeweled/docs/CODE_MODS_DESIGN.md 6.9); see HeadlessApp.h. Not part
// of the original game. Every WinDMApp member a board or the replay player (WatchController) calls, and every
// virtual function of the class (its vtable), defined for an app without window, sound, images, dialogs or
// files on disk. The game's own definitions are in games/bejeweled/src/WinDMApp.cpp, which the verifier does
// not compile.

#include "HeadlessApp.h"
#include "Board.h"
#include "WatchController.h"
#include "WidgetManager.h"
#include "SoundManager.h"
#include "MusicInterface.h"
#include "ImageFont.h"
#include "Image.h"

using namespace Sexy;

WinDMApp::WinDMApp()
{
	mShowInstructions = true;
	mImages = new Image*[NUM_IMAGES];
	for (int i = 0; i < NUM_IMAGES; i++)
		mImages[i] = NULL;
	mGemShrinkImages = NULL;
	mGemShrinkColors = 0;
	mTitleScreen = NULL;
	mBoard = NULL;
	mNoPopLink = false;
	mProdName = "Bejeweled";
	mProductVersion = "1.87";
	mTitle = "Bejeweled Deluxe " + mProductVersion;
	mRegKey = "PopCap\\Bejeweled";

	// Fonts with the metrics the verifier gives them (Font.h); images none (Image.h: drawing does nothing)
	mAlphaDigitsFont = new ImageFont();
	mScoreFont = new ImageFont();
	mHeaderFont = new ImageFont();
	mMainFont = new ImageFont();
	mOverFont = new ImageFont();
	mHighScoreWidget = NULL;
	for (int i = 0; i < 6; i++)
	{
		mUnusedImagesA[i] = NULL;
		mUnusedImagesB[i] = NULL;
	}
	for (int i = 0; i < NUM_TIMERBAR_IMAGES; i++)
		mTimerBarImages[i] = NULL;
	mCustomCursorsEnabled = false;
	mIsDiamondMine = false;

	// The original data\diff.txt (500 150 / 1.0 0.5 / 250 10 / 2.0 0.5 / 1.0 2.0 / 0.01 0.005, WinDMApp.h); a
	// replay's RULE carries its own, which the replay player swaps in for each step
	mNormalPointsBase = 500;
	mNormalPointsPerLevel = 150.0;
	mNormalMultBase = 1.0;
	mNormalMultPerLevel = 0.5;
	mTimedPointsBase = 250;
	mTimedPointsPerLevel = 10.0;
	mTimedMultBase = 2.0;
	mTimedMultPerLevel = 0.5;
	mTimeDrainBase = 1.0;
	mTimeDrainPerLevel = 2.0;
	mTimeDrainAccelBase = 0.01;
	mTimeDrainAccelPerLevel = 0.005;
	mNormalModeButton = NULL;
	mTimedModeButton = NULL;
	mTimedMode = false;

	mTraceHashFile = NULL;
	mAutoplay = false;
	mAutoplaySeed = 0;
	mAutoplayTimed = false;
	mAutoplayCap = 0;
	mAutoplayBot = NULL;
	mCodecCheck = NULL;
	mWatch = NULL;
	mReplayQuit = false;
	mReplayOverlay = NULL;
	mPendingPlaySeed = 0;

	mWidgetManager = new WidgetManager();
	mSoundManager = new SoundManager();
	mMusicInterface = new MusicInterface();
}

WinDMApp::~WinDMApp()
{
	ProcessSafeDeleteList();
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		delete mBoard;
		mBoard = NULL;
	}
	delete mWatch;
	mWatch = NULL;
	delete [] mImages;
	delete mAlphaDigitsFont;
	delete mScoreFont;
	delete mHeaderFont;
	delete mMainFont;
	delete mOverFont;
	delete mMusicInterface;
	delete mSoundManager;
	delete mWidgetManager;
}

// As the game's (0x4d43f9): a game in progress is saved (a playback board has none: Board::IsGameInProgress)
void WinDMApp::Shutdown()
{
	if (!mShutdown)
	{
		if ((mBoard != NULL) && (mBoard->IsGameInProgress()))
		{
			mBoard->SaveGame();
			mWidgetManager->RemoveWidget(mBoard);
			SafeDeleteWidget(mBoard);
			mBoard = NULL;
		}
		mShutdown = true;
	}
}

// The board's game ended (after New Game's fall): the watched replay ends, else the end flag
void WinDMApp::NewGame()
{
	if ((mWatch != NULL) && mWatch->OnBoardEnd(false))
		return;
	HeadlessApp* anApp = dynamic_cast<HeadlessApp*>(this);
	if (anApp != NULL)
		anApp->mNewGameCalled = true;
}

// The game-over screen: the watched replay ends, else the end flag
void WinDMApp::ShowHighScoreWidget()
{
	if ((mWatch != NULL) && mWatch->OnBoardEnd(true))
		return;
	HeadlessApp* anApp = dynamic_cast<HeadlessApp*>(this);
	if (anApp != NULL)
		anApp->mHighScoresCalled = true;
}

// No images
void WinDMApp::EnsureGemColors(int /*theColors*/)
{
}

void WinDMApp::SetTimedMode(bool isTimed, bool startNewGame)
{
	mTimedMode = isTimed;
	if (startNewGame)
		NewGame();
}

bool WinDMApp::KillDialog(int theDialogId)
{
	return false;
}

void WinDMApp::ShowInstructionsDialog()
{
}

void WinDMApp::ConfirmLoadGame()
{
}

// Watching from the game (stage 13): the verifier's watch quits at its end (-replayquit) and has no overlay or dialogs
void WinDMApp::OnWatchBoard()
{
}

void WinDMApp::OnWatchEnded()
{
}

void WinDMApp::EndWatch()
{
}

// One frame: the replay player's step, as the game's UpdateFrames (0x4d4921) starts
void WinDMApp::UpdateFrames()
{
	if (mWatch != NULL)
		mWatch->BeforeFrame();
	mUpdateCount++;
}

// The other virtual functions of WinDMApp: nothing in the verifier calls them
void WinDMApp::LoadingThreadCompleted() {}
void WinDMApp::URLOpenSucceeded(const std::string& theURL) {}
MusicInterface* WinDMApp::CreateMusicInterface() { return NULL; }
Dialog* WinDMApp::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines,
	const SexyString& theDialogFooter, int theButtonMode) { return NULL; }
void WinDMApp::ModalOpen() {}
void WinDMApp::LostFocus() {}
void WinDMApp::ModalClose() {}
void WinDMApp::GotFocus() {}
void WinDMApp::ButtonPress(int theId) {}
void WinDMApp::SetMusicVolume(double theVolume) {}
std::string WinDMApp::GetGameSEHInfo() { return ""; }
void WinDMApp::URLOpenFailed(const std::string& theURL) {}
bool WinDMApp::OpenURL(const std::string& theURL, bool shutdownOnOpen) { return false; }
void WinDMApp::WriteToRegistry() {}
void WinDMApp::ReadFromRegistry() {}
void WinDMApp::PreDisplayHook() {}
void WinDMApp::Init() {}
void WinDMApp::ButtonDepress(int theId) {}
void WinDMApp::LoadingThreadProc() {}
void WinDMApp::GetShutdownSaves(std::vector<ShutdownSave>* theSaves) {}
void WinDMApp::WriteShutdownRegistry() {}
void WinDMApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue) {}
bool WinDMApp::HandleScriptCommand(const ScriptCommand& theCommand) { return false; }
bool WinDMApp::HasGameInProgress() { return false; }
bool WinDMApp::CanRestartNow(SexyString* theReason) { return false; }
void WinDMApp::GetRestartArgs(std::vector<std::string>* theArgs) {}
void WinDMApp::GetModMessages(std::vector<ModMessage>* theMessages) {}

HeadlessApp::HeadlessApp()
{
	mNewGameCalled = false;
	mHighScoresCalled = false;
}

HeadlessApp::~HeadlessApp()
{
}

void HeadlessApp::SetPointsAscent(int theAscent)
{
	mAlphaDigitsFont->mAscent = theAscent;
}
