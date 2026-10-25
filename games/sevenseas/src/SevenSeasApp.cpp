// The application class of Seven Seas Deluxe 1.1 (WinSS.exe 0x41bc00-0x421a10). See SevenSeasApp.h and
// docs/GAME_NOTES.md section 2.

#include "SevenSeasApp.h"
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
#include <cstdlib>
#include <cstring>
#include <sstream>

using namespace Sexy;

SevenSeasApp* Sexy::gApp = NULL;

// the fonts' descriptor files (data\font1.txt ...), in FontId order
static const char* const gFontFiles[NUM_FONTS] =
{
	"data\\font1.txt", "data\\ItalianOldStyleMT11Bold.txt", "data\\ItalianOldStyleMT22Bold.txt", "data\\yesno.txt",
	"data\\scorefont.txt", "", "data\\mediumfont.txt", "data\\tutfont.txt"
};

// 0x41bc00
SevenSeasApp::SevenSeasApp()
{
	gApp = this;
	mProdName = "SevenSeas";
	mProductVersion = "1.1";
	mTitle = "Seven Seas Deluxe " + mProductVersion;
	mRegKey = "PopCap\\SevenSeas";
	mWidth = GAME_WIDTH;
	mHeight = GAME_HEIGHT;

	mImages = NULL;
	for (int i = 0; i < NUM_FONTS; i++)
		mFonts[i] = NULL;
	mSidePanel = NULL;
	mTitleScreen = NULL;
	mIntroWidget = NULL;
	mHighScoreWidget = NULL;
	mTrialWidget = NULL;
	mBoard = NULL;
	mMapWidget = NULL;
	mHelpWidget = NULL;
	mUndoButton = NULL;
	mOptionsButton = NULL;
	mNewGameButton = NULL;
	mQuitButton = NULL;
	mBigPanel = NULL;
	for (int i = 0; i < 9; i++)
		mArrows[i] = NULL;
	mMusicSlot = 0;
	mTutorial = true;
	mDifficulty = 1;
	mGamesPlayed = 0;
	mFlag4c4 = false;
	mTalking = false;
	mTalkWidget = NULL;
	mBonusNag = true;
	mWarnBlue = true;
	mWarnGhost = true;
	mWarnSerpent = true;
	mWarnWhirl = true;
	mIsRegistered = true;
	mMenuDifficultySaved = false;
	mAutoplay = false;
	mAutoplayGames = 1;
	mAutoplayCap = 200000;
	mAutoplayState = NULL;
}

// 0x41bfc0: the widgets the app holds come off the widget manager before they go
SevenSeasApp::~SevenSeasApp()
{
	Widget* aWidgets[] = {mBoard, mTitleScreen, mIntroWidget, mHighScoreWidget, mMapWidget, mTrialWidget, mTalkWidget, mSidePanel};
	for (size_t i = 0; i < sizeof(aWidgets) / sizeof(aWidgets[0]); i++)
	{
		if (aWidgets[i] != NULL)
		{
			mWidgetManager->RemoveWidget(aWidgets[i]);
			delete aWidgets[i];
		}
	}
	mBoard = NULL;
	for (std::list<ButtonWidget*>::iterator anItr = mButtons.begin(); anItr != mButtons.end(); ++anItr)
	{
		mWidgetManager->RemoveWidget(*anItr);
		delete *anItr;
	}
	mButtons.clear();
	delete mBigPanel;
	for (int i = 0; i < 9; i++)
		delete mArrows[i];
	for (std::map<std::string, AnimDef*>::iterator anItr = mAnimDefs.begin(); anItr != mAnimDefs.end(); ++anItr)
		delete anItr->second;
	if (mImages != NULL)
		delete[] mImages;
}

// 0x41d880: images\<name>, also entered in the name map
bool SevenSeasApp::LoadImage(int theImageId)
{
	std::string aPath = std::string("images\\") + gImageNames[theImageId];
	mImages[theImageId] = GetImage(aPath);
	if (mImages[theImageId] == NULL)
	{
		Popup("Failed to load " + aPath);
		mLoadingFailed = true;
		return false;
	}
	mImageMap[gImageNames[theImageId]] = mImages[theImageId];
	return true;
}

