// The application class of Alchemy Deluxe 1.6 (Alchemy.exe 0x4118c9-0x414903): construction, start-up, the loading
// thread, the registry, and the side panel's buttons. The screens' flow (selector, board, dialogs) is in
// AppScreens.cpp, the button handler in AppButtons.cpp. See AlchemyApp.h and docs/GAME_NOTES.md.

#include "AlchemyApp.h"
#include "Board.h"
#include "SidePanel.h"
#include "TitleScreen.h"
#include "Effects.h"
#include "GameText.h"
#include "HighScores.h"
#include "Selector.h"
#include "DDImage.h"
#include "testhooks/Autoplay.h"
#include "Compat2004.h"
#include "ButtonWidget.h"
#include "Dialog.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "MusicInterface.h"
#include "SDLMusicInterface.h"
#include "SoundManager.h"
#include "WidgetManager.h"

#include <SDL.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace Sexy;

AlchemyApp* Sexy::gApp = NULL;

// 0x5101f8: the image names; the file is images\<name> (the framework tries .gif, .jpg, .png and the "_" alpha masks)
static const char* const gImageNames[NUM_IMAGES] =
{
	"title", "backdrop", "greenbar", "cursor1", "pointer2", "overhand", "typer", "dialog", "dbutton",
	"waitbar", "symbols", "nullblock", "skulls", "cartoonstar", "light", "britelight", "buttonstrip", "quitgamestrip",
	"helpstrip", "soundstrip", "scorefont", "overfont1", "next", "wizardhead", "wizmouth2", "wizmouth3", "wizmouth4",
	"wizmouth5", "slider", "slidergem", "unchecked", "checked", "editbox", "bubbles2", "bubbles7", "wave1",
	"tinybubble1", "forgeback2", "bubbleburst", "gems1", "gems2", "levelfont1", "bigexplosion", "smsparkle",
	"tinysparkle", "selectorbutton", "glassbuttons", "level2", "level3", "level4", "level5", "level6", "level7",
	"level8", "level9", "gameselector_base", "gameselector_greenon", "gameselector_redon", "gameselector_tuton",
	"gameselector_newgameon", "hiscorebox", "hiscorebtn", "gameover", "redcircle", "helparrow"
};

// 0x510300: the sound names (sounds\<name>); "finalvanish" is in the table twice
static const char* const gSoundNames[NUM_SOUNDS] =
{
	"click", "bad", "skull", "thunk", "melt", "forgedown", "finalvanish", "endofgame", "finalgong", "warning",
	"finalvanish", "gemchange", "v4-welcome", "v4-levelcomplete", "v4-gameover"
};

// 0x4118c9
AlchemyApp::AlchemyApp()
{
	gApp = this;
	mProdName = "Alchemy";
	mProductVersion = "1.6";
	mTitle = "Alchemy " + mProductVersion;
	mRegKey = "PopCap\\Alchemy";

	mSkillLevel = 1;
	mTimeTrial = false;
	mTutorialMode = true;
	mSelectorFlag = true;
	mPendingSkill = 0;
	mSelector = NULL;
	mTitleScreen = NULL;
	mBoard = NULL;
	mEffects = NULL;
	mSidePanel = NULL;
	mAFont1 = NULL;
	mFont1 = NULL;
	mScoreFont = NULL;
	mOverFont = NULL;
	mTitleFont = NULL;
	mLevelFont = NULL;
	mHelp = NULL;
	mHighScores = NULL;
	mSoundButton = NULL;
	mImages = NULL;
	for (int i = 0; i < NUM_BACKDROPS; i++)
		mBackdrops[i] = NULL;
	for (int i = 0; i < 6; i++)
		mSoundImages[i] = NULL;
	for (int i = 0; i < NUM_SHAPE_SIZES; i++)
		for (int j = 0; j < NUM_SHAPE_IMAGES; j++)
			mShapeImages[i][j] = NULL;
	for (int i = 0; i < 4; i++)
		mDiff[i] = 0;

	mAutoplay = false;
	mAutoplaySeed = 0;
	mAutoplayMode = "strategic";
	mAutoplayCap = 20000;
	mAutoplayGames = 1;
	mAutoplayBot = NULL;
	mAutoplayUi = false;
	mAutoplaySkill = -1;
	mAutoplayMiss = 0;
	mAutoplayExtra = 0;
	mAutoplayStart = 0;
	mStateLog = NULL;
}

