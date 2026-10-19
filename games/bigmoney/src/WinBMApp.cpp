#include "WinBMApp.h"
#include "Compat12.h"
#include "Board.h"
#include "TitleScreen.h"
#include "GameSelector.h"
#include "HelpWidget.h"
#include "MrMoney.h"
#include "HighScoreWidget.h"
#include "HighScoreDialog.h"
#include "OptionsDialog.h"
#include "RecordScoreDialog.h"
#include "CreditsDialog.h"
#include "testhooks/Autoplay.h"
#include "ModIndicators.h"
#include "HDPixels.h"
#include "platform/ModManager.h"

#include "WidgetManager.h"
#include "ButtonWidget.h"
#include "Checkbox.h"
#include "EditWidget.h"
#include "Dialog.h"
#include "ImageFont.h"
#include "DDImage.h"
#include "MemoryImage.h"
#include "Graphics.h"
#include "Insets.h"
#include "SoundManager.h"
#include "SDLMusicInterface.h"

using namespace Sexy;

// The highlighted-coin colour (Init): every colour channel c -> min(255, c * 280 / 255 + 48), alpha kept
static ulong BrightCoinPixel(ulong thePixel)
{
	ulong aBlue = ((thePixel & 0xff) * 280) / 255 + 48;
	ulong aGreen = (((thePixel >> 8) & 0xff) * 280) / 255 + 48;
	ulong aRed = (((thePixel >> 16) & 0xff) * 280) / 255 + 48;
	if (aBlue > 255)
		aBlue = 255;
	if (aGreen > 255)
		aGreen = 255;
	if (aRed > 255)
		aRed = 255;
	return (thePixel & 0xff000000) | (aRed << 16) | (aGreen << 8) | aBlue;
}

// The grey selector illustrations (LoadingThreadProc): (r + g + b) / 3, alpha kept
static ulong GreyPixel(ulong thePixel)
{
	ulong aGrey = (((thePixel >> 16) & 0xff) + ((thePixel >> 8) & 0xff) + (thePixel & 0xff)) / 3;
	return (aGrey << 16) | (aGrey << 8) | (thePixel & 0xff000000) | aGrey;
}

// 0x41c790
WinBMApp::WinBMApp()
{
	mPendingSeed = 0; // port(mods)

	// port: test hooks
	mAutoplay = false;
	mAutoplaySeed = 0;
	mAutoplayMode = GAMETYPE_ACTION;
	mAutoplaySkill = 1;
	mAutoplayGames = 1;
	mAutoplayCap = 200000;
	mAutoplayBot = NULL;

	mFont1 = NULL;
	mFont1NoShadow = NULL;
	mFont1OutlineNoShadow = NULL;
	mFont1Underlined = NULL;
	mFont1OutlineUnderlined = NULL;
	mFont3 = NULL;
	mFont3Underlined = NULL;
	mFont3Outline = NULL;
	mFont3OutlineUnderlined = NULL;
	mScoreFont = NULL;
	mScoreFontSmall = NULL;
	mLevelFont = NULL;
	mFloatFont = NULL;
	mCheckboxCheckedImage = NULL;
	mCheckboxUncheckedImage = NULL;
	mStartArrowsMirrored = NULL;
	mImages = NULL;
	mGameSelector = NULL;
	mMrMoney = NULL;
	mCobrandImage = NULL;

	// port: the original's SexyAppBase ctor reads this from the exe's version resource (ProductVersion)
	mProductVersion = "1.3";
	mProdName = "BigMoney";
	mTitle = "Big Money Deluxe " + mProductVersion;
	mRegKey = "PopCap\\BigMoney";

	mCustomCursorsEnabled = false;
	mGameType = GAMETYPE_ACTION;
	mDontShowHelp[0] = false;
	mDontShowHelp[1] = false;
	mDontShowHelp[2] = false;
	mBagCollectedOnce = false;
	mBombSeenOnce = false;
	mDangerHintShown = false;
	mHasClickedGroup = false;
	mNoPopLink = false;
	mWelcomePlayed = false;
	mSkillLevel = 1;

	for (int i = 0; i < NUM_GAMETYPES; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			mGameTypeButtonImages[i][j] = NULL;
			mSkillButtonImages[i][j] = NULL;
		}
		for (int j = 0; j < 3; j++)
			mSelectorButtonImages[i][j] = NULL;
	}
	for (int i = 0; i < NUM_MRMONEY_IMAGES; i++)
		mMrMoneyImages[i] = NULL;
	for (int i = 0; i < NUM_BILL_IMAGES; i++)
		mBillImages[i] = NULL;
	for (int i = 0; i < NUM_GAMETYPES; i++)
		mHelpShownThisSession[i] = false;

	// port: the original leaves these to the zero-initialised global app object
	mBoard = NULL;
	mTitleScreen = NULL;
	mHelpWidget = NULL;
	mBrightCoinsImage = NULL;
	mShowInstructor = false;
}

