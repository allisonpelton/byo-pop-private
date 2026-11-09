// The full-screen widgets of Mummy Maze Deluxe (see Screens.h).

#include "Screens.h"
#include "MummyApp.h"
#include "Board.h"
#include "GameLevels.h"
#include "GameText.h"
#include "Compat2004.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "MusicInterface.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

using namespace Sexy;

// ---------------------------------------------------------------------------------------------------------
// TitleScreen

// 0x42a5f0
TitleScreen::TitleScreen(MummyApp* theApp) :
	mApp(theApp),
	mProgress(0.0),
	mPlayShown(false)
{
	// port: the registered game (no plays counted, no register link, no delay; docs/GAME_NOTES.md)
	mPlayLink = new HyperlinkWidget(1, this);
	mPlayLink->SetFont(mApp->mBiggestFont);
	mPlayLink->mUnderlineSize = 0;
	mPlayLink->SetVisible(false);
	mPlayLink->mColor = Color(0xff, 0x7d, 5);
	mPlayLink->mLabel = GT(0x00495510);
}

// 0x42aa80
TitleScreen::~TitleScreen()
{
	delete mPlayLink;
}

// 0x42b250 / 0x42b280
void TitleScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mPlayLink);
}

void TitleScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mPlayLink);
}

// 0x42b2b0
void TitleScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	int aWidth = mPlayLink->mFont->StringWidth(mPlayLink->mLabel);
	mPlayLink->Resize((mWidth - aWidth) / 2, 0x1bb, aWidth, 0x20);
}

// 0x42b0d0
void TitleScreen::Update()
{
	Widget::Update();
	if (!mPlayShown)
	{
		if (mUpdateCnt % 4 == 0)
		{
			mProgress = mApp->GetLoadingThreadProgress();
			if ((mProgress == 1.0) && mApp->mLoaded)
			{
				mPlayLink->SetVisible(true);
				mProgress = 0.0;
				mPlayShown = true;
				mProgress = 1.0;
			}
			MarkDirty();
		}
	}
	if (!mPlayLink->mVisible && mPlayShown)
	{
		mPlayLink->SetVisible(true);
		MarkDirty();
	}
}

// 0x42aaf0
void TitleScreen::Draw(Graphics* g)
{
	g->DrawImage(mApp->mImages[IMAGE_TITLE], 0, 0);
	g->DrawImage(mApp->mImages[IMAGE_TRIALCIRCLE], 0, 0x17f);
	int aBar = (int) (mProgress * 340.0);
	g->DrawImage(mApp->mImages[IMAGE_TITLEBAR], 0x93, 0x183, Rect(0, 0, aBar, 0x18));
	g->SetFont(mApp->mHeaderFont);
	SexyString aMessage = GT(0x00495540);
	g->SetColor(Color(0, 0, 0));
	for (int dx = -1; dx < 1; dx++)
		for (int dy = -1; dy < 1; dy++)
			g->DrawString(aMessage, (mWidth - g->GetFont()->StringWidth(aMessage)) / 2 + dx, 0x177 + dy);
	g->SetColor(Color(0xff, 0xff, 0));
	g->DrawString(aMessage, (mWidth - g->GetFont()->StringWidth(aMessage)) / 2, 0x177);
	g->SetFont(mApp->mFont1);
	g->SetColor(Color(0xff, 0xff, 0xff));
	g->DrawString(GT(0x00495534) + mApp->mProductVersion, 0x16, 0x7e);
	if (mPlayLink->mVisible)
	{
		int aWidth = mPlayLink->mFont->StringWidth(mPlayLink->mLabel);
		g->SetColor(Color(0, 0, 0));
		g->SetFont(mApp->mBiggestFont);
		for (int dx = -1; dx < 1; dx++)
			for (int dy = -1; dy < 1; dy++)
				g->DrawString(mPlayLink->mLabel, (mWidth - aWidth) / 2 + dx, dy + 0x1d8);
	}
}

