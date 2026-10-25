// The application class of Noah's Ark Deluxe 1.1 (WinNoah.exe 0x4166d0-0x41c230). See NoahApp.h and
// docs/GAME_NOTES.md section 3.

#include "NoahApp.h"
#include "Board.h"
#include "Screens.h"
#include "Dialogs.h"
#include "GameText.h"
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

#include <cstdio>
#include <cstring>

using namespace Sexy;

NoahApp* Sexy::gApp = NULL;

// the fonts' descriptor files (data\fontbig.txt ...), in FontId order
static const char* const gFontFiles[NUM_FONTS] =
{
	"data\\fontbig.txt", "data\\fontmedium.txt", "data\\fontsmall.txt", "data\\colorlessfont.txt",
	"data\\garamondfontmed.txt", "data\\garamondfontsmall.txt", "data\\garamondfonttiny.txt",
	"data\\scorefont.txt", "data\\registerfont.txt"
};

// the songs: 0 is the game's (an intro that leads into a loop), 1 the title and menu, 2 the game over
enum { SONG_GAME = 0, SONG_MENU = 1, SONG_GAMEOVER = 2, SONG_GAME_INTRO = 3 };

// 0x4166d0
NoahApp::NoahApp()
{
	gApp = this;
	mProdName = "NoahsArk";
	mProductVersion = "1.1";
	mTitle = "Noah's Ark Deluxe " + mProductVersion;
	mRegKey = "PopCap\\NoahsArk";
	mWidth = GAME_WIDTH;
	mHeight = GAME_HEIGHT;

	mImages = NULL;
	for (int i = 0; i < NUM_FONTS; i++)
		mFonts[i] = NULL;
	mBoard = NULL;
	mNextLevelWidget = NULL;
	mIntroWidget = NULL;
	mTitleScreen = NULL;
	mHighScoreWidget = NULL;
	mDifficulty = 0;
	mLevel = 0;
	mGameMode = GAMEMODE_ACTION;
	mSavedGameMode = GAMEMODE_ACTION;
	mBonusRound = false;
	mPuzzleNoticeShown = false;
	mGameScore = 0;
	mShownScore = 0;
	mFloodStart = 0;
	mSavedThisLevel = 0;
	mAnimalsSaved = 0;
	mComboCount = 0;
	mBestCombo = 0;
	mShowHints = true;
	mShowTutorial = true;
	mGameStartTick = 0;
	mTimesPlayed = 0;
	mRegistered = true;
	mQuitButtonImage = NULL;
	mOptionsButtonImage = NULL;
	mQuitButton = NULL;
	mOptionsButton = NULL;
	mMainSongStarted = false;
	mMainSongLooping = false;
	mAutoplay = false;
	mAutoplayMode = GAMEMODE_ACTION;
	mAutoplayDiff = 0;
	mAutoplayGames = 1;
	mAutoplayCap = 200000;
	mAutoplayState = NULL;
}

NoahApp::~NoahApp()
{
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		delete mBoard;
	}
	delete mNextLevelWidget;
	delete mIntroWidget;
	delete mTitleScreen;
	delete mHighScoreWidget;
	delete mQuitButtonImage;
	delete mOptionsButtonImage;
	if (mImages != NULL)
		delete[] mImages;
}

// 0x417450: images\<name>
bool NoahApp::LoadImage(int theImageId)
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

// 0x417970
void NoahApp::Init()
{
	SexyApp::Init();

	// the 2002 SexyApp::Init seeds the C library generator with the tick count; -seed=N makes a run repeatable
	CrtSRand(mHasSeedParam ? (unsigned int) mRandSeed : (unsigned int) GetTickCount());
	LoadGameTexts();

	mImages = new Image*[NUM_IMAGES];
	for (int i = 0; i < NUM_IMAGES; i++)
		mImages[i] = NULL;
	for (int i = 0; i < NUM_IMAGES; i++)
	{
		if (!LoadImage(i))
		{
			Shutdown();
			return;
		}
	}
	for (int i = 0; i < NUM_FONTS; i++)
		mFonts[i] = new ImageFont(this, gFontFiles[i]);

	// the music: the title's, the game's (its intro and its loop are two files) and the game over's
	mMusicInterface->LoadMusic(SONG_MENU, "music\\loading.ogg");
	mMusicInterface->LoadMusic(SONG_GAME_INTRO, "music\\mainintro.ogg");
	mMusicInterface->LoadMusic(SONG_GAME, "music\\mainloop.ogg");
	mMusicInterface->LoadMusic(SONG_GAMEOVER, "music\\gameover.ogg");
	mSoundManager->SetVolume(mSfxVolume);

	mTitleScreen = new TitleScreen(this);
	mTitleScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mTitleScreen);

	mNumLoadingThreadTasks = NUM_LOADING_TASKS + 1;
	mMusicInterface->FadeIn(SONG_MENU, -1, 0.002, false);
}