// 0x41c240
WinBMApp::~WinBMApp()
{
	delete mAutoplayBot;  // port: test hook

	// This is where a game in progress is saved on exit: Shutdown is not overridden
	if ((mBoard != NULL) && (mBoard->CanSaveGame()))
		mBoard->SaveGame();

	if (mTitleScreen != NULL)
	{
		mWidgetManager->RemoveWidget(mTitleScreen);
		delete mTitleScreen;
	}

	if (mGameSelector != NULL)
	{
		mWidgetManager->RemoveWidget(mGameSelector);
		delete mGameSelector;
	}

	if (mMrMoney != NULL)
	{
		mWidgetManager->RemoveWidget(mMrMoney);
		delete mMrMoney;
	}

	delete mFont1;
	delete mFont1NoShadow;
	delete mFont1OutlineNoShadow;
	delete mFont1Underlined;
	delete mFont1OutlineUnderlined;
	delete mFont3;
	delete mFont3Outline;
	delete mFont3Underlined;
	delete mFont3OutlineUnderlined;
	delete mScoreFont;
	delete mScoreFontSmall;
	delete mLevelFont;
	delete mFloatFont;
	delete mCheckboxCheckedImage;
	delete mCheckboxUncheckedImage;
	delete mStartArrowsMirrored;
	delete mCobrandImage;

	for (int i = 0; i < NUM_GAMETYPES; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			delete mGameTypeButtonImages[i][j];
			delete mSkillButtonImages[i][j];
		}
		for (int j = 0; j < 3; j++)
			delete mSelectorButtonImages[i][j];
	}
	for (int i = 0; i < NUM_MRMONEY_IMAGES; i++)
		delete mMrMoneyImages[i];
	for (int i = 0; i < NUM_BILL_IMAGES; i++)
		delete mBillImages[i];

	// port: the original frees the list nodes but leaves the side buttons in the WidgetManager, undeleted
	for (std::list<ButtonWidget*>::iterator anItr = mButtonList.begin(); anItr != mButtonList.end(); ++anItr)
	{
		mWidgetManager->RemoveWidget(*anItr);
		delete *anItr;
	}

	for (std::list<Image*>::iterator anItr = mButtonImageList.begin(); anItr != mButtonImageList.end(); ++anItr)
		delete *anItr;

	if (mImages != NULL)
	{
		for (int i = 0; i < NUM_IMAGES; i++)
			delete mImages[i];
		delete [] mImages;
	}

	delete mBrightCoinsImage; // port: the original never deletes it

	// mBoard and mHelpWidget are not deleted (as in the original: the process ends)
}

// 0x41bd90
Dialog* WinBMApp::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader, const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	Dialog* aDialog = new Dialog(mImages[IMAGE_OLDDIALOG], mImages[IMAGE_DIALOGBUTTON], theDialogId, isModal, theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
	aDialog->SetButtonFont(mFont3);
	aDialog->SetHeaderFont(mFont1);
	aDialog->SetLinesFont(mFont3);
	aDialog->SetColor(Dialog::COLOR_HEADER, Color(255, 255, 255));
	aDialog->SetColor(Dialog::COLOR_LINES, Color(0, 255, 0));
	aDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0, 255, 0));
	aDialog->mContentInsets = Insets(36, 36, 36, 36);
	aDialog->Resize(206, 42, 348, aDialog->GetPreferredHeight(348));
	return aDialog;
}

// 0x41bf00: a modal dialog pauses only the timed game type
void WinBMApp::ModalOpen()
{
	if ((mBoard != NULL) && (mGameType == GAMETYPE_ACTION))
		mBoard->Pause(true);
}

// 0x41bf20
void WinBMApp::ModalClose()
{
	if ((mBoard != NULL) && (mGameType == GAMETYPE_ACTION))
		mBoard->Pause(false);
}

// 0x41bf40
void WinBMApp::GotFocus()
{
	SexyApp::GotFocus();
	if (mBoard != NULL)
		mBoard->Pause(false);
}

// 0x41bf60
void WinBMApp::LostFocus()
{
	SexyApp::LostFocus();
	if (mBoard != NULL)
		mBoard->Pause(true);
}

// 0x41bf80
void WinBMApp::URLOpenSucceeded(const std::string& theURL)
{
	SexyApp::URLOpenSucceeded(theURL);
	KillDialog(DIALOG_OPENING_BROWSER);
}

// 0x41bfa0
void WinBMApp::ButtonPress(int theId)
{
	PlaySample(SOUND_BUTTONCLICK);
}

// 0x41b6a0
void WinBMApp::RemoveBoard()
{
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = NULL;
		KillDialog(DIALOG_RECORD_SCORE);
	}
}

// 0x41b6f0
void WinBMApp::ShowHelp()
{
	KillDialog(DIALOG_OPTIONS);

	if (mHelpWidget != NULL)
		return;

	if (mGameSelector != NULL)
		mGameSelector->SlideOut();

	mHelpWidget = new HelpWidget(this);
	mHelpWidget->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mHelpWidget);
	if (mBoard != NULL)
		mWidgetManager->PutInfront(mHelpWidget, mBoard);
	else
		mWidgetManager->BringToBack(mHelpWidget);

	if (mBoard != NULL)
		mBoard->Pause(true);
}

// 0x41b7f0
void WinBMApp::RemoveHelp()
{
	if (mHelpWidget != NULL)
	{
		mWidgetManager->RemoveWidget(mHelpWidget);
		SafeDeleteWidget(mHelpWidget);
		mHelpWidget = NULL;
		if (mBoard != NULL)
			mBoard->Pause(false);
	}
}

// 0x41b840
void WinBMApp::ShowGameSelector()
{
	KillDialog(DIALOG_RECORD_SCORE);

	for (int aSong = 0; aSong < NUM_SONGS; aSong++)
	{
		if (aSong != SONG_TITLE)
			mMusicInterface->FadeOut(aSong, true, 0.01);
	}

	// The old selector (if any) is safe-deleted before it is removed; the pointer is simply replaced
	if (mGameSelector != NULL)
	{
		SafeDeleteWidget(mGameSelector);
		mWidgetManager->RemoveWidget(mGameSelector);
	}

	mMrMoney->mTargetX = 0;
	mMrMoney->mTargetY = 290;

	mGameSelector = new GameSelector(this);
	mGameSelector->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mGameSelector);
	mWidgetManager->BringToBack(mGameSelector);

	// Over a running board: the board freezes behind the selector, which slides in and removes the board
	// (GameSelector::Update -> RemoveBoard) when it is in place
	if (mBoard != NULL)
	{
		mBoard->Freeze();
		mWidgetManager->BringToBack(mBoard);
		mGameSelector->SlideIn();
	}

	if (!mMusicInterface->IsPlaying(SONG_TITLE))
		mMusicInterface->FadeIn(SONG_SELECTOR, ORDER_SELECTOR, 0.005, false);
}

