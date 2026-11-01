// The full-screen widgets of Seven Seas Deluxe (see Screens.h).

#include "Screens.h"
#include "SevenSeasApp.h"
#include "Board.h"
#include "GameText.h"
#include "Compat2004.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "MemoryImage.h"
#include "MusicInterface.h"
#include "SexyAppBase.h"
#include "WidgetManager.h"
#include "platform/UserFiles.h"
#include "platform/VFS.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

using namespace Sexy;

// ---------------------------------------------------------------------------------------------------------
// TitleScreen

// 0x4256f0
TitleScreen::TitleScreen(SevenSeasApp* theApp) :
	mApp(theApp),
	mProgress(0.0),
	mDelayProgress(0.0),
	mPlayShown(false),
	mRegisterClicked(false)
{
	// port: the shareware branches (plays remaining, the register link, the delay) are not ported; the screen
	// takes the registered path (docs/GAME_NOTES.md section 2)
	int aGames = mApp->mGamesPlayed;
	if (aGames < 0xb)
		mDivisor = 10;
	else if (aGames < 0x15)
		mDivisor = aGames;
	else if (aGames < 0x33)
		mDivisor = aGames * 2 - 0x14;
	else
		mDivisor = 0x50;
	if (aGames < 2)
		mPlaysLeft = 0;
	else if (aGames < 10)
		mPlaysLeft = aGames;
	else
		mPlaysLeft = 2;
	mDelay = !((aGames < 0xf) || mApp->mIsRegistered);

	mRegisterLink = new HyperlinkWidget(0, this);
	mRegisterLink->SetFont(mApp->mFonts[FONT_MT11]);
	mRegisterLink->mColor = Color(0xff, 0xff, 0);
	mRegisterLink->SetVisible(!mApp->mIsRegistered);
	mRegisterLink->mLabel = mDelay ? GT(0x004a6670) : GT(0x004a6694);

	mPlayLink = new HyperlinkWidget(1, this);
	mPlayLink->SetFont(mApp->mFonts[FONT_FONT1]);
	mPlayLink->SetVisible(false);
	mPlayLink->mColor = Color(0xff, 0xff, 0);
	mPlayLink->mLabel = GT(0x004a665c);

	for (int i = 0; i < 25; i++)
	{
		mSparkles[i][0] = abs(CrtRand() % 0x253) + 0x13;
		mSparkles[i][1] = abs(CrtRand() % 0x61) + 7;
		mSparkles[i][2] = abs(i % 0xe);
	}
}

// 0x425b80
TitleScreen::~TitleScreen()
{
	delete mRegisterLink;
	delete mPlayLink;
}

// 0x426180 / 0x4261b0
void TitleScreen::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mRegisterLink);
	theWidgetManager->AddWidget(mPlayLink);
}

void TitleScreen::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mRegisterLink);
	theWidgetManager->RemoveWidget(mPlayLink);
}

// 0x4261e0
void TitleScreen::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	int aWidth = mRegisterLink->mFont->StringWidth(mRegisterLink->mLabel);
	mRegisterLink->Resize((mWidth - aWidth) / 2, 0x1c0, aWidth, 0x10);
	aWidth = mPlayLink->mFont->StringWidth(mPlayLink->mLabel);
	mPlayLink->Resize((mWidth - aWidth) / 2, 0x177, aWidth, 0x18);
}

// 0x425fb0
void TitleScreen::Update()
{
	Widget::Update();
	if (!mDelay)
	{
		if ((mUpdateCnt % 4 == 0) && !mPlayShown)
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
	mRegisterLink->SetVisible(!mApp->mIsRegistered);
	if (mUpdateCnt % 8 == 0)
	{
		for (int i = 0; i < 25; i++)
		{
			mSparkles[i][2] = mSparkles[i][2] + 1;
			if (mSparkles[i][2] == 0xe)
			{
				mSparkles[i][2] = 0;
				mSparkles[i][0] = abs(CrtRand() % 0x253) + 0x13;
				mSparkles[i][1] = abs(CrtRand() % 0x61) + 7;
			}
		}
		MarkDirty();
	}
}

// 0x425c00
void TitleScreen::Draw(Graphics* g)
{
	Image** anImages = mApp->mImages;
	g->DrawImage(anImages[IMAGE_TITLE], 0, 0);
	g->DrawImage(anImages[IMAGE_ANCHOR], 10, 0x15f);
	int aBarWidth = mPlayShown ? 0x141 : (int) (mProgress * 321.0);
	g->DrawImage(anImages[IMAGE_GOLDBAR], 0x99, 0x197, Rect(0, 0, aBarWidth, 0x18));
	g->SetFont(mApp->mFonts[FONT_MT11]);
	g->DrawImage(anImages[IMAGE_POPCAPURL], 0x206, 0x1cb);
	g->DrawImage(anImages[IMAGE_POPCAPLOGO], 0x206, 0x16a);
	g->SetColor(Color(0xff, 0xff, 0));
	SexyString aLine = GT(0x004a66bc);
	WriteCenteredLine(g, 0x16f, aLine);
	g->SetColor(Color(0xff, 0xff, 0xff));
	SexyString aVersion = GT(0x004a66b0) + mApp->mProductVersion;
	g->DrawString(aVersion, 0x219 - g->GetFont()->StringWidth(aVersion) / 2, 0xdd);
	g->SetDrawMode(Graphics::DRAWMODE_ADDITIVE);
	for (int i = 0; i < 25; i++)
		g->DrawImage(anImages[IMAGE_SMSPARKLE], mSparkles[i][0], mSparkles[i][1], Rect(mSparkles[i][2] * 0x14, 0, 0x14, 0x14));
	g->SetDrawMode(Graphics::DRAWMODE_NORMAL);
}

// 0x426270: the play link starts the game's menus
void TitleScreen::ButtonDepress(int theId)
{
	if (theId == 0)
		mRegisterClicked = true;
	else
		mApp->TitleScreenDone();
}

// ---------------------------------------------------------------------------------------------------------
// IntroWidget

// 0x416900
IntroWidget::IntroWidget(SevenSeasApp* theApp) :
	mApp(theApp),
	mStarting(false)
{
	Image** anImages = mApp->mImages;
	static const int kIds[5] = {0, 1, 0xb, 0xc, 0xd};
	static const Rect kRects[5] = {Rect(0, 0x37, 0xb6, 0x37), Rect(0, 0, 0xb6, 0x37), Rect(0, 0, 0x76, 0x73), Rect(0x76, 0, 0x77, 0x73), Rect(0xed, 0, 0x76, 0x73)};
	ButtonWidget* aButtons[5];
	for (int i = 0; i < 5; i++)
	{
		aButtons[i] = new ButtonWidget(kIds[i], this);
		aButtons[i]->mDoFinger = true;
		Image* aSource = (i < 2) ? anImages[IMAGE_INTROBUTTONS] : anImages[IMAGE_INTROWHEELS];
		aButtons[i]->mDownImage = mApp->CopyImage(aSource, kRects[i]);
		aButtons[i]->mOverImage = mApp->CopyImage(aSource, kRects[i]);
		aButtons[i]->mMouseInsets = Insets(-1, -1, -1, -1);
	}
	mPlayButton = aButtons[0];
	mTutorialButton = aButtons[1];
	for (int i = 0; i < 3; i++)
		mWheels[i] = aButtons[2 + i];
	SetDifficulty(mApp->mDifficulty);
	mStarting = false;
}

IntroWidget::~IntroWidget()
{
	delete mPlayButton;
	delete mTutorialButton;
	for (int i = 0; i < 3; i++)
		delete mWheels[i];
}

// 0x4171c0: the wheel of the difficulty shows lit
void IntroWidget::SetDifficulty(int theDifficulty)
{
	mDifficulty = theDifficulty;
	for (int i = 0; i < 3; i++)
	{
		mWheels[i]->mButtonImage = (i == theDifficulty - 1) ? mWheels[i]->mDownImage : NULL;
		mWheels[i]->MarkDirty();
	}
}

// 0x417290 / 0x4172e0
void IntroWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mPlayButton);
	theWidgetManager->AddWidget(mTutorialButton);
	for (int i = 0; i < 3; i++)
		theWidgetManager->AddWidget(mWheels[i]);
}

void IntroWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mPlayButton);
	theWidgetManager->RemoveWidget(mTutorialButton);
	for (int i = 0; i < 3; i++)
		theWidgetManager->RemoveWidget(mWheels[i]);
}

// 0x417330
void IntroWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mPlayButton->Resize(mX + 0xee, 0x173, 0xb6, 0x37);
	mTutorialButton->Resize(mX + 0x2e, 0x173, 0xb6, 0x37);
	mWheels[0]->Resize(mX + 0x37, 0xf9, 0x76, 0x73);
	mWheels[1]->Resize(mX + 0xad, 0xf9, 0x77, 0x73);
	mWheels[2]->Resize(mX + 0x124, 0xf9, 0x76, 0x73);
}

// 0x417200: after the pirate has talked the game begins
void IntroWidget::Update()
{
	Widget::Update();
	// the pirate welcomes the player: the first time the menu shows after the program starts
	if (mApp->mFlag4c4 && (mUpdateCnt == 0x3c))
	{
		mApp->PlaySample(0xf);
		mApp->mTalkWidget->SetMouth(0);
	}
	if ((mUpdateCnt > 0x3c) && !mApp->mTalkWidget->mTalkingNow)
	{
		if (mStarting)
		{
			mStarting = false;
			mApp->EndTutorial();
			return;
		}
		mPlayButton->SetDisabled(false);
		mTutorialButton->SetDisabled(false);
		return;
	}
	mPlayButton->SetDisabled(true);
	mTutorialButton->SetDisabled(true);
}

// 0x40f750: the mouse position goes to the side panel too
void IntroWidget::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	mApp->FollowMouse(mX + x, y);
}

// 0x416df0: the hint is told by the button's clip flag (+0x19, set in every widget), so the hint is always shown
void IntroWidget::Draw(Graphics* g)
{
	Image** anImages = mApp->mImages;
	g->DrawImage(anImages[IMAGE_INTROSCREEN], 0, 0);
	g->SetFont(mApp->mFonts[FONT_MT22]);
	g->SetColor(Color(0xff, 0xff, 0xff));
	if (!mApp->mFlag4c4)
	{
		SexyString aText = GT(0x004a3c58);
		g->DrawString(aText, 0x12a - g->GetFont()->StringWidth(aText) / 2, 0x32);
	}
	else
	{
		SexyString aText = GT(0x004a3284);
		g->DrawString(aText, 0x12a - g->GetFont()->StringWidth(aText) / 2, 0x27);
		aText = GT(0x004a3c64);
		g->DrawString(aText, 0x12a - g->GetFont()->StringWidth(aText) / 2, 0x43);
	}
	g->SetFont(mApp->mFonts[FONT_MT11]);
	g->SetColor(Color(0xff, 0xff, 0));
	if (!mTutorialButton->mClip)
	{
		SexyString aText = GT(0x004a3bf8);
		g->DrawString(aText, 0x12a - g->GetFont()->StringWidth(aText) / 2, 0x71);
	}
	else
	{
		SexyString aText = GT(0x004a3c38);
		g->DrawString(aText, 0x12a - g->GetFont()->StringWidth(aText) / 2, 0x61);
		aText = GT(0x004a3c18);
		g->DrawString(aText, 0x12a - g->GetFont()->StringWidth(aText) / 2, 0x75);
	}
}