// 0x41c960: data\anims.txt
void SevenSeasApp::LoadAnims()
{
	std::vector<uint8_t> aBytes;
	if (!Platform::VFS::Get().ReadAll("data/anims.txt", aBytes))
	{
		Popup("Failed open data\\anims.txt");
		mLoadingFailed = true;
		return;
	}
	std::istringstream aStream(std::string(aBytes.begin(), aBytes.end()));
	std::string aLine;
	AnimDef* aDef = NULL;
	bool aInEntry = false;
	while (std::getline(aStream, aLine))
	{
		while (!aLine.empty() && ((aLine.back() == ' ') || (aLine.back() == '\r') || (aLine.back() == '\n')))
			aLine.pop_back();
		if (aLine.empty())
		{
			aInEntry = false;
			continue;
		}
		std::vector<std::string> aTokens;
		size_t aPos = 0;
		for (;;)
		{
			size_t aSpace = aLine.find(' ', aPos);
			aTokens.push_back(aLine.substr(aPos, aSpace == std::string::npos ? std::string::npos : aSpace - aPos));
			if (aSpace == std::string::npos)
				break;
			aPos = aSpace + 1;
		}
		if (!aInEntry)
		{
			aInEntry = true;
			aDef = new AnimDef();
			std::string aImageName = aTokens.size() > 1 ? aTokens[1] : "";
			std::map<std::string, Image*>::iterator anItr = mImageMap.find(aImageName);
			if (anItr == mImageMap.end())
			{
				Image* anImage = GetImage("images\\" + aImageName);
				if (anImage == NULL)
				{
					Popup("Failed to load images\\" + aImageName);
					mLoadingFailed = true;
					delete aDef;
					return;
				}
				mImageMap[aImageName] = anImage;
				aDef->mImage = anImage;
			}
			else
				aDef->mImage = anItr->second;
			aDef->mFlagA = aTokens.size() > 2 ? atoi(aTokens[2].c_str()) : 0;
			aDef->mFlagB = aTokens.size() > 3 ? atoi(aTokens[3].c_str()) : 0;
			aDef->mLoop = (aTokens.size() > 4 ? atoi(aTokens[4].c_str()) : 0) & 1;
			mAnimDefs[aTokens[0]] = aDef;
		}
		else if ((aTokens[0] == "hr") && (aTokens.size() >= 7))
		{
			int aCount = atoi(aTokens[1].c_str());
			int aX = atoi(aTokens[2].c_str());
			int aY = atoi(aTokens[3].c_str());
			int aW = atoi(aTokens[4].c_str());
			int aH = atoi(aTokens[5].c_str());
			int aDelay = atoi(aTokens[6].c_str());
			for (int i = 0; i < aCount; i++)
			{
				AnimFrame aFrame;
				aFrame.mX = aX;
				aFrame.mY = aY;
				aFrame.mW = aW;
				aFrame.mH = aH;
				aFrame.mDelay = aDelay;
				aDef->mFrames.push_back(aFrame);
				aX += aW;
			}
		}
	}
}

// 0x422420: the animation of a name
AnimDef* SevenSeasApp::GetAnimDef(const std::string& theName)
{
	std::map<std::string, AnimDef*>::iterator anItr = mAnimDefs.find(theName);
	if (anItr == mAnimDefs.end())
		return NULL;
	return anItr->second;
}

// 0x41e580
void SevenSeasApp::Init()
{
	SexyApp::Init();

	// the 2003 SexyApp::Init seeds the C library generator with the tick count; -seed=N makes a run repeatable
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
	mFonts[FONT_FONT1] = new ImageFont(mImages[IMAGE_FONT1], gFontFiles[FONT_FONT1]);
	mFonts[FONT_MT11] = new ImageFont(this, gFontFiles[FONT_MT11]);
	mFonts[FONT_MT22] = new ImageFont(this, gFontFiles[FONT_MT22]);
	mFonts[FONT_SCORE] = new ImageFont(mImages[IMAGE_SCOREFONT], gFontFiles[FONT_SCORE]);
	mFonts[FONT_YESNO] = new ImageFont(mImages[IMAGE_YESNO], gFontFiles[FONT_YESNO]);
	mFonts[FONT_MEDIUM] = new ImageFont(mImages[IMAGE_MEDIUMFONT], gFontFiles[FONT_MEDIUM]);
	mFonts[FONT_TUT] = new ImageFont(mImages[IMAGE_TUTFONT], gFontFiles[FONT_TUT]);

	mTitleScreen = new TitleScreen(this);
	mTitleScreen->Resize(0, 0, mWidth, mHeight);
	mWidgetManager->AddWidget(mTitleScreen);

	SetCursorImage(0, mImages[IMAGE_CURSOR1]);
	SetCursorImage(1, mImages[IMAGE_POINTER2]);
	SetCursorImage(2, mImages[IMAGE_OVERHAND]);
	SetCursorImage(3, mImages[IMAGE_TYPER]);
	SetCursorImage(4, mImages[IMAGE_CURSORSHOVEL]);

	// the game's music is one module; both slots play it from the order of the piece
	mMusicInterface->LoadMusic(0, "music\\game.it");
	mMusicInterface->LoadMusic(1, "music\\game.it");
	mMusicInterface->PlayMusic(0, 0x16, false);
	mMusicSlot = 0;
}