// 0x41b9c0
void WinBMApp::RemoveGameSelector()
{
	if (mGameSelector != NULL)
	{
		mWidgetManager->RemoveWidget(mGameSelector);
		SafeDeleteWidget(mGameSelector);
		mGameSelector = NULL;
	}
}

// 0x41ba00
void WinBMApp::ShowHighScoreDialog()
{
	HighScoreDialog* aDialog = new HighScoreDialog(this);
	aDialog->SetColor(Dialog::COLOR_HEADER, Color(255, 255, 255));
	aDialog->SetColor(Dialog::COLOR_LINES, Color(0, 255, 0));
	aDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0, 255, 0));
	aDialog->Resize(196, 16, 388, aDialog->GetPreferredHeight(388));
	AddDialog(DIALOG_HIGH_SCORES, aDialog);
}

// 0x41baf0
void WinBMApp::ShowCreditsDialog()
{
	CreditsDialog* aDialog = new CreditsDialog(this);
	aDialog->SetColor(Dialog::COLOR_HEADER, Color(255, 255, 255));
	aDialog->SetColor(Dialog::COLOR_LINES, Color(0, 255, 0));
	aDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0, 255, 0));
	aDialog->Resize(196, 16, 388, aDialog->GetPreferredHeight(388));
	AddDialog(DIALOG_CREDITS, aDialog);
}

// 0x41bbe0
void WinBMApp::ShowOptionsDialog()
{
	OptionsDialog* aDialog = new OptionsDialog(this);
	aDialog->SetColor(Dialog::COLOR_HEADER, Color(255, 255, 255));
	aDialog->SetColor(Dialog::COLOR_LINES, Color(0, 255, 0));
	aDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0, 255, 0));
	aDialog->mContentInsets = Insets(36, 36, 36, 36);
	aDialog->Resize(186, 26, 388, aDialog->GetPreferredHeight(388));
	AddDialog(DIALOG_OPTIONS, aDialog);
}

// 0x41bd00 (HighScoreWidget::DoCheckForHighScore)
void WinBMApp::ShowRecordScoreDialog(const SexyString& theLines)
{
	RecordScoreDialog* aDialog = new RecordScoreDialog(this, theLines);
	aDialog->Resize(250, 32, 316, aDialog->GetPreferredHeight(316));
	AddDialog(DIALOG_RECORD_SCORE, aDialog);
}

// 0x41bfb0
void WinBMApp::NewGame()
{
	KillDialog(DIALOG_RECORD_SCORE);
	RemoveHelp();

	for (int aSong = 0; aSong < NUM_SONGS; aSong++)
		mMusicInterface->FadeOut(aSong, true, 0.005);

	if (mGameType == GAMETYPE_ACTION)
		mMusicInterface->FadeIn(SONG_ACTION, ORDER_ACTION, 0.005, false);
	else
		mMusicInterface->FadeOut(SONG_ACTION, true, 0.005);

	if (mGameType == GAMETYPE_STRATEGY)
		mMusicInterface->FadeIn(SONG_STRATEGY, ORDER_STRATEGY, 0.005, false);
	else
		mMusicInterface->FadeOut(SONG_STRATEGY, true, 0.005);

	if (mGameType == GAMETYPE_PUZZLE)
		mMusicInterface->FadeIn(SONG_PUZZLE, ORDER_PUZZLE, 0.005, false);
	else
		mMusicInterface->FadeOut(SONG_PUZZLE, true, 0.005);

	if (mGameSelector != NULL)
		mGameSelector->SlideOut();

	RemoveBoard();

	mBoard = new Board(this);
	mBoard->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mBoard);
	mWidgetManager->BringToBack(mBoard);
	mWidgetManager->SetFocus(mBoard);
}

// 0x41c170 (GameSelector::StartGame, after it stored mGameType and mSkillLevel): the help screen comes
// first the first time a type is played in a run, unless "don't show this screen again" was chosen
void WinBMApp::StartGame()
{
	if ((!mDontShowHelp[mGameType]) && (!mHelpShownThisSession[mGameType]))
	{
		mShowInstructor = true;
		ShowHelp();
		mHelpShownThisSession[mGameType] = true;
		return;
	}

	mShowInstructor = false;
	NewGame();
}

// 0x41c1c0
bool WinBMApp::KillDialog(int theDialogId)
{
	if (SexyApp::KillDialog(theDialogId))
	{
		if (mDialogMap.size() == 0)
			mWidgetManager->SetFocus(mBoard);
		return true;
	}

	return false;
}

// 0x41c9f0
void WinBMApp::WriteToRegistry()
{
	SexyApp::WriteToRegistry();

	// port: the original opens HKEY_LOCAL_MACHINE\SOFTWARE\<mRegKey> itself (RegOpenKeyExA, never closed)
	// and writes only if that key exists; the port uses the framework's registry, same value names
	if (mLastScoreName.length() != 0)
		RegistryWriteString("LastScoreName", mLastScoreName);
	RegistryWriteInteger("SkillLevel", mSkillLevel);
	RegistryWriteInteger("GameType", mGameType);
	RegistryWriteInteger("DontShowHelp0", mDontShowHelp[0] ? 1 : 0);
	RegistryWriteInteger("DontShowHelp1", mDontShowHelp[1] ? 1 : 0);
	RegistryWriteInteger("DontShowHelp2", mDontShowHelp[2] ? 1 : 0);
}

