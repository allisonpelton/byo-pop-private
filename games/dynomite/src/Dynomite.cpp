#include "Dynomite.h"
#include "ButtonWidget.h"
#include "Effects.h"
#include "Egg.h"
#include "EggGrid.h"
#include "FossilizedApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "HighScores.h"
#include "ImageFont.h"
#include "InchySmall.h"
#include "FirePrompt.h"
#include "Launcher.h"
#include "MenuTalker.h"
#include "MusicInterface.h"
#include "PortCompat.h"
#include "SideBar.h"
#include "SoundInstance.h"
#include "Sprite.h"
#include "Steggy.h"
#include "Tank.h"
#include "Thunderbolt.h"
#include "WidgetManager.h"
#include <cstdio>
#include <cstring>

using namespace Sexy;

int Sexy::gBoardGlobal5304a0 = 0;
std::vector<std::string> Sexy::gScoreNames;

// port(livesplit): the games' numbers, which the autosplit block tells apart (LiveSplitObserver.h)
unsigned int Dynomite::NextSerial()
{
	static unsigned int sSerials = 0;
	return ++sSerials;
}

// 0x404030
Dynomite::Dynomite()
{
	gBoardGlobal5304a0 = 0;
	mSerial = NextSerial();		// port(livesplit)
	mResumed = false;			// port(livesplit)
	mInitialized = false;
	mPlaying = false;
	mLineY = 0x160;
	mFireBlocked = false;
	mApp = NULL;
	mCeilingGoal = 0;
	mNextEggKind = 0;
	mPowerUpAngle = 0;
	mLosing = false;
	mGameOver = false;
	mShakeY = 0;
	mShakeX = 0;
	mGameOverAlpha = 0;	// port: the original leaves it to the mode
	mPowerUps.Clear();
	mTalker = NULL;
	mCountsChains = true;
	mChain = -1;
	mPauseCount = 0;
	mThunderbolt = NULL;
	mGameOverDelay = 0x15e;
	mGameOverShown = false;
	mPanelTop = -0x1e0;
	mPlayTicks = 0;
	mHighScores = NULL;
	mHighScoreTitle[0] = 0;	// port: the mode sets it
	mBabies = 0;
	mLongestChain = 0;
	mReplayButton = NULL;
	mPastLineLoses = true;
	mLauncherOffsetX = 0;
	mBonusBits = 0;
	mAvoidColor = -1;
	mLastFiredColor = -1;
	mFiredThisRound = false;
	mRank = 0;
	mRankChanged = false;
	mCeilingAtGoal = false;
	mSong = 0;
	mSongOffset = 0;
	mInputLocked = false;
	mStartTicks = GetTickCount();
	mShotCount = 0;
	// port: the fields the original constructor leaves to Init (slot 46)
	mLauncher = NULL;
	mFirePrompt = NULL;
	mSavedZero = 0;
	mSteggy = NULL;
	mTank = NULL;
	mGrid = NULL;
	mDifficulty = 0;
	mGameType = 0;
}

// 0x4041e0
Dynomite::~Dynomite()
{
	if (mApp != NULL)
	{
		mApp->mMusicInterface->FadeOut(mSong, true, 0.004);
		mApp->mMusicInterface->FadeOut(4, true, 0.004);
	}
	if (mReplayButton != NULL)
	{
		mApp->mWidgetManager->RemoveWidget(mReplayButton);
		mApp->SafeDeleteWidget(mReplayButton);
		mReplayButton = NULL;
	}
	if (mHighScores != NULL)
	{
		delete mHighScores;
		mHighScores = NULL;
	}
	if (mThunderbolt != NULL)
	{
		delete mThunderbolt;
		mThunderbolt = NULL;
	}
	if (mInitialized == true)
	{
		if (mApp->mFootfallRumbleInstance->IsPlaying() == true)
			mApp->mFootfallRumbleInstance->Stop();
		if (mApp->mGrindInstance->IsPlaying() == true)
			mApp->mGrindInstance->Stop();
		if (mApp->mTicktockInstance->IsPlaying() == true)
			mApp->mTicktockInstance->Stop();
		mFlyingEggs.ResetIter();
		FlyingEgg* anEgg = mFlyingEggs.Next();
		while (anEgg != NULL)
		{
			delete anEgg;
			anEgg = mFlyingEggs.Next();
		}
		mEffects.ResetIter();
		Effect* anEffect = mEffects.Next();
		while (anEffect != NULL)
		{
			delete anEffect;
			anEffect = mEffects.Next();
		}
		mPopups.ResetIter();
		anEffect = mPopups.Next();
		while (anEffect != NULL)
		{
			delete anEffect;
			anEffect = mPopups.Next();
		}
		mPowerUps.ResetIter();
		PowerUp* aPowerUp = mPowerUps.Next();
		while (aPowerUp != NULL)
		{
			delete aPowerUp;
			aPowerUp = mPowerUps.Next();
		}
		if (mGrid != NULL)
			delete mGrid;
		if (mLauncher != NULL)
			delete mLauncher;
		if (mFirePrompt != NULL)
			delete mFirePrompt;
		if (mSteggy != NULL)
			delete mSteggy;
		if (mTank != NULL)
			delete mTank;
		mInitialized = false;
	}
	if (mTalker != NULL)
	{
		delete mTalker;
		mTalker = NULL;
	}
}

