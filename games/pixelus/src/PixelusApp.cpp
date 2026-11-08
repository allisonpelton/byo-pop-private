#include "PixelusApp.h"
#include "DialogButton.h"
#include "testhooks/StateLog.h"
#include "GameDialog.h"
#include "GameText.h"
#include "LoadingScreen.h"
#include "LevelData.h"
#include "ProgressManager.h"
#include "UserDirectory.h"
#include "Board.h"
#include "MainMenu.h"
#include "IslandMap.h"
#include "TempleScreen.h"
#include "PuzzleCompleted.h"
#include "CompletionScreen.h"
#include "StoryScreen.h"
#include "OptionsDialog.h"
#include "CreditsDialog.h"
#include "HelpScreen.h"
#include "PuzzleImages.h"
#include "Grid.h"
#include "Temples.h"
#include "UserDialogs.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "MusicInterface.h"
#include "WidgetManager.h"
#include "Compat2004.h"

using namespace Sexy;

PixelusApp* Sexy::gApp = NULL;

static int GridRandom()
{
	return gApp->RandNext();
}

// 0x435880 (members cleared) + 0x434660 (names). The product version, build number and build date are what the
// 2004 SexyApp reads from the exe's version resource and build-info block; saved games carry them.
PixelusApp::PixelusApp()
{
	gApp = this;
	Grid::sRandom = &GridRandom;

	mProductVersion = "1.0";
	mBuildNum = 51;
	mBuildDate = "Mon Jul 05 10:43:35 2004";

	mBoard = NULL;
	mHelpScreen = NULL;
	mTempleScreen = NULL;
	mLoadingScreen = NULL;
	mMainMenu = NULL;
	mIslandMap = NULL;
	mPuzzleCompleted = NULL;
	mHintOverlay = NULL;
	mHintScreen = NULL;
	mCompletion = NULL;
	mStoryScreen = NULL;
	for (int i = 0; i < NUM_HEADS; i++)
		for (int j = 0; j < NUM_HEAD_EXPRESSIONS; j++)
			mHeads[i][j] = NULL;
	mLevelData = NULL;
	mUserDirectory = NULL;
	mProgress = NULL;
	mCurrentTemple = 0;
	mAutoplay = false;
	mAutoplayPuzzles = 3;
	mAutoplayCap = 200000;
	mAutoplayState = NULL;
	mAutoplayUI = false;
	mAutoplaySkip = false;
	mRandDraws = 0;
	mAppRandDraws = 0;
	SeedMTRand2004(mAppRand, 4357);
	mStateLog = NULL;

	mProdName = "Pixelus";
	mTitle = "Pixelus " + mProductVersion;
	mRegKey = "PopCap\\Pixelus";

	// 0x4e30f0: the Nc layer's volumes (the music is mixed lower than the effects)
	mMusicVolume = 0.595;
	mSfxVolume = 0.85;

	mColorDark = new Color(0x1a, 0x5e, 0x49);
	mColorMid = new Color(0x39, 0x79, 0x68);
	mColorLight = new Color(0x49, 0x8c, 0x7a);
}

PixelusApp::~PixelusApp()
{
	delete mStateLog;
	delete mUserDirectory;
	delete mProgress;
	delete mLevelData;
	delete mColorDark;
	delete mColorMid;
	delete mColorLight;
	for (int i = 0; i < NUM_HEADS; i++)
		for (int j = 0; j < NUM_HEAD_EXPRESSIONS; j++)
			delete mHeads[i][j];
}

// 0x4336f0
void PixelusApp::Init()
{
	NcApp::Init();
	SeedMTRand2004(mRand, mRandSeed);
	LoadGameTexts();

	mLevelData = new LevelData(this);
	mProgress = new ProgressManager(this, mLevelData);
	mUserDirectory = new UserDirectory(this);

	mLoadingScreen = new LoadingScreen(this);
	mLoadingScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mLoadingScreen);
}