// 0x41cb60 (runs inside SexyAppBase::Init): each value keeps its current value when it is missing
void WinBMApp::ReadFromRegistry()
{
	SexyApp::ReadFromRegistry();

	// port: the original reads HKEY_LOCAL_MACHINE\SOFTWARE\<mRegKey> with RegQueryValueExA
	RegistryReadString("LastScoreName", &mLastScoreName);

	int aValue;
	if (RegistryReadInteger("SkillLevel", &aValue))
		mSkillLevel = aValue;
	if (RegistryReadInteger("GameType", &aValue))
		mGameType = aValue;
	if (RegistryReadInteger("NoPopLink", &aValue))
		mNoPopLink = aValue != 0;
	if (RegistryReadInteger("DontShowHelp0", &aValue))
		mDontShowHelp[0] = aValue != 0;
	if (RegistryReadInteger("DontShowHelp1", &aValue))
		mDontShowHelp[1] = aValue != 0;
	if (RegistryReadInteger("DontShowHelp2", &aValue))
		mDontShowHelp[2] = aValue != 0;
}

// 0x41cd60 (TitleScreen, OptionsDialog's Register button, and the YES of dialogs 4 and 15)
void WinBMApp::ShowRegisterDialog()
{
	// The unregistered branch (new RegisterDialog as dialog 2, Resize((mWidth - 381) / 2, 32, 381, 385)) is
	// not ported: the Steam build is registered (partner.xml NoReg=true)
	DoDialog(DIALOG_INFO, true, GetString("ALREADY_REGISTERED"), GetString("YOU_HAVE_ALREADY_REGISTERED_BIG_MONEY"), GetString("DIALOG_BUTTON_OK"), Dialog::BUTTONS_FOOTER);
}

// 0x41d090 ShowInvalidCodeDialog (dialog 8) is not ported: registration only
// 0x41d2e0 ShowPuzzleTrialDialog and 0x41d4c0 ShowPuzzleNextLevelLocked (dialog 15) are not ported:
//          Board calls them only when unregistered
// 0x41d6a0 ShowRegisterToRecordScoreDialog (dialog 15) is not ported: HighScoreWidget calls it only when
//          unregistered

// 0x41d7d0
bool WinBMApp::LoadAppImage(int theImageId)
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

// 0x41d9a0
void WinBMApp::Init()
{
	SexyApp::Init();

	// port: the MSVC rand() emulation (Compat12.h) gets the seed 1.2's SexyAppBase::Init gave srand()
	CrtSRand(mHasSeedParam ? (unsigned int) mRandSeed : (unsigned int) GetTickCount());

	LoadProperties();

	DIALOG_YES_STRING = GetString("DIALOG_BUTTON_YES");
	DIALOG_NO_STRING = GetString("DIALOG_BUTTON_NO");
	DIALOG_OK_STRING = GetString("DIALOG_BUTTON_OK");
	DIALOG_CANCEL_STRING = GetString("DIALOG_BUTTON_CANCEL");

	// The RealArcade branch (GetBoolean("RealArcade"): mIsRegistered from a registry value named in
	// BigMoney.ini) is not ported: registration only

	mImages = new Image*[NUM_IMAGES];
	for (int i = 0; i < NUM_IMAGES; i++)
		mImages[i] = NULL;

	// port: the original also calls Palletize() on every image and logs "%s - PALLETIZED!" /
	// "%s - 32 Bit Only" (a memory saving with no visible effect)
	for (int i = 0; i < NUM_IMAGES; i++)
	{
		if (!LoadAppImage(i))
		{
			Shutdown();
			return;
		}
	}

	// Highlighted coins: every colour channel c -> min(255, c * 280 / 255 + 48), alpha unchanged
	MemoryImage* aCoinsImage = (MemoryImage*) mImages[IMAGE_COINS];
	mBrightCoinsImage = new MemoryImage(this);
	mBrightCoinsImage->Create(aCoinsImage->GetWidth(), aCoinsImage->GetHeight());
	ulong* aSrcBits = aCoinsImage->GetBits();
	ulong* aDestBits = mBrightCoinsImage->GetBits();
	int aNumPixels = aCoinsImage->GetWidth() * aCoinsImage->GetHeight();
	for (int i = 0; i < aNumPixels; i++)
		aDestBits[i] = BrightCoinPixel(aSrcBits[i]);
	// port: the same for a mod's HD coins (HDPixels.h)
	HDPixels aCoinsHD = GetHDPixels(aCoinsImage);
	for (size_t i = 0; i < aCoinsHD.mBits.size(); i++)
		aCoinsHD.mBits[i] = (uint32_t) BrightCoinPixel(aCoinsHD.mBits[i]);
	mBrightCoinsImage->BitsChanged();
	SetHDPixels(mBrightCoinsImage, aCoinsHD);

	mFont1 = new ImageFont(this, FONT_FILE_1);

	mFont1NoShadow = new ImageFont(*mFont1);
	mFont1NoShadow->AddTag("noshadow");

	mFont1OutlineNoShadow = new ImageFont(*mFont1);
	mFont1OutlineNoShadow->AddTag("outline");
	mFont1OutlineNoShadow->AddTag("noshadow");

	mFont1Underlined = new ImageFont(*mFont1);
	mFont1Underlined->AddTag("underlined");
	mFont1Underlined->Prepare();

	mFont1OutlineUnderlined = new ImageFont(*mFont1);
	mFont1OutlineUnderlined->AddTag("outline");
	mFont1OutlineUnderlined->AddTag("underlined");
	mFont1OutlineUnderlined->Prepare();

	mFont3 = new ImageFont(this, FONT_FILE_3);

	mFont3Outline = new ImageFont(*mFont3);
	mFont3Outline->AddTag("outline");
	mFont3Outline->Prepare();

	mFont3Underlined = new ImageFont(*mFont3);
	mFont3Underlined->AddTag("underlined");
	mFont3Underlined->Prepare();

	mFont3OutlineUnderlined = new ImageFont(*mFont3);
	mFont3OutlineUnderlined->AddTag("outline");
	mFont3OutlineUnderlined->AddTag("underlined");
	mFont3OutlineUnderlined->Prepare();

	mScoreFont = new ImageFont(this, FONT_FILE_SCORE);
	if (!Platform::ModManager::Get().GetEnabledMods().empty())
		AddModsDoubleStrokeToDollar(mScoreFont);  // port: a sign that mods are in use (ModIndicators.h)
	mScoreFontSmall = new ImageFont(*mScoreFont);
	mScoreFontSmall->SetPointSize(20);

	mLevelFont = new ImageFont(mImages[IMAGE_LEVELFONT], FONT_FILE_LEVEL);
	mFloatFont = new ImageFont(this, FONT_FILE_FLOAT);

	mTitleScreen = new TitleScreen(this);
	mTitleScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mTitleScreen);

	// The same module is every song; each song id starts it at its own order
	for (int aSong = 0; aSong < NUM_SONGS; aSong++)
		mMusicInterface->LoadMusic(aSong, MUSIC_FILE);
	mMusicInterface->PlayMusic(SONG_TITLE, ORDER_TITLE, false);

	SetCursorImage(CURSOR_POINTER, mImages[IMAGE_CURSOR_POINTER]);
	SetCursorImage(CURSOR_HAND, mImages[IMAGE_CURSOR_HAND]);
	SetCursorImage(CURSOR_DRAGGING, mImages[IMAGE_CURSOR_DRAGGING]);
	SetCursorImage(CURSOR_TEXT, mImages[IMAGE_CURSOR_TEXT]);

	if (GetBoolean("Cobranded", false))
		mCobrandImage = GetImage("images\\cobrand", true);
}