// 0x407ac0
void Dynomite::Init(FossilizedApp* theApp, int theDifficulty)
{
	mApp = theApp;
	gScoreNames = mApp->GetStringVector("gScoreNames");
	mInitialized = true;
	mPowerUpAngle = 0;
	mPowerUpSpin = 0;
	strcpy(mReplayText, mApp->GetString("PLAY_THIS_PUZZLE_AGAIN").c_str());
	mDifficulty = theDifficulty;
	mPlayWidth = 0x132;
	mPlayHalfWidth = 0x99;
	mPlayMiddleX = 0xfa;
	mLauncher = new Launcher(theApp, this);
	mApp->mSideBar->mShowTime = false;
	mGrid = new EggGrid(theApp, this);
	mFlyingEggs.Clear();
	mFirePrompt = new FirePrompt(theApp, this);
	mEffects.Clear();
	mPopups.Clear();
	mSteggy = new Steggy(theApp, this);
	mTank = new Tank(theApp, this);
	mApp->mSideBar->mInchy->Rise(100);
	SetActive(true);
}

// 0x4044a0: white while a thunderbolt flashes; the flying eggs that have crossed the launcher's middle go
// behind the slingshot's front, the rest in front of it
void Dynomite::Draw(Graphics* g)
{
	SetDrawGraphics(g);
	if ((mThunderbolt != NULL) && (mThunderbolt->mBomb == true))
	{
		g->SetColor(Color(0xff, 0xff, 0xff, 0xff));
		g->FillRect(0, 0, 0x1ea, 0x1e0);
	}
	else
		mApp->mPicJungleback->Draw(0, 0);
	mGrid->Draw();
	int x = mPlayMiddleX - mPlayHalfWidth;
	if (x < mPlayHalfWidth + mPlayMiddleX)
	{
		Pic* aRope = mApp->mPicRope;
		do
		{
			aRope->Draw(x, mLineY + 0x21);
			aRope = mApp->mPicRope;
			x += aRope->mWidth;
		} while (x < mPlayHalfWidth + mPlayMiddleX);
	}
	mLauncher->Draw(g);
	mFlyingEggs.ResetIter();
	FlyingEgg* anEgg = mFlyingEggs.Next();
	while (anEgg != NULL)
	{
		if (anEgg->mInFrontOfSling == false)
			anEgg->Draw();
		anEgg = mFlyingEggs.Next();
	}
	mLauncher->DrawSling(g);
	mFlyingEggs.ResetIter();
	anEgg = mFlyingEggs.Next();
	while (anEgg != NULL)
	{
		if (anEgg->mInFrontOfSling == true)
			anEgg->Draw();
		anEgg = mFlyingEggs.Next();
	}
	mLauncher->DrawBand();
	DrawScenery();
	DrawMode();
	DrawEffects();
	mFirePrompt->Draw();
	if (mGameOverShown == true)
		DrawGameOver(g);
	if (mTalker != NULL)
		mTalker->Draw();
}

// 0x406aa0: nothing while the game is in the background, and only the end's panel once it is shown
void Dynomite::Update()
{
	if (mApp->mActive != false)
	{
		if (mGameOverShown == true)
		{
			UpdateGameOver();
			return;
		}
		if (mTalker != NULL)
			mTalker->Update();
		if ((mThunderbolt != NULL) && (mThunderbolt->Update() == false))
		{
			if (mThunderbolt != NULL)
				delete mThunderbolt;
			mThunderbolt = NULL;
		}
		mBonusBits = 0;
		mPowerUps.ResetIter();
		PowerUp* aPowerUp = mPowerUps.Next();
		while (aPowerUp != NULL)
		{
			if (aPowerUp->Update() == false)
			{
				mPowerUps.Remove(aPowerUp);
				if (aPowerUp != NULL)
					delete aPowerUp;
			}
			else
				mBonusBits |= aPowerUp->mFlag;
			aPowerUp = mPowerUps.Next();
		}
		mFirePrompt->Update();
		mLauncher->Update();
		mSteggy->Update();
		mTank->Update();
		mGrid->Update();
		if ((mPastLineLoses == true) && (0x145 < mGrid->mPastLineFrames) && ((mBonusBits & 0x10) == 0))
		{
			if (mLosing == false)
				mApp->mSideBar->mInchy->Sink(0);
			mLosing = true;
			mGrid->mBreakPastLine = true;
		}
		UpdateBoard();
		if (mGameOver == true)
		{
			mGameOverDelay--;
			if ((mGameOverDelay == 0) && (mGameOverShown == false))
			{
				mReplayButton = new ButtonWidget(5, mApp);
				mReplayButton->mButtonImage = mApp->mPicLargedbutton->mImage;
				mReplayButton->mDoFinger = true;
				mReplayButton->mLabel = mReplayText;
				mReplayButton->SetFont(mApp->mWhiteFont);
				mReplayButton->SetColor(ButtonWidget::COLOR_LABEL, Color(0xff, 0xff, 0xff));
				mReplayButton->SetColor(ButtonWidget::COLOR_LABEL_HILITE, Color(0xff, 0xff, 0xff));
				mApp->mWidgetManager->AddWidget(mReplayButton);
				mGameOverShown = true;
			}
		}
		MarkDirty();
	}
}