// 0x433950: everything the game loads, as 328 tasks (4 songs, 28 sounds, 16 fonts, 236 images, then the 44 head
// pictures: for each of the eleven gods' heads four copies with the four expressions painted over the face)
void PixelusApp::LoadingThreadProc()
{
	mNumLoadingThreadTasks = NUM_LOADING_TASKS;

	// port: the in-game songs are played from their own files (music\pixelus in-game playlist.txt), not from
	// the cached concatenation the original builds here on the first run (0x4e9a40-0x4e9f10)
	LoadMusicNames(gPixelusMusicNames, NUM_MUSICS);
	StartMusic(this, MUSIC_PIXELUS_MAIN_MENU_LOOP_V02, false);
	LoadSounds(gPixelusSoundNames, NUM_SOUNDS);
	LoadFonts(gPixelusFontNames, NUM_FONTS);
	LoadImages(gPixelusImageNames, gPixelusImageGroups, NUM_IMAGES);
	SetCursorImages(IMAGE_CURSOR_POINTER, IMAGE_CURSOR_HAND, IMAGE_CURSOR_DRAGGING, IMAGE_CURSOR_TEXT);

	for (int i = 0; i < NUM_HEADS; i++)
	{
		Image* aHead = mImages[IMAGE_CYCLOP_HEAD + i];
		Image* anExpressions = mImages[IMAGE_CYCLOP_EXP + i];
		int aWidth = aHead->GetWidth();
		int aHeight = aHead->GetHeight();
		for (int j = 0; j < NUM_HEAD_EXPRESSIONS; j++)
		{
			MemoryImage* anImage = new MemoryImage(this);
			anImage->Create(aWidth, aHeight);
			anImage->SetImageMode(true, true);
			Graphics g(anImage);
			g.DrawImage(aHead, 0, 0);
			g.DrawImage(anExpressions, 0, 0, Rect(0, j * aHeight, aWidth, aHeight));
			mHeads[i][j] = anImage;
			mCompletedLoadingThreadTasks++;
		}
	}
}

// 0x433b90 (slot 79): the play link of the loading screen was clicked; the main menu takes its place, and a game
// that has no player yet asks for a name
void PixelusApp::PlayClicked()
{
	mWidgetManager->RemoveWidget(mLoadingScreen);
	SafeDeleteWidget(mLoadingScreen);
	mLoadingScreen = NULL;

	ShowMainMenu();
	if (mUserDirectory->GetCurrent() == NULL)
		ShowNewUser();
	StartMusic(this, MUSIC_PIXELUS_MAIN_MENU_LOOP_V02, false);
}

// 0x40b1c0
void PixelusApp::ShowMainMenu()
{
	if (mMainMenu != NULL)
	{
		mWidgetManager->RemoveWidget(mMainMenu);
		SafeDeleteWidget(mMainMenu);
	}
	mMainMenu = new MainMenu(this);
	mMainMenu->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mMainMenu);
	mWidgetManager->BringToBack(mMainMenu);
	mWidgetManager->SetFocus(mMainMenu);
}

// 0x432a40: the play button. The first time a player plays, the story comes before the island
void PixelusApp::PlayGame()
{
	if (mMainMenu != NULL)
	{
		mWidgetManager->RemoveWidget(mMainMenu);
		SafeDeleteWidget(mMainMenu);
		mMainMenu = NULL;
	}

	UserProfile* aUser = mUserDirectory->GetCurrent();
	if ((aUser != NULL) && aUser->mFlag7A)
	{
		ShowIslandMap();
		return;
	}
	if (aUser != NULL)
		aUser->mFlag7A = true;
	ShowStory(false, 0);
}

// 0x432e40 (the player dialogs ask SizeDialog for 400 pixels at least: 0x432e89, 0x432f0a, 0x43301a)
void PixelusApp::ShowUserSelect()
{
	KillDialog(0x12);
	SelectUserDialog* aDialog = new SelectUserDialog(this);
	StyleDialog(aDialog);
	SizeDialog(this, aDialog, 0x190);
	AddDialog(0x12, aDialog);
}

// 0x432ec0
void PixelusApp::ShowNewUser()
{
	KillDialog(0x13);
	NewUserDialog* aDialog = new NewUserDialog(this, false);
	StyleDialog(aDialog);
	SizeDialog(this, aDialog, 0x190);
	AddDialog(0x13, aDialog);
}

// 0x432fc0
void PixelusApp::ShowRenameDialog(const std::string& theName)
{
	KillDialog(0x15);
	NewUserDialog* aDialog = new NewUserDialog(this, true);
	aDialog->SetName(theName);
	StyleDialog(aDialog);
	SizeDialog(this, aDialog, 0x190);
	AddDialog(0x15, aDialog);
}

// 0x434bc0
void PixelusApp::ShowDeleteQuestion(const std::string& theName)
{
	KillDialog(0x14);
	char aBuffer[256];
	snprintf(aBuffer, sizeof(aBuffer), GTC(0x5125f8), theName.c_str());
	DoDialog(0x14, true, GT(0x512634), aBuffer, "", Dialog::BUTTONS_YES_NO);
}

