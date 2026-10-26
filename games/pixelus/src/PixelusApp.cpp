#include "PixelusApp.h"
#include "GameDialog.h"
#include "GameText.h"
#include "LoadingScreen.h"
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
	mSessionLog = NULL;

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

// 0x433b90 (slot 79): the play link of the loading screen was clicked
void PixelusApp::PlayClicked()
{
	mWidgetManager->RemoveWidget(mLoadingScreen);
	SafeDeleteWidget(mLoadingScreen);
	mLoadingScreen = NULL;
}

// 0x4333a0 (slot 84)
void PixelusApp::HandlePaused(bool thePaused, bool theWasPaused, int theCount)
{
}

// 0x4331e0 slot 21: every dialog the game opens is a GameDialog, 336 wide at the top of the screen
Dialog* PixelusApp::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
	const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	GameDialog* aDialog = new GameDialog(this, mImages[IMAGE_DIALOG], mImages[IMAGE_DIALOGBUTTONCOMPONENTS],
		theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode, NULL);
	aDialog->SetHeaderFont(mFonts[FONT_GARAMOND20]);
	aDialog->SetLinesFont(mFonts[FONT_GARAMOND14]);
	aDialog->SetColor(Dialog::COLOR_HEADER, Color(255, 255, 255, 255));
	aDialog->SetColor(Dialog::COLOR_LINES, Color(255, 255, 255, 255));
	aDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(255, 255, 255, 255));
	aDialog->mContentInsets = Insets(0x30, 0x3c, 0x30, 0x20);
	aDialog->Resize(0xce, 0x2a, 0x150, aDialog->GetPreferredHeight(0x150));
	return aDialog;
}