// 0x404930
void Dynomite::KeyDown(KeyCode theKey)
{
	if ((theKey == KEYCODE_SPACE) || (theKey == KEYCODE_ESCAPE))
		mApp->mSideBar->SetPauseState(0);
	if ((theKey == KEYCODE_F1) || (theKey == (KeyCode) 'P'))
		mApp->mSideBar->SetPauseState(0);
}

// 0x404a30
void Dynomite::MouseMove(int x, int y)
{
	Widget::MouseMove(x, y);
	if ((mPauseCount == 0) && (mInputLocked == false))
		mLauncher->Aim(x, y);
}

// 0x404a30, the same function as MouseMove
void Dynomite::MouseDrag(int x, int y)
{
	Widget::MouseDrag(x, y);
	if ((mPauseCount == 0) && (mInputLocked == false))
		mLauncher->Aim(x, y);
}

// 0x404980: any button
void Dynomite::MouseDown(int x, int y, int theClickCount)
{
	if ((mInputLocked != true) && (mPauseCount < 1) && (mFireBlocked != true) && (mThunderbolt == NULL) &&
		(mLosing == false) && (mGameOver == false))
	{
		mLauncher->Fire();
		mShotCount++;
		if ((gBoardGlobal5304a0 == 0) && (mApp->mInTutorial == false))
		{
			mApp->mMusicInterface->FadeIn(mSong, mSongOffset, 0.002, false);
			gBoardGlobal5304a0 = 1;
		}
		mFiredThisRound = true;
		mPlaying = true;
	}
}

// 0x405070
void Dynomite::UpdateGameOver()
{
	mPanelTop = (int) ((double) mPanelTop * (double) 0.9f);
	mReplayButton->Resize(400 - mApp->mPicLargedbutton->mWidth / 2, mPanelTop + 0x172, 0x17e, 0x41);
	MarkDirty();
}

// 0x406160
void Dynomite::UpdateBoard()
{
	int aSpin = mPowerUpSpin + 1;
	if (0x167 < aSpin)
		aSpin = mPowerUpSpin - 0x167;
	if (aSpin < 0)
		aSpin += 0x168;
	mPowerUpSpin = aSpin;
	mFlyingEggs.ResetIter();
	FlyingEgg* anEgg = mFlyingEggs.Next();
	while (anEgg != NULL)
	{
		if (anEgg->Update() == false)
		{
			mFlyingEggs.Remove(anEgg);
			if (mLauncher->mFlying == anEgg)
				mLauncher->mFlying = NULL;
			delete anEgg;
		}
		anEgg = mFlyingEggs.Next();
	}
	EggGrid* aGrid = mGrid;
	float aTarget = (float) mCeilingGoal;
	if ((aGrid->mCeiling < aTarget) && (mCeilingAtGoal == false))
		aGrid->mCeiling = (float) (((double) aTarget - aGrid->mCeiling) * (double) 0.05f + aGrid->mCeiling);
	if (mCeilingGoal <= (int) mGrid->mCeiling)
		mCeilingAtGoal = true;
	if ((mPlaying == true) && (mGameOver == false))
	{
		mPlayTicks++;
	}
	mEffects.ResetIter();
	Effect* anEffect = mEffects.Next();
	while (anEffect != NULL)
	{
		if (anEffect->Update() == false)
		{
			mEffects.Remove(anEffect);
			delete anEffect;
		}
		anEffect = mEffects.Next();
	}
	mPopups.ResetIter();
	anEffect = mPopups.Next();
	while (anEffect != NULL)
	{
		if (anEffect->Update() == false)
		{
			mPopups.Remove(anEffect);
			delete anEffect;
		}
		anEffect = mPopups.Next();
	}
}

// 0x404660
void Dynomite::DrawScenery()
{
	mApp->mPicLeftwall->Draw((mPlayMiddleX - mPlayHalfWidth) - 0x38, 0);
	mApp->mPicRightwall->Draw(mPlayHalfWidth - 0x1e + mPlayMiddleX, 0);
	mSteggy->Draw();
	mTank->Draw();
	mApp->mPicEggpilesheet_c1->Draw(mLauncherOffsetX - 0x110 + mPlayMiddleX, 0x185);
	mApp->mPicEggpilesheet_c1->Draw(mLauncherOffsetX - 0x1a6 + mPlayMiddleX, 0x185);
	mApp->mPicEggpilesheet_c0->Draw(mLauncherOffsetX + 0x50 + mPlayMiddleX, 400);
	mApp->mPicEggpilesheet_c0->Draw(mLauncherOffsetX + 0xe6 + mPlayMiddleX, 400);
	mPowerUps.ResetIter();
	PowerUp* aPowerUp = mPowerUps.Next();
	while (aPowerUp != NULL)
	{
		aPowerUp->Draw(mPowerUpSpin);
		aPowerUp = mPowerUps.Next();
	}
	if (mThunderbolt != NULL)
		mThunderbolt->Draw();
	Point anOffset = GetDrawOffset();
	SetDrawOffset(Point(0, 0));
	mApp->mPicMenuoverlap->Draw(0, 0);
	SetDrawOffset(anOffset);
}