// 0x417720: the sounds (the loading bar counts them)
void NoahApp::LoadingThreadProc()
{
	mNumLoadingThreadTasks = NUM_SOUNDS + 1;
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
	mCompletedLoadingThreadTasks++;
}

void NoahApp::LoadingThreadCompleted()
{
	SexyApp::LoadingThreadCompleted();
}

// 0x41af10 / 0x41b010: the registry values of the game
void NoahApp::WriteToRegistry()
{
	SexyApp::WriteToRegistry();
	if (!mLastScoreName.empty())
		RegistryWriteString("LastScoreName", mLastScoreName);
	RegistryWriteBoolean("ShowHints", mShowHints);
	RegistryWriteBoolean("ShowTutorial", mShowTutorial);
}

void NoahApp::ReadFromRegistry()
{
	SexyApp::ReadFromRegistry();
	std::string aName;
	if (RegistryReadString("LastScoreName", &aName))
		mLastScoreName = aName;
	bool aValue;
	if (RegistryReadBoolean("ShowHints", &aValue))
		mShowHints = aValue;
	if (RegistryReadBoolean("ShowTutorial", &aValue))
		mShowTutorial = aValue;
}

// 0x416d50
Dialog* NoahApp::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
	const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	Dialog* aDialog = new Dialog2004(mImages[IMAGE_DIALOG], mImages[IMAGE_DBUTTON], theDialogId, isModal,
		theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
	aDialog->SetButtonFont(mFonts[FONT_SMALL]);
	aDialog->SetHeaderFont(mFonts[FONT_MEDIUM]);
	aDialog->SetLinesFont(mFonts[FONT_GARAMOND_SMALL]);
	aDialog->SetColor(Dialog::COLOR_LINES, Color(0, 0, 0));
	aDialog->mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	aDialog->Resize(0xed, 0x32, 0x13c, aDialog->GetPreferredHeight(0x132));
	return aDialog;
}

// 0x418c30: every button press clicks
void NoahApp::ButtonPress(int theId)
{
	(void) theId;
	PlaySample(SOUND_CLICK);
}

// the menu music goes on while the game's plays are not on: the intro of the game song leads into its loop
void NoahApp::UpdateFrames()
{
	SexyApp::UpdateFrames();
	if (mAutoplay)
		AutoplayUpdate();
	if (mMainSongStarted && !mMusicInterface->IsPlaying(SONG_GAME_INTRO) && !mMusicInterface->IsPlaying(SONG_GAME) &&
		mMainSongLooping == false && !mMusicInterface->IsPlaying(SONG_MENU) && !mMusicInterface->IsPlaying(SONG_GAMEOVER))
	{
		mMusicInterface->PlayMusic(SONG_GAME, 0, false);
		mMainSongLooping = true;
	}
}

void NoahApp::PlayGameMusic()
{
	mMusicInterface->StopMusic(SONG_MENU);
	mMusicInterface->StopMusic(SONG_GAME);
	mMusicInterface->StopMusic(SONG_GAME_INTRO);
	mMusicInterface->PlayMusic(SONG_GAME_INTRO, 0, true);
	mMainSongStarted = true;
	mMainSongLooping = false;
}

void NoahApp::LostFocus()
{
	SexyApp::LostFocus();
	if (mBoard != NULL)
		mBoard->MarkDirty();
}

// 0x417320: a button of the menus, in the app's list
DialogButton* NoahApp::AddDialogButton(int theId, const Rect& theRect, Image* theImage)
{
	DialogButton* aButton = new DialogButton(theImage, theId, this);
	aButton->Resize(theRect);
	aButton->mDoFinger = true;
	aButton->mMouseInsets = Insets(-1, -1, -1, -1);
	mWidgetManager->AddWidget(aButton);
	mButtons.push_back(aButton);
	return aButton;
}