// port: the original uses the framework's default (BASS); the port's music interface plays the module
MusicInterface* WinBMApp::CreateMusicInterface()
{
	return new SDLMusicInterface();
}

// 0x41efd0 (loading thread; on the web it runs synchronously)
void WinBMApp::LoadingThreadProc()
{
	mNumLoadingThreadTasks = NUM_LOADING_TASKS;

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

	// A grey copy of the illustrations: (r + g + b) / 3, alpha kept
	MemoryImage* anIllustrations = dynamic_cast<MemoryImage*>(mImages[IMAGE_ILLUSTRATIONS]);
	MemoryImage* aGreyImage = new MemoryImage(this);
	aGreyImage->Create(anIllustrations->GetWidth(), anIllustrations->GetHeight());
	ulong* aSrcBits = anIllustrations->GetBits();
	ulong* aDestBits = aGreyImage->GetBits();
	int aNumPixels = anIllustrations->GetWidth() * anIllustrations->GetHeight();
	for (int i = 0; i < aNumPixels; i++)
		aDestBits[i] = GreyPixel(aSrcBits[i]);
	// port: the same for a mod's HD illustrations (HDPixels.h)
	HDPixels anIllustrationsHD = GetHDPixels(anIllustrations);
	for (size_t i = 0; i < anIllustrationsHD.mBits.size(); i++)
		anIllustrationsHD.mBits[i] = (uint32_t) GreyPixel(anIllustrationsHD.mBits[i]);
	aGreyImage->BitsChanged();
	SetHDPixels(aGreyImage, anIllustrationsHD);

	// Game-type tiles, 154 px columns of the selector background: normal (darkened, grey text and
	// picture), over (yellow text) and down (green text, coloured picture)
	for (int aType = 0; aType < NUM_GAMETYPES; aType++)
	{
		int x = aType * 154;

		DDImage* aNormal = CopyImage(mImages[IMAGE_SELECTBACK], Rect(x + 5, 58, 154, 266));
		mGameTypeButtonImages[aType][0] = aNormal;
		{
			Graphics g(aNormal);
			g.SetColor(Color(0, 0, 0, 128));
			g.FillRect(0, 0, 154, 266);
			g.SetColorizeImages(true);
			g.SetColor(Color(120, 120, 120));
			g.DrawImage(mImages[IMAGE_GAMETYPESTEXT], 0, 10, Rect(x, 0, 154, 248));
			g.SetColor(Color(120, 120, 120));
			g.DrawImage(aGreyImage, 10, 42, Rect(x, 0, 144, 128));
		}

		DDImage* anOver = CopyImage(mImages[IMAGE_SELECTBACK], Rect(x + 5, 58, 154, 266));
		mGameTypeButtonImages[aType][1] = anOver;
		{
			Graphics g(anOver);
			g.SetColor(Color(0, 0, 0, 64));
			g.FillRect(0, 0, 154, 266);
			g.SetColorizeImages(true);
			g.SetColor(Color(220, 220, 0));
			g.DrawImage(mImages[IMAGE_GAMETYPESTEXT], 0, 10, Rect(x, 0, 154, 248));
			g.SetColor(Color(180, 180, 180));
			g.DrawImage(aGreyImage, 10, 42, Rect(x, 0, 144, 128));
		}

		DDImage* aDown = CopyImage(mImages[IMAGE_SELECTBACK], Rect(x + 5, 58, 154, 266));
		mGameTypeButtonImages[aType][2] = aDown;
		{
			Graphics g(aDown);
			g.SetColorizeImages(true);
			g.SetColor(Color(120, 255, 0));
			g.DrawImage(mImages[IMAGE_GAMETYPESTEXT], 0, 10, Rect(x, 0, 154, 248));
			g.SetColor(Color(255, 255, 255));
			g.DrawImage(mImages[IMAGE_ILLUSTRATIONS], 10, 42, Rect(x, 0, 144, 128));
		}

		mCompletedLoadingThreadTasks++;
	}

	delete aGreyImage;

	// Skill tiles: normal (grey text), over (yellow), down (the pressed background, green text 1 px down-right)
	static const int aSkillX[3] = { 29, 186, 339 };
	for (int aSkill = 0; aSkill < 3; aSkill++)
	{
		int x = aSkillX[aSkill];

		DDImage* aNormal = CopyImage(mImages[IMAGE_SELECTBACK], Rect(x, 359, 113, 52));
		mSkillButtonImages[aSkill][0] = aNormal;
		{
			Graphics g(aNormal);
			g.SetColorizeImages(true);
			g.SetColor(Color(120, 120, 120));
			g.DrawImage(mImages[IMAGE_SKILLSTEXT], 0, 0, Rect(x, 0, 113, 52));
		}

		DDImage* anOver = CopyImage(mImages[IMAGE_SELECTBACK], Rect(x, 359, 113, 52));
		mSkillButtonImages[aSkill][1] = anOver;
		{
			Graphics g(anOver);
			g.SetColorizeImages(true);
			g.SetColor(Color(220, 220, 0));
			g.DrawImage(mImages[IMAGE_SKILLSTEXT], 0, 0, Rect(x, 0, 113, 52));
		}

		DDImage* aDown = CopyImage(mImages[IMAGE_SKILLSPRESSED], Rect(x, 0, 113, 52));
		mSkillButtonImages[aSkill][2] = aDown;
		{
			Graphics g(aDown);
			g.SetColorizeImages(true);
			g.SetColor(Color(120, 255, 0));
			g.DrawImage(mImages[IMAGE_SKILLSTEXT], 1, 1, Rect(x, 0, 113, 52));
		}

		mCompletedLoadingThreadTasks++;
	}

	// The start button's background (GameSelector button 1); the other selector button images stay NULL
	mSelectorButtonImages[1][0] = CopyImage(mImages[IMAGE_SELECTBACK], Rect(7, 429, 455, 38));

	mCheckboxUncheckedImage = CopyImage(mImages[IMAGE_CHECKBOXES], Rect(0, 0, 22, 23));
	mCheckboxCheckedImage = CopyImage(mImages[IMAGE_CHECKBOXES], Rect(22, 0, 22, 23));

	mStartArrowsMirrored = CopyImage(mImages[IMAGE_STARTARROWS]);
	MirrorImage(mStartArrowsMirrored);

	// Mr. Money with each mouth frame; frame 5 also gets mouth 0 drawn over mouth 5
	for (int aFrame = 0; aFrame < NUM_MRMONEY_IMAGES; aFrame++)
	{
		int aWidth = mImages[IMAGE_MRMONEY]->GetWidth();
		int aHeight = mImages[IMAGE_MRMONEY]->GetHeight();

		MemoryImage* anImage = new MemoryImage(this);
		anImage->Create(aWidth, aHeight);
		anImage->SetImageMode(true, true);

		Graphics g(anImage);
		g.DrawImage(mImages[IMAGE_MRMONEY], 0, 0);
		g.DrawImage(mImages[IMAGE_MOUTHS], 0, 0, Rect(0, aHeight * aFrame, aWidth, aHeight));
		if (aFrame == 5)
			g.DrawImage(mImages[IMAGE_MOUTHS], 0, 0, Rect(0, 0, aWidth, aHeight));

		mMrMoneyImages[aFrame] = anImage;
		mCompletedLoadingThreadTasks++;
	}

	// Smaller bills: 1728x45, 1536x40, 1344x35, 1152x30 (int x double, truncated)
	static const double aBillScale[NUM_BILL_IMAGES] = { 0.9, 0.8, 0.7, 0.6 };
	for (int i = 0; i < NUM_BILL_IMAGES; i++)
	{
		int aWidth = mImages[IMAGE_BILL]->GetWidth();
		int aHeight = mImages[IMAGE_BILL]->GetHeight();
		int aScaledWidth = (int) (aWidth * aBillScale[i]);
		int aScaledHeight = (int) (aHeight * aBillScale[i]);

		MemoryImage* anImage = new MemoryImage(this);
		anImage->Create(aScaledWidth, aScaledHeight);
		anImage->SetImageMode(true, true);

		Graphics g(anImage);
		g.DrawImage(mImages[IMAGE_BILL], Rect(0, 0, aScaledWidth, aScaledHeight), Rect(0, 0, aWidth, aHeight));

		mBillImages[i] = anImage;
		mCompletedLoadingThreadTasks++;
	}

	// 48 + 3 + 3 + 6 + 4 = 64 of the 72 tasks: the progress stays at 0.89 until mLoaded
}