// 0x42b330
void TitleScreen::ButtonDepress(int theId)
{
	(void) theId;		// the play link's
	mApp->TitleDone();
}

void TitleScreen::ButtonPress(int theId)
{
	(void) theId;
	mApp->PlaySample(SOUND_CLICK);
}

// ---------------------------------------------------------------------------------------------------------
// ScrollScreen

// 0x42b390
ScrollScreen::ScrollScreen(MummyApp* theApp) :
	mApp(theApp),
	mOffset(0x1dc),
	mTorchFrame(0),
	mScrollState(0),
	mScrollTime(0.0)
{
	mDoFinger = true;
}

// 0x42b3f0
ScrollScreen::~ScrollScreen()
{
}

// 0x416e80
void ScrollScreen::ButtonPress(int theId)
{
	(void) theId;
	mApp->PlaySample(SOUND_CLICK);
}

void ScrollScreen::ButtonDepress(int theId)
{
	(void) theId;
}

// 0x42b400: the front of the scroll and its torch (a 17-frame cycle, drawn through a 58 x 58 window)
void ScrollScreen::Draw(Graphics* g)
{
	g->DrawImage(mApp->mImages[IMAGE_MENUFRONT], 0, mHeight - 0x1dc + mOffset);
	Graphics gc(*g);
	gc.ClipRect(0x11e, mHeight - 0x173 + mOffset, 0x3a, 0x3a);
	static const int kFrames[17] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 7, 6, 5, 4, 3, 2, 1, 0};
	gc.DrawImage(mApp->mImages[IMAGE_EYESTRIP], 0x11e, mHeight + kFrames[mTorchFrame] * -0x3a - 0x173 + mOffset);
}

// 0x42b580
void ScrollScreen::Update()
{
	Widget::Update();
	bool aMoved = false;
	switch (mScrollState)
	{
	case 0:
		mScrollTime += 0.0167;
		if (1.570795 <= mScrollTime)
		{
			mOffset = 0;
			mScrollState = 1;
			MarkDirty();
		}
		else
		{
			mOffset = (int) ((1.0 - sin(mScrollTime)) * 476.0);
			aMoved = true;
		}
		break;
	case 2:
		mScrollTime += 0.06;
		if (mScrollTime < 3.14159)
		{
			mOffset = (int) ((1.0 - cos(mScrollTime)) * 100.0);
			MarkDirty();
		}
		else
			mScrollState = 3;
		break;
	case 4:
		mScrollTime += 0.06;
		if (3.14159 <= mScrollTime)
		{
			mOffset = 0;
			mScrollState = 1;
			MarkDirty();
		}
		else
		{
			mOffset = (int) ((1.0 - cos(mScrollTime + 3.14159)) * 100.0);
			aMoved = true;
		}
		break;
	case 5:
		mScrollTime -= 0.0167;
		if (0.0 < mScrollTime)
			mOffset = (int) ((1.0 - sin(mScrollTime)) * 476.0);
		else
		{
			mOffset = 0x1dc;
			mScrollState = 6;
		}
		MarkDirty();
		break;
	default:
		break;
	}
	if (aMoved)
		MarkDirty();
	if (mUpdateCnt % 6 == 0)
	{
		mTorchFrame = (mTorchFrame + 1) % 0x11;
		MarkDirty();
	}
}

// ---------------------------------------------------------------------------------------------------------
// MainMenu

static HyperlinkWidget* MakeMenuLink(MummyApp* theApp, int theId, ButtonListener* theListener, const SexyString& theLabel)
{
	HyperlinkWidget* aLink = new HyperlinkWidget(theId, theListener);
	aLink->mLabel = theLabel;
	aLink->mColor = Color(0, 0, 0);
	aLink->mOverColor = Color(0xff, 0, 0);
	aLink->SetFont(theApp->mBiggestFont);
	aLink->mUnderlineSize = 0;
	return aLink;
}