// 0x41159c: a game in progress is saved here (the web build does it in ShutdownHook, the app is never deleted there);
// then the widgets are taken off the screen and deleted, and the images and fonts
AlchemyApp::~AlchemyApp()
{
	if (mBoard != NULL && mBoard->HasProgress())
		mBoard->SaveGame();

	HideHelp();

	for (int i = 0; i < NUM_SHAPE_SIZES; i++)
		for (int j = 0; j < NUM_SHAPE_IMAGES; j++)
			delete mShapeImages[i][j];

	if (mHighScores != NULL)
	{
		mWidgetManager->RemoveWidget(mHighScores);
		delete mHighScores;
	}
	if (mTitleScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mTitleScreen);
		delete mTitleScreen;
	}
	if (mSelector != NULL)
	{
		mWidgetManager->RemoveWidget(mSelector);
		delete mSelector;
	}
	if (mEffects != NULL)
	{
		mWidgetManager->RemoveWidget(mEffects);
		delete mEffects;
	}
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		delete mBoard;
	}
	if (mSidePanel != NULL)
	{
		mWidgetManager->RemoveWidget(mSidePanel);
		delete mSidePanel;
	}

	for (std::list<ButtonWidget*>::iterator anItr = mButtons.begin(); anItr != mButtons.end(); ++anItr)
	{
		mWidgetManager->RemoveWidget(*anItr);
		delete *anItr;
	}
	mButtons.clear();
	for (std::list<MemoryImage*>::iterator anItr = mButtonImages.begin(); anItr != mButtonImages.end(); ++anItr)
		delete *anItr;
	mButtonImages.clear();

	delete mAFont1;
	delete mFont1;
	delete mScoreFont;
	delete mOverFont;
	delete mTitleFont;
	delete mLevelFont;
	for (int i = 0; i < NUM_BACKDROPS; i++)
		delete mBackdrops[i];
	for (int i = 0; i < 6; i++)
		delete mSoundImages[i];

	delete mAutoplayBot;
	delete mStateLog;
	delete[] mImages;
}

// port: test hooks (see testhooks/Autoplay.h). The original does not override UpdateFrames.
void AlchemyApp::UpdateFrames()
{
	SexyApp::UpdateFrames();

	if (!mStateLogFileName.empty())
	{
		if (mStateLog == NULL)
			mStateLog = new StateLog(this);
		mStateLog->Update();
	}

	if (mAutoplay)
	{
		if (mAutoplayBot == NULL)
			mAutoplayBot = new AutoplayBot(this, mAutoplaySeed);
		mAutoplayBot->Update();
	}
}

// port (web): the save the original's ~AlchemyApp writes now (see SexyAppBase::UpdateUserDataBackup): savegame.dat of
// a game in progress
void AlchemyApp::GetShutdownSaves(std::vector<ShutdownSave>* theSaves)
{
	if (mBoard == NULL || !mBoard->HasProgress())
		return;

	ShutdownSave aSave;
	aSave.mFileName = Board::GetSaveGameFileName();
	aSave.mData = mBoard->SerializeGame();
	theSaves->push_back(aSave);
}

// port (web): the original saves a game in progress only in ~AlchemyApp, and the web build never deletes the app;
// without this the save of a quit game would exist only in the localStorage backup (GetShutdownSaves)
void AlchemyApp::ShutdownHook()
{
#ifdef __EMSCRIPTEN__
	if (mBoard != NULL && mBoard->HasProgress())
		mBoard->SaveGame();
#endif
}

void AlchemyApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-autoplay")
	{
		mAutoplay = true;
		mAutoplaySeed = strtoul(theParamValue.c_str(), NULL, 10);
		mScriptedInput = true;  // the bot's input only, as in a -script run
	}
	else if (theParamName == "-autoplaymode")
		mAutoplayMode = theParamValue;
	else if (theParamName == "-autoplaygames")
		mAutoplayGames = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaycap")
		mAutoplayCap = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaylog")
		mAutoplayLogFileName = theParamValue;
	else if (theParamName == "-autoplayui")
		mAutoplayUi = true;
	else if (theParamName == "-autoplayskill")
		mAutoplaySkill = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaymiss")
		mAutoplayMiss = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplayextra")
		mAutoplayExtra = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplayscript")
		mAutoplayScriptFileName = theParamValue;
	else if (theParamName == "-autoplaystart")
		mAutoplayStart = atoi(theParamValue.c_str());
	else if (theParamName == "-statelog")
		mStateLogFileName = theParamValue;
	else if (theParamName == "-autoplayshots")
		mAutoplayShotPrefix = theParamValue;
	else
		SexyApp::HandleCmdLineParam(theParamName, theParamValue);
}

// port: the 2004 SexyApp creates a BassMusicInterface (BASS 2.0): the port's music interface gets the full range
// (docs/FRAMEWORK_2004.md section 6, 4.2)
MusicInterface* AlchemyApp::CreateMusicInterface()
{
	if (mNoSoundNeeded)
		return new MusicInterface;

	SDLMusicInterface* aMusicInterface = new SDLMusicInterface();
	aMusicInterface->mMaxMusicVolume = 100;
	return aMusicInterface;
}

// 0x4123f1: images\<name>; a missing file ends the game with a message
bool AlchemyApp::LoadImage(int theIndex)
{
	std::string aPath = std::string("images\\") + gImageNames[theIndex];
	mImages[theIndex] = GetImage(aPath);
	if (mImages[theIndex] == NULL)
	{
		Popup(std::string("Failed to load image: ") + aPath);
		mLoadingFailed = true;
		return false;
	}
	return true;
}

// 0x412d69 slot 17
void AlchemyApp::Init()
{
	SexyApp::Init();

	// port: the 2004 SexyApp::Init seeds the C library generator with srand(GetTickCount()) (Compat2004.h); -seed=N
	// test runs seed it with N so a run is reproducible
	CrtSRand(mHasSeedParam ? (unsigned int) mRandSeed : (unsigned int) GameTickCount());

	LoadProperties();  // 0x46d740: properties\default.xml, the game's texts
	LoadGameTexts();   // port: the score tables' default names, from the player's own Alchemy.exe

	DIALOG_YES_STRING = GetString("DIALOG_BUTTON_YES");
	DIALOG_NO_STRING = GetString("DIALOG_BUTTON_NO");
	DIALOG_OK_STRING = GetString("DIALOG_BUTTON_OK");
	DIALOG_CANCEL_STRING = GetString("DIALOG_BUTTON_CANCEL");

	mImages = new Image*[NUM_IMAGES];
	for (int i = 0; i < NUM_IMAGES; i++)
		mImages[i] = NULL;

	for (int i = 0; i < NUM_INIT_IMAGES; i++)
	{
		if (!LoadImage(i))
		{
			Shutdown();
			return;
		}
	}

	mAFont1 = new ImageFont(this, "data\\afont1.txt");
	mFont1 = new ImageFont(this, "data\\font1.txt");
	mTitleFont = new ImageFont(this, "data\\titlefont.txt");

	mTitleScreen = new TitleScreen(this);
	mTitleScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mTitleScreen);

	SetCursorImage(CURSOR_POINTER, mImages[IMAGE_CURSOR1]);
	SetCursorImage(CURSOR_HAND, mImages[IMAGE_POINTER2]);
	SetCursorImage(CURSOR_DRAGGING, mImages[IMAGE_OVERHAND]);
	SetCursorImage(CURSOR_TEXT, mImages[IMAGE_TYPER]);

	mMusicInterface->LoadMusic(SONG_INTRO, "music\\alch_intro.s3m");
	mMusicInterface->LoadMusic(SONG_PLAY, "music\\1985-HeHasNoFace.s3m");
	mMusicInterface->PlayMusic(SONG_INTRO, 0, false);
}