// 0x406d60
void Dynomite::DrawEffects()
{
	mEffects.ResetIter();
	Effect* anEffect = mEffects.Next();
	while (anEffect != NULL)
	{
		anEffect->Draw();
		anEffect = mEffects.Next();
	}
	mPopups.ResetIter();
	anEffect = mPopups.Next();
	while (anEffect != NULL)
	{
		anEffect->Draw();
		anEffect = mPopups.Next();
	}
	if ((0 < mPauseCount) && (mApp->mInTutorial == false))
	{
		Graphics* g = GetDrawGraphics();
		g->SetColorizeImages(true);
		g->SetColor(Color(0, 0, 0, 0xff));
		std::string aText = mApp->GetString("PAUSED");
		PicFont* aFont = mApp->mAnnounceFont1928;
		// the shadow is PAUSED's text; both are measured, and the yellow one drawn, with the exe's own word
		aFont->Draw(aText.c_str(), (mPlayMiddleX - aFont->Width(GTC(0x4f7594)) / 2) + 4, 0x194);
		g->SetColor(Color(0xff, 0xff, 0, 0xff));
		aFont->Draw(GTC(0x4f7594), mPlayMiddleX - aFont->Width(GTC(0x4f7594)) / 2, 400);
		g->SetColorizeImages(false);
	}
}

// 0x4b1930
void Dynomite::DrawMode()
{
}

// 0x406f70: a dialog panel (mPanelTop from the top) with the game's time, the rank, the mode's title and its high
// scores; each text has a black shadow two pixels down and right (the labels' shadows are the properties' texts,
// the labels themselves the exe's)
void Dynomite::DrawGameOver(Graphics* g)
{
	mApp->mDialogCells[0]->Draw(10, mPanelTop);
	int x = 0x3a;
	do
	{
		mApp->mDialogCells[1]->Draw(x, mPanelTop);
		x = x + 0x30;
	} while (x < 0x1ba);
	mApp->mDialogCells[2]->Draw(x, mPanelTop);
	int aRight = x;
	int y = mPanelTop + 0x30;
	if (y < mPanelTop + 0x1b0)
	{
		do
		{
			mApp->mDialogCells[3]->Draw(10, y);
			int aX = 0x3a;
			do
			{
				mApp->mDialogCells[4]->Draw(aX, y);
				aX = aX + 0x30;
			} while (aX < 0x1ba);
			mApp->mDialogCells[5]->Draw(aRight, y);
			y = y + 0x30;
		} while (y < mPanelTop + 0x1b0);
	}
	mApp->mDialogCells[6]->Draw(10, y);
	x = 0x3a;
	do
	{
		mApp->mDialogCells[7]->Draw(x, y);
		x = x + 0x30;
	} while (x < 0x1ba);
	mApp->mDialogCells[8]->Draw(x, y);

	g->SetFont(mApp->mWhiteFontSmall);
	char aTime[32];
	int aSeconds = mPlayTicks / 100;
	sprintf(aTime, "%d:%.2d", aSeconds / 60, aSeconds % 60);
	int aTimeWidth = g->GetFont()->StringWidth(aTime);
	g->SetColor(Color(0, 0, 0));
	g->DrawString(mApp->GetString("GAME_TIME"), 0xe3, mPanelTop + 0x43);
	g->DrawString(aTime, 0x1b7 - aTimeWidth, mPanelTop + 0x43);
	g->SetColor(Color(0xff, 0xff, 0xff));
	g->DrawString(GT(0x4f75b4), 0xe1, mPanelTop + 0x41);
	g->DrawString(aTime, 0x1b5 - aTimeWidth, mPanelTop + 0x41);

	g->SetFont(mApp->mWhiteFont);
	g->SetColor(Color(0, 0, 0));
	g->DrawString(mApp->GetString("RANK"), 0xe3, mPanelTop + 0xbb);
	g->DrawString(gScoreNames[mRank], 0xe3, mPanelTop + 0xd2);
	g->SetColor(Color(0xff, 200, 200));
	g->DrawString(GT(0x4f75a4), 0xe1, mPanelTop + 0xb9);
	g->SetColor(Color(200, 0xff, 200));
	g->DrawString(gScoreNames[mRank], 0xe1, mPanelTop + 0xd0);

	int aHalf = g->GetFont()->StringWidth(mHighScoreTitle) / 2;
	g->SetColor(Color(0, 0, 0));
	g->DrawString(mHighScoreTitle, 0xf7 - aHalf, mPanelTop + 0x106);
	g->SetColor(Color(0xff, 0xff, 0xff));
	g->DrawString(mHighScoreTitle, 0xf5 - aHalf, mPanelTop + 0x104);
	g->SetFont(mApp->mWhiteFontSmall);
	mHighScores->Draw(g, 0x3a, mPanelTop + 0x10e, 0x180, 0x5a);
}

// 0x404fb0
void Dynomite::SetPaused(bool isPaused)
{
	if (isPaused == true)
		mPauseCount++;
	else
		mPauseCount--;
	if (mPauseCount < 0)
		mPauseCount = 0;
	SetActive(mPauseCount == 0);
}

