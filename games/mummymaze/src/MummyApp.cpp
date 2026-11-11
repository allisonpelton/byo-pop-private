// The application class of Mummy Maze Deluxe 1.1 (WinMM.exe 0x419bb0-0x4210xx). See MummyApp.h and docs/GAME_NOTES.md.

#include "Button2003.h"
#include "MummyApp.h"
#include "Board.h"
#include "Screens.h"
#include "Dialogs.h"
#include "GameText.h"
#include "GameLevels.h"
#include "Compat2004.h"
#include "Dialog.h"
#include "DialogButton.h"
#include "ButtonWidget.h"
#include "Checkbox.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "MusicInterface.h"
#include "SoundManager.h"
#include "WidgetManager.h"
#include "platform/UserFiles.h"
#include "platform/VFS.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

MMApp* Sexy::gApp = NULL;

// 0x419bb0
MMApp::MMApp() :
	mImages(this)
{
	gApp = this;
	mProdName = "MummyMaze";
	mProductVersion = "1.1";
	mTitle = "Mummy Maze Deluxe " + mProductVersion;
	mRegKey = "PopCap\\MummyMaze";
	mWidth = GAME_WIDTH;
	mHeight = GAME_HEIGHT;

	for (int i = 0; i < NUM_SOUNDS; i++)
		mSoundTried[i] = false;
	mFont1 = NULL;
	mScoreFont = NULL;
	mHeaderFont = NULL;
	mBiggestFont = NULL;
	mPyramidFont = NULL;
	mTitleScreen = NULL;
	mMainMenu = NULL;
	mGamePanel = NULL;
	mLevelComplete = NULL;
	mHighScores = NULL;
	mHighScoreBack = NULL;
	mWorldMap = NULL;
	mBoard = NULL;
	mVictory = NULL;
	mSidePanel = NULL;
	mGameMode = MODE_CLASSIC;
	mCurrentSlot = 0;
	mSpeed = 1;
	mShowClock = false;
	mShowAnkh = true;
	mTipTwoMummies = true;
	mTipGate = true;
	mTipTrap = true;
	mTipRed = true;
	mTipScorpion = true;
	mTipDark = true;
	mTipAnkh = true;
	mMusicSlot = 0;
	mScore = 0;
	mAdvChamber = mAdvPyramid = 0;
	mClassicPyramid = mClassicChamber = 0;
	mRestoring = false;
	mRestoreFile = mRestoreMaze = 0;
	mFixedFile = false;
	mFixedFileNumber = mFixedMaze = 0;
	mElapsedBase = 0;
	mTotalTime = 0.0;
	mDeaths = 0;
	mChamberDone = false;
	memset(mLightMask, 0, sizeof(mLightMask));
	// 0x419bb0: the high half is drawn first (the C library's generator is seeded later, in Init)
	int aHigh = CrtRand();
	int aLow = CrtRand();
	SeedMTRand2004(mRand, (aHigh << 16) | (aLow & 0xffff));

	mAutoplay = false;
	mAutoplayGames = 1;
	mAutoplayCap = 200000;
	mAutoplayState = NULL;
}

// 0x419fc0: the widgets the app holds come off the widget manager before they go
MMApp::~MMApp()
{
	Widget* aWidgets[] = {mLevelComplete, mHighScores, mHighScoreBack, mWorldMap, mBoard, mVictory, mSidePanel, mGamePanel,
		mMainMenu, mTitleScreen};
	for (size_t i = 0; i < sizeof(aWidgets) / sizeof(aWidgets[0]); i++)
	{
		if (aWidgets[i] != NULL)
		{
			mWidgetManager->RemoveWidget(aWidgets[i]);
			delete aWidgets[i];
		}
	}
	mBoard = NULL;
	delete mFont1;
	delete mScoreFont;
	delete mHeaderFont;
	delete mBiggestFont;
	delete mPyramidFont;
}

// port: the table of the pictures: none is decoded until the game asks for it
ImageTable::ImageTable(MMApp* theApp) :
	mApp(theApp)
{
	for (int i = 0; i < NUM_IMAGES; i++)
		mImages[i] = NULL;
}

// port: a picture's first use decodes it
Image* ImageTable::Load(int theId)
{
	Image* anImage = mApp->DoLoadImage(theId);
	if (anImage == NULL)
	{
		// port: a picture that is not there is told at its first use, not at start; a blank one stands in while the game ends
		static MemoryImage* aBlank = NULL;
		if (aBlank == NULL)
		{
			aBlank = new MemoryImage();
			aBlank->Create(1, 1);
		}
		return aBlank;
	}
	mImages[theId] = anImage;
	return anImage;
}

// 0x41db90: images\<name>
Image* MMApp::DoLoadImage(int theImageId)
{
	std::string aPath = std::string("images\\") + gImageNames[theImageId];
	Image* anImage = GetImage(aPath);
	if (anImage == NULL)
	{
		Popup("Failed to load " + aPath);
		mLoadingFailed = true;
		Shutdown();
	}
	return anImage;
}