// 0x4173e0: Play starts the game at the chosen difficulty, Tutorial the tutorial, the wheels set the difficulty
void IntroWidget::ButtonDepress(int theId)
{
	switch (theId)
	{
	case 0:
	case 1:
	{
		int aTalk = CrtRand() % 3;
		if (aTalk == 0)
		{
			mApp->mTalkWidget->SetMouth(5);
			mApp->PlaySample(0x13);
		}
		else if (aTalk == 1)
		{
			mApp->mTalkWidget->SetMouth(6);
			mApp->PlaySample(0x14);
		}
		else if (aTalk == 2)
		{
			mApp->mTalkWidget->SetMouth(7);
			mApp->PlaySample(0x15);
		}
		if (theId == 0)
		{
			mApp->mTutorial = false;
			mApp->OnPlay(mDifficulty);
		}
		else
			mApp->mTutorial = true;
		mStarting = true;
		mApp->mMusicInterface->FadeOut(0, true, 0.004);
		mApp->mMusicInterface->FadeOut(1, true, 0.004);
		break;
	}
	case 0xb:
	case 0xc:
	case 0xd:
		SetDifficulty(theId - 10);
		break;
	}
}

// ---------------------------------------------------------------------------------------------------------
// MapWidget

// the route of the voyage: the dots of the path, the places marked (WinSS.exe 0x4a3c70, 0x4a3da4) and how many
// dots are lit in each chapter (0x4a3d84)
static const int kMapDots[35][2] =
{
	{254, 87}, {266, 102}, {280, 115}, {293, 129}, {304, 147}, {316, 164}, {324, 181}, {327, 196}, {296, 218},
	{277, 220}, {257, 218}, {237, 211}, {217, 202}, {200, 198}, {182, 195}, {165, 192}, {150, 191}, {122, 207},
	{126, 222}, {132, 237}, {138, 251}, {146, 266}, {182, 296}, {197, 300}, {212, 305}, {228, 309}, {240, 315},
	{285, 314}, {301, 312}, {319, 312}, {339, 314}, {352, 317}, {373, 335}, {363, 344}, {363, 359}
};
static const int kMapMarks[7][2] = {{222, 59}, {309, 202}, {108, 172}, {141, 275}, {246, 303}, {362, 302}, {352, 363}};
static const int kMapChapterDots[8] = {359, 0, 8, 17, 22, 27, 32, 35};

// 0x4175d0
MapWidget::MapWidget(SevenSeasApp* theApp) :
	mApp(theApp)
{
	mNextButton = new DialogButton2003(mApp->mImages[IMAGE_MAPBUTTONLIT], 100, this);
	mNextButton->mOverImage = mApp->CopyImage(mApp->mImages[IMAGE_MAPBUTTONLIT]);
	mNextButton->mButtonImage = mApp->CopyImage(mApp->mImages[IMAGE_MAPBUTTONUNLIT]);
	mNextButton->mDoFinger = true;
	mNextButton->mMouseInsets = Insets(-1, -1, -1, -1);
	mApp->mWidgetManager->AddWidget(mNextButton);
	mChapter = mApp->mBoard->mLevel / 10;
	if (mChapter < 1)
		mDots = 0;
	else
		mDots = kMapChapterDots[mChapter];
	mApp->SetCursor(0);
}

MapWidget::~MapWidget()
{
}

void MapWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
}

void MapWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
}

// 0x417980
void MapWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	Image* anImage = mApp->mImages[IMAGE_MAPBUTTONLIT];
	int aW = anImage->GetWidth();
	int aH = anImage->GetHeight();
	mNextButton->Resize((mWidth - aW) / 2 + 10 + mX, mHeight - aH, aW, aH);
}

// 0x40f750: the mouse position goes to the side panel too
void MapWidget::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	mApp->FollowMouse(mX + x, y);
}

// 0x4177d0
void MapWidget::Draw(Graphics* g)
{
	Image** anImages = mApp->mImages;
	g->DrawImage(anImages[IMAGE_PIRATEMAP], 0, 0);
	g->DrawImage(anImages[IMAGE_BACKDROP], 0, 0, Rect(0xad, 0, 0x1d3, 0x1e0));
	if (mChapter < 7)
	{
		for (int i = 0; i < mDots; i++)
		{
			if (i < kMapChapterDots[mChapter + 1])
				g->DrawImage(anImages[IMAGE_MAPDOT], kMapDots[i][0], kMapDots[i][1]);
		}
		for (int i = 0; i < mChapter; i++)
			g->DrawImage(anImages[IMAGE_MAPX], kMapMarks[i][0], kMapMarks[i][1]);
		if (kMapChapterDots[mChapter + 1] < mDots)
			g->DrawImage(anImages[IMAGE_MAPX], kMapMarks[mChapter][0], kMapMarks[mChapter][1]);
	}
	else
	{
		for (int i = 0; i < 35; i++)
			g->DrawImage(anImages[IMAGE_MAPDOT], kMapDots[i][0], kMapDots[i][1]);
		for (int i = 0; i < 7; i++)
			g->DrawImage(anImages[IMAGE_MAPX], kMapMarks[i][0], kMapMarks[i][1]);
	}
}

// 0x4179e0: the button closes the map
void MapWidget::ButtonDepress(int theId)
{
	if (theId == 100)
	{
		mApp->mWidgetManager->RemoveWidget(mNextButton);
		mApp->SafeDeleteWidget(mNextButton);
		mNextButton = NULL;
		mApp->mMapWidget = NULL;
		mApp->mWidgetManager->RemoveWidget(this);
		mApp->mWidgetManager->BringToFront(mApp->mBoard);
		mApp->mWidgetManager->SetFocus(mApp->mBoard);
		mApp->SafeDeleteWidget(this);
	}
}

// ---------------------------------------------------------------------------------------------------------
// SidePanel

// 0x41b560
SidePanel::SidePanel(SevenSeasApp* theApp) :
	mApp(theApp),
	mShownScore(0),
	mMouseX(0),
	mMouseY(0),
	mPupilX(0x15),
	mPupilY(0x33),
	mTargetX(0x15),
	mTargetY(0x33)
{
}