// 0x4047e0: with the LockCursor option a windowed game keeps the cursor between the board's top left and the
// window's bottom right while the board is active
void Dynomite::SetActive(bool isActive)
{
	FossilizedApp* anApp = mApp;
	if (anApp->mLockCursor == false)
		isActive = false;
	if (anApp->mIsWindowed != false)
	{
		mClipRect[0] = 0;
		mClipRect[1] = 0;
		mClipRect[2] = 800;
		mClipRect[3] = 600;
		mWindowLeft = 0;
		mWindowTop = 0;
		mClipPointX = 400;
		mClipPointY = 400;
		if (isActive == true)
		{
			// port: the window's client area is the app's size at the screen's origin; the original asks
			// GetClientRect and ClientToScreen
			int aLeft = 0;
			int aTop = 0;
			int aRight = mApp->mWidth;
			int aBottom = mApp->mHeight;
			int aBoardTop = mY + aTop;
			mClipRect[2] = aRight;
			mClipRect[3] = aBottom;
			int aBoardLeft = mX + aLeft;
			mWindowLeft = aLeft;
			mClipRect[1] = aBoardTop;
			mClipRect[0] = aBoardLeft;
			mWindowTop = aTop;
			mClipPointX = (aRight - aBoardLeft) / 2 + aBoardLeft;
			mClipPointY = aBoardTop + 400;
			// port: the original clips the cursor to mClipRect here (ClipCursor). Only the registry's LockCursor turns it
			// on, which no dialog sets, and the platform has no call for it, so the port leaves it out (README.md)
			return;
		}
		// port: and frees it here (ClipCursor(NULL)), as above
	}
}

// 0x4060c0: the next one 20 degrees on round the circle
void Dynomite::AddPowerUp(int theKind)
{
	PowerUp* aPowerUp = new PowerUp(this, theKind, mPowerUpAngle);
	mPowerUps.Add(aPowerUp);
	int anAngle = mPowerUpAngle + 0x14;
	if (0x167 < anAngle)
		anAngle = mPowerUpAngle - 0x154;
	if (anAngle < 0)
		anAngle = anAngle + 0x168;
	mPowerUpAngle = anAngle;
}

// 0x404f30
void Dynomite::GetPowerUpCenter(int* x, int* y)
{
}

// 0x404a70
Egg* Dynomite::NextEgg()
{
	unsigned char aPresent[20];
	memset(aPresent, 0, sizeof(aPresent));
	EggGrid* aGrid = mGrid;
	int aFirstRow = (int) (aGrid->mRows - 10.0f);
	if (aFirstRow < 0)
		aFirstRow = 0;
	int aCol = 0;
	int i = 0xf;
	do
	{
		int aRow = aFirstRow;
		if ((float) aFirstRow <= mGrid->mRows)
		{
			do
			{
				unsigned int aColor = mGrid->mCells[aCol][aRow].mBits & 0xff;
				if ((aColor != 0) && (aColor < 0x12))
					aPresent[aColor] = 1;
				aRow++;
			} while ((float) aRow <= mGrid->mRows);
		}
		aCol++;
		i--;
	} while (i != 0);
	int aTries = 0;
	int aColor = 0;
	aPresent[0] = 0;
	while (true)
	{
		if (aColor == 0)
		{
			do
			{
				aColor = mApp->mPicker.Random(0x14, false);
				if (aPresent[aColor] == 0)
					aColor = 0;
				if (aColor == mAvoidColor)
					aColor = 0;
				aTries++;
			} while ((aTries <= 0x32) && (aColor == 0));
		}
		if (aTries <= 0x32)
			break;
		// past 50 tries the colour fired last is allowed once more; then mLastFiredColor or any colour
		if (mAvoidColor == -1)
		{
			aColor = mLastFiredColor;
			if (aColor == -1)
				aColor = mApp->mPicker.Random(mGrid->mMaxColor, false) + 1;
			break;
		}
		mAvoidColor = -1;
	}
	while (aColor == 0)
		aColor = mApp->mRand.NextNoAssert() % (unsigned int) mGrid->mMaxColor + 1;
	Egg* anEgg = new Egg();
	anEgg->Init(mApp, this, aColor);
	return anEgg;
}