// 0x4124cf slot 18 (runs synchronously in the port): data\diff.txt, the remaining images, the sounds, three fonts,
// the 20 sizes of the 24 piece pictures and the nine backdrops. mCompletedLoadingThreadTasks drives the title's bar.
void AlchemyApp::LoadingThreadProc()
{
	mNumLoadingThreadTasks = NUM_LOADING_TASKS;

	// data\diff.txt: four numbers (the difficulty constants of the board, 0x620-0x638)
	Buffer aDiffBuffer;
	if (ReadBufferFromFile("data\\diff.txt", &aDiffBuffer))
	{
		std::string aText((const char*) aDiffBuffer.GetDataPtr(), (size_t) aDiffBuffer.GetDataLen());
		const char* aPtr = aText.c_str();
		for (int i = 0; i < 4; i++)
		{
			char* anEnd;
			mDiff[i] = strtod(aPtr, &anEnd);
			aPtr = anEnd;
		}
	}

	for (int i = NUM_INIT_IMAGES; i < NUM_IMAGES; i++)
	{
		if (!LoadImage(i))
			return;
		mCompletedLoadingThreadTasks++;
	}

	// The 15 sound tasks are counted only without a sound device
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

	mOverFont = new ImageFont(mImages[IMAGE_OVERFONT1], "data\\overfont1.txt");
	mScoreFont = new ImageFont(mImages[IMAGE_SCOREFONT], "data\\scorefont.txt");
	mLevelFont = new ImageFont(mImages[IMAGE_LEVELFONT1], "data\\levelfont1.txt");
	mCompletedLoadingThreadTasks++;

	// The piece pictures: 20 sizes (the shrink animation), each 24 pictures of 50 x 50: the 22 symbols, the blank
	// block (nullblock) and the skull (skulls), scaled into a square of (1 - (k + 1) / 21) * 50 pixels (odd)
	for (int k = 0; k < NUM_SHAPE_SIZES; k++)
	{
		int aSize = ((int) ((1.0 - (k + 1.0) * 0.047619047619047616) * 50.0)) | 1;
		for (int i = 0; i < NUM_SHAPE_IMAGES; i++)
		{
			MemoryImage* anImage = new MemoryImage(this);
			anImage->Create(50, 50);
			Graphics g(anImage);

			Image* aSource;
			Rect aSrcRect(0, 0, 50, 50);
			if (i == 22)
				aSource = mImages[IMAGE_NULLBLOCK];
			else if (i == 23)
				aSource = mImages[IMAGE_SKULLS];
			else
			{
				aSource = mImages[IMAGE_SYMBOLS];
				aSrcRect = Rect(i * 50, 0, 50, 50);
			}
			Rect aDestRect((50 - aSize) / 2, (50 - aSize) / 2, aSize, aSize);
			g.DrawImage(aSource, aDestRect, aSrcRect);
			mShapeImages[k][i] = anImage;
		}
		mCompletedLoadingThreadTasks++;
	}

	// The backdrops: [0] the board's area of IMAGE_BACKDROP, [1..8] level2..level9; each is darkened and tinted by
	// a gamma curve of its red channel (pixel = 0xff, v, v, 0: a sepia-like yellow)
	for (int i = 0; i < NUM_BACKDROPS; i++)
	{
		MemoryImage* anImage;
		if (i == 0)
			anImage = CopyImage(mImages[IMAGE_BACKDROP], Rect(0xa0, 0x27, 0x1d4, 0x1a0));
		else
			anImage = CopyImage(mImages[IMAGE_LEVEL2 + i - 1]);
		mBackdrops[i] = anImage;

		unsigned long* aBits = anImage->GetBits();
		int aCount = anImage->GetWidth() * anImage->GetHeight();
		int aTable[256];
		for (int j = 0; j < 256; j++)
		{
			int aValue = (int) (j * 1.35);
			double aGamma = pow(aValue * 0.00390625 * 2.3255813953488373, 2.2) * 0.43;
			if (aGamma >= 1.0)
				aGamma = 0.999;
			int aResult = (int) (aGamma * 256.0);
			if (aResult > 255)
				aResult = 255;
			aTable[j] = aResult;
		}
		for (int j = 0; j < aCount; j++)
		{
			unsigned long aV = (unsigned long) aTable[(aBits[j] >> 16) & 0xff];
			aBits[j] = ((((aV | 0xffffff00) << 8) | aV) << 8) & 0xffffffff;
		}
		anImage->BitsChanged();
		mCompletedLoadingThreadTasks++;
	}
}