// 0x41db50: the sounds (the loading bar counts them), then the images made from others
void SevenSeasApp::LoadingThreadProc()
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

	// a 0xc0 x 0xc0 copy of the dialog's picture with the options button's picture on it
	if (mBigPanel == NULL)
	{
		mBigPanel = (MemoryImage*) CopyImage(mImages[IMAGE_DIALOG], Rect(0, 0, 0xc0, 0xc0));
		Graphics g(mBigPanel);
		g.DrawImage(mImages[IMAGE_DIALOGWOOD], 0x4b, 0);
	}
	BuildArrows();
	LoadAnims();
	mCompletedLoadingThreadTasks++;
}

// 0x41db50, the second half: the arrows around the ship, made by flipping and turning two pictures
void SevenSeasApp::BuildArrows()
{
	// the vertical arrow, 0x1f x 0x29
	mArrows[3] = new MemoryImage(this);
	mArrows[3]->Create(0x1f, 0x29);
	{
		Graphics g(mArrows[3]);
		g.DrawImage(mImages[IMAGE_ARROWVERT], 0, 0);
	}
	// turned: 0x29 x 0x1f
	mArrows[1] = new MemoryImage(this);
	mArrows[1]->Create(0x29, 0x1f);
	{
		uint32_t* aSrc = (uint32_t*) mArrows[3]->GetBits();
		uint32_t* aDst = (uint32_t*) mArrows[1]->GetBits();
		int aRow = 1;
		while (aRow < mArrows[1]->GetHeight())
		{
			int aCol = 1;
			while (aCol < mArrows[1]->GetWidth())
			{
				int aSrcIdx = mArrows[3]->GetWidth() * aCol + aRow;
				int aDstIdx = mArrows[1]->GetWidth() * aRow + aCol;
				aCol = aCol + 1;
				aDst[aDstIdx] = aSrc[aSrcIdx];
			}
			aRow = aRow + 1;
		}
		mArrows[1]->BitsChanged();
	}
	// the vertical arrow upside down
	mArrows[5] = new MemoryImage(this);
	mArrows[5]->Create(0x1f, 0x29);
	{
		uint32_t* aSrc = (uint32_t*) mArrows[3]->GetBits();
		uint32_t* aDst = (uint32_t*) mArrows[5]->GetBits();
		int aRow = 1;
		while (aRow < mArrows[5]->GetHeight())
		{
			int aCol = 1;
			while (aCol < mArrows[5]->GetWidth())
			{
				int aSrcIdx = (mArrows[3]->GetHeight() - aRow) * mArrows[3]->GetWidth() + aCol;
				int aDstIdx = mArrows[5]->GetWidth() * aRow + aCol;
				aCol = aCol + 1;
				aDst[aDstIdx] = aSrc[aSrcIdx];
			}
			aRow = aRow + 1;
		}
		mArrows[5]->BitsChanged();
	}
	// the turned one, mirrored
	mArrows[7] = new MemoryImage(this);
	mArrows[7]->Create(0x29, 0x1f);
	{
		uint32_t* aSrc = (uint32_t*) mArrows[1]->GetBits();
		uint32_t* aDst = (uint32_t*) mArrows[7]->GetBits();
		int aRow = 1;
		while (aRow < mArrows[7]->GetHeight())
		{
			int aCol = 1;
			while (aCol < mArrows[7]->GetWidth())
			{
				int aSrcIdx = mArrows[1]->GetWidth() * aRow - aCol + mArrows[1]->GetWidth();
				int aDstIdx = mArrows[7]->GetWidth() * aRow + aCol;
				aCol = aCol + 1;
				aDst[aDstIdx] = aSrc[aSrcIdx];
			}
			aRow = aRow + 1;
		}
		mArrows[7]->BitsChanged();
	}
	// the diagonal arrow, 0x20 x 0x22
	mArrows[6] = new MemoryImage(this);
	mArrows[6]->Create(0x20, 0x22);
	{
		Graphics g(mArrows[6]);
		g.DrawImage(mImages[IMAGE_ARROWDIAG], 0, 0);
	}
	mArrows[0] = new MemoryImage(this);
	mArrows[0]->Create(0x20, 0x22);
	{
		uint32_t* aSrc = (uint32_t*) mArrows[6]->GetBits();
		uint32_t* aDst = (uint32_t*) mArrows[0]->GetBits();
		int aRow = 1;
		while (aRow < mArrows[0]->GetHeight() - 1)
		{
			int aCol = 1;
			while (aCol < mArrows[0]->GetWidth())
			{
				int aSrcIdx = mArrows[6]->GetWidth() * aRow - aCol + mArrows[6]->GetWidth();
				int aDstIdx = mArrows[0]->GetWidth() * aRow + aCol;
				aCol = aCol + 1;
				aDst[aDstIdx] = aSrc[aSrcIdx];
			}
			aRow = aRow + 1;
		}
		mArrows[0]->BitsChanged();
	}
	mArrows[2] = new MemoryImage(this);
	mArrows[2]->Create(0x20, 0x22);
	{
		uint32_t* aSrc = (uint32_t*) mArrows[0]->GetBits();
		uint32_t* aDst = (uint32_t*) mArrows[2]->GetBits();
		int aRow = 1;
		while (aRow < mArrows[2]->GetHeight())
		{
			int aCol = 1;
			while (aCol < mArrows[2]->GetWidth())
			{
				int aSrcIdx = (mArrows[0]->GetHeight() - aRow) * mArrows[0]->GetWidth() + aCol;
				int aDstIdx = mArrows[2]->GetWidth() * aRow + aCol;
				aCol = aCol + 1;
				aDst[aDstIdx] = aSrc[aSrcIdx];
			}
			aRow = aRow + 1;
		}
		mArrows[2]->BitsChanged();
	}
	mArrows[8] = new MemoryImage(this);
	mArrows[8]->Create(0x20, 0x22);
	{
		uint32_t* aSrc = (uint32_t*) mArrows[2]->GetBits();
		uint32_t* aDst = (uint32_t*) mArrows[8]->GetBits();
		int aRow = 1;
		while (aRow < mArrows[8]->GetHeight())
		{
			int aCol = 1;
			while (aCol < mArrows[8]->GetWidth())
			{
				int aSrcIdx = mArrows[2]->GetWidth() * aRow - aCol + mArrows[2]->GetWidth();
				int aDstIdx = mArrows[8]->GetWidth() * aRow + aCol;
				aCol = aCol + 1;
				aDst[aDstIdx] = aSrc[aSrcIdx];
			}
			aRow = aRow + 1;
		}
		mArrows[8]->BitsChanged();
	}
}