// 0x404c30
void Dynomite::AddPoints(int x, int y, int thePoints, int theMultiplier, bool isChain)
{
	char aText[100];
	if (mApp->mInTutorial == true)
		return;
	if (0 < theMultiplier)
		thePoints = thePoints * theMultiplier;
	if ((1 < mChain) && (isChain == true))
	{
		int aBonus = 0;
		switch (mChain)
		{
		case 2:
			aBonus = 3;
			break;
		case 3:
			aBonus = 4;
			break;
		case 4:
			aBonus = 6;
			break;
		case 5:
			aBonus = 8;
			break;
		case 6:
			aBonus = 10;
			break;
		case 7:
			aBonus = 0xf;
			break;
		}
		thePoints = thePoints + aBonus;
	}
	if (mDifficulty == 1)
	{
		thePoints = thePoints / 5;
		if (thePoints < 1)
			thePoints = 1;
	}
	else if (mDifficulty == 3)
		thePoints = thePoints * 2;
	int aScore = thePoints * 10;
	sprintf(aText, "+%d", aScore);
	if ((mBonusBits & 2) != 0)
	{
		strcat(aText, "x2");
		aScore = aScore * 2;
	}
	if ((mBonusBits & 4) != 0)
	{
		strcat(aText, "x3");
		aScore = aScore * 3;
	}
	if ((mBonusBits & 8) != 0)
	{
		strcat(aText, "x4");
		aScore = aScore << 2;
	}
	ScorePopup* aPopup = new ScorePopup();
	aPopup->Init(x, y, aText, mApp);
	mPopups.Add(aPopup);
	mApp->mSideBar->mScore = mApp->mSideBar->mScore + aScore;
	int anOldRank = mRank;
	int* aRank = &mScoreRanks[1];
	int i = 2;
	int aNext;
	do
	{
		if (aRank[-1] < mApp->mSideBar->mScore)
			mRank = i - 2;
		if (aRank[0] < mApp->mSideBar->mScore)
			mRank = i - 1;
		if (aRank[1] < mApp->mSideBar->mScore)
			mRank = i;
		if (aRank[2] < mApp->mSideBar->mScore)
			mRank = i + 1;
		if (aRank[3] < mApp->mSideBar->mScore)
			mRank = i + 2;
		if (aRank[4] < mApp->mSideBar->mScore)
			mRank = i + 3;
		if (aRank[5] < mApp->mSideBar->mScore)
			mRank = i + 4;
		if (aRank[6] < mApp->mSideBar->mScore)
			mRank = i + 5;
		if (aRank[7] < mApp->mSideBar->mScore)
			mRank = i + 6;
		if (aRank[8] < mApp->mSideBar->mScore)
			mRank = i + 7;
		aNext = i + 8;
		aRank = aRank + 10;
		i = i + 10;
	} while (aNext < 0x14);
	if (mRank != anOldRank)
		mRankChanged = true;
}

// 0x404f40
void Dynomite::Chain(int x, int y, bool isMatch)
{
	if (isMatch != false)
	{
		if (mCountsChains == false)
			return;
		mApp->mSideBar->mInchy->Combo(0x32, mChain);
		int aChain = mChain + 1;
		mChain = aChain;
		if (mLongestChain < aChain)
			mLongestChain = aChain;
		if (aChain < 8)
			return;
	}
	mChain = -1;
}

// 0x405010
void Dynomite::MakeDynamite()
{
	mThunderbolt = new Thunderbolt(mApp, this);
}

// 0x4b1930
void Dynomite::Blasted()
{
}

// 0x4050d0
void Dynomite::EggAlone(int theColor)
{
}

// 0x406040
void Dynomite::DrawPiece(int theColor, int x, int y)
{
}

// 0x4050d0
void Dynomite::PieceFell(int theColor)
{
}

// 0x4050e0
void Dynomite::CreateSaveFile(const char* theName)
{
	mBinFile.Create(theName);
}

// 0x4050f0
void Dynomite::WriteSaveHeader()
{
	mBinFile.Write(mApp->mSaveMode, 0x14);
	mBinFile.Write(mApp->mSaveName, 0x14);
	mBinFile.Write(mApp->mSavePic->GetBits(), 0x3840);
}