// 0x411a41 slot 19
void AlchemyApp::WriteToRegistry()
{
	SexyApp::WriteToRegistry();

	if (!mLastScoreName.empty())
		RegistryWriteString("LastScoreName", mLastScoreName);
	if (!mLastInternetScoreName.empty())
		RegistryWriteString("LastInternetScoreName", mLastInternetScoreName);
	RegistryWriteInteger("SkillLevel", mSkillLevel);
	RegistryWriteInteger("TimeTrial", mTimeTrial ? 1 : 0);
}

// 0x411b43 slot 20
void AlchemyApp::ReadFromRegistry()
{
	SexyApp::ReadFromRegistry();

	std::string aString;
	int anInt;
	if (RegistryReadString("LastScoreName", &aString))
		mLastScoreName = aString;
	if (RegistryReadString("LastInternetScoreName", &aString))
		mLastInternetScoreName = aString;
	if (RegistryReadInteger("SkillLevel", &anInt))
		mSkillLevel = anInt;
	if (RegistryReadInteger("TimeTrial", &anInt))
		mTimeTrial = anInt != 0;
}

// 0x410bb6 slot 21: the plain dialog (black text on the parchment, gold buttons)
Dialog* AlchemyApp::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
	const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	Dialog* aDialog = new Dialog2004(mImages[IMAGE_DIALOG], mImages[IMAGE_DBUTTON], theDialogId, isModal,
		theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
	aDialog->SetButtonFont(mFont1);
	aDialog->SetHeaderFont(mTitleFont);
	aDialog->SetLinesFont(mFont1);
	aDialog->SetColor(Dialog::COLOR_LINES, Color(0, 0, 0));
	aDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0xe0, 0xb7, 0x28));
	aDialog->mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	aDialog->Resize(0xdd, 0x32, 0x15c, aDialog->GetPreferredHeight(0x15c));
	return aDialog;
}

// 0x4143e3 + 0x41448f: a button of the side panel. The backdrop shows the button as it rests; the button draws only
// its over and its down picture (two rows of the strip), and shows the hand.
ButtonWidget* AlchemyApp::AddPanelButton(int theId, const Rect& theRect, Image* theStrip, const Rect& theOverRect,
	const Rect& theDownRect)
{
	MemoryImage* anOverImage = CopyImage(theStrip, theOverRect);
	mButtonImages.push_back(anOverImage);
	MemoryImage* aDownImage = CopyImage(theStrip, theDownRect);
	mButtonImages.push_back(aDownImage);

	ButtonWidget* aButton = new ButtonWidget(theId, this);
	aButton->Resize(theRect.mX, theRect.mY, theRect.mWidth, theRect.mHeight);
	aButton->mDoFinger = true;
	aButton->mMouseInsets = Insets(-1, -1, -1, -1);
	aButton->mOverImage = anOverImage;
	aButton->mDownImage = aDownImage;
	mWidgetManager->AddWidget(aButton);
	mButtons.push_back(aButton);
	return aButton;
}