// 0x433f20: the player picked in "Who are you?" becomes the current one
void PixelusApp::SelectUserOK(bool theOK)
{
	SelectUserDialog* aDialog = (SelectUserDialog*) GetDialog(0x12);
	if (aDialog == NULL)
		return;
	if (theOK)
	{
		UserProfile* aUser = mUserDirectory->FindUserByName(aDialog->GetSelectedName());
		if (aUser != NULL)
		{
			mUserDirectory->Select(aUser->mId);
			if (mMainMenu != NULL)
				mMainMenu->UserChanged();
			mWidgetManager->MarkAllDirty();
		}
	}
	KillDialog(0x12);
}

// 0x434860
void PixelusApp::NewUserOK(bool theOK)
{
	NewUserDialog* aDialog = (NewUserDialog*) GetDialog(0x13);
	if (aDialog == NULL)
		return;

	std::string aName = aDialog->GetName();
	bool anExpert = aDialog->IsExpert();
	if (theOK && !aName.empty())
	{
		if (mUserDirectory->FindUserByName(aName) != NULL)
		{
			DoDialog(0x16, true, GT(0x512528), GT(0x512538), GT(0x50b558), Dialog::BUTTONS_FOOTER);
			return;
		}
		UserProfile* aUser = mUserDirectory->AddUser(aName);
		if (anExpert)
			aUser->mFlag38 = true;
		mUserDirectory->Save();
		KillDialog(0x12);
		KillDialog(0x13);
		if (mMainMenu != NULL)
			mMainMenu->UserChanged();
		mWidgetManager->MarkAllDirty();
		return;
	}

	// cancelling is only allowed when there is a player to go back to; an empty name asks again
	if (!theOK && (mUserDirectory->GetCurrent() != NULL))
	{
		KillDialog(0x13);
		return;
	}
	DoDialog(0x16, true, GT(0x512588), GT(0x512598), GT(0x50b558), Dialog::BUTTONS_FOOTER);
}

// 0x434cf0: delete the player picked in "Who are you?"
void PixelusApp::DeleteUserOK(bool theOK)
{
	KillDialog(0x14);
	SelectUserDialog* aDialog = (SelectUserDialog*) GetDialog(0x12);
	if (!theOK || (aDialog == NULL))
		return;

	UserProfile* aUser = mUserDirectory->FindUserByName(aDialog->GetSelectedName());
	if (aUser != NULL)
		mUserDirectory->DeleteUser(aUser->mId);
	UserProfile* aCurrent = mUserDirectory->GetCurrent();
	aDialog->FillList(aCurrent != NULL ? aCurrent->mName : "");
	mUserDirectory->Save();
	if (aCurrent == NULL)
		ShowNewUser();
	if (mMainMenu != NULL)
		mMainMenu->UserChanged();
	mWidgetManager->MarkAllDirty();
}

// 0x4350a0: the new name of the player picked in "Who are you?"
void PixelusApp::RenameUserOK(bool theOK)
{
	if (!theOK)
	{
		KillDialog(0x15);
		return;
	}

	SelectUserDialog* aSelect = (SelectUserDialog*) GetDialog(0x12);
	NewUserDialog* aRename = (NewUserDialog*) GetDialog(0x15);
	if ((aSelect == NULL) || (aRename == NULL))
		return;
	std::string anOldName = aSelect->GetSelectedName();
	std::string aNewName = aRename->GetName();
	if (aNewName.empty())
		return;
	if (!mUserDirectory->RenameUser(anOldName, aNewName))
	{
		DoDialog(0x17, true, GT(0x512528), GT(0x5126d8), GT(0x50b558), Dialog::BUTTONS_FOOTER);
		return;
	}
	mUserDirectory->Save();
	aSelect->RenameSelected(aNewName);
	mWidgetManager->MarkAllDirty();
	KillDialog(0x15);
}

// 0x4e3970: "Are you sure you want to quit the game?"
void PixelusApp::ShowQuitDialog()
{
	Dialog* aDialog = DoDialog(4, true, GT(0x5281c0), GT(0x5281c8), "", Dialog::BUTTONS_YES_NO);
	CenterDialog(aDialog);
}

// 0x432a10
void PixelusApp::ShowOptions()
{
	ShowOptionsDialog(false);
}