// 0x405140. Where the original writes two neighbouring fields as one block of 8 bytes, the port writes them one at
// a time (the same bytes), and a float the original writes through WriteInt goes through WriteFloat.
void Dynomite::Save()
{
	mBinFile.WriteInt((int) GetTickCount());
	mBinFile.WriteBool(gBoardGlobal5304a0 != 0);
	mBinFile.WriteInt((int) mStartTicks);
	mBinFile.WriteInt(mApp->mSideBar->mScore);
	mBinFile.WriteInt(mDifficulty);
	mBinFile.WriteInt(mGameType);
	mBinFile.WriteInt(mSavedZero);
	mBinFile.WriteInt(mPlayWidth);
	mBinFile.WriteInt(mPlayHalfWidth);
	mBinFile.WriteInt(mPlayMiddleX);
	mBinFile.WriteInt(mLineY);
	mBinFile.WriteBool(mPlaying);
	mBinFile.WriteBool(mInitialized);
	mBinFile.WriteInt(mCeilingGoal);
	mBinFile.WriteInt(mNextEggKind);
	mBinFile.WriteInt(mPowerUpAngle);
	mBinFile.WriteInt(mPowerUpSpin);
	mBinFile.WriteInt(mBonusBits);
	mBinFile.WriteBool(mLosing);
	mBinFile.WriteBool(mGameOver);
	mBinFile.WriteBool(mGameOverShown);
	mBinFile.WriteInt(mGameOverDelay);
	mBinFile.WriteInt(mShakeX);
	mBinFile.WriteInt(mShakeY);
	mBinFile.WriteInt(mGameOverAlpha);
	mBinFile.WriteInt(mChain);
	mBinFile.WriteInt(mLauncherOffsetX);
	mBinFile.WriteInt(mAvoidColor);
	mBinFile.WriteInt(mLastFiredColor);
	mBinFile.WriteInt(mPanelTop);
	mBinFile.WriteInt(mPlayTicks);
	mBinFile.WriteInt(mLongestChain);
	mBinFile.WriteInt(mBabies);
	mBinFile.WriteString(mHighScoreTitle);
	mBinFile.WriteBool(mFiredThisRound);
	mBinFile.WriteBool(mCeilingAtGoal);
	mBinFile.WriteInt(mRank);
	mBinFile.WriteBool(mRankChanged);
	mBinFile.WriteBool(mFireBlocked);
	mGrid->SaveState(&mBinFile);
	mLauncher->SaveState(&mBinFile);
	mSteggy->SaveState(&mBinFile);
	mFirePrompt->SaveState(&mBinFile);
	if (mThunderbolt == NULL)
		mBinFile.WriteBool(false);
	else
	{
		mBinFile.WriteBool(true);
		mThunderbolt->SaveState(&mBinFile);
	}
	mFlyingEggs.ResetIter();
	FlyingEgg* anEgg = mFlyingEggs.Next();
	while (anEgg != NULL)
	{
		mBinFile.WriteBool(true);
		mBinFile.WriteInt(anEgg->mColor);
		mBinFile.WriteInt(anEgg->mKind);
		mBinFile.WriteFloat(anEgg->mSparkle);
		mBinFile.WriteFloat(anEgg->mX);
		mBinFile.WriteFloat(anEgg->mY);
		mBinFile.WriteFloat(anEgg->mVX);
		mBinFile.WriteFloat(anEgg->mVY);
		mBinFile.WriteInt(anEgg->mCol);
		mBinFile.WriteInt(anEgg->mRow);
		mBinFile.WriteBool(anEgg->mInFrontOfSling);
		mBinFile.WriteBool(anEgg->mGhost);
		anEgg = mFlyingEggs.Next();
	}
	mBinFile.WriteBool(false);
	mPowerUps.ResetIter();
	PowerUp* aPowerUp = mPowerUps.Next();
	while (aPowerUp != NULL)
	{
		mBinFile.WriteBool(true);
		aPowerUp->SaveState(&mBinFile);
		aPowerUp = mPowerUps.Next();
	}
	mBinFile.WriteBool(false);
	mEffects.ResetIter();
	Effect* anEffect = mEffects.Next();
	while (anEffect != NULL)
	{
		mBinFile.WriteBool(true);
		mBinFile.WriteInt(anEffect->mType);
		anEffect->Save(&mBinFile);
		anEffect = mEffects.Next();
	}
	mBinFile.WriteBool(false);
	mPopups.ResetIter();
	anEffect = mPopups.Next();
	while (anEffect != NULL)
	{
		mBinFile.WriteBool(true);
		mBinFile.WriteInt(anEffect->mType);
		anEffect->Save(&mBinFile);
		anEffect = mPopups.Next();
	}
	mBinFile.WriteBool(false);
}

// 0x405540
void Dynomite::OpenSaveFile(const char* theName)
{
	mBinFile.Open(theName, true);
}

// 0x405560
void Dynomite::SkipSaveHeader()
{
	mBinFile.Skip(0x3868);
}

// 0x405580
void Dynomite::CloseSaveFile()
{
	mBinFile.Close();
}

// port: split out of Load 0x405590: the effect of a saved kind, made as the original does before its Load reads the
// rest. The original has this switch twice, for the effects and the popups; a kind above 9 makes nothing and gives the last one again.
static Effect* NewSavedEffect(int theType, FossilizedApp* theApp, Dynomite* theBoard, Effect* theLast)
{
	switch (theType)
	{
	case 0:
		return new Effect();
	case 1:
	{
		FallingEggEffect* anEffect = new FallingEggEffect();
		anEffect->Init(theApp, 0, 0, 1, 0);
		return anEffect;
	}
	case 2:
	{
		YolkEffect* anEffect = new YolkEffect();
		anEffect->Init(0, 0, true, 0, theApp);
		return anEffect;
	}
	case 3:
	{
		EggWhiteEffect* anEffect = new EggWhiteEffect();
		anEffect->Init(0, 0, 0, theApp);
		return anEffect;
	}
	case 4:
	{
		BoomEffect* anEffect = new BoomEffect();
		anEffect->Init(0, 0, theApp, 1.0f);
		return anEffect;
	}
	case 5:
	{
		ShellEffect* anEffect = new ShellEffect();
		anEffect->Init(0, 0, 0, 1, 0, theApp);
		return anEffect;
	}
	case 6:
	{
		ScorePopup* anEffect = new ScorePopup();
		anEffect->Init(0, 0, "No", theApp);
		return anEffect;
	}
	case 7:
	{
		BabyEffect* anEffect = new BabyEffect();
		anEffect->Init(0, 0, 0, theApp);
		return anEffect;
	}
	case 8:
	{
		BonusItemEffect* anEffect = new BonusItemEffect();
		anEffect->Init(0, 0, 1, 5, 5, theBoard);
		return anEffect;
	}
	case 9:
	{
		ConfettiEffect* anEffect = new ConfettiEffect();
		anEffect->Init(5, 5, theApp);
		return anEffect;
	}
	}
	return theLast;
}