// 0x4171f0: a picture button (the sidebar's)
ButtonWidget* NoahApp::AddButton(int theId, const Rect& theRect, Image* theImage, Image* theDownImage)
{
	ButtonWidget* aButton = new ButtonWidget(theId, this);
	aButton->Resize(theRect);
	aButton->mDoFinger = true;
	aButton->mButtonImage = theImage;
	aButton->mOverImage = theImage;
	aButton->mDownImage = theDownImage;
	aButton->mMouseInsets = Insets(-1, -1, -1, -1);
	mWidgetManager->AddWidget(aButton);
	mButtons.push_back(aButton);
	return aButton;
}

// 0x4189d0: the two buttons beside the board are made once, the title screen's click shows the menu
void NoahApp::TitleScreenDone()
{
	Image* aBack = mImages[IMAGE_BUTTONBACK];
	int aWidth = aBack->mWidth;
	int aHeight = aBack->mHeight;

	mQuitButtonImage = new MemoryImage(this);
	mQuitButtonImage->Create(aWidth, aHeight);
	{
		Graphics g(mQuitButtonImage);
		g.DrawImage(aBack, 0, 0);
		Image* aText = mImages[IMAGE_QUITTEXT];
		g.DrawImage(aText, (aWidth - aText->mWidth) / 2, (aHeight - aText->mHeight) / 2);
	}
	mQuitButton = AddButton(BUTTON_SIDEBAR_QUIT, Rect(5, 431, aWidth, aHeight), mQuitButtonImage, mQuitButtonImage);

	mOptionsButtonImage = new MemoryImage(this);
	mOptionsButtonImage->Create(aWidth, aHeight);
	{
		Graphics g(mOptionsButtonImage);
		g.DrawImage(aBack, 0, 0);
		Image* aText = mImages[IMAGE_OPTIONSTEXT];
		g.DrawImage(aText, (aWidth - aText->mWidth) / 2, (aHeight - aText->mHeight) / 2);
	}
	mOptionsButton = AddButton(BUTTON_SIDEBAR_OPTIONS, Rect(5, 390, aWidth, aHeight), mOptionsButtonImage, mOptionsButtonImage);

	ShowIntro(true);
}

// 0x4185c0: the menu (removing whatever was on: the title, the game, the level-up panel, the scores)
void NoahApp::ShowIntro(bool theKeepMusic)
{
	if (!theKeepMusic)
	{
		mMusicInterface->FadeOut(SONG_GAME, true, 0.004);
		mMusicInterface->FadeOut(SONG_GAMEOVER, true, 0.004);
		mMusicInterface->FadeIn(SONG_MENU, -1, 0.002, false);
		mMainSongStarted = false;
	}
	if (mTitleScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mTitleScreen);
		SafeDeleteWidget(mTitleScreen);
		mTitleScreen = NULL;
	}
	RemoveHighScoreWidget();
	RemoveNextLevel();
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = NULL;
	}
	RemoveIntro();
	mIntroWidget = new IntroWidget(this);
	mIntroWidget->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mIntroWidget);
	mWidgetManager->BringToBack(mIntroWidget);
	mWidgetManager->SetFocus(mIntroWidget);
	mWidgetManager->BringToFront(mOptionsButton);
	mWidgetManager->BringToFront(mQuitButton);
	// the sidebar's buttons belong to the board screens only
	mOptionsButton->SetVisible(false);
	mQuitButton->SetVisible(false);
}

// 0x418790: the menu's buttons go with the menu
void NoahApp::RemoveIntro()
{
	if (mIntroWidget != NULL)
	{
		for (std::list<ButtonWidget*>::iterator anItr = mButtons.begin(); anItr != mButtons.end(); )
		{
			ButtonWidget* aButton = *anItr;
			if ((aButton == mQuitButton) || (aButton == mOptionsButton))
			{
				++anItr;
				continue;
			}
			mWidgetManager->RemoveWidget(aButton);
			SafeDeleteWidget(aButton);
			anItr = mButtons.erase(anItr);
		}
		mWidgetManager->RemoveWidget(mIntroWidget);
		SafeDeleteWidget(mIntroWidget);
		mIntroWidget = NULL;
	}
}