// 0x4165a0: theEntry 0 comes from the title (the scroll comes down), 1 from the adventure (it slides back), 2 shows it at once
MainMenu::MainMenu(MummyApp* theApp, int theEntry) :
	ScrollScreen(theApp),
	mFade(0.0)
{
	mLinks[0] = MakeMenuLink(theApp, 0, this, GT(0x0049224c));
	mLinks[1] = MakeMenuLink(theApp, 1, this, GT(0x00492240));
	mLinks[2] = MakeMenuLink(theApp, 2, this, GT(0x00492234));
	mLinks[3] = MakeMenuLink(theApp, 3, this, GT(0x00492228));
	if (theEntry == 0)
	{
		mApp->PlaySample(SOUND_TOMBSLIDE);
		mFade = 1.0;
		mScrollState = 0;
	}
	else if (theEntry == 1)
	{
		mFade = 0.0;
		mScrollTime = 0.0;
		mScrollState = 4;
		mOffset = 200;
	}
	else if (theEntry == 2)
	{
		mFade = 0.0;
		mScrollState = 0;
		mScrollTime = 100.0;
		mFade = 1.0;
	}
}

// 0x416b90
MainMenu::~MainMenu()
{
	for (int i = 0; i < 4; i++)
		delete mLinks[i];
}

// 0x416e00 / 0x416e40
void MainMenu::AddedToManager(WidgetManager* theWidgetManager)
{
	ScrollScreen::AddedToManager(theWidgetManager);
	for (int i = 0; i < 4; i++)
		theWidgetManager->AddWidget(mLinks[i]);
}

void MainMenu::RemovedFromManager(WidgetManager* theWidgetManager)
{
	ScrollScreen::RemovedFromManager(theWidgetManager);
	for (int i = 0; i < 4; i++)
		theWidgetManager->RemoveWidget(mLinks[i]);
}

// 0x416cd0
void MainMenu::Update()
{
	int aOffset = mOffset;
	ScrollScreen::Update();
	if (aOffset != mOffset)
	{
		mLinks[1]->Resize(mX + 0x32, mHeight + mY - 0x46 + mOffset, mWidth / 2 - 0x32, 0x20);
		mLinks[0]->Resize(mX + 0x32, mHeight + mY - 0x78 + mOffset, mWidth / 2 - 0x32, 0x20);
		mLinks[2]->Resize(mWidth / 2 + mX, mHeight + mY - 0x46 + mOffset, mWidth / 2 - 0x32, 0x20);
		mLinks[3]->Resize(mWidth / 2 + mX, mHeight + mY - 0x78 + mOffset, mWidth / 2 - 0x32, 0x20);
	}
	if (mFade < 1.0)
	{
		mFade += 0.015;
		MarkDirty();
		if (mFade > 1.0)
			mFade = 1.0;
	}
}

// 0x416c20
void MainMenu::Draw(Graphics* g)
{
	g->SetColorizeImages(true);
	int aValue = (int) (mFade * 255.0);
	g->SetColor(Color(aValue, aValue, aValue));
	g->DrawImage(mApp->mImages[IMAGE_MENUBACK], 0, 0);
	g->SetColorizeImages(false);
	ScrollScreen::Draw(g);
	g->DrawImage(mApp->mImages[IMAGE_MENULOGO], (mWidth - 0x1c7) / 2, 10 - mOffset);
}

// 0x416e90
void MainMenu::ButtonDepress(int theId)
{
	switch (theId)
	{
	case 0:
		mApp->mGameMode = MODE_CLASSIC;
		mApp->mClassicChamber = 0;
		mApp->mClassicPyramid = 0;
		mApp->mScore = 0;
		// the data for the Internet score list start over: a byte, the registered name and code (the port has neither) and
		// the time
		mApp->mScoreBuffer.Clear();
		mApp->mScoreBuffer.WriteByte(1);
		mApp->mScoreBuffer.WriteString("");
		mApp->mScoreBuffer.WriteString("");
		mApp->mScoreBuffer.WriteLong((long) time(NULL));
		mApp->StartClassicOrContinue();
		return;
	case 1:
		mApp->mGameMode = MODE_ADVENTURE;
		mApp->ShowAdventureSlots();
		return;
	case 2:
		mApp->ShowQuitDialog();
		return;
	case 3:
		mApp->StartTutorial();
		mApp->mGameMode = MODE_TUTORIAL;
		return;
	}
}