// 0x4194d0: the options dialog, in the middle of the window's top half; the options of a puzzle come with the way back to the
// main menu
void PixelusApp::ShowOptionsDialog(bool theInGame)
{
	KillDialog(OptionsDialog::DIALOG_ID);
	OptionsDialog* aDialog = new OptionsDialog(this, theInGame);
	StyleDialog(aDialog);
	int aHeight = aDialog->GetPreferredHeight(0x1a4);
	aDialog->Resize(0x6e, 0xf0 - aHeight / 2, 0x1a4, aHeight);
	AddDialog(OptionsDialog::DIALOG_ID, aDialog);
}

// 0x432a00 (0x402290): over the options dialog
void PixelusApp::ShowCredits()
{
	KillDialog(CreditsDialog::DIALOG_ID);
	CreditsDialog* aDialog = new CreditsDialog(this);
	StyleDialog(aDialog);
	int aHeight = aDialog->GetPreferredHeight(0x1a4);
	aDialog->Resize(0x6e, 0xf0 - aHeight / 2, 0x1a4, aHeight);
	AddDialog(CreditsDialog::DIALOG_ID, aDialog);
}

// 0x4328c0: the "how to play" screen over everything, and the game stands still
void PixelusApp::ShowHelp()
{
	if (mHelpScreen != NULL)
		return;
	mHelpScreen = new HelpScreen(this);
	mHelpScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mHelpScreen);
		mWidgetManager->SetFocus(mHelpScreen);
	Pause(true);
}

// 0x432c20: the board that was under it takes its fade back with the end of the pause
void PixelusApp::CloseHelp()
{
	if (mHelpScreen == NULL)
		return;
	mWidgetManager->RemoveWidget(mHelpScreen);
	SafeDeleteWidget(mHelpScreen);
	mHelpScreen = NULL;
	if (mBoard != NULL)
	{
		if (GetDialog(OptionsDialog::DIALOG_ID) == NULL)
			mBoard->mPauseFade = 0;
		mWidgetManager->SetFocus(mBoard);
	}
	Pause(false);
}

// 0x4191e0 with 0x433e70: the puzzle is saved where it is, the board goes, and the main menu comes back
void PixelusApp::OptionsToMainMenu()
{
	if (mBoard != NULL)
	{
		mBoard->SaveProgress();
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = NULL;
	}
	ShowMainMenu();
	if (mUserDirectory->GetCurrent() == NULL)
		ShowNewUser();
	StartMusic(this, MUSIC_PIXELUS_MAIN_MENU_LOOP_V02, false);
	KillDialog(OptionsDialog::DIALOG_ID);
	Pause(false);
}

// 0x432600: the island; the music of the menu fades out (the sea and the birds are all there is to hear)
void PixelusApp::ShowIslandMap()
{
	if (mIslandMap != NULL)
	{
		mWidgetManager->RemoveWidget(mIslandMap);
		SafeDeleteWidget(mIslandMap);
		mIslandMap = NULL;
	}
	mIslandMap = new IslandMap(this);
	mIslandMap->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mIslandMap);
	mWidgetManager->BringToBack(mIslandMap);
	mWidgetManager->SetFocus(mIslandMap);
	FadeOutAllMusic(this);
}

// 0x433c90: the way back from the island
void PixelusApp::BackToMainMenu()
{
	if (mIslandMap != NULL)
	{
		mWidgetManager->RemoveWidget(mIslandMap);
		SafeDeleteWidget(mIslandMap);
		mIslandMap = NULL;
	}
	ShowMainMenu();
	if (mUserDirectory->GetCurrent() == NULL)
		ShowNewUser();
	StartMusic(this, MUSIC_PIXELUS_MAIN_MENU_LOOP_V02, true);
}

// 0x4170d0: the player chose a temple on the island
void PixelusApp::EnterTemple(int theTemple)
{
	const char* aName = gTemples[theTemple].mName;
	UserProfile* aUser = mUserDirectory->GetCurrent();
	if (aUser != NULL)
		aUser->mTempleFlags[aName] |= 4;
	if (mCurrentTemple != theTemple || mLevelData->mCurrentLevel != aName)
		mLevelData->mCurrentPuzzle = -1;
	mCurrentTemple = theTemple;
	mLevelData->mCurrentLevel = aName;

	ShowTemple();
	if (mIslandMap != NULL)
	{
		mWidgetManager->RemoveWidget(mIslandMap);
		SafeDeleteWidget(mIslandMap);
		mIslandMap = NULL;
	}
}