// 0x41ba60: the shown score runs after the board's, the pupils after the mouse
void SidePanel::Update()
{
	Widget::Update();
	Board* aBoard = mApp->mBoard;
	if (aBoard != NULL)
	{
		int aStep = aBoard->mSavedLevels;
		int aGap = aBoard->mScore - mShownScore;
		if (aGap < aStep * 0x96)
		{
			if (aStep * 100 <= aGap)
			{
				mShownScore = mShownScore + aStep * 2;
				MarkDirty();
			}
			else if (aStep * 0x32 <= aGap)
			{
				mShownScore = mShownScore + aStep;
				MarkDirty();
			}
			else if (mShownScore < aBoard->mScore)
			{
				if (mUpdateCnt % 2 == 0)
				{
					mShownScore = mShownScore + aStep;
					MarkDirty();
				}
			}
		}
		else
		{
			mShownScore = mShownScore + aStep * 3;
			MarkDirty();
		}
		if (aBoard->mScore < mShownScore)
		{
			mShownScore = aBoard->mScore;
			MarkDirty();
		}
	}
	// the target is the mouse position (kept by MouseMove) scaled to the skull's eye room: x over 640 times 12, y over
	// the panel's height times 14, both cut at the decimal point
	mTargetX = (int) (mMouseX * 0.0015625 * 12.0 + 21.0);
	mTargetY = (int) ((double) mMouseY / mHeight * 14.0 + 51.0);
	if (mPupilX > mTargetX)
		mPupilX = mPupilX - 1;
	if (mPupilX < mTargetX)
		mPupilX = mPupilX + 1;
	if (mPupilY > mTargetY)
		mPupilY = mPupilY - 1;
	if (mPupilY < mTargetY)
		mPupilY = mPupilY + 1;
}

// 0x41bba0
void SidePanel::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	mMouseX = x;
	mMouseY = y;
	mApp->SetCursor(0);
}

// 0x41b5a0
void SidePanel::Draw(Graphics* g)
{
	Image** anImages = mApp->mImages;
	g->DrawImage(anImages[IMAGE_BACKDROP], 0, 0, Rect(0, 0, 0xad, 0x1e0));
	g->DrawImage(anImages[IMAGE_SKULLBOTTOMLAYER], 0x26, 0x136);
	g->DrawImage(anImages[IMAGE_PUPIL], mPupilX + 0x26, mPupilY + 0x136);
	g->DrawImage(anImages[IMAGE_SKULLTOPLAYER], 0x26, 0x136);
	Board* aBoard = mApp->mBoard;
	char aBuf[64];
	if (aBoard != NULL)
	{
		// the lives in reserve, from the right
		int aLives = aBoard->mLives;
		int aX = 0x80;
		for (int aNum = 1; aNum < aLives; aNum++)
		{
			if (aX < -0xb)
				break;
			if (!((aNum == aLives - 1) && (0 < aBoard->mLifeFlash) && (aBoard->mLifeFlash / 10 % 2 == 0)))
				g->DrawImage(anImages[IMAGE_EXTRALIFE], aX, 0x6b);
			aX = aX - 0x23;
		}
		if (0 < aBoard->mLevel)
		{
			g->SetFont(mApp->mFonts[FONT_FONT1]);
			g->SetColor(Color(0xff, 0, 0));
			sprintf(aBuf, "%d", aBoard->mLevel);
			g->DrawString(aBuf, 0x59 - g->GetFont()->StringWidth(aBuf) / 2, 0x158);
		}
	}
	// the difficulty on the plate
	Graphics g2(*g);
	g2.ClipRect(0x35, 0xc6, 0x41, 0xd);
	g2.DrawImage(anImages[IMAGE_DIFFLEVEL], 0x36, mApp->mDifficulty * -0xd + 0xd3);
	g->SetFont(mApp->mFonts[FONT_SCORE]);
	g->SetColor(Color(0xff, 0xff, 0xff));
	sprintf(aBuf, "%d", mShownScore);
	g->DrawString(aBuf, 0x55 - g->GetFont()->StringWidth(aBuf) / 2, 0x61);
}

// ---------------------------------------------------------------------------------------------------------
// TalkWidget

// 0x4199f0
TalkWidget::TalkWidget(SevenSeasApp* theApp) :
	mApp(theApp),
	mProgress(0.0),
	mMode(1),
	mBobX(0.0),
	mBobY(0.0),
	mTalkingNow(false),
	mSet(0),
	mStep(0),
	mStepTime(0),
	mBlink(false)
{
	mHasTransparencies = true;
	mBlinkTick = mUpdateCnt;
}