// ---------------------------------------------------------------------------------------------------------
// GamePanel

// 0x412ea0
GamePanel::GamePanel(MummyApp* theApp) :
	ScrollScreen(theApp),
	mLeaving(false),
	mFade(1.0)
{
	static const unsigned kLabels[4] = {0x00491f78, 0x00491f6c, 0x00491f5c, 0x00491f40};
	for (int i = 0; i < 4; i++)
	{
		unsigned aLabel = kLabels[i];
		if (i == 3 && theApp->mGameMode == MODE_ADVENTURE)
			aLabel = 0x00491f50;
		mLinks[i] = MakeMenuLink(theApp, i, this, GT(aLabel));
		mLinks[i]->SetDisabled(false);
	}
	mLinks[0]->SetFont(theApp->mBiggestFont);
	// the first two are not offered while a solution is shown
	if (theApp->mGameMode == MODE_CLASSIC && theApp->mBoard != NULL && theApp->mBoard->mShowingSolution)
	{
		mLinks[0]->SetDisabled(true);
		mLinks[1]->SetDisabled(true);
	}
	mFade = 0.0;
	mLeaving = false;
	SetVisible(true);
}

// 0x4136a0 (its deleting form 0x413680)
GamePanel::~GamePanel()
{
	for (int i = 0; i < 4; i++)
		delete mLinks[i];
}

// 0x403430: a press on a link of the panel makes no sound (the main menu and the other screens click: 0x416e80); the
// links' releases choose
void GamePanel::ButtonPress(int theId)
{
	(void) theId;
}

// 0x4138d0
void GamePanel::AddedToManager(WidgetManager* theWidgetManager)
{
	ScrollScreen::AddedToManager(theWidgetManager);
	for (int i = 0; i < 4; i++)
		theWidgetManager->AddWidget(mLinks[i]);
}

// 0x413910
void GamePanel::RemovedFromManager(WidgetManager* theWidgetManager)
{
	ScrollScreen::RemovedFromManager(theWidgetManager);
	for (int i = 0; i < 4; i++)
		theWidgetManager->RemoveWidget(mLinks[i]);
}

// 0x4137a0
void GamePanel::Update()
{
	int aOffset = mOffset;
	ScrollScreen::Update();
	if (mOffset != aOffset)
	{
		mLinks[0]->Resize(mX + 0x32, mHeight + mY - 0x46 + mOffset, mWidth / 2 - 0x32, 0x20);
		mLinks[1]->Resize(mX + 0x32, mHeight + mY - 0x78 + mOffset, mWidth / 2 - 0x32, 0x20);
		mLinks[2]->Resize(mWidth / 2 + mX, mHeight + mY - 0x46 + mOffset, mWidth / 2 - 0x32, 0x20);
		mLinks[3]->Resize(mWidth / 2 + mX, mHeight + mY - 0x78 + mOffset, mWidth / 2 - 0x32, 0x20);
	}
	if (mLeaving)
	{
		mFade -= 0.025;
		MarkDirty();
		if (mFade < 0.0)
		{
			mFade = 0.0;
			if (mScrollState == 3)
			{
				mApp->ShowMainMenu(1, true);
				mApp->RemoveGamePanel();
			}
		}
	}
}

// 0x413730
void GamePanel::Draw(Graphics* g)
{
	if (mFade < 1.0)
	{
		g->SetColor(Color(0, 0, 0, (int) ((1.0 - mFade) * 255.0)));
		g->FillRect(0, 0, mWidth, mHeight);
	}
	ScrollScreen::Draw(g);
}