// 0x432420: the temple screen, over a picture of the island if the island is there
void PixelusApp::ShowTemple()
{
	MemoryImage* aBackdrop = NULL;
	if (mIslandMap != NULL)
	{
		aBackdrop = new MemoryImage(this);
		aBackdrop->Create(mWidth, mHeight);
		aBackdrop->SetImageMode(false, false);
		Graphics g(aBackdrop);
		mIslandMap->Draw(&g);
	}
	if (mTempleScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mTempleScreen);
		SafeDeleteWidget(mTempleScreen);
	}
	mTempleScreen = new TempleScreen(this, aBackdrop);
	mTempleScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mTempleScreen);
	mWidgetManager->SetFocus(mTempleScreen);
}

// 0x432ac0
void PixelusApp::BackToIsland()
{
	if (mTempleScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mTempleScreen);
		SafeDeleteWidget(mTempleScreen);
		mTempleScreen = NULL;
	}
	ShowIslandMap();
}

// 0x433820: the puzzle the cursor is on, from the start or from where the player left it
void PixelusApp::StartPuzzle(bool theRestart)
{
	Pause(false);
	if (mTempleScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mTempleScreen);
		SafeDeleteWidget(mTempleScreen);
		mTempleScreen = NULL;
	}

	const LevelData::Level* aLevel = mLevelData->GetLevel(mLevelData->mCurrentLevel);
	int aPuzzle = mLevelData->mCurrentPuzzle;
	UserProfile* aUser = mUserDirectory->GetCurrent();
	if (theRestart && aUser != NULL && aLevel != NULL && aPuzzle >= 0 && aPuzzle < (int) aLevel->mPuzzles.size())
		mProgress->RemoveHistory(aUser->mId, aLevel->mPuzzles[aPuzzle].mUuid);
	ShowBoard(mLevelData->mCurrentLevel, aPuzzle);

	// a player who has asked for it sees the "how to play" screen before every puzzle outside the tutorial
	if (aUser != NULL && aUser->mFlag2D && !mBoard->mTutorial)
		ShowHelp();
}

// 0x434f70
void PixelusApp::ShowResumeQuestion()
{
	KillDialog(0x20);
	Dialog* aDialog = DoDialog(0x20, true, GT(0x512684), GT(0x512698), "", Dialog::BUTTONS_YES_NO);
	CenterDialog(aDialog);
}

// 0x433de0: yes carries on with the saved puzzle, no starts it again
void PixelusApp::ResumeAnswer(bool theResume)
{
	KillDialog(0x20);
	StartPuzzle(!theResume);
}

// 0x4101d0: the player skips the tutorial
void PixelusApp::SkipTutorialOK()
{
	KillDialog(0x1b);
	UserProfile* aUser = mUserDirectory->GetCurrent();
	if (aUser != NULL)
		aUser->mFlag79 = true;
	// 0x432dc0: the temple goes and the story of the opened temple plays (it ends on the island map)
	if (mTempleScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mTempleScreen);
		SafeDeleteWidget(mTempleScreen);
		mTempleScreen = NULL;
	}
	ShowStory(false, 1);
}

// 0x432960: a conversation, named by the temple the player is in and the one that has just opened. A script that is not
// there is no conversation
void PixelusApp::ShowStory(bool theFromMenu, int theMode, int theTo)
{
	int aFrom = mCurrentTemple;
	int aTo = theTo;
	switch (theMode)
	{
	case 0:
		aFrom = aTo = 0;
		break;
	case 1:
		// 0x41cf50: the temple that the solved count opens, the first of the nine (theTo is only the caller's guess)
		aTo = theTo;
		{
			UserProfile* aUser = mUserDirectory->GetCurrent();
			int aOutside = aUser != NULL ? aUser->CountSolved() - aUser->CountSolved(gTemples[0].mName) : 0;
			for (int i = 1; i < 10; i++)
				if (aOutside == gTemples[i].mUnlock)		// 0x433530
				{
					aTo = i;
					break;
				}
		}
		break;
	case 2:
		aTo = aFrom;
		break;
	case 3:
		aFrom = NUM_TEMPLES - 1;
		aTo = 0;
		break;
	}

	StoryScreen* aStory = new StoryScreen(this, theFromMenu, aFrom, aTo, theMode);
	if (!aStory->HasSpeeches())
	{
		delete aStory;
		EndStory(theFromMenu);
		return;
	}
	if (mStoryScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mStoryScreen);
		SafeDeleteWidget(mStoryScreen);
	}
	mStoryScreen = aStory;
	mStoryScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mStoryScreen);
	mWidgetManager->SetFocus(mStoryScreen);
	FadeOutAllMusic(this);
}