// the mouth of each line (WinSS.exe 0x4a4008, 800 bytes a line): pairs of the mouth picture (0 closed, 1-4 the
// pictures after the eye) and the updates it stays, ended by a time of 0; the draw reads the picture of the step, the
// update the time
static const int kMouthSteps[8][21][2] =
{
	{{0, 1}, {2, 13}, {4, 8}, {1, 9}, {2, 11}, {4, 8}, {1, 12}, {3, 13}, {2, 13}, {4, 7}, {1, 7}, {2, 12}, {1, 7}, {0, 6}, {4, 7}, {2, 18}, {4, 12}, {4, 16}, {0, 0}},
	{{0, 1}, {3, 10}, {2, 8}, {0, 13}, {3, 8}, {2, 13}, {3, 5}, {2, 11}, {1, 9}, {2, 14}, {1, 10}, {3, 18}, {2, 13}, {1, 8}, {3, 6}, {2, 14}, {4, 10}, {1, 7}, {4, 17}, {1, 26}, {0, 0}},
	{{0, 1}, {3, 11}, {1, 8}, {3, 7}, {2, 14}, {4, 17}, {1, 7}, {2, 7}, {1, 5}, {2, 10}, {4, 7}, {0, 9}, {2, 9}, {1, 5}, {2, 13}, {1, 15}, {4, 25}, {1, 8}, {0, 0}},
	{{0, 1}, {3, 9}, {2, 21}, {1, 10}, {3, 8}, {2, 13}, {1, 11}, {3, 12}, {2, 29}, {1, 9}, {4, 27}, {0, 0}},
	{{0, 1}, {4, 5}, {2, 41}, {1, 17}, {3, 5}, {2, 13}, {1, 6}, {2, 11}, {1, 11}, {0, 0}},
	{{0, 1}, {3, 8}, {2, 12}, {1, 8}, {0, 7}, {1, 8}, {2, 32}, {1, 10}, {3, 6}, {2, 41}, {1, 15}, {4, 26}, {1, 20}, {0, 0}},
	{{0, 1}, {3, 18}, {2, 18}, {4, 20}, {3, 22}, {2, 22}, {3, 8}, {4, 8}, {1, 25}, {0, 0}},
	{{0, 1}, {2, 11}, {1, 8}, {2, 12}, {1, 8}, {3, 7}, {2, 5}, {1, 5}, {4, 27}, {1, 25}, {0, 0}}
};

// 0x419a70: the pirate says a line (the samples 0xe-0x15 are the lines); the mouth moves while it plays
void TalkWidget::SetMouth(int theSet)
{
	mTalkingNow = true;
	mSet = theSet;
	mStep = 0;
	mStepTime = kMouthSteps[theSet][0][1];
}

// 0x419ce0
void TalkWidget::Update()
{
	Widget::Update();
	if (mTalkingNow)
	{
		mStepTime--;
		if (mStepTime < 1)
		{
			mStep++;
			mStepTime = kMouthSteps[mSet][mStep][1];
			if (mStepTime == 0)
				mTalkingNow = false;
			MarkDirty();
		}
	}
	if (mUpdateCnt % 6 == 0)
	{
		mBobX = mBobX + 0.08;
		mBobY = mBobY + 0.2;
		while (6.28318 < mBobX)
			mBobX = mBobX - 6.28318;
		while (6.28318 < mBobY)
			mBobY = mBobY - 6.28318;
		MarkDirty();
	}
	if ((mMode > 0) && (mProgress < 1.0))
	{
		mProgress = mProgress + 0.01;
		if (1.0 <= mProgress)
			mProgress = 1.0;
		MarkDirty();
	}
	else if ((mMode < 0) && (0.0 < mProgress))
	{
		mProgress = mProgress - 0.01;
		if (mProgress <= 0.0)
			mProgress = 0.0;
		MarkDirty();
	}
	int aSince = mUpdateCnt - mBlinkTick;
	if ((aSince > 0x118) && !mBlink)
	{
		if (CrtRand() % 10 == 0)
		{
			mBlink = true;
			mBlinkTick = mUpdateCnt;
			MarkDirty();
		}
	}
	else if ((aSince >= 0x33) && mBlink)
	{
		if (CrtRand() % 10 == 0)
		{
			mBlink = false;
			mBlinkTick = mUpdateCnt;
			MarkDirty();
		}
	}
}

// 0x419ac0
void TalkWidget::Draw(Graphics* g)
{
	if ((0.001 <= mProgress) && (mApp->mMapWidget == NULL))
	{
		if (mProgress < 1.0)
		{
			g->SetColorizeImages(true);
			g->SetColor(Color(0xff, 0xff, 0xff, (int) (mProgress * 255.0)));
		}
		int aX = -(int) (sin(mBobX) * -4.0) + 4;
		int aY = -(int) (sin(mBobY) * -8.0);
		Image** anImages = mApp->mImages;
		g->DrawImage(anImages[IMAGE_PIRATEHEAD], aX, aY + 8);
		int aMouth = 0;
		if (mTalkingNow)
			aMouth = kMouthSteps[mSet][mStep][0];
		Image* aHead = anImages[IMAGE_PIRATEHEAD];
		if (aMouth != 0)
		{
			// the head in two slices around the mouth, the mouth at the bottom right
			Image* aMouthImage = anImages[IMAGE_PIRATEEYE + aMouth];
			int aRest = aHead->GetHeight() - aMouthImage->GetHeight();
			g->DrawImage(aHead, aX, aY + 8, Rect(0, 0, aHead->GetWidth(), aRest));
			g->DrawImage(aHead, aX, aY + 0x73, Rect(0, aRest, aHead->GetWidth() - aMouthImage->GetWidth(), aHead->GetHeight() - aRest));
			g->DrawImage(aMouthImage, aX + aHead->GetWidth() - aMouthImage->GetWidth(), aY + 8 + aRest);
		}
		if (mBlink && (mProgress >= 1.0))
			g->DrawImage(anImages[IMAGE_PIRATEEYE], aX + 0x5b, aY + 0x58);
		g->SetColorizeImages(false);
	}
}

// 0x419ed0
void TalkWidget::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	mApp->FollowMouse(mX + x, mY + y);
}

// ---------------------------------------------------------------------------------------------------------
// HighScoreWidget

static const int kRankThresholds[18] = {100, 200, 500, 1000, 2000, 4000, 6000, 8000, 10000, 12500, 15000, 18000, 22000, 25000, 30000, 40000, 50000, 75000};

// 0x4a3608..: Wharf Rat ... Grand Admiral: the rank names are read from the exe by their addresses
static const unsigned int kRankNameAddr[9] = {0x4a3708, 0x4a36f4, 0x4a36e0, 0x4a36cc, 0x4a36b8, 0x4a36a4, 0x4a3684, 0x4a3668, 0x4a3650};

const char* HighScoreWidget::RankName(int theScore)
{
	int aIdx = 0;
	if (kRankThresholds[0] < theScore)
	{
		do
		{
			aIdx++;
		}
		while ((aIdx < 17) && (kRankThresholds[aIdx] < theScore));
	}
	int aRank = aIdx / 2;
	if (aRank > 8)
		aRank = 8;
	return GT(kRankNameAddr[aRank]).c_str();
}