// 0x41fe80
void WinBMApp::URLOpenFailed(const std::string& theURL)
{
	SexyApp::URLOpenFailed(theURL);
	KillDialog(DIALOG_OPENING_BROWSER);

	// PLEASE_OPEN_THE_FOLLOWING_URL_IN_YOUR_BROWSER ends with a line break
	SexyString aLines = GetString("PLEASE_OPEN_THE_FOLLOWING_URL_IN_YOUR_BROWSER") + theURL;
	Dialog* aDialog = NewDialog(DIALOG_URL_FAILED, true, GetString("OPEN_BROWSER"), aLines, GetString("OK"), Dialog::BUTTONS_FOOTER);
	aDialog->Resize(100, 50, 440, aDialog->GetPreferredHeight(440));
	AddDialog(DIALOG_URL_FAILED, aDialog);
}

// 0x420140
// port: the original keeps dialog 10 up until the browser takes the focus, and after 12 s with the window
// still active replaces it with dialog 11 (URLOpenFailed). SDL cannot see the browser take the focus, so
// SexyAppBase::OpenURL reports success at once and dialog 10 is killed before it is drawn.
bool WinBMApp::OpenURL(const std::string& theURL, bool shutdownOnOpen)
{
	Dialog* aDialog = NewDialog(DIALOG_OPENING_BROWSER, true, "", GetString("OPENING_BROWSER"), "", Dialog::BUTTONS_NONE);
	aDialog->mContentInsets = Insets(36, 36, 36, 36);
	aDialog->Resize(164, 140, 312, 98);
	AddDialog(DIALOG_OPENING_BROWSER, aDialog);

	return SexyApp::OpenURL(theURL, shutdownOnOpen);
}