// 0x432d40 / 0x432d80: the story is over
void PixelusApp::EndStory(bool theToMenu)
{
	if (mStoryScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mStoryScreen);
		SafeDeleteWidget(mStoryScreen);
		mStoryScreen = NULL;
	}
	if (theToMenu)
	{
		ShowMainMenu();
		StartMusic(this, MUSIC_PIXELUS_MAIN_MENU_LOOP_V02, true);
	}
	else
		ShowIslandMap();
}

// 0x432550: the puzzle screen over the whole window
void PixelusApp::ShowBoard(const std::string& theLevel, int thePuzzle)
{
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = NULL;
	}
	FadeOutAllMusic(this);
	mBoard = new Board(this, theLevel, thePuzzle);
	mBoard->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mBoard);
	mWidgetManager->SetFocus(mBoard);
}

// 0x433910
void PixelusApp::LeaveBoard()
{
	if (mBoard == NULL)
		return;
	mWidgetManager->RemoveWidget(mBoard);
	SafeDeleteWidget(mBoard);
	mBoard = NULL;
	ShowTemple();
}

// 0x434e40: the question before a puzzle is started over
void PixelusApp::ShowResetDialog()
{
	KillDialog(24);
	Dialog* aDialog = DoDialog(24, true, GT(0x512644), GT(0x512654), "", Dialog::BUTTONS_OK_CANCEL);
	aDialog->Move(((mWidth - 173) - aDialog->mWidth) / 2 + 173, (mHeight - aDialog->mHeight) / 2);
}

// 0x4194d0 over a puzzle: the puzzle stands still while the options are open
void PixelusApp::ShowGameMenu()
{
	Pause(true);
	ShowOptionsDialog(true);
}

void PixelusApp::ShowHint()
{
	Pause(false);
	if (mBoard != NULL)
		mBoard->ShowHint();
}

// 0x435250: the picture of the finished puzzle goes into the tablet of the puzzle-completed screen, which comes over
// the board
void PixelusApp::PuzzleCompleted(Board* theBoard)
{
	Grid* aGrid = theBoard->mGrid;
	if (!aGrid->mHasPicture)
		LoadPuzzlePicture(this, aGrid);
	std::string aSolution;
	for (size_t i = 0; i < aGrid->mHistory.size(); i++)
		aSolution += Grid::MoveToString(aGrid->mHistory[i]);

	if (mPuzzleCompleted != NULL)
	{
		mWidgetManager->RemoveWidget(mPuzzleCompleted);
		SafeDeleteWidget(mPuzzleCompleted);
	}
	mPuzzleCompleted = new PuzzleCompletedScreen(this, MakeSolvedImage(this, *aGrid), Board::GRID_X, Board::GRID_Y,
		aGrid->mTime, aGrid->mMoveCount, aSolution, aGrid->mTitle, theBoard->mUuid);
	mPuzzleCompleted->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mPuzzleCompleted);
	mWidgetManager->SetFocus(mPuzzleCompleted);
}

// the tablet's button: the board goes, and the temple's puzzles or, when a temple has opened or been finished, the island
// comes
// the tablet and the board go (0x432b60, 0x432c90, 0x433e00)
void PixelusApp::RemoveTabletAndBoard()
{
	if (mPuzzleCompleted != NULL)
	{
		mWidgetManager->RemoveWidget(mPuzzleCompleted);
		SafeDeleteWidget(mPuzzleCompleted);
		mPuzzleCompleted = NULL;
	}
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = NULL;
	}
}

// 0x432b60: the screen that says what was completed
void PixelusApp::ShowCompletion(int theMode, int theSub, int theOpenedTemple)
{
	RemoveTabletAndBoard();
	mCompletionSub = theSub;
	mCompletionOpened = theOpenedTemple;
	mCompletion = new CompletionScreen(this, theMode);
	mCompletion->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mCompletion);
	mWidgetManager->SetFocus(mCompletion);
	PlaySample(theMode == 2 || theMode == 4 ? SOUND_PIXELUS_GAME_COMPLETED_STING_V01 : SOUND_PIXELUS_TEMPLE_COMPLETED_STING_V01);
}