// 0x40ff70
HighScoreWidget::HighScoreWidget(SevenSeasApp* theApp) :
	mApp(theApp),
	mTodayScore(0),
	mShimmer(0),
	mReveal(0)
{
	mNewGameButton = new DialogButton2003(mApp->mImages[IMAGE_DBUTTONWOOD], 1, this);
	mNewGameButton->SetFont(mApp->mFonts[FONT_MT11]);
	mNewGameButton->mLabel = GT(0x004a3744);
	mNewGameButton->SetColor(DialogButton::COLOR_LABEL, Color(0xff, 0xe2, 0x3c));
	static const unsigned int kDefaultNames[5] = {0x004a373c, 0x004a3734, 0x004a372c, 0x004a3724, 0x004a3720};
	for (int i = 0; i < 5; i++)
	{
		mNames[i] = GT(kDefaultNames[i]);
		mScores[i] = 1000 - i * 100;
	}
	LoadScores();
}

HighScoreWidget::~HighScoreWidget()
{
	delete mNewGameButton;
}

void HighScoreWidget::AddedToManager(WidgetManager* theWidgetManager)
{
	Widget::AddedToManager(theWidgetManager);
	theWidgetManager->AddWidget(mNewGameButton);
}

void HighScoreWidget::RemovedFromManager(WidgetManager* theWidgetManager)
{
	Widget::RemovedFromManager(theWidgetManager);
	theWidgetManager->RemoveWidget(mNewGameButton);
}

// 0x412c00: the original also puts "Submit to Internet High Score List" at y 0x164; that button is not ported
void HighScoreWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
	Widget::Resize(theX, theY, theWidth, theHeight);
	mNewGameButton->Resize(mWidth / 2 - 0xb9 + mX, 0x184, 0x172, 0x1e);
}

// 0x412af0: the screen is shown from the top down, 6 pixels an update; when it is all there, the game says what the
// score earned (ShowScoreNote). The button grows out of the frame as the edge passes it. (The original's timer at
// +0xfc, which re-enables the internet score button, is not ported.)
void HighScoreWidget::Update()
{
	Widget::Update();
	if (mReveal < mHeight)
	{
		mReveal += 6;
		if (mReveal >= mHeight)
		{
			mReveal = mHeight;
			ShowScoreNote();
		}
		int aHeight = mY - 0x1c + mReveal - mNewGameButton->mY;
		if (aHeight < 1)
			aHeight = 0;
		else if (aHeight > 0x1d)
			aHeight = 0x1e;
		mNewGameButton->Resize(mNewGameButton->mX, mNewGameButton->mY, mNewGameButton->mWidth, aHeight);
		MarkDirty();
	}
	if (mUpdateCnt % 10 == 0)
	{
		mShimmer++;
		MarkDirty();
	}
}

// 0x411160: the place a score takes in the five best (1 to 5), or 0 when it is not good enough
int HighScoreWidget::PlaceFor(int theScore)
{
	int aPlace = 0;
	for (int i = 4; i >= 0; i--)
	{
		if (mScores[i] < theScore)
			aPlace = i + 1;
	}
	return aPlace;
}

// 0x410fb0: "1st", "2nd", "3rd", "4th" ... for the text of the note
static SexyString Ordinal(int thePlace)
{
	char aBuf[16];
	sprintf(aBuf, "%d", thePlace);
	unsigned int aSuffix = 0x004a3974;   // th
	if ((thePlace % 100) / 10 != 1)
	{
		switch (thePlace % 10)
		{
		case 1:
			aSuffix = 0x004a3968;
			break;
		case 2:
			aSuffix = 0x004a396c;
			break;
		case 3:
			aSuffix = 0x004a3970;
			break;
		}
	}
	return SexyString(aBuf) + GT(aSuffix);
}

// 0x410ae0: what the score of this game earned, said in the name dialog (the registered game's branch: the
// original asks an unregistered player to register instead)
void HighScoreWidget::ShowScoreNote()
{
	int aScore = mApp->mBoard != NULL ? mApp->mBoard->mScore : 0;
	bool aNotBest = aScore <= mTodayScore;
	int aPlace = PlaceFor(aScore);
	SexyString aMessage;
	if (aNotBest)
	{
		if (aPlace < 1)
			return;
		aMessage = GT(0x004a3880) + Ordinal(aPlace) + GT(0x004a37e0);
	}
	else if (aPlace < 1)
		aMessage = GT(0x004a37f4);
	else
		aMessage = GT(0x004a393c) + Ordinal(aPlace) + GT(0x004a381c);
	mApp->ShowRecordScore(aMessage);
}

// 0x4111f0: a line of the totals: the label at the left and the value at the right of a 0x91, 0x44, 0x125 x 0x56
// box, moved by theDX (the original's callers draw the labels and the values in two passes, the other part empty)
static void DrawTotalsLine(Graphics* g, int theDX, int theY, const SexyString& theLabel, const SexyString& theValue)
{
	g->DrawString(theLabel, 0x91 + 0x10 + theDX, theY + 0x44);
	g->DrawString(theValue, (0x125 - g->GetFont()->StringWidth(theValue)) + 0x91 - 0x10 + theDX, theY + 0x44);
}

// 0x40f750: the mouse position goes to the side panel too
void HighScoreWidget::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	mApp->FollowMouse(mX + x, y);
}