// 0x413950
void GamePanel::ButtonDepress(int theId)
{
	Board* aBoard = mApp->mBoard;
	switch (theId)
	{
	case 0:
		aBoard->Undo();
		break;
	case 1:
		aBoard->Retry();
		break;
	case 2:
		if (mApp->mGameMode == MODE_ADVENTURE)
			mApp->AdventureLeave();
		aBoard->SaveClassic();
		for (int i = 0; i < 4; i++)
			mLinks[i]->SetVisible(true);
		mLeaving = true;
		mScrollState = 2;
		mScrollTime = 0;
		mFade = 1.0;
		return;
	case 3:
		if (mApp->mGameMode == MODE_ADVENTURE)
		{
			mApp->AdventureLeave();
			aBoard->Retry();
			mApp->LoadAdventure(mApp->mCurrentSlot, false);
		}
		else
		{
			aBoard->mShowingSolution = false;
			aBoard->Retry();
			aBoard->m270 = aBoard->mUpdateCnt;
			aBoard->mState = STATE_SOLVING;
		}
		break;
	default:
		return;
	}
	mApp->RemoveGamePanel();
}

// ---------------------------------------------------------------------------------------------------------
// SidePanel

// 0x4291d0
SidePanel::SidePanel(MummyApp* theApp) :
	mApp(theApp),
	mScore(theApp->mScore),
	mAlpha(0),
	mAnkhAlpha(0)
{
	if (mApp->mGameMode == MODE_ADVENTURE_DONE || mApp->mGameMode == MODE_CLASSIC_DONE || mApp->mLevelComplete != NULL)
		mMumY = 0xd;
	else
		mMumY = -0x46;
	// the buttons: pictures cut from the strip IMAGE_BUTTONSTRIP. The backdrop holds the buttons' normal look: they have only
	// the lit picture (while the pointer is over), the pressed one and, for the world map, the disabled one (0x42a2f0 ff.)
	static const int kButtons[5][4] = {
		{10, 0, 0x2a, -1},         // undo (lit row, pressed row)
		{11, 0x54, 0x7e, -1},      // retry
		{12, 0xa8, 0xd2, -1},      // options
		{13, 0xfc, 0x126, 0x150},  // world map (adventure only): lit, pressed, disabled
		{14, 0x17a, 0x1a4, -1}     // quit
	};
	for (int i = 0; i < 5; i++)
	{
		ButtonWidget* aButton = new ButtonWidget(kButtons[i][0], this);
		aButton->mDoFinger = true;
		Image* aStrip = mApp->mImages[IMAGE_BUTTONSTRIP];
		aButton->mOverImage = mApp->CopyImage(aStrip, Rect(0, kButtons[i][1], 0x87, 0x2a));
		aButton->mDownImage = mApp->CopyImage(aStrip, Rect(0, kButtons[i][2], 0x87, 0x2a));
		if (kButtons[i][3] >= 0)
			aButton->mDisabledImage = mApp->CopyImage(aStrip, Rect(0, kButtons[i][3], 0x87, 0x2a));
		aButton->mMouseInsets = Insets(-1, -1, -1, -1);
		mButtons[i] = aButton;
	}
	if (mApp->mGameMode != MODE_ADVENTURE && mApp->mGameMode != MODE_ADVENTURE_DONE)
		mButtons[3]->SetDisabled(true);
}

// (the original's buttons own the pictures that CopyImage made for them; here they are deleted with the button)
// 0x4296f0
SidePanel::~SidePanel()
{
	for (int i = 0; i < 5; i++)
	{
		delete mButtons[i]->mOverImage;
		delete mButtons[i]->mDownImage;
		delete mButtons[i]->mDisabledImage;
		delete mButtons[i];
	}
}

// 0x42a2a0 / 0x42a2f0
void SidePanel::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	for (int i = 0; i < 5; i++)
		theWidgetManager->AddWidget(mButtons[i]);
}

void SidePanel::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	for (int i = 0; i < 5; i++)
		theWidgetManager->RemoveWidget(mButtons[i]);
}