// 0x418460: a new game, or the saved one
void NoahApp::StartGame(bool theContinue)
{
	if (mTitleScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mTitleScreen);
		SafeDeleteWidget(mTitleScreen);
		mTitleScreen = NULL;
	}
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = NULL;
	}
	RemoveIntro();
	mGameScore = 0;
	mShownScore = 0;
	mSavedThisLevel = 0;
	mAnimalsSaved = 0;
	mComboCount = 0;
	mBestCombo = 0;
	mBonusRound = false;
	mLevel = 0;

	mBoard = new Board(this, theContinue);
	mBoard->Resize(0, 0, GAME_WIDTH, GAME_HEIGHT);
	mWidgetManager->AddWidget(mBoard);
	mWidgetManager->BringToBack(mBoard);
	mWidgetManager->SetFocus(mBoard);
	mGameStartTick = GameTickCount();
	mOptionsButton->SetVisible(true);
	mQuitButton->SetVisible(true);
	mWidgetManager->BringToFront(mOptionsButton);
	mWidgetManager->BringToFront(mQuitButton);
	PlayGameMusic();
}

// 0x4187d0: the level-up panel
void NoahApp::ShowNextLevel()
{
	if (mBoard != NULL)
		mBoard->ClearLink();
	mNextLevelWidget = new NextLevelWidget(this);
	mNextLevelWidget->Resize(Board::CellLeft(0) - 10, Board::CellTop(0) - 10, mImages[IMAGE_FRAME]->mWidth,
		mImages[IMAGE_FRAME]->mHeight);
	mWidgetManager->AddWidget(mNextLevelWidget);
	mWidgetManager->SetFocus(mNextLevelWidget);
	mWidgetManager->BringToFront(mOptionsButton);
	mWidgetManager->BringToFront(mQuitButton);
}

// 0x4188c0
void NoahApp::RemoveNextLevel()
{
	if (mNextLevelWidget != NULL)
	{
		mWidgetManager->RemoveWidget(mNextLevelWidget);
		SafeDeleteWidget(mNextLevelWidget);
		mNextLevelWidget = NULL;
	}
}

// 0x418900: the game is over
void NoahApp::ShowHighScoreWidget()
{
	if (mHighScoreWidget == NULL)
	{
		mMusicInterface->FadeOut(SONG_GAME, true, 0.004);
		mMusicInterface->FadeIn(SONG_GAMEOVER, -1, 0.002, false);
		mHighScoreWidget = new HighScoreWidget(this);
		mHighScoreWidget->Resize(0, 0, mWidth, mHeight);
		mWidgetManager->AddWidget(mHighScoreWidget);
		DeleteSavedGame();
	}
}

void NoahApp::RemoveHighScoreWidget()
{
	if (mHighScoreWidget != NULL)
	{
		mWidgetManager->RemoveWidget(mHighScoreWidget);
		SafeDeleteWidget(mHighScoreWidget);
		mHighScoreWidget = NULL;
	}
}

// 0x418ea0
void NoahApp::ShowDifficultyDialog()
{
	DifficultyDialog* aDialog = new DifficultyDialog(this);
	aDialog->mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	int aHeight = aDialog->GetPreferredHeight(0x17c);
	aDialog->Resize(0x78, (mHeight - aHeight) / 2, 400, aHeight);
	AddDialog(DIALOG_DIFFICULTY, aDialog);
}

// 0x418c40
void NoahApp::ShowOptions()
{
	OptionsDialog* aDialog = new OptionsDialog(this);
	aDialog->mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	aDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0, 0, 0));
	int aHeight = aDialog->GetPreferredHeight(0x17c);
	aDialog->Resize(0xd5, (mHeight - aHeight) / 2, 0x17c, aHeight);
	AddDialog(DIALOG_OPTIONS, aDialog);
}

// 0x418d70
void NoahApp::ShowTutorial()
{
	TutorialDialog* aDialog = new TutorialDialog(this);
	aDialog->mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	int aHeight = aDialog->GetPreferredHeight(0x17c);
	aDialog->Resize(0xd5, (mHeight - aHeight) / 2, 0x17c, aHeight);
	aDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0, 0, 0));
	AddDialog(DIALOG_TUTORIAL, aDialog);
}