// 0x420370
void WinBMApp::ConfirmContinueGame()
{
	if (SaveGameExists())
		DoDialog(DIALOG_CONTINUE_GAME, true, GetString("CONTINUE"), GetString("DO_YOU_WANT_TO_CONTINUE_YOUR_LAST_GAME"), "", Dialog::BUTTONS_YES_NO);
}

// 0x420560
void WinBMApp::ConfirmQuit()
{
	SexyString aLines = GetString("ARE_YOU_SURE_YOU_WANT_TO_QUIT_THE_GAME");
	if ((mBoard != NULL) && (mBoard->CanSaveGame()))
		aLines += GetString("YOUR_CURRENT_GAME_IN_PROGRESS_WILL_BE_SAVED_SO_YOU_CAN_CONTINUE_NEXT_TIME");

	DoDialog(DIALOG_QUIT, true, GetString("QUIT"), aLines, "", Dialog::BUTTONS_YES_NO);
}

// 0x4207e0 (side button "New Game")
void WinBMApp::ConfirmNewGame()
{
	if (mBoard == NULL)
	{
		// At the selector the button starts the selected game
		if ((mGameSelector != NULL) && (!mGameSelector->mSlidingOut))
		{
			mGameSelector->StartGame();
			return;
		}
	}
	else if (mBoard->mState != STATE_GAME_OVER_SCORES)
	{
		DoDialog(DIALOG_NEW_GAME, true, GetString("NEW_GAME"), GetString("ARE_YOU_SURE_YOU_WANT_TO_END_THE_CURRENT_GAME"), "", Dialog::BUTTONS_YES_NO);
		return;
	}

	ShowGameSelector();
}

// 0x420a30 ConfirmCheckForUpdates (dialog 6) and 0x420c50 DoUpdateCheck (UpdateCheckDialog 7, HTTP version
// check, results 10007/20007) are not ported: the port never checks for updates

// 0x420f10
void WinBMApp::ButtonDepress(int theId)
{
	if ((theId % 10000 >= 2000) && (theId % 10000 < 3000))
	{
		// YES / OK / footer of dialog theId - 2000. The update check results 10007 (OpenUpdateURL) and
		// 20007, dialog 2 (RegisterDialog: Validate and register) and dialogs 4, 6 and 15 (which opened
		// the register / update dialogs) are not ported and fall to the default.
		int aDialogId = theId - 2000;
		switch (aDialogId)
		{
		case DIALOG_NEW_GAME:
			KillDialog(DIALOG_NEW_GAME);
			ShowGameSelector();
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

		case DIALOG_RECORD_SCORE:
			{
				RecordScoreDialog* aRecordDialog = (RecordScoreDialog*) GetDialog(DIALOG_RECORD_SCORE);
				std::string aName = aRecordDialog->mNameWidget->mString;
				if (aName.length() == 0)
				{
					// The record dialog stays open
					DoDialog(DIALOG_INFO, true, GetString("ENTER_NAME"), GetString("IF_YOU_WANT_TO_RECORD_YOUR_SCORE_ENTER_YOUR_NAME_IN_THE_HIGH_SCORE_BOX"), GetString("OK"), Dialog::BUTTONS_FOOTER);
				}
				else
				{
					mBoard->mHighScoreWidget->AddHighScore(aName); // no NULL checks, as the original
					mLastScoreName = aRecordDialog->mNameWidget->mString;
					KillDialog(DIALOG_RECORD_SCORE);
				}
			}
			break;

		case DIALOG_CONTINUE_GAME:
			KillDialog(DIALOG_CONTINUE_GAME);
			NewGame();
			mBoard->LoadGame();
			DeleteSaveGame();
			break;

		case DIALOG_QUIT:
			KillDialog(DIALOG_QUIT);
			Shutdown();
			break;

		default:
			KillDialog(aDialogId);
			break;
		}
	}
	else if ((theId % 10000 >= 3000) && (theId % 10000 < 4000))
	{
		// NO / CANCEL of dialog theId - 3000 (the update dialog 10007's NO is not ported)
		int aDialogId = theId - 3000;
		if (aDialogId == DIALOG_CONTINUE_GAME)
		{
			KillDialog(DIALOG_CONTINUE_GAME);
			DeleteSaveGame();
		}
		else
			KillDialog(aDialogId);
	}

	switch (theId)
	{
	case BUTTON_NEW_GAME:
		ConfirmNewGame();
		break;

	case BUTTON_OPTIONS:
		ShowOptionsDialog();
		break;

	case BUTTON_QUIT:
		ConfirmQuit();
		break;
	}
}