// 0x411260: the frame, the totals of the game (each line is drawn 25 times, black around the gold or yellow one,
// for an outline), the rank, the five best scores in colours that cycle, and the best of the day. Nothing shows
// below mReveal (Update makes it grow); the bottom edge of the frame is drawn at that line while it moves.
void HighScoreWidget::Draw(Graphics* g)
{
	Graphics aClipped(*g);
	aClipped.ClipRect(0, 0, mWidth, mReveal);
	Graphics* gc = &aClipped;
	Image** anImages = mApp->mImages;
	gc->DrawImageBox(Rect(0x14, 0x14, mWidth - 0x28, mHeight - 0x28), anImages[IMAGE_DIALOG]);
	gc->DrawImageBox(Rect(0x14, 0x14, mWidth - 0x28, mHeight - 0x28), anImages[IMAGE_DIALOG]);

	Board* aBoard = mApp->mBoard;
	int aScore = aBoard != NULL ? aBoard->mScore : 0;
	char aBuf[64];
	SexyString aValues[4];
	int aTime = aBoard != NULL ? aBoard->mSeconds : 0;
	sprintf(aBuf, GT(0x004a39ac).c_str(), aTime / 60, aTime % 60);
	aValues[0] = aBuf;
	sprintf(aBuf, "%d", aBoard != NULL ? aBoard->mKillsTotal : 0);
	aValues[1] = aBuf;
	sprintf(aBuf, "%d", aBoard != NULL ? aBoard->mTreasures : 0);
	aValues[2] = aBuf;
	sprintf(aBuf, "%d", aScore);
	aValues[3] = aBuf;
	static const unsigned int kLabels[4] = {0x004a39e4, 0x004a39d4, 0x004a39c4, 0x004a39b8};
	static const int kOffsets[5] = {-2, -1, 1, 2, 0};
	for (int aDY = 0; aDY < 5; aDY++)
	{
		for (int aDX = 0; aDX < 5; aDX++)
		{
			bool aCenter = (aDX == 4) && (aDY == 4);
			gc->SetFont(mApp->mFonts[FONT_MEDIUM]);
			gc->SetColor(aCenter ? Color(0xcf, 0xb8, 0) : Color(0, 0, 0));
			for (int i = 0; i < 4; i++)
				DrawTotalsLine(gc, kOffsets[aDX], kOffsets[aDY] + 0xd + i * 0x16, GT(kLabels[i]), "");
			gc->SetColor(aCenter ? Color(0xff, 0xff, 0) : Color(0, 0, 0));
			for (int i = 0; i < 4; i++)
				DrawTotalsLine(gc, kOffsets[aDX], kOffsets[aDY] + 0xd + i * 0x16, "", aValues[i]);
		}
	}

	// the rank: all in gold, then the name again in yellow
	SexyString aRankLine = StrFormat(GT(0x004a39a0).c_str(), RankName(aScore));
	SexyString aRankLabel = GT(0x004a3994);
	int aRankWidth = gc->GetFont()->StringWidth(aRankLine);
	gc->SetColor(Color(0xcf, 0xb8, 0));
	gc->DrawString(aRankLine, (mWidth - aRankWidth) / 2, 0xb2);
	gc->SetColor(Color(0xff, 0xff, 0));
	gc->DrawString(StrFormat(GT(0x004a399c).c_str(), RankName(aScore)),
		gc->GetFont()->StringWidth(aRankLabel) + (mWidth - aRankWidth) / 2, 0xb2);

	// the five best: the number in black, the name in a dark colour, the score in a light one, all of a hue that moves
	gc->DrawImage(anImages[IMAGE_HIGHSCOREPANEL], 0x24, 0xb9);
	gc->SetFont(mApp->mFonts[FONT_MEDIUM]);
	int aY = 0xf7;
	for (int aRow = 0; aY < 0x15b; aRow++, aY += 0x14)
	{
		int aHue = (aRow + 2 + mShimmer) * 10 % 0xaa - 0x55;
		if (aHue < 0)
			aHue = -aHue;
		Color aLight((int) mApp->HSLToRGB(aHue, 0xff, 0x96));
		Color aDark((int) mApp->HSLToRGB(aHue, 0xff, 0x60));
		gc->SetColor(Color(0, 0, 0));
		sprintf(aBuf, GT(0x004a3990).c_str(), aRow + 1);
		gc->DrawString(aBuf, 0x41, aY);
		gc->SetColor(aDark);
		gc->DrawString(mNames[aRow], 0x53, aY);
		gc->SetColor(aLight);
		sprintf(aBuf, "%d", mScores[aRow]);
		gc->DrawString(aBuf, 0x104 - gc->GetFont()->StringWidth(aBuf), aY);
	}

	// the best of the day, in the panel's right half
	gc->SetFont(mApp->mFonts[FONT_MEDIUM]);
	gc->SetColor(Color(0x6c, 0, 0));
	SexyString aToday = GT(0x004a3988);
	gc->DrawString(aToday, 0x159 - gc->GetFont()->StringWidth(aToday) / 2, 0xfb);
	SexyString aTop = GT(0x004a397c);
	gc->DrawString(aTop, 0x159 - gc->GetFont()->StringWidth(aTop) / 2, 0x113);
	if (mTodayScore == 0)
		strcpy(aBuf, GT(0x004a3978).c_str());
	else
		sprintf(aBuf, "%d", mTodayScore);
	gc->SetColor(Color(0xfc, 0, 0x0a));
	gc->DrawString(aBuf, 0x159 - gc->GetFont()->StringWidth(aBuf) / 2, 0x131);
	gc->SetColor(Color(0xfa, 0, 0));
	gc->DrawString(mTodayName, 0x159 - gc->GetFont()->StringWidth(mTodayName) / 2, 0x14d);

	// the bottom edge of the frame at the line that is being revealed
	Image* aFrame = anImages[IMAGE_DIALOG];
	int anEdge = mReveal - 0x13;
	if (anEdge < 0x1b2)
	{
		int aPieceW = aFrame->GetWidth() / 3;
		int aPieceH = aFrame->GetHeight() / 3;
		int aFrameW = mWidth - 0x28;
		Graphics anEdgeG(*g);
		anEdgeG.ClipRect(0xd, 0x29, 0x1d0, 0x19c);
		int aDestY = anEdge - aPieceH + 0x14;
		anEdgeG.DrawImage(aFrame, 0x14, aDestY, Rect(0, aPieceH * 2, aPieceW, aPieceH));
		int aRight = aFrameW - aPieceW;
		anEdgeG.DrawImage(aFrame, aRight + 0x14, aDestY, Rect(aPieceW * 2, aPieceH * 2, aPieceW, aPieceH));
		anEdgeG.ClipRect(aPieceW + 0x14, 0x14, aFrameW - aPieceW * 2, anEdge);
		int aCount = (aRight - 1) / aPieceW;
		for (int x = aPieceW + 0x14; aCount > 0; aCount--, x += aPieceW)
			anEdgeG.DrawImage(aFrame, x, aDestY, Rect(aPieceW, aPieceH * 2, aPieceW, aPieceH));
	}
}