void SevenSeasApp::LoadingThreadCompleted()
{
	SexyApp::LoadingThreadCompleted();
}

// 0x41c4a0 / 0x41c630: the registry values of the game
void SevenSeasApp::WriteToRegistry()
{
	SexyApp::WriteToRegistry();
	if (!mLastScoreName.empty())
		RegistryWriteString("LastScoreName", mLastScoreName);
	RegistryWriteBoolean("BonusNag", mBonusNag);
	RegistryWriteBoolean("WarnBlueNag", mWarnBlue);
	RegistryWriteBoolean("WarnGhostNag", mWarnGhost);
	RegistryWriteBoolean("WarnSerpNag", mWarnSerpent);
	RegistryWriteBoolean("WarnWhirlNag", mWarnWhirl);
}

void SevenSeasApp::ReadFromRegistry()
{
	SexyApp::ReadFromRegistry();
	std::string aName;
	if (RegistryReadString("LastScoreName", &aName))
		mLastScoreName = aName;
	bool aValue;
	if (RegistryReadBoolean("BonusNag", &aValue))
		mBonusNag = aValue;
	if (RegistryReadBoolean("WarnBlueNag", &aValue))
		mWarnBlue = aValue;
	if (RegistryReadBoolean("WarnGhostNag", &aValue))
		mWarnGhost = aValue;
	if (RegistryReadBoolean("WarnSerpNag", &aValue))
		mWarnSerpent = aValue;
	if (RegistryReadBoolean("WarnWhirlNag", &aValue))
		mWarnWhirl = aValue;
}

// 0x41d1f0
Dialog* SevenSeasApp::NewDialog(int theDialogId, bool isModal, const SexyString& theDialogHeader,
	const SexyString& theDialogLines, const SexyString& theDialogFooter, int theButtonMode)
{
	Dialog* aDialog = new Dialog2004(mImages[IMAGE_DIALOG], mImages[IMAGE_DBUTTONBLACK], theDialogId, isModal,
		theDialogHeader, theDialogLines, theDialogFooter, theButtonMode);
	aDialog->SetButtonFont(mFonts[FONT_MT11]);
	aDialog->SetHeaderFont(mFonts[FONT_FONT1]);
	aDialog->SetLinesFont(mFonts[FONT_MT11]);
	aDialog->SetColor(Dialog::COLOR_HEADER, Color(0x66, 0x5c, 0x38));
	aDialog->SetColor(Dialog::COLOR_LINES, Color(0xf3, 0xb4, 0x4d));
	aDialog->SetColor(Dialog::COLOR_BUTTON_TEXT, Color(0xe0, 0xb7, 0x28));
	aDialog->mContentInsets = Insets(0x26, 0x26, 0x26, 0x26);
	aDialog->Resize(0xed, 0x32, 0x13c, aDialog->GetPreferredHeight(0x13c));
	return aDialog;
}

// 0x420540: every button press clicks
void SevenSeasApp::ButtonPress(int theId)
{
	(void) theId;
	PlaySample(SOUND_CLICK);
}

// 0x420510
void SevenSeasApp::UpdateFrames()
{
	SexyApp::UpdateFrames();
	if (mAutoplay)
		AutoplayUpdate();
}