// 0x421450: the original offers an update check when ShouldCheckForUpdate() (ConfirmCheckForUpdates); not
// ported, the port never checks for updates
void WinBMApp::LoadingThreadCompleted()
{
}

// 0x421470: no mButtonImage, the left bar art shows the buttons; the widget only adds the hover and
// pressed images (which the app owns, see mButtonImageList)
ButtonWidget* WinBMApp::MakeButton(int theId, const Rect& theRect, Image* theOverImage, Image* theDownImage)
{
	mButtonImageList.push_back(theOverImage);
	mButtonImageList.push_back(theDownImage);

	ButtonWidget* aButton = new ButtonWidget(theId, this);
	aButton->Resize(theRect);
	aButton->mDoFinger = true;
	aButton->mOverImage = theOverImage;
	aButton->mDownImage = theDownImage;
	aButton->mMouseInsets = Insets(1, 1, 3, 3);
	mWidgetManager->AddWidget(aButton);
	mButtonList.push_back(aButton);
	return aButton;
}

// 0x4215c0 (TitleScreen's play link). The title song keeps playing.
void WinBMApp::TitleScreenIsFinished()
{
	mWidgetManager->RemoveWidget(mTitleScreen);
	SafeDeleteWidget(mTitleScreen);
	mTitleScreen = NULL;

	// Rows of IMAGE_BUTTONSTATES: hover images at y 0, 29, 58, pressed images at 87, 116, 145. Each
	// button's pressed image is copied before its hover image.
	Image* aDownImage = CopyImage(mImages[IMAGE_BUTTONSTATES], Rect(0, 87, 108, 29));
	Image* anOverImage = CopyImage(mImages[IMAGE_BUTTONSTATES], Rect(0, 0, 108, 29));
	MakeButton(BUTTON_NEW_GAME, Rect(30, 126, 108, 29), anOverImage, aDownImage);

	aDownImage = CopyImage(mImages[IMAGE_BUTTONSTATES], Rect(0, 116, 108, 29));
	anOverImage = CopyImage(mImages[IMAGE_BUTTONSTATES], Rect(0, 29, 108, 29));
	MakeButton(BUTTON_OPTIONS, Rect(31, 189, 108, 29), anOverImage, aDownImage);

	aDownImage = CopyImage(mImages[IMAGE_BUTTONSTATES], Rect(0, 145, 108, 29));
	anOverImage = CopyImage(mImages[IMAGE_BUTTONSTATES], Rect(0, 58, 108, 29));
	MakeButton(BUTTON_QUIT, Rect(30, 444, 108, 29), anOverImage, aDownImage);

	mMrMoney = new MrMoney(this);
	mMrMoney->Resize(0, 0, 184, 136);
	mMrMoney->Move(-200, 0);
	mWidgetManager->AddWidget(mMrMoney);

	ShowGameSelector();
	ConfirmContinueGame();
}

// port (web): the save the original's ~WinBMApp would write now (see SexyAppBase::UpdateUserDataBackup)
bool WinBMApp::GetShutdownSave(std::string* theFileName, Buffer* theBuffer)
{
	if ((mBoard == NULL) || (!mBoard->CanSaveGame()))
		return false;

	*theFileName = Board::GetSaveGameFileName();
	mBoard->SaveGameSnapshot(theBuffer);
	return true;
}

// port (web): Shutdown's WriteToRegistry is the last registry write (the save in ~WinBMApp changes no
// registry value), so the registry a shutdown leaves is WriteToRegistry's
void WinBMApp::WriteShutdownRegistry()
{
	WriteToRegistry();
}

// port (web): the original saves a game in progress only in ~WinBMApp, and the web build never deletes
// the app; without this the save of a quit game would exist only in the localStorage backup
// (GetShutdownSave).
void WinBMApp::ShutdownHook()
{
#ifdef __EMSCRIPTEN__
	if ((mBoard != NULL) && (mBoard->CanSaveGame()))
		mBoard->SaveGame();
#endif
}

// port: test hooks (see WinBMApp.h). The original does not override UpdateFrames.
void WinBMApp::UpdateFrames()
{
	SexyApp::UpdateFrames();

	if ((mAutoplay) && (mAutoplayBot == NULL))
		mAutoplayBot = new AutoplayBot(this, mAutoplaySeed, mAutoplayMode, mAutoplaySkill, mAutoplayGames, mAutoplayCap,
			mAutoplayLogFileName, mAutoplayShotPrefix);
	if (mAutoplayBot != NULL)
		mAutoplayBot->Update();
}

// port: the game's command-line test hooks (see WinBMApp.h); everything else goes to the framework
void WinBMApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-gameseed")
		return; // port(mods): main.cpp checked it and set mPendingSeed
	if (theParamName == "-autoplay")
	{
		mAutoplay = true;
		mAutoplaySeed = strtoul(theParamValue.c_str(), NULL, 10);
		// the bot's input only, as in a -script run (from the start: focus changes would pause the game)
		mScriptedInput = true;
	}
	else if (theParamName == "-autoplaymode")
		mAutoplayMode = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplayskill")
		mAutoplaySkill = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaygames")
		mAutoplayGames = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaycap")
		mAutoplayCap = atoi(theParamValue.c_str());
	else if (theParamName == "-autoplaylog")
		mAutoplayLogFileName = theParamValue;
	else if (theParamName == "-autoplayshot")
		mAutoplayShotPrefix = theParamValue;
	else
		SexyApp::HandleCmdLineParam(theParamName, theParamValue);
}