// 0x41e200
void MMApp::Init()
{
	SexyApp::Init();
	mFixes = LoadFixSettings();

	// the 2002 SexyApp::Init seeds the C library generator with the tick count; -seed=N makes a run repeatable
	CrtSRand(mHasSeedParam ? (unsigned int) mRandSeed : (unsigned int) GetTickCount());
	LoadGameTexts();
	LoadGameLevels();

	// the original decodes every picture here; mImages decodes each when it is first asked for (a picture that is not there
	// is told then: a popup and the end of the game)
	mFont1 = new ImageFont(mImages[IMAGE_FONT1], "data\\font1.txt");
	mHeaderFont = new ImageFont(mImages[IMAGE_HEADERFONT], "data\\headerfont.txt");
	mBiggestFont = new ImageFont(mImages[IMAGE_BIGGESTFONT], "data\\biggestfont.txt");

	SetCursorImage(0, mImages[IMAGE_POINTER]);
	SetCursorImage(1, mImages[IMAGE_HAND]);
	SetCursorImage(2, mImages[IMAGE_DRAGGING]);
	SetCursorImage(3, mImages[IMAGE_TEXT]);

	mTitleScreen = new TitleScreen(this);
	mTitleScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mTitleScreen);

	// the game's music is one module; both slots play it from the order of the piece
	mMusicInterface->LoadMusic(0, "music\\game.it");
	mMusicInterface->LoadMusic(1, "music\\game.it");
	mMusicInterface->PlayMusic(0);
	mMusicSlot = 0;
}

// port: a sound is decoded when it is first played; the original decodes all of them in its loading thread (0x41dd60), a
// fifth of the load. One that is not there stays silent, and so does the one MP3 (pummel) while the platform has no
// decoder for it.
void MMApp::LoadSoundOnce(int theSoundNum)
{
	if (theSoundNum < 0 || theSoundNum >= NUM_SOUNDS || mSoundTried[theSoundNum] || mSoundManager == NULL)
		return;
	mSoundTried[theSoundNum] = true;
	mSoundManager->LoadSound(theSoundNum, std::string("sounds\\") + gSoundNames[theSoundNum]);
}

// port: a sound's first play loads it (LoadSoundOnce)
void MMApp::PlaySample(int theSoundNum)
{
	LoadSoundOnce(theSoundNum);
	SexyApp::PlaySample(theSoundNum);
}

// port: the same, for a panned sound
void MMApp::PlaySample(int theSoundNum, int thePan)
{
	LoadSoundOnce(theSoundNum);
	SexyApp::PlaySample(theSoundNum, thePan);
}

// 0x41dd60: the sounds (the loading bar counts them: their files are decoded at their first play, see LoadSoundOnce), the
// score and pyramid fonts, then the light mask
void MMApp::LoadingThreadProc()
{
	mNumLoadingThreadTasks = NUM_LOADING_TASKS;
	mCompletedLoadingThreadTasks += NUM_SOUNDS;
	mCompletedLoadingThreadTasks++;
	mScoreFont = new ImageFont(mImages[IMAGE_SCOREFONT], "data\\scorefont.txt");
	mPyramidFont = new ImageFont(mImages[IMAGE_PYRAMIDFONT], "data\\pyramidfont.txt");
	mCompletedLoadingThreadTasks++;

	// the light mask of the dark pyramids: 255 inside a radius of 80 pixels around the middle, falling off to 0
	for (int y = 0; y < 256; y++)
	{
		for (int x = 0; x < 256; x++)
		{
			int aDx = abs(x - 0x80);
			int aDy = abs(y - 0x80);
			double aDist = sqrt((double) (aDx * aDx + aDy * aDy)) - 80.0;
			if (aDist <= 0.0)
				aDist = 0.0;
			double aFrac = aDist * 0.020833333333333332;
			int aValue = (int) (255.0 - pow(aFrac, 2.0) * 256.0);
			if (aValue < 0)
				aValue = 0;
			else if (aValue > 0xff)
				aValue = 0xff;
			mLightMask[y][x] = (uint8_t) aValue;
		}
	}
}

// 0x41a3c0: the registry values of the game
void MMApp::WriteToRegistry()
{
	SexyApp::WriteToRegistry();
	if (!mLastScoreName.empty())
		RegistryWriteString("LastScoreName", mLastScoreName);
	RegistryWriteInteger("Speed", mSpeed);
	RegistryWriteInteger("Name", mCurrentSlot);
	RegistryWriteInteger("Clock", mShowClock ? 1 : 0);
	RegistryWriteInteger("Ankh", mShowAnkh ? 1 : 0);
	RegistryWriteInteger("TwoMummyNag", mTipTwoMummies ? 1 : 0);
	RegistryWriteInteger("GateNag", mTipGate ? 1 : 0);
	RegistryWriteInteger("TrapNag", mTipTrap ? 1 : 0);
	RegistryWriteInteger("RedNag", mTipRed ? 1 : 0);
	RegistryWriteInteger("ScorpNag", mTipScorpion ? 1 : 0);
	RegistryWriteInteger("DarkNag", mTipDark ? 1 : 0);
	RegistryWriteInteger("AnkhNag", mTipAnkh ? 1 : 0);
}