void SevenSeasApp::LostFocus()
{
	SexyApp::LostFocus();
}

// 0x420350: the cursor of the game (the images that Init registered are shown by the framework)
void SevenSeasApp::UpdateCursor()
{
}

// 0x420550: music from an order of the game's module; the two slots take turns so that one can fade out as the
// other fades in
void SevenSeasApp::SetMusicOrder(int theOrder, bool theFade)
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

// 0x417320..: the pirate head (shown while the pirate talks)
void SevenSeasApp::StartTalk()
{
	mTalkWidget->SetVisible(true);
	mTalkWidget->mMode = 1;
	mTalking = true;
}

void SevenSeasApp::StopTalk()
{
	mTalkWidget->mMode = -1;
}

// 0x41d750: a picture button; the copies of the pictures belong to the button
ButtonWidget* SevenSeasApp::AddButton(int theId, const Rect& theRect, Image* theImage, Image* theDownImage)
{
	ButtonWidget* aButton = new ButtonWidget(theId, this);
	aButton->Resize(theRect);
	aButton->mDoFinger = true;
	aButton->mButtonImage = CopyImage(theImage);
	aButton->mOverImage = aButton->mButtonImage;
	aButton->mDownImage = theDownImage != NULL ? CopyImage(theDownImage) : NULL;
	aButton->mMouseInsets = Insets(-1, -1, -1, -1);
	mWidgetManager->AddWidget(aButton);
	mButtons.push_back(aButton);
	return aButton;
}

// the menu's button (DialogButton with a picture)
DialogButton* SevenSeasApp::AddDialogButton(int theId, const Rect& theRect, Image* theImage)
{
	DialogButton* aButton = new DialogButton(theImage, theId, this);
	aButton->Resize(theRect);
	aButton->mDoFinger = true;
	aButton->mMouseInsets = Insets(-1, -1, -1, -1);
	mWidgetManager->AddWidget(aButton);
	mButtons.push_back(aButton);
	return aButton;
}

// 0x41fa20: the title screen's click makes the sidebar (undo, options, new game, quit), the pirate and the menu
void SevenSeasApp::TitleScreenDone()
{
	mWidgetManager->RemoveWidget(mTitleScreen);
	SafeDeleteWidget(mTitleScreen);
	mTitleScreen = NULL;

	// the panel at the left, under everything
	mSidePanel = new SidePanel(this);
	mSidePanel->Resize(0, 0, 0xad, 0x1e0);
	mWidgetManager->AddWidget(mSidePanel);

	// the three picture buttons: the backdrop's piece with the label's strip, lit, unlit and pushed
	static const int kButtons[3][3] = {{BUTTON_UNDO, 0x96, 0}, {BUTTON_OPTIONS, 0xf4, 1}, {BUTTON_NEW_GAME, 0x1a6, 2}};
	ButtonWidget* aButtons[3] = {NULL, NULL, NULL};
	for (int i = 0; i < 3; i++)
	{
		int aLabelY = (i == 0) ? 1 : ((i == 1) ? -0x29 : -0x51);
		int aPushedY = (i == 0) ? 2 : ((i == 1) ? -0x28 : -0x50);
		int aClipY = (i == 0) ? 1 : -1;
		MemoryImage* anUnlit = new MemoryImage(this);
		anUnlit->Create(0x86, 0x26);
		{
			Graphics g(anUnlit);
			g.DrawImage(mImages[IMAGE_BACKDROP], 0, 0, Rect(0x14, kButtons[i][1], 0x86, 0x26));
			g.ClipRect(0x13, aClipY, 100, 0x28);
			g.DrawImage(mImages[IMAGE_UNLITBUTTONTEXT], 0x13, aLabelY);
		}
		MemoryImage* aLit = new MemoryImage(this);
		aLit->Create(0x86, 0x26);
		{
			Graphics g(aLit);
			g.DrawImage(mImages[IMAGE_BACKDROP], 0, 0, Rect(0x14, kButtons[i][1], 0x86, 0x26));
			g.ClipRect(0x13, aClipY, 100, 0x28);
			g.DrawImage(mImages[IMAGE_LITBUTTONTEXT], 0x13, aLabelY);
		}
		MemoryImage* aPushed = new MemoryImage(this);
		aPushed->Create(0x86, 0x26);
		{
			Graphics g(aPushed);
			g.DrawImage(mImages[IMAGE_BUTTONPUSHED], 0, 0);
			g.ClipRect(0x14, (i == 0) ? 2 : 0, 100, 0x28);
			g.DrawImage(mImages[IMAGE_LITBUTTONTEXT], 0x14, aPushedY);
		}
		ButtonWidget* aButton = new ButtonWidget(kButtons[i][0], this);
		aButton->Resize(Rect(0x14, kButtons[i][1], 0x86, 0x26));
		aButton->mDoFinger = true;
		aButton->mButtonImage = anUnlit;
		aButton->mOverImage = aLit;
		aButton->mDownImage = aPushed;
		aButton->mMouseInsets = Insets(-1, -1, -1, -1);
		mWidgetManager->AddWidget(aButton);
		mButtons.push_back(aButton);
		aButtons[i] = aButton;
	}
	mUndoButton = aButtons[0];
	mOptionsButton = aButtons[1];
	mNewGameButton = aButtons[2];
	mQuitButton = AddButton(BUTTON_QUIT, Rect(0x134, 0x1aa, 0xc8, 0x35), mImages[IMAGE_QUITTOWINDOWSLIT], mImages[IMAGE_QUITTOWINDOWSUNLIT]);
	mQuitButton->mDownImage = NULL;

	// the pirate's head (visible once it speaks)
	mTalkWidget = new TalkWidget(this);
	mTalkWidget->mMode = 0;
	mTalkWidget->mUpdateCnt = mTalkWidget->mBlinkTick = 0;
	mTalkWidget->Resize(0x8e, 10, 200, 200);
	mTalkWidget->SetVisible(false);
	mWidgetManager->AddWidget(mTalkWidget);

	ShowIntro();
}

