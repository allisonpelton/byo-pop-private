#include "Dynomite.h"
#include "ButtonWidget.h"
#include "Effects.h"
#include "Egg.h"
#include "EggGrid.h"
#include "FossilizedApp.h"
#include "Graphics.h"
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

// 0x404030
Dynomite::Dynomite()
{
	gBoardGlobal5304a0 = 0;
	mInitialized = false;
	mFieldB0 = false;
	mField74 = 0x160;
	mField80 = false;
	mApp = NULL;
	mField104 = 0;
	mField108 = 0;
	mPowerUpAngle = 0;
	mField130 = false;
	mField131 = false;
	mField13c = 0;
	mField138 = 0;
	mPowerUps.Clear();
	mTalker = NULL;
	mField148 = true;
	mChain = -1;
	mPauseCount = 0;
	mThunderbolt = NULL;
	mField134 = 0x15e;
	mField132 = false;
	mField1c8 = -0x1e0;
	mField1cc = 0;
	// TODO: +0x1d8 = 0
	mBabies = 0;
	mLongestChain = 0;
	mReplayButton = NULL;
	mField149 = true;
	mField158 = 0;
	mBonusBits = 0;
	mField15c = -1;
	mField160 = -1;
	mField16c = false;
	mRank = 0;
	mRankChanged = false;
	mField100 = false;
	mSong = 0;
	mField23c = 0;
	mField244 = false;
	mStartTicks = GetTickCount();
	mFieldB4 = 0;
	// port: the fields the original constructor leaves to Init (slot 46)
	mLauncher = NULL;
	mFirePrompt = NULL;
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
	// TODO: +0x1d8 (the high scores) comes with the game's end
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
	mField128 = 0;
	strcpy(mReplayText, mApp->GetString("PLAY_THIS_PUZZLE_AGAIN").c_str());
	mDifficulty = theDifficulty;
	mField68 = 0x132;
	mField6c = 0x99;
	mField70 = 0xfa;
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
	int x = mField70 - mField6c;
	if (x < mField6c + mField70)
	{
		Pic* aRope = mApp->mPicRope;
		do
		{
			aRope->Draw(x, mField74 + 0x21);
			aRope = mApp->mPicRope;
			x += aRope->mWidth;
		} while (x < mField6c + mField70);
	}
	mLauncher->Draw(g);
	mFlyingEggs.ResetIter();
	FlyingEgg* anEgg = mFlyingEggs.Next();
	while (anEgg != NULL)
	{
		if (anEgg->mField20 == false)
			anEgg->Draw();
		anEgg = mFlyingEggs.Next();
	}
	mLauncher->DrawSling(g);
	mFlyingEggs.ResetIter();
	anEgg = mFlyingEggs.Next();
	while (anEgg != NULL)
	{
		if (anEgg->mField20 == true)
			anEgg->Draw();
		anEgg = mFlyingEggs.Next();
	}
	mLauncher->DrawBand();
	DrawScenery();
	DrawMode();
	DrawEffects();
	mFirePrompt->Draw();
	if (mField132 == true)
		DrawGameOver(g);
	if (mTalker != NULL)
		mTalker->Draw();
}