// 0x42a170 (and 0x42a280, its last piece)
void SidePanel::Update()
{
	Widget::Update();
	if (mApp->mBoard != NULL && mMumY < 0xd && mApp->mBoard->mFade >= 1.0)
	{
		mMumY++;
		MarkDirty();
	}
	if (mUpdateCnt % 10 == 0)
	{
		int aValue = (int) ((double) mUpdateCnt * 1.5);
		int aMod = abs(aValue % 200);
		mAlpha = (aMod < 0x65) ? aMod : 200 - aMod;
		aMod = abs(aValue % 512);
		mAnkhAlpha = (aMod < 0x101) ? aMod : 0x200 - aMod;
		MarkDirty();
	}
	if (mScore < mApp->mScore)
	{
		if (mUpdateCnt % 3 == 0)
		{
			int aDiff = mApp->mScore - mScore;
			if (aDiff > 99)
				mScore += 100;
			else if (aDiff > 9)
				mScore += 10;
			else
				mScore += 1;
			MarkDirty();
		}
	}
}

// 0x42a570
void SidePanel::MouseDown(int x, int y, int theClickCount)
{
	(void) theClickCount;
	if (x > 0x5c && y > 0x140 && x < 0x8b && y < 0x177 && mApp->mBoard != NULL && mApp->mShowAnkh)
		mApp->ShowAnkhHelp(mApp->mBoard->mSolvable);
}

// 0x42a340: the buttons' places (the creation order is undo, retry, options, world map, quit)
void SidePanel::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	static const int kY[5] = {0x83, 0xae, 0xe2, 0x10d, 0x1ae};
	for (int i = 0; i < 5; i++)
		mButtons[i]->Resize(7, kY[i], 0x87, 0x2a);
}