// 0x41f8b0: the menu (the difficulty and the tutorial)
void SevenSeasApp::ShowIntro()
{
	if (mIntroWidget != NULL)
		return;
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
		mBoard = NULL;
	}
	mTutorial = false;
	mUndoButton->SetVisible(false);
	mNewGameButton->SetVisible(false);
	mQuitButton->SetVisible(true);
	mIntroWidget = new IntroWidget(this);
	mIntroWidget->Resize(0xad, 0, 0x1d3, 0x1e0);
	mWidgetManager->AddWidget(mIntroWidget);
	mTalking = true;
	mTalkWidget->mMode = 1;
	mTalkWidget->SetVisible(true);
	mWidgetManager->BringToFront(mTalkWidget);
	mWidgetManager->BringToFront(mQuitButton);
	SetMusicOrder(0x19, true);
}

void SevenSeasApp::RemoveIntro()
{
	if (mIntroWidget != NULL)
	{
		mWidgetManager->RemoveWidget(mIntroWidget);
		SafeDeleteWidget(mIntroWidget);
		mIntroWidget = NULL;
	}
}

// 0x41d730: the menu's play button
void SevenSeasApp::OnPlay(int theDifficulty)
{
	mTutorial = false;
	mDifficulty = theDifficulty;
}

// 0x41ed70: the menu has said its piece and the game begins (also the tutorial's last click)
void SevenSeasApp::EndTutorial()
{
	if (mHighScoreWidget != NULL)
	{
		mWidgetManager->RemoveWidget(mHighScoreWidget);
		SafeDeleteWidget(mHighScoreWidget);
		mHighScoreWidget = NULL;
	}
	if (mTrialWidget != NULL)
	{
		mWidgetManager->RemoveWidget(mTrialWidget);
		SafeDeleteWidget(mTrialWidget);
		mTrialWidget = NULL;
	}
	if (mBoard != NULL)
	{
		mWidgetManager->RemoveWidget(mBoard);
		SafeDeleteWidget(mBoard);
		mBoard = NULL;
	}
	KillDialog(DIALOG_QUIT);
	KillDialog(DIALOG_OPTIONS);
	KillDialog(DIALOG_HIGH_SCORES);
	KillDialog(DIALOG_REGISTER);
	if (HasSavedGame() && !mTutorial)
	{
		RemoveIntro();
		mTalkWidget->mMode = -1;
		ShowContinueDialog();
	}
	else
	{
		RemoveIntro();
		mTalkWidget->mMode = -1;
		StartGame();
	}
}

// 0x41eef0: a new game (the saved game is lost)
void SevenSeasApp::StartGame()
{
	DeleteSavedGame();
	RemoveIntro();
	// port: the beginner's demo screen (WinSS.exe 0x4132b0) is not ported; the game starts at once
	mBoard = new Board(this, false);
	mBoard->Resize(0xad, 0, 0x1d3, 0x1e0);
	mWidgetManager->AddWidget(mBoard);
	mWidgetManager->BringToFront(mBoard);
	mWidgetManager->SetFocus(mBoard);
	mUndoButton->SetVisible(true);
	mQuitButton->SetVisible(false);
	mNewGameButton->SetVisible(true);
	mWidgetManager->BringToFront(mTalkWidget);
}

// 0x41ed70 + 0x421a10 case 0xf: continue the saved game
void SevenSeasApp::ContinueGame()
{
	RemoveIntro();
	mBoard = new Board(this, true);
	mBoard->Resize(0xad, 0, 0x1d3, 0x1e0);
	mWidgetManager->AddWidget(mBoard);
	mWidgetManager->BringToFront(mBoard);
	mWidgetManager->SetFocus(mBoard);
	mUndoButton->SetVisible(true);
	mQuitButton->SetVisible(false);
	mNewGameButton->SetVisible(true);
	mWidgetManager->BringToFront(mTalkWidget);
}