// 0x416e80
void NoahApp::ShowQuitDialog()
{
	Dialog* aDialog;
	if (mBoard != NULL)
	{
		aDialog = DoDialog(DIALOG_QUIT, true, GT(0x004956ec), GT(0x004956fc), GT(0x0049c510), Dialog::BUTTONS_YES_NO);
		aDialog->Resize(0xbb, 0x57, 0x1a0, 300);
	}
	else
	{
		aDialog = DoDialog(DIALOG_QUIT, true, GT(0x0049c510), GT(0x00495760), GT(0x0049c510), Dialog::BUTTONS_YES_NO);
		aDialog->Resize(0xbb, 0x57, 0x1a0, 300);
	}
}

// 0x41a0b0 (case 5, 6, 7): the saved game would be lost
void NoahApp::ShowNewGameDialog(int theGameMode)
{
	mGameMode = theGameMode;
	mTimesPlayed++;
	if (HasSavedGame())
	{
		Dialog* aDialog = DoDialog(DIALOG_NEW_GAME, true, GT(0x00495e94), GT(0x00495ea4), GT(0x0049c510), Dialog::BUTTONS_YES_NO);
		int aY = (theGameMode == GAMEMODE_ACTION) ? 0x73 : ((theGameMode == GAMEMODE_STRATEGY) ? 100 : 100);
		int aHeight = (theGameMode == GAMEMODE_ACTION) ? 0xfa : 0x118;
		aDialog->Resize(0x6e, aY, 0x1a4, aHeight);
	}
	else
		ShowDifficultyDialog();
}

// 0x419650: how to play in the current mode (a dialog the player closes to start)
void NoahApp::ShowModeIntroDialog(int theMode)
{
	SexyString aHeader, aLines;
	int aX = 0x5a, aY = 0x37, aW = 0x1cc, aH = 0x186;
	switch (theMode)
	{
	case GAMEMODE_ACTION:
		aHeader = GT(0x00495ac0);
		aLines = GT(0x00495acc);
		break;
	case GAMEMODE_STRATEGY:
		aHeader = GT(0x00495bc8);
		aLines = GT(0x00495bd8);
		break;
	case GAMEMODE_PUZZLE:
		if (!mBonusRound)
		{
			aHeader = GT(0x00495cc4);
			aLines = GT(0x00495cd0);
		}
		else
		{
			aHeader = GT(0x00495d3c);
			aLines = GT(0x00495d4c);
		}
		aX = 0x78;
		aY = 0x5a;
		aW = 0x1a4;
		aH = 300;
		break;
	case GAMEMODE_STAMPEDE:
		aHeader = GT(0x00495dac);
		aLines = GT(0x00495dbc);
		aX = 0x5f;
		aY = 0x5a;
		aW = 0x1c2;
		aH = 300;
		break;
	default:
		return;
	}
	if (mBoard != NULL)
		mBoard->mPaused = true;
	Dialog* aDialog = DoDialog(DIALOG_MODE_INTRO, true, aHeader, aLines, GT(0x00495998), Dialog::BUTTONS_FOOTER);
	aDialog->Resize(aX, aY, aW, aH);
}

// 0x41ae80
void NoahApp::ShowHighScores()
{
	HighScoreDialog* aDialog = new HighScoreDialog(this);
	int aHeight = aDialog->GetPreferredHeight(300);
	aDialog->Resize(0xf5, 0x5a, 300, aHeight);
	AddDialog(DIALOG_HIGH_SCORES, aDialog);
}

// 0x418f70 (registered): the player has nothing to register
void NoahApp::ShowRegisteredDialog()
{
	Dialog* aDialog = DoDialog(DIALOG_REGISTER, false, GT(0x00495964), GT(0x00495970), GT(0x00495998), Dialog::BUTTONS_FOOTER);
	aDialog->Resize(0x78, 0x78, 400, 0xf0);
}

// 0x418c40 case 4 of the options dialog
void NoahApp::ShowCredits()
{
	Dialog* aDialog = DoDialog(8, true, GT(0x00496000), GT(0x00496008), GT(0x00495998), Dialog::BUTTONS_FOOTER);
	aDialog->Resize(0x78, 0x41, 400, 0x15e);
}

// 0x41add0: the name for a high score
void NoahApp::ShowRecordScore(const SexyString& thePlace)
{
	SexyString aText = std::string(GT(0x00494798)) + thePlace + GT(0x00494734);
	RecordScoreDialog* aDialog = new RecordScoreDialog(this, aText);
	int aHeight = aDialog->GetPreferredHeight(0x13c);
	aDialog->Resize(0x96, (mHeight - aHeight) / 2, 0x13c, aHeight);
	AddDialog(DIALOG_RECORD_SCORE, aDialog);
}