// 0x429790
void SidePanel::Draw(Graphics* g)
{
	g->DrawImage(mApp->mImages[IMAGE_BACKDROP], 0, 0, Rect(0, 0, 0x96, 0x1e0));
	Board* aBoard = mApp->mBoard;
	int aLow = 0x78, aHigh = 0x9b;
	if (aBoard != NULL && aBoard->mExitStyle == 1)
	{
		aLow = 200;
		aHigh = 0xeb;
	}
	int aShake = 0;
	bool aShaking = (aBoard != NULL) && (aBoard->mDeathTimer >= aLow) && (aBoard->mDeathTimer < aHigh);
	if (aShaking)
	{
		aShake = (aBoard->mDeathTimer % 8) - 4;
		g->Translate(aShake, aShake);
	}
	g->DrawImage(mApp->mImages[IMAGE_MUMLOGO], 0xd, mMumY);
	if (aShaking)
		g->Translate(-aShake, -aShake);
	g->SetColorizeImages(true);
	g->SetDrawMode(1);
	int anAlpha = mAlpha < 0 ? 0 : (mAlpha > 0xff ? 0xff : mAlpha);
	g->SetColor(Color(0x94, 0xf6, 0x1a, anAlpha));
	if (aShaking)
		g->Translate(aShake, aShake);
	g->DrawImage(mApp->mImages[IMAGE_MUMLOGO], 0xd, mMumY);
	if (aShaking)
		g->Translate(-aShake, -aShake);
	g->SetDrawMode(0);
	g->SetColorizeImages(false);

	int aMode = mApp->mGameMode;
	int aLevel = mApp->mClassicPyramid;
	int aSub = mApp->mClassicChamber;
	if (aMode == MODE_ADVENTURE || aMode == MODE_ADVENTURE_DONE)
	{
		aSub = mApp->mAdvPyramid;
		aLevel = mApp->mAdvChamber;
	}
	// the pyramid map: the chamber's mark, and the marks of the chambers before it
	int aChamber = mApp->mClassicChamber;
	int aPyramid = mApp->mClassicPyramid;
	if (aMode == MODE_ADVENTURE || aMode == MODE_ADVENTURE_DONE)
	{
		aChamber = mApp->mAdvChamber;
		aPyramid = mApp->mAdvPyramid;
		if (aChamber > 0xe)
			aChamber = 0xe;
	}
	if (aPyramid == 9)
		aPyramid = 10;
	else if (aPyramid == 10)
		aPyramid = 0xb;
	else if (aPyramid == 0xb)
		aPyramid = 9;
	g->DrawImage(mApp->mImages[IMAGE_MAP], 8, 0x13d);
	{
		Graphics aClip(*g);
		aClip.ClipRect(0x3b, 0x145, 0x1e, 0x1e);
		aClip.DrawImage(mApp->mImages[IMAGE_SMALLTREASURES], aPyramid * -0x1e + 0x3b, 0x145);
	}
	g->DrawImage(mApp->mImages[IMAGE_MAPHEAD], kPyramidX[aChamber] + 3, kPyramidY[aChamber] + 0x138);
	for (int i = 0; i < aChamber; i++)
		g->DrawImage(mApp->mImages[IMAGE_MAPX], kPyramidX[i] + 6, kPyramidY[i] + 0x13a);
	if (aMode == MODE_CLASSIC || aMode == MODE_CLASSIC_DONE)
	{
		// the pyramid's number on the card below the map
		g->DrawImage(mApp->mImages[IMAGE_PYRAMIDTEXT], 0xf, 0x147);
		char aNumber[256];
		sprintf(aNumber, GTC(0x0048d338), mApp->mClassicPyramid + 1);
		SexyString aNumberStr = aNumber;
		int aNumberWidth = mApp->mPyramidFont->StringWidth(aNumberStr);
		g->SetFont(mApp->mPyramidFont);
		g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
		g->DrawString(aNumberStr, (0x2a - aNumberWidth) / 2 + 0xf, 0x15d);
	}
	// the Ankh
	if (aBoard != NULL && (aMode == 8 || (aMode < 2 && mApp->mShowAnkh)) && aBoard->mSize == 6)
	{
		int aY = aBoard->mSolvable ? 0 : 0x6a;
		g->DrawImage(mApp->mImages[IMAGE_ANKH], 0x5d, 0x141, Rect(0, aY, 0x2d, 0x35));
		g->SetColorizeImages(true);
		g->SetColor(Color(0xff, 0xff, 0xff, mAnkhAlpha));
		g->DrawImage(mApp->mImages[IMAGE_ANKH], 0x5d, 0x141, Rect(0, aBoard->mSolvable ? 0x35 : 0x9f, 0x2d, 0x35));
		g->SetColorizeImages(false);
	}
	if (aMode != MODE_CLASSIC && aMode != MODE_CLASSIC_DONE)
	{
		g->DrawImage(mApp->mImages[IMAGE_SCORECOVER], 0, 0x6d);
		g->DrawImage(mApp->mImages[IMAGE_SNAKE], 7, 0x55);
		return;
	}
	// the score of a classic game
	g->SetFont(mApp->mScoreFont);
	char aBuf[64];
	sprintf(aBuf, "%d", mScore);
	g->SetColorizeImages(true);
	g->SetColor(Color(0xff, 0xff, 0xff));
	SexyString aStr = aBuf;
	g->DrawString(aStr, 0x49 - g->GetFont()->StringWidth(aStr) / 2, 100);
	g->SetColorizeImages(false);
	(void) aLevel;
	(void) aSub;
}

// 0x42a3d0
void SidePanel::ButtonDepress(int theId)
{
	switch (theId)
	{
	case 10:
		if (mApp->mVictory == NULL && mApp->mBoard != NULL)
			mApp->mBoard->Undo();
		break;
	case 11:
		if (mApp->mVictory == NULL && mApp->mBoard != NULL)
			mApp->mBoard->Retry();
		break;
	case 12:
		mApp->ShowOptions();
		break;
	case 13:
		mApp->OnWorldMapButton();
		break;
	case 14:
		mApp->ShowQuitToMainDialog();
		break;
	}
}

void SidePanel::ButtonPress(int theId)
{
	(void) theId;
	mApp->PlaySample(SOUND_CLICK);
}