// 0x40b760: its button
void PixelusApp::ContinueAfterCompletion()
{
	int aMode = mCompletion->mMode;
	if (mCompletion != NULL)
	{
		mWidgetManager->RemoveWidget(mCompletion);
		SafeDeleteWidget(mCompletion);
		mCompletion = NULL;
	}
	bool anOpened = mCompletionOpened >= 0;
	if ((aMode != 1 && mCompletionSub != 3 && aMode != 5) || aMode == 2)
		ShowStory(false, 3);
	else if (anOpened)
		ShowStory(false, 1, mCompletionOpened);
	else if (mCompletionSub == 3)
		ShowStory(false, 2);
	else
		BackToIsland();
}

// 0x419c20: the tablet's button
void PixelusApp::ContinueAfterPuzzle(int theMode, int theSub, int theOpenedTemple)
{
	if (theMode != 0)
		ShowCompletion(theMode, theSub, theOpenedTemple);
	else if (theOpenedTemple >= 0)
	{
		RemoveTabletAndBoard();
		ShowStory(false, 1, theOpenedTemple);
	}
	else if (theSub == 4)
		ShowCompletion(4, 4, -1);
	else if (theSub == 3)
	{
		RemoveTabletAndBoard();
		ShowStory(false, 2);
	}
	else
	{
		RemoveTabletAndBoard();
		ShowTemple();
	}
}

// 0x435310: the answers of the dialogs the game opens (2000 + id: the yes or OK button, 3000 + id: the no)
void PixelusApp::ButtonDepress(int theId)
{
	switch (theId)
	{
	case 2002:		// the options dialog is done
		KillDialog(OptionsDialog::DIALOG_ID);
		if (mBoard != NULL)
			Pause(false);
		return;
	case 2024:		// reset the puzzle
		KillDialog(24);
		Pause(false);
		if (mBoard != NULL)
			mBoard->RestartPuzzle();
		return;
	case 3024:
		KillDialog(24);
		Pause(false);
		return;
	case 2025:		// "Not yet" / mystery puzzle notice
		KillDialog(0x19);
		if (mTempleScreen != NULL)
			mWidgetManager->SetFocus(mTempleScreen);
		return;
	case 2031:		// a hint is read
		KillDialog(0x1f);
		return;
	case 2027:		// skip the tutorial
		SkipTutorialOK();
		return;
	case 2032:		// a saved puzzle: carry on or start again
		ResumeAnswer(true);
		return;
	case 3032:
		ResumeAnswer(false);
		return;
	case 2018:
		SelectUserOK(true);
		return;
	case 3018:
		SelectUserOK(false);
		return;
	case 2019:
		NewUserOK(true);
		return;
	case 3019:
		NewUserOK(false);
		return;
	case 2020:
		DeleteUserOK(true);
		return;
	case 3020:
		DeleteUserOK(false);
		return;
	case 2021:
		RenameUserOK(true);
		return;
	case 3021:
		RenameUserOK(false);
		return;
	}
	NcApp::ButtonDepress(theId);
}

// 0x4e2f80: the quit question was answered yes
void PixelusApp::UpdateFrames()
{
	NcApp::UpdateFrames();
	if (!mStateLogFileName.empty())
	{
		if (mStateLog == NULL)
			mStateLog = new StateLog(this);
		mStateLog->Update();
	}
	if (mQuitRequested)
	{
		mQuitRequested = false;
		Shutdown();
		return;
	}
	if (mAutoplay)
		AutoplayUpdate();
	TestUpdate();
}

// 0x4333a0 (slot 84)
void PixelusApp::HandlePaused(bool theWasPaused, bool theIsPaused, int theCount)
{
	// the music stops with the game, except while the options are open over a puzzle: the sliders are tried against it
	if (theIsPaused && GetDialog(OptionsDialog::DIALOG_ID) != NULL && mBoard != NULL && mHasFocus)
		StartMusic(this, MUSIC_CACHED_PIXELUS_IN_GAME, true);
	else if (theIsPaused)
		mMusicInterface->PauseAllMusic();
	else if (theWasPaused)
		mMusicInterface->ResumeAllMusic();

	// 0x403010: a pause that starts shows the pause screen, the one that ends restarts the idle count
	if (mBoard != NULL && !theWasPaused && theIsPaused)
		mBoard->mPauseStarted = true;
	if (mBoard != NULL && theWasPaused && !theIsPaused)
		mBoard->mIdleTicks = 0;
}