// 0x405590: the picker is seeded with the tick count saved, and the music comes back if it was on
void Dynomite::Load()
{
	mApp->mPicker.Seed((unsigned int) mBinFile.ReadInt());
	bool wasPlaying = mBinFile.ReadBool();
	gBoardGlobal5304a0 = wasPlaying;
	if (wasPlaying == true)
		mApp->mMusicInterface->FadeIn(mSong, mSongOffset, 0.002, false);
	mStartTicks = (unsigned int) mBinFile.ReadInt();
	mApp->mSideBar->mScore = mBinFile.ReadInt();
	mDifficulty = mBinFile.ReadInt();
	mGameType = mBinFile.ReadInt();
	mSavedZero = mBinFile.ReadInt();
	mPlayWidth = mBinFile.ReadInt();
	mPlayHalfWidth = mBinFile.ReadInt();
	mPlayMiddleX = mBinFile.ReadInt();
	mLineY = mBinFile.ReadInt();
	mPlaying = mBinFile.ReadBool();
	mInitialized = mBinFile.ReadBool();
	mCeilingGoal = mBinFile.ReadInt();
	mNextEggKind = mBinFile.ReadInt();
	mPowerUpAngle = mBinFile.ReadInt();
	mPowerUpSpin = mBinFile.ReadInt();
	mBonusBits = mBinFile.ReadInt();
	mLosing = mBinFile.ReadBool();
	mGameOver = mBinFile.ReadBool();
	mGameOverShown = mBinFile.ReadBool();
	mGameOverDelay = mBinFile.ReadInt();
	mShakeX = mBinFile.ReadInt();
	mShakeY = mBinFile.ReadInt();
	mGameOverAlpha = mBinFile.ReadInt();
	mChain = mBinFile.ReadInt();
	mLauncherOffsetX = mBinFile.ReadInt();
	mAvoidColor = mBinFile.ReadInt();
	mLastFiredColor = mBinFile.ReadInt();
	mPanelTop = mBinFile.ReadInt();
	mPlayTicks = mBinFile.ReadInt();
	mLongestChain = mBinFile.ReadInt();
	mBabies = mBinFile.ReadInt();
	mBinFile.ReadString(mHighScoreTitle);
	mFiredThisRound = mBinFile.ReadBool();
	mCeilingAtGoal = mBinFile.ReadBool();
	mRank = mBinFile.ReadInt();
	mRankChanged = mBinFile.ReadBool();
	mFireBlocked = mBinFile.ReadBool();
	mGrid->LoadState(&mBinFile);
	mLauncher->LoadState(&mBinFile);
	mSteggy->LoadState(&mBinFile);
	mFirePrompt->LoadState(&mBinFile);
	if (mThunderbolt != NULL)
	{
		delete mThunderbolt;
		mThunderbolt = NULL;
	}
	if (mBinFile.ReadBool() == true)
	{
		mThunderbolt = new Thunderbolt(mApp, this);
		mThunderbolt->LoadState(&mBinFile);
	}

	mFlyingEggs.ResetIter();
	FlyingEgg* anEgg = mFlyingEggs.Next();
	while (anEgg != NULL)
	{
		delete anEgg;
		anEgg = mFlyingEggs.Next();
	}
	mFlyingEggs.Clear();
	while (mBinFile.ReadBool())
	{
		anEgg = new FlyingEgg();
		anEgg->mBoard = this;
		anEgg->mApp = mApp;
		anEgg->mColor = mBinFile.ReadInt();
		anEgg->mKind = mBinFile.ReadInt();
		anEgg->mSparkle = mBinFile.ReadFloat();
		anEgg->mX = mBinFile.ReadFloat();
		anEgg->mY = mBinFile.ReadFloat();
		anEgg->mVX = mBinFile.ReadFloat();
		anEgg->mVY = mBinFile.ReadFloat();
		anEgg->mCol = mBinFile.ReadInt();
		anEgg->mRow = mBinFile.ReadInt();
		anEgg->mInFrontOfSling = mBinFile.ReadBool();
		anEgg->mGhost = mBinFile.ReadBool();
		mFlyingEggs.Add(anEgg);
	}

	mPowerUps.ResetIter();
	PowerUp* aPowerUp = mPowerUps.Next();
	while (aPowerUp != NULL)
	{
		delete aPowerUp;
		aPowerUp = mPowerUps.Next();
	}
	mPowerUps.Clear();
	while (mBinFile.ReadBool())
	{
		aPowerUp = new PowerUp(this, 4, 0);
		aPowerUp->LoadState(&mBinFile);
		mPowerUps.Add(aPowerUp);
	}

	mEffects.ResetIter();
	Effect* anEffect = mEffects.Next();
	while (anEffect != NULL)
	{
		delete anEffect;
		anEffect = mEffects.Next();
	}
	mEffects.Clear();
	anEffect = NULL;
	while (mBinFile.ReadBool())
	{
		anEffect = NewSavedEffect(mBinFile.ReadInt(), mApp, this, anEffect);
		if (anEffect == NULL)	// port: a kind above 9 first (the original loads into what it made last)
			return;
		anEffect->Load(&mBinFile);
		mEffects.Add(anEffect);
	}

	mPopups.ResetIter();
	anEffect = mPopups.Next();
	while (anEffect != NULL)
	{
		delete anEffect;
		anEffect = mPopups.Next();
	}
	mPopups.Clear();
	anEffect = NULL;
	while (mBinFile.ReadBool())
	{
		anEffect = NewSavedEffect(mBinFile.ReadInt(), mApp, this, anEffect);
		if (anEffect == NULL)	// port: as above
			return;
		anEffect->Load(&mBinFile);
		mPopups.Add(anEffect);
	}
}

// 0x4b1930
void Dynomite::EggLanded()
{
}