// 0x41ac10 / 0x41ac40: the saved game is data/savegame.dat in the original, savegame.dat in the user folder here
bool NoahApp::HasSavedGame()
{
	std::string aData;
	return Platform::ReadHostFile(Platform::VFS::Get().GetUserDir() + "/" + Board::SavePath(), &aData);
}

void NoahApp::DeleteSavedGame()
{
	Platform::RemoveUserFile(Board::SavePath());
}

// 0x41a0b0: every button of the game
void NoahApp::ButtonDepress(int theId)
{
	if ((theId >= 2000) && (theId < 3000))
	{
		switch (theId - 2000)
		{
		case DIALOG_NEW_GAME:
			KillDialog(DIALOG_NEW_GAME);
			ShowDifficultyDialog();
			break;
		case DIALOG_QUIT:
			KillDialog(DIALOG_QUIT);
			if (mBoard == NULL)
				Shutdown();
			else
			{
				mBoard->SaveGame();
				ShowIntro(false);
			}
			break;
		case DIALOG_MODE_INTRO:
			KillDialog(DIALOG_MODE_INTRO);
			if (mBoard != NULL)
				mBoard->mPaused = false;
			break;
		case DIALOG_OPTIONS:
		{
			OptionsDialog* aDialog = (OptionsDialog*) GetDialog(DIALOG_OPTIONS);
			if (aDialog != NULL)
			{
				bool aWindowed = !aDialog->mFullscreenCheckbox->IsChecked();
				if (mIsWindowed != aWindowed)
					SwitchScreenMode(aWindowed);
				mShowHints = aDialog->mHintsCheckbox->IsChecked();
				if (!mShowHints && (mBoard != NULL))
					mBoard->ClearHint();
			}
			if (mBoard != NULL)
				mBoard->mPaused = false;
			KillDialog(DIALOG_OPTIONS);
			break;
		}
		case DIALOG_TUTORIAL:
		{
			TutorialDialog* aDialog = (TutorialDialog*) GetDialog(DIALOG_TUTORIAL);
			if (aDialog != NULL)
				mShowTutorial = !aDialog->mAgainCheckbox->IsChecked();
			if (mBoard != NULL)
				mBoard->mPaused = false;
			KillDialog(DIALOG_TUTORIAL);
			break;
		}
		case DIALOG_RECORD_SCORE:
		{
			RecordScoreDialog* aDialog = (RecordScoreDialog*) GetDialog(DIALOG_RECORD_SCORE);
			if (aDialog != NULL)
			{
				SexyString aName = aDialog->mNameEdit->mString;
				if (mHighScoreWidget != NULL)
					mHighScoreWidget->AddScore(aName);
				mLastScoreName = aName;
			}
			KillDialog(DIALOG_RECORD_SCORE);
			break;
		}
		default:
			KillDialog(theId - 2000);
			break;
		}
	}
	else if ((theId >= 3000) && (theId < 4000))
	{
		if (theId == 3000 + DIALOG_QUIT)
		{
			if (mBoard != NULL)
				mBoard->mPaused = false;
		}
		KillDialog(theId - 3000);
	}

	switch (theId)
	{
	case BUTTON_ACTION:
		ShowNewGameDialog(GAMEMODE_ACTION);
		break;
	case BUTTON_STRATEGY:
		ShowNewGameDialog(GAMEMODE_STRATEGY);
		break;
	case BUTTON_PUZZLE:
		ShowNewGameDialog(GAMEMODE_PUZZLE);
		break;
	case BUTTON_CONTINUE:
		KillDialog(DIALOG_NEW_GAME);
		RemoveIntro();
		StartGame(true);
		break;
	case BUTTON_QUIT:
	case BUTTON_SIDEBAR_QUIT:
	case BUTTON_QUIT_KEY:
		if (mBoard != NULL)
			mBoard->mPaused = true;
		ShowQuitDialog();
		break;
	case BUTTON_SIDEBAR_OPTIONS:
		if (mBoard != NULL)
			mBoard->mPaused = true;
		ShowOptions();
		break;
	}
}