// what the dialog factory does to every dialog it makes: the fonts, colours and margins
void PixelusApp::StyleDialog(GameDialog* theDialog)
{
	theDialog->SetHeaderFont(mFonts[FONT_GARAMOND20]);
	theDialog->SetLinesFont(mFonts[FONT_GARAMOND14]);
	theDialog->SetColor(Dialog::COLOR_HEADER, Color(255, 255, 255, 255));
	theDialog->SetColor(Dialog::COLOR_LINES, Color(255, 255, 255, 255));
	theDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(255, 255, 255, 255));
	theDialog->mContentInsets = Insets(0x30, 0x3c, 0x30, 0x20);
}

// 0x40f160 (the dialog is 0x40ef30): a hint in a speech bubble over the board, one OK button, centred on the grid at 0x196
// (and the screen's middle plus theY: the tutorial's -1 and the glue hint's 200)
void PixelusApp::ShowHint(int theWidth, int theY, const SexyString& theHeader, const SexyString& theText, int theCellCount, const int (*theCells)[2])
{
	KillDialog(0x1f);
	GameDialog* aDialog = new GameDialog(this, mImages[IMAGE_DIALOG_HINTBUBBLE], mImages[IMAGE_DIALOGBUTTON_HINTBUBBLE], 0x1f, true,
		theHeader, theText, GT(0x50b558), Dialog::BUTTONS_FOOTER, NULL);
	aDialog->SetHeaderFont(mFonts[FONT_GARAMOND20]);
	aDialog->SetLinesFont(mFonts[FONT_GARAMOND14]);
	aDialog->SetColor(Dialog::COLOR_HEADER, Color(255, 255, 255, 255));
	aDialog->SetColor(Dialog::COLOR_LINES, Color(255, 255, 255, 255));
	aDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(255, 255, 255, 255));
	aDialog->mContentInsets.mLeft = 24;
	aDialog->mContentInsets.mRight = 24;
	StyleButton(this, aDialog->mYesButton, 3);
	// 0x40f160: the height is the wrapped text's plus 0x32; the middle is measured from the original's screen
	Graphics aGraphics;
	aGraphics.SetFont(mFonts[FONT_GARAMOND14]);
	int aHeight = GetWordWrappedHeight2004(&aGraphics, theWidth - aDialog->mContentInsets.mRight - aDialog->mContentInsets.mLeft -
		aDialog->mBackgroundInsets.mRight - aDialog->mBackgroundInsets.mLeft - 4, theText,
		mFonts[FONT_GARAMOND14]->GetLineSpacing() + aDialog->mLineSpacingOffset) + 0x32;
	aDialog->mContentInsets.mBottom = 10;
	aDialog->Resize(428 - theWidth / 2, theY + 161 - aHeight / 2, theWidth, aHeight);
	// the button as the original's screen shows it: 24 pixels in from both sides, 25 high, 10 above the bottom
	if (theCellCount < 0)
		aDialog->Resize(theCells[0][0], theCells[0][1], theWidth, aHeight);	// a measured top left corner
	else if (theCellCount > 0)
	{
		// the original's screen: the bubble is centred on the lesson's cells, 20 pixels above them when they lie in the lower
		// half of the screen and 19 below them otherwise (measured on the first two tutorial puzzles)
		int aSumX = 0, aMinY = 1000, aMaxY = 0;
		for (int i = 0; i < theCellCount; i++)
		{
			aSumX += Board::GRID_X + theCells[i][1] * Grid::CELL_SIZE + Grid::CELL_SIZE / 2;
			aMinY = std::min(aMinY, Board::GRID_Y + theCells[i][0] * Grid::CELL_SIZE);
			aMaxY = std::max(aMaxY, Board::GRID_Y + (theCells[i][0] + 1) * Grid::CELL_SIZE);
		}
		int aY = (aMinY + aMaxY) / 2 < 240 ? aMaxY + 19 : aMinY - 20 - aHeight;
		aDialog->Resize(aSumX / theCellCount + 2 - theWidth / 2, aY, theWidth, aHeight);
	}
	aDialog->mYesButton->Resize(aDialog->mX + 24, aDialog->mY + aHeight - 10 - 25, theWidth - 48, 25);
	AddDialog(0x1f, aDialog);
}

// 0x4331e0 slot 21: every dialog the game opens is a GameDialog, 336 wide at the top of the screen
Dialog* PixelusApp::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
	const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	GameDialog* aDialog = new GameDialog(this, mImages[IMAGE_DIALOG], mImages[IMAGE_DIALOGBUTTONCOMPONENTS],
		theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode, NULL);
	StyleDialog(aDialog);
	aDialog->Resize(0xce, 0x2a, 0x150, aDialog->GetPreferredHeight(0x150));
	return aDialog;
}