// 0x420770: a level is done: the bonus for the pirates sunk, then the panel
void SevenSeasApp::LevelComplete()
{
	mBoard->mState = 11;
	mBoard->mLevelComplete = false;
	if (!mBoard->AllPortsDone())
	{
		if (!mBoard->mLevelOver)
			mBoard->mLevelComplete = true;
		return;
	}
	int aBonus = mBoard->mKills * 10;
	if ((mBoard->mLevel < 10) && (mDifficulty == 1))
		aBonus = aBonus / 2;
	if (mBoard->mScore / 1000 < (aBonus + mBoard->mScore) / 1000)
	{
		PlaySample(SOUND_BONUSSHIP);
		mBoard->mLives++;
		mBoard->mLifeFlash = 0x46;
	}
	mBoard->mScore += aBonus;
	mMusicInterface->StopMusic(1);
	mMusicInterface->StopMusic(0);
	SetMusicOrder(0xc, false);
	SetMusicOrder(0xc, false);

	NextLevelDialog* aDialog = new NextLevelDialog(this);
	aDialog->Resize(0xc1, 0x32, 0x1a9, aDialog->GetPreferredHeight(0x17c));
	AddDialog(DIALOG_NEXT_LEVEL, aDialog);
}

// 0x420430: the game is over; the list of high scores comes up
void SevenSeasApp::ShowHighScoreWidget()
{
	if (mHighScoreWidget == NULL)
	{
		mBoard->mSeconds = (GameTickCount() - mBoard->mStartTick) / 1000;
		mHighScoreWidget = new HighScoreWidget(this);
		mHighScoreWidget->Resize(0xad, 0, 0x1d3, 0x1e0);
		mWidgetManager->AddWidget(mHighScoreWidget);
		mWidgetManager->BringToFront(mTalkWidget);
		SetMusicOrder(0x1c, true);
	}
}

// 0x420620
void SevenSeasApp::ShowOptions()
{
	OptionsDialog* aDialog = new OptionsDialog(this);
	aDialog->Resize(0xd7, 0x32, 0x17c, aDialog->GetPreferredHeight(0x17c));
	AddDialog(DIALOG_OPTIONS, aDialog);
}

// 0x41d570
void SevenSeasApp::ShowQuitDialog()
{
	DoDialog(DIALOG_QUIT, true, GT(0x004a62dc), GT(0x004a39f4), GT(0x004a62dc), Dialog::BUTTONS_YES_NO);
}

// 0x41d360
void SevenSeasApp::ShowNewGameDialog()
{
	if ((mBoard == NULL) || (mBoard->mState == STATE_LEVEL_END))
	{
		mTutorial = false;
		ShowIntro();
		return;
	}
	DoDialog(DIALOG_NEW_GAME, true, GT(0x004a3a3c), GT(0x004a3a08), GT(0x004a62dc), Dialog::BUTTONS_YES_NO);
}

// 0x41f0c0
void SevenSeasApp::ShowContinueDialog()
{
	DoDialog(DIALOG_CONTINUE, true, GT(0x004a3aa4), GT(0x004a3a60), GT(0x004a62dc), Dialog::BUTTONS_YES_NO);
}

// 0x421590
void SevenSeasApp::ShowHighScores()
{
	HighScoreDialog* aDialog = new HighScoreDialog(this);
	aDialog->Resize(0xd3, 0x32, 0x170, aDialog->GetPreferredHeight(0x170));
	AddDialog(DIALOG_HIGH_SCORES, aDialog);
}

// 0x4212d0: the name for a high score
void SevenSeasApp::ShowRecordScore(const SexyString& thePlace)
{
	RecordScoreDialog* aDialog = new RecordScoreDialog(this, thePlace);
	aDialog->Resize(0xfa, 0x20, 0x13c, aDialog->GetPreferredHeight(0x13c));
	AddDialog(DIALOG_RECORD_SCORE, aDialog);
}

// 0x420ba0
void SevenSeasApp::ShowBonusNag()
{
	if (mBonusNag)
	{
		NagDialog* aDialog = new NagDialog(this, DIALOG_BONUS_NAG, -1);
		aDialog->Resize(0xbe, 0x14, 0x1af, aDialog->GetPreferredHeight(0x17c));
		AddDialog(DIALOG_BONUS_NAG, aDialog);
	}
}