// 0x41a620
void MMApp::ReadFromRegistry()
{
	SexyApp::ReadFromRegistry();
	std::string aName;
	if (RegistryReadString("LastScoreName", &aName))
		mLastScoreName = aName;
	int aValue;
	if (RegistryReadInteger("Speed", &aValue))
		mSpeed = aValue;
	if (RegistryReadInteger("Name", &aValue))
		mCurrentSlot = aValue;
	if (RegistryReadInteger("Clock", &aValue))
		mShowClock = aValue != 0;
	if (RegistryReadInteger("Ankh", &aValue))
		mShowAnkh = aValue != 0;
	if (RegistryReadInteger("TwoMummyNag", &aValue))
		mTipTwoMummies = aValue != 0;
	if (RegistryReadInteger("GateNag", &aValue))
		mTipGate = aValue != 0;
	if (RegistryReadInteger("TrapNag", &aValue))
		mTipTrap = aValue != 0;
	if (RegistryReadInteger("RedNag", &aValue))
		mTipRed = aValue != 0;
	if (RegistryReadInteger("ScorpNag", &aValue))
		mTipScorpion = aValue != 0;
	if (RegistryReadInteger("DarkNag", &aValue))
		mTipDark = aValue != 0;
	if (RegistryReadInteger("AnkhNag", &aValue))
		mTipAnkh = aValue != 0;
}

// 0x41aa90
Dialog* MMApp::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
	const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	Dialog* aDialog = new Dialog2004(mImages[IMAGE_DIALOG], mImages[IMAGE_DBUTTON], theDialogId, isModal,
		theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
	Use2003Buttons(aDialog, mImages[IMAGE_DBUTTON]);
	aDialog->SetButtonFont(mFont1);
	aDialog->SetHeaderFont(mHeaderFont);
	aDialog->SetLinesFont(mFont1);
	aDialog->SetColor(Dialog::COLOR_HEADER, Color(0x66, 0x5c, 0x38));
	aDialog->SetColor(Dialog::COLOR_LINES, Color(0x2e, 0x14, 0));
	aDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0xe0, 0xb7, 0x28));
	aDialog->mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	aDialog->Resize(MESSAGE_X, MESSAGE_Y, MESSAGE_WIDTH, aDialog->GetPreferredHeight(MESSAGE_WIDTH));
	return aDialog;
}

// 0x41e9d0: every button press clicks
void MMApp::ButtonPress(int theId)
{
	(void) theId;
	PlaySample(SOUND_CLICK);
}

// 0x41e9b0: the music's update runs a second time (the fades take half as many updates as the framework's one step gives)
void MMApp::UpdateFrames()
{
	SexyApp::UpdateFrames();
	mMusicInterface->Update();
	if (mAutoplay)
		AutoplayUpdate();
}

// port: the test bot's options (-autoplay and the ones that go with it)
void MMApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-autoplay")
	{
		mAutoplay = true;
		mScriptedInput = true;   // the bot's input only, as in a -script run
		HandleAutoplayParam(theParamName, theParamValue);
	}
	else if ((theParamName == "-autoplaylevels") || (theParamName == "-autoplaystart") || (theParamName == "-autoplaycontinue") || (theParamName == "-autoplaytrace") || (theParamName == "-autoplaysave") || (theParamName == "-autoplayquit"))
		HandleAutoplayParam(theParamName, theParamValue);
	else if (theParamName == "-autoplaymode")
		mAutoplayMode = theParamValue;
	else if (theParamName == "-autoplaygames")
		mAutoplayGames = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaycap")
		mAutoplayCap = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaylog")
		mAutoplayLog = theParamValue;
	else if (theParamName == "-autoplayshots")
		mAutoplayShots = theParamValue;
	else
		SexyApp::HandleCmdLineParam(theParamName, theParamValue);
}

// 0x41e9e0: music from an order of the game's module; the two slots take turns so that one can fade out as the
// other fades in
void MMApp::DoMusic(int theOrder, bool theFade)
{
	if (theFade)
	{
		mMusicInterface->FadeOut(0, true, 0.004);
		mMusicInterface->FadeOut(1, true, 0.004);
		mMusicSlot = abs((mMusicSlot + 1) % 2);
		mMusicInterface->FadeIn(mMusicSlot, theOrder, 0.002, false);
		return;
	}
	mMusicInterface->StopMusic(0);
	mMusicInterface->StopMusic(1);
	mMusicSlot = abs((mMusicSlot + 1) % 2);
	mMusicInterface->PlayMusic(mMusicSlot, theOrder, false);
}