void HighScoreWidget::ButtonDepress(int theId)
{
	if (theId == 1)
		mApp->ShowIntro();
}

// 0x410520: the day, as the file keeps it
static int TodayNumber()
{
	time_t aNow = time(NULL);
	struct tm* aTime = localtime(&aNow);
	return aTime->tm_yday + aTime->tm_year * 0x16e;
}

// 0x410550: hiscores.dat: the day, the best of the day, then the five best; each is a name (a length and bytes), a
// score and a check value. The best of the day is taken only when the day is today's and its check is good. The five
// keep their names always, and their scores when the check is good, else the score modulo 256 (C's signed %).
void HighScoreWidget::LoadScores()
{
	std::string aData;
	if (!Platform::ReadHostFile(Platform::VFS::Get().GetUserDir() + "/hiscores.dat", &aData))
		return;
	size_t aPos = 0;
	bool aOk = true;
	struct Local
	{
		static int Int(const std::string& d, size_t& p, bool& ok)
		{
			int v = 0;
			if (p + 4 > d.size())
				ok = false;
			else
			{
				memcpy(&v, d.data() + p, 4);
				p += 4;
			}
			return v;
		}
		static std::string Name(const std::string& d, size_t& p, bool& ok)
		{
			if (p + 2 > d.size())
			{
				ok = false;
				return "";
			}
			unsigned short aLen;
			memcpy(&aLen, d.data() + p, 2);
			p += 2;
			if (p + aLen > d.size())
			{
				ok = false;
				return "";
			}
			std::string s = d.substr(p, aLen);
			p += aLen;
			return s;
		}
		static unsigned int Check(const std::string& name, int score)
		{
			unsigned int aCheck = (unsigned int) score ^ 0x42beef;
			for (size_t i = 0; i < name.size(); i++)
				aCheck = aCheck ^ (unsigned int) ((signed char) name[i] * 0x11);
			return aCheck;
		}
	};
	int aDay = Local::Int(aData, aPos, aOk);
	std::string aTodayName = Local::Name(aData, aPos, aOk);
	int aTodayScore = Local::Int(aData, aPos, aOk);
	unsigned int aTodayCheck = (unsigned int) Local::Int(aData, aPos, aOk);
	if (aOk && (aDay == TodayNumber()) && (aTodayCheck == Local::Check(aTodayName, aTodayScore)))
	{
		mTodayName = aTodayName;
		mTodayScore = aTodayScore;
	}
	for (int i = 0; i < 5; i++)
	{
		std::string aName = Local::Name(aData, aPos, aOk);
		int aScore = Local::Int(aData, aPos, aOk);
		unsigned int aCheck = (unsigned int) Local::Int(aData, aPos, aOk);
		if (!aOk)
			return;
		mNames[i] = aName;
		if (aCheck == Local::Check(aName, aScore))
			mScores[i] = aScore;
		else
			mScores[i] = aScore % 256;
	}
}

// 0x410870
void HighScoreWidget::SaveScores()
{
	std::string aData;
	struct Local
	{
		static void Int(std::string& d, int v) { d.append((const char*) &v, 4); }
		static void Name(std::string& d, const std::string& n)
		{
			unsigned short aLen = (unsigned short) n.size();
			d.append((const char*) &aLen, 2);
			d.append(n);
		}
		static int Check(const std::string& name, int score)
		{
			unsigned int aCheck = (unsigned int) score ^ 0x42beef;
			for (size_t i = 0; i < name.size(); i++)
				aCheck = aCheck ^ (unsigned int) ((signed char) name[i] * 0x11);
			return (int) aCheck;
		}
	};
	Local::Int(aData, TodayNumber());
	Local::Name(aData, mTodayName);
	Local::Int(aData, mTodayScore);
	Local::Int(aData, Local::Check(mTodayName, mTodayScore));
	for (int i = 0; i < 5; i++)
	{
		Local::Name(aData, mNames[i]);
		Local::Int(aData, mScores[i]);
		Local::Int(aData, Local::Check(mNames[i], mScores[i]));
	}
	Platform::WriteUserFileAtomic("hiscores.dat", aData);
}

// 0x412cc0: the score of this game goes into the list under the name the player gave (an empty name adds nothing).
// The original saves the list when the widget is deleted; the port also saves here, so that closing the page keeps it.
void HighScoreWidget::AddScore(const SexyString& theName)
{
	if (theName.empty())
		return;
	int aScore = mApp->mBoard != NULL ? mApp->mBoard->mScore : 0;
	int aPlace = PlaceFor(aScore);
	if (mTodayScore < aScore)
	{
		mTodayScore = aScore;
		mTodayName = theName;
	}
	if (aPlace == 0)
	{
		SaveScores();
		return;
	}
	for (int i = 4; i > aPlace - 1; i--)
	{
		mNames[i] = mNames[i - 1];
		mScores[i] = mScores[i - 1];
	}
	mNames[aPlace - 1] = theName;
	mScores[aPlace - 1] = aScore;
	SaveScores();
}