// 0x4209e0: the warning about the kind of ship that comes (a check box turns it off)
void SevenSeasApp::ShowWarning(int theWhich)
{
	switch (theWhich)
	{
	case 0:
		if (!mWarnBlue)
			return;
		break;
	case 1:
		if (!mWarnGhost)
			return;
		break;
	case 2:
		if (!mWarnSerpent)
			return;
		break;
	case 3:
		if (!mWarnWhirl)
			return;
		break;
	default:
		// 4: the tutorial's own box, which the original creates and then closes again at once
		return;
	}
	NagDialog* aDialog = new NagDialog(this, DIALOG_WARNING, theWhich);
	aDialog->Resize(0xdc, 0x46, 0x172, aDialog->GetPreferredHeight(0x17c));
	AddDialog(DIALOG_WARNING, aDialog);
}

// 0x41f2b0 / 0x421980: the map between the chapters; the help
void SevenSeasApp::ShowMap()
{
	MapWidget* aMap = new MapWidget(this);
	aMap->Resize(0xad, 0, 0x1d3, 0x1e0);
	mMapWidget = aMap;
	mWidgetManager->AddWidget(aMap);
	mWidgetManager->BringToBack(aMap);
	mWidgetManager->BringToBack(mBoard);
	mWidgetManager->SetFocus(aMap);
}

void SevenSeasApp::ShowHelp()
{
}

// 0x41ac10 / 0x41ac40: the saved game is data/savegame.dat in the original, savegame.dat in the user folder here
bool SevenSeasApp::HasSavedGame()
{
	std::string aData;
	return Platform::ReadHostFile(Platform::VFS::Get().GetUserDir() + "/" + Board::SavePath(), &aData);
}

void SevenSeasApp::DeleteSavedGame()
{
	Platform::RemoveUserFile(Board::SavePath());
}

// 0x421a10: every button of the game
void SevenSeasApp::ButtonDepress(int theId)
{
	if ((theId >= 2000) && (theId < 3000))
	{
		switch (theId - 2000)
		{
		case DIALOG_NEW_GAME:
			KillDialog(DIALOG_NEW_GAME);
			if (!mTutorial && (mBoard != NULL))
				mBoard->SaveGame();
			mTutorial = false;
			ShowIntro();
			break;
		case DIALOG_QUIT:
			KillDialog(DIALOG_QUIT);
			Shutdown();
			break;
		case DIALOG_OPTIONS:
		{
			OptionsDialog* aDialog = (OptionsDialog*) GetDialog(DIALOG_OPTIONS);
			if (aDialog != NULL)
			{
				bool aWindowed = !aDialog->mFullscreenCheckbox->IsChecked();
				if (mIsWindowed != aWindowed)
					SwitchScreenMode(aWindowed);
				mCustomCursorsEnabled = aDialog->mCursorsCheckbox->IsChecked();
			}
			KillDialog(DIALOG_OPTIONS);
			break;
		}
		case DIALOG_NEXT_LEVEL:
			KillDialog(DIALOG_NEXT_LEVEL);
			mBoard->NewLevel();
			break;
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
		case DIALOG_BONUS_NAG:
		{
			NagDialog* aDialog = (NagDialog*) GetDialog(DIALOG_BONUS_NAG);
			if (aDialog != NULL)
				mBonusNag = !aDialog->mCheckbox->IsChecked();
			KillDialog(DIALOG_BONUS_NAG);
			break;
		}
		case DIALOG_WARNING:
		{
			NagDialog* aDialog = (NagDialog*) GetDialog(DIALOG_WARNING);
			if (aDialog != NULL)
			{
				bool aShow = !aDialog->mCheckbox->IsChecked();
				switch (aDialog->mWhich)
				{
				case 0:
					mWarnBlue = aShow;
					break;
				case 1:
					mWarnGhost = aShow;
					break;
				case 2:
					mWarnSerpent = aShow;
					break;
				case 3:
					mWarnWhirl = aShow;
					break;
				}
			}
			KillDialog(DIALOG_WARNING);
			break;
		}
		case DIALOG_CONTINUE:
			KillDialog(DIALOG_CONTINUE);
			ContinueGame();
			break;
		default:
			KillDialog(theId - 2000);
			break;
		}
	}
	else if ((theId >= 3000) && (theId < 4000))
	{
		if (theId - 3000 == DIALOG_CONTINUE)
		{
			KillDialog(DIALOG_CONTINUE);
			StartGame();
		}
		else
			KillDialog(theId - 3000);
	}

	switch (theId)
	{
	case BUTTON_UNDO:
		if (mBoard != NULL)
			mBoard->Undo();
		break;
	case BUTTON_OPTIONS:
		ShowOptions();
		break;
	case BUTTON_NEW_GAME:
		ShowNewGameDialog();
		break;
	case BUTTON_QUIT:
		ShowQuitDialog();
		break;
	}
}

void SevenSeasApp::HandleCmdLineParam(const std::string& theParamName, const std::string& theParamValue)
{
	if (theParamName == "-autoplay")
		mAutoplay = true;
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
