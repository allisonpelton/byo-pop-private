#include "PixelusApp.h"
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

// 0x435880 (members cleared) + 0x434660 (names). The product version, build number and build date are what the
// 2004 SexyApp reads from the exe's version resource and build-info block; saved games carry them.
PixelusApp::PixelusApp()
{
	gApp = this;

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
	mMoreGames = NULL;
	mHintOverlay = NULL;
	mHintScreen = NULL;
	mStoryScreen = NULL;
	for (int i = 0; i < NUM_HEADS; i++)
		for (int j = 0; j < NUM_HEAD_EXPRESSIONS; j++)
			mHeads[i][j] = NULL;
	mLevelData = NULL;
	mUserDirectory = NULL;
	mProgress = NULL;
	mCurrentTemple = 0;

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

// 0x432e40
void PixelusApp::ShowUserSelect()
{
	KillDialog(0x12);
	SelectUserDialog* aDialog = new SelectUserDialog(this);
	StyleDialog(aDialog);
	SizeDialog(this, aDialog, 0x150);
	AddDialog(0x12, aDialog);
}

// 0x432ec0
void PixelusApp::ShowNewUser()
{
	KillDialog(0x13);
	NewUserDialog* aDialog = new NewUserDialog(this, false);
	StyleDialog(aDialog);
	SizeDialog(this, aDialog, 0x150);
	AddDialog(0x13, aDialog);
}

// 0x432fc0
void PixelusApp::ShowRenameDialog(const std::string& theName)
{
	KillDialog(0x15);
	NewUserDialog* aDialog = new NewUserDialog(this, true);
	aDialog->SetName(theName);
	StyleDialog(aDialog);
	SizeDialog(this, aDialog, 0x150);
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

// the screens that are not ported yet
void PixelusApp::ShowMoreGames()
{
}

void PixelusApp::ShowOptions()
{
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
	BackToIsland();
}

void PixelusApp::ShowStory(bool theFromMenu, int theChapter)
{
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
	PlaySample(SOUND_PIXELUS_START_OF_PUZZLE_V02);
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

void PixelusApp::ShowGameMenu()
{
}

void PixelusApp::ShowHint()
{
	Pause(false);
	if (mBoard != NULL)
		mBoard->ShowHint();
}

void PixelusApp::PuzzleCompleted(Board* theBoard)
{
}

// 0x435310: the answers of the dialogs the game opens (2000 + id: the yes or OK button, 3000 + id: the no)
void PixelusApp::ButtonDepress(int theId)
{
	switch (theId)
	{
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

// 0x4333a0 (slot 84)
void PixelusApp::HandlePaused(bool thePaused, bool theWasPaused, int theCount)
{
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