// 0x4144f1: the title screen goes; the side panel, its buttons, the effects overlay and the selector come
void AlchemyApp::TitleScreenDone()
{
	mWidgetManager->RemoveWidget(mTitleScreen);
	SafeDeleteWidget(mTitleScreen);
	mTitleScreen = NULL;

	mSidePanel = new SidePanel(this);
	mSidePanel->Resize(0, 0, BOARD_X, SCREEN_HEIGHT);
	mWidgetManager->AddWidget(mSidePanel);
	mSidePanel->MarkDirty();

	Image* aStrip = mImages[IMAGE_BUTTONSTRIP];
	AddPanelButton(BUTTON_NEW_GAME, Rect(0x21, 0x6f, 0x51, 0x18), aStrip, Rect(0, 0xa8, 0x51, 0x18), Rect(0, 0x150, 0x51, 0x18));
	AddPanelButton(BUTTON_OPTIONS, Rect(0x21, 0xe7, 0x51, 0x18), aStrip, Rect(0, 0xc0, 0x51, 0x18), Rect(0, 0x168, 0x51, 0x18));
	AddPanelButton(BUTTON_FORGE, Rect(0x21, 0x112, 0x51, 0x18), aStrip, Rect(0, 0xd8, 0x51, 0x18), Rect(0, 0x180, 0x51, 0x18));
	AddPanelButton(BUTTON_QUIT, Rect(0x22, 0x1b5, 0x4f, 0x19), mImages[IMAGE_QUITGAMESTRIP], Rect(0, 0x19, 0x4f, 0x19), Rect(0, 0x32, 0x4f, 0x19));
	AddPanelButton(BUTTON_HINT, Rect(0x15, 0x19b, 0x19, 0x18), mImages[IMAGE_HELPSTRIP], Rect(0, 0x18, 0x19, 0x18), Rect(0, 0x30, 0x19, 0x18));

	// the sound toggle: six frames of 26 x 25 cut from soundstrip, frames 4 and 5 for the button
	for (int i = 0; i < 6; i++)
		mSoundImages[i] = CopyImage(mImages[IMAGE_SOUNDSTRIP], Rect(0, i * 25, 26, 25));
	mSoundButton = new ButtonWidget(BUTTON_SOUND, this);
	mSoundButton->Resize(0x66, 0x19a, 0x1a, 0x19);
	mSoundButton->mDoFinger = true;
	mSoundButton->mMouseInsets = Insets(-1, -1, -1, -1);
	mSoundButton->mOverImage = mSoundImages[4];
	mSoundButton->mDownImage = mSoundImages[5];
	mWidgetManager->AddWidget(mSoundButton);
	mButtons.push_back(mSoundButton);
	UpdateSoundButton();

	mEffects = new EffectsWidget(this);
	mEffects->Resize(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	mWidgetManager->AddWidget(mEffects);

	SetTimeTrial(mTimeTrial);
	SetSkillLevel(mSkillLevel);
	ShowSelector();

	mMusicInterface->FadeOut(SONG_INTRO, true, 0.004);
	mMusicInterface->FadeIn(SONG_PLAY, -1, 0.002, false);

	if (HasSavedGame())
		ConfirmLoad();
}

// 0x410d2a slot 71: the sound toggle's frames follow the mute state
void AlchemyApp::UpdateSoundButton()
{
	if (mSoundButton == NULL)
		return;
	if (mMuteCount > 0)
	{
		mSoundButton->mButtonImage = mSoundImages[1];
		mSoundButton->mOverImage = mSoundImages[2];
		mSoundButton->mDownImage = mSoundImages[3];
	}
	else
	{
		mSoundButton->mButtonImage = NULL;
		mSoundButton->mOverImage = mSoundImages[4];
		mSoundButton->mDownImage = mSoundImages[5];
	}
}

// 0x410d01
void AlchemyApp::SetTimeTrial(bool isTimeTrial)
{
	mTimeTrial = isTimeTrial;
}

// 0x410ced
void AlchemyApp::SetSkillLevel(int theSkill)
{
	mTutorialMode = false;
	mSkillLevel = theSkill;
}