// 0x406aa0: nothing while the game is in the background, and only the end's panel once it is shown
void Dynomite::Update()
{
	if (mApp->mActive != false)
	{
		if (mField132 == true)
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
		if ((mField149 == true) && (0x145 < mGrid->mPastLineFrames) && ((mBonusBits & 0x10) == 0))
		{
			if (mField130 == false)
				mApp->mSideBar->mInchy->Sink(0);
			mField130 = true;
			mGrid->mBreakPastLine = true;
		}
		UpdateBoard();
		if (mField131 == true)
		{
			mField134--;
			if ((mField134 == 0) && (mField132 == false))
			{
				mReplayButton = new ButtonWidget(5, mApp);
				mReplayButton->mButtonImage = mApp->mPicLargedbutton->mImage;
				mReplayButton->mDoFinger = true;
				mReplayButton->mLabel = mReplayText;
				mReplayButton->SetFont(mApp->mWhiteFont);
				mReplayButton->SetColor(ButtonWidget::COLOR_LABEL, Color(0xff, 0xff, 0xff));
				mReplayButton->SetColor(ButtonWidget::COLOR_LABEL_HILITE, Color(0xff, 0xff, 0xff));
				mApp->mWidgetManager->AddWidget(mReplayButton);
				mField132 = true;
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
	if ((mPauseCount == 0) && (mField244 == false))
		mLauncher->Aim(x, y);
}

// 0x404a30, the same function as MouseMove
void Dynomite::MouseDrag(int x, int y)
{
	Widget::MouseDrag(x, y);
	if ((mPauseCount == 0) && (mField244 == false))
		mLauncher->Aim(x, y);
}

// 0x404980: any button
void Dynomite::MouseDown(int x, int y, int theClickCount)
{
	if ((mField244 != true) && (mPauseCount < 1) && (mField80 != true) && (mThunderbolt == NULL) &&
		(mField130 == false) && (mField131 == false))
	{
		mLauncher->Fire();
		mFieldB4++;
		if ((gBoardGlobal5304a0 == 0) && (mApp->mInTutorial == false))
		{
			mApp->mMusicInterface->FadeIn(mSong, mField23c, 0.002, false);
			gBoardGlobal5304a0 = 1;
		}
		mField16c = true;
		mFieldB0 = true;
	}
}

// 0x405070
void Dynomite::UpdateGameOver()
{
	mField1c8 = (int) ((double) mField1c8 * (double) 0.9f);
	mReplayButton->Resize(400 - mApp->mPicLargedbutton->mWidth / 2, mField1c8 + 0x172, 0x17e, 0x41);
	MarkDirty();
}

// 0x406160
void Dynomite::UpdateBoard()
{
	int aSpin = mField128 + 1;
	if (0x167 < aSpin)
		aSpin = mField128 - 0x167;
	if (aSpin < 0)
		aSpin += 0x168;
	mField128 = aSpin;
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
	float aTarget = (float) mField104;
	if ((aGrid->mCeiling < aTarget) && (mField100 == false))
		aGrid->mCeiling = (float) (((double) aTarget - aGrid->mCeiling) * (double) 0.05f + aGrid->mCeiling);
	if (mField104 <= (int) mGrid->mCeiling)
		mField100 = true;
	if ((mFieldB0 == true) && (mField131 == false))
	{
		mField1cc++;
		if (mField1cc == 6000)
			mApp->mField4d8++;
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
	mApp->mPicLeftwall->Draw((mField70 - mField6c) - 0x38, 0);
	mApp->mPicRightwall->Draw(mField6c - 0x1e + mField70, 0);
	mSteggy->Draw();
	mTank->Draw();
	mApp->mPicEggpilesheet_c1->Draw(mField158 - 0x110 + mField70, 0x185);
	mApp->mPicEggpilesheet_c1->Draw(mField158 - 0x1a6 + mField70, 0x185);
	mApp->mPicEggpilesheet_c0->Draw(mField158 + 0x50 + mField70, 400);
	mApp->mPicEggpilesheet_c0->Draw(mField158 + 0xe6 + mField70, 400);
	mPowerUps.ResetIter();
	PowerUp* aPowerUp = mPowerUps.Next();
	while (aPowerUp != NULL)
	{
		aPowerUp->Draw(mField128);
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
		// port: the original measures both, and draws the yellow one, with its own English word, which is the
		// same as PAUSED's text; the port takes PAUSED's text for all four
		aFont->Draw(aText.c_str(), (mField70 - aFont->Width(aText.c_str()) / 2) + 4, 0x194);
		g->SetColor(Color(0xff, 0xff, 0, 0xff));
		aFont->Draw(aText.c_str(), mField70 - aFont->Width(aText.c_str()) / 2, 400);
		g->SetColorizeImages(false);
	}
}

// 0x4b1930
void Dynomite::DrawMode()
{
}

// 0x406f70 (TODO with the game's end: it needs the high scores at +0x1d8)
void Dynomite::DrawGameOver(Graphics* g)
{
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
		mFieldA0 = 0;
		mFieldA4 = 0;
		mFieldA8 = 400;
		mFieldAc = 400;
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
			mFieldA0 = aLeft;
			mClipRect[1] = aBoardTop;
			mClipRect[0] = aBoardLeft;
			mFieldA4 = aTop;
			mFieldA8 = (aRight - aBoardLeft) / 2 + aBoardLeft;
			mFieldAc = aBoardTop + 400;
			// TODO: ClipCursor(mClipRect) wants a platform call (a shared change); nothing sets LockCursor
			// until the registry and the options dialog are ported
			return;
		}
		// TODO: ClipCursor(NULL), as above
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
				if (aColor == mField15c)
					aColor = 0;
				aTries++;
			} while ((aTries <= 0x32) && (aColor == 0));
		}
		if (aTries <= 0x32)
			break;
		// past 50 tries the colour fired last is allowed once more; then the board's +0x160 or any colour
		if (mField15c == -1)
		{
			aColor = mField160;
			if (aColor == -1)
				aColor = mApp->mPicker.Random(mGrid->mMaxColor, false) + 1;
			break;
		}
		mField15c = -1;
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
		if (mField148 == false)
			return;
		mApp->mSideBar->mInchy->SayChain(0x32, mChain);
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

// 0x4b1930
void Dynomite::EggLanded()
{
}

// 0x40ad90 (TODO)
void Dynomite::Hint()
{
}
