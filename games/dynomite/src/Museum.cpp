#include "Museum.h"
#include "BinFile.h"
#include "ButtonWidget.h"
#include "CellButton.h"
#include "DynomiteFossil.h"
#include "Fossil.h"
#include "FossilizedApp.h"
#include "GameText.h"
#include "Graphics.h"
#include "ImageFont.h"
#include "InchySmall.h"
#include "MusicInterface.h"
#include "SideBar.h"
#include "Sprite.h"
#include "WidgetManager.h"

using namespace Sexy;

// 0x52b320: where each fossil hangs in the gallery (x, y), by its number
static const int gMuseumPos[21][2] =
{
	{1, 0}, {70, 40}, {220, 186}, {457, 40}, {703, 19}, {1012, 92}, {1299, 56}, {1610, 21}, {1852, 36}, {2222, 18},
	{2670, 16}, {3050, 20}, {3447, 21}, {3915, 21}, {4345, 19}, {4833, 19}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
};

// 0x42c070
RexAttack::RexAttack(Museum* theMuseum)
{
	mMuseum = theMuseum;
	mSndFootstep = new GameSound(mMuseum->mApp, "sounds\\footstep");
	mSndRoar = new GameSound(mMuseum->mApp, "sounds\\roar");
	mPicCrack = new Pic(mMuseum->mApp, "images\\fossils\\rexcrack", true);
	mPicHole = new Pic(mMuseum->mApp, "images\\fossils\\rexhole", true);
	mPicRex = new Pic(mMuseum->mApp, "images\\fossils\\tyrannosaurusattack", true);
	Cell** aCell = mCells;
	int i = 0;
	do
	{
		*aCell = new Cell(mPicRex, 0x7d, 0xd8, i);
		i++;
		aCell++;
	} while (i < 10);
	mCells[10] = new Cell(mPicRex, 0x7d, 0xd8, 3);
	mCells[11] = new Cell(mPicRex, 0x7d, 0xd8, 2);
	mCells[12] = new Cell(mPicRex, 0x7d, 0xd8, 1);
	mCells[13] = new Cell(mPicRex, 0x7d, 0xd8, 10);
	mCells[14] = new Cell(mPicRex, 0x7d, 0xd8, 0xb);
	mCells[15] = new Cell(mPicRex, 0x7d, 0xd8, 0xc);
	mCells[16] = new Cell(mPicRex, 0x7d, 0xd8, 0xd);
	mCells[17] = new Cell(mPicRex, 0x7d, 0xd8, 0xc);
	mCells[18] = new Cell(mPicRex, 0x7d, 0xd8, 0xb);
	mStepTimer = 0;
	mFrame = 0.0f;
	mDelay = 100;
	mSteps = 4;
	mDone = false;
}

// 0x42c420
RexAttack::~RexAttack()
{
	if (mSndFootstep != NULL)
	{
		delete mSndFootstep;
		mSndFootstep = NULL;
	}
	if (mSndRoar != NULL)
	{
		delete mSndRoar;
		mSndRoar = NULL;
	}
	if (mPicCrack != NULL)
	{
		delete mPicCrack;
		mPicCrack = NULL;
	}
	Cell** aCell = mCells;
	int i = 0x13;
	do
	{
		if (*aCell != NULL)
		{
			delete *aCell;
			*aCell = NULL;
		}
		aCell++;
		i--;
	} while (i != 0);
	if (mPicRex != NULL)
	{
		delete mPicRex;
		mPicRex = NULL;
	}
	// port: the original never deletes the hole
	delete mPicHole;
}

// 0x42d3f0
void RexAttack::Update()
{
	if (0 < mDelay)
	{
		mDelay--;
		return;
	}
	if (mSteps < 1)
	{
		mFrame = (float) ((double) mFrame + (double) ((mDone == false) ? 0.06f : 0.03f));
		if ((double) mFrame > 18.99)
		{
			mFrame = 15.0f;
			if (mDone == false)
			{
				FossilizedApp* anApp = mMuseum->mApp;
				anApp->mSideBar->mInchy->Sink(0);
				anApp->mSndChime->Play();
				anApp->mSideBar->mScore = anApp->mSideBar->mScore + 10000;
				int anOldRank = mMuseum->mBoard->mRank;
				int aRank = 0;
				do
				{
					if (mMuseum->mBoard->mScoreRanks[aRank] < mMuseum->mApp->mSideBar->mScore)
						mMuseum->mBoard->mRank = aRank;
					aRank++;
				} while (aRank < 0x14);
				if (mMuseum->mBoard->mRank != anOldRank)
					mMuseum->mBoard->mRankChanged = true;
				mMuseum->mApp->mSideBar->MarkDirty();
				mMuseum->mCloseButton->mLabel = mMuseum->mApp->GetString("END_GAME");
				mMuseum->mCloseButton->SetDisabled(false);
			}
			mDone = true;
		}
	}
	else
	{
		mStepTimer--;
		if (mStepTimer < 1)
		{
			mSndFootstep->Play();
			mStepTimer = 0xa0;
			mSteps--;
			if (mSteps == 0)
			{
				mSndRoar->Play();
				return;
			}
		}
	}
}

// 0x42d620: the wall behind the last fossil cracks a step at a time; then the hole and the tyrannosaurus, and at the
// end the congratulations (each line with a black shadow two pixels down and right)
void RexAttack::Draw()
{
	Graphics* g = GetDrawGraphics();
	g->SetColorizeImages(true);
	g->SetColor(Color(0xff, 0xff, 0xff, 200));
	Point anOffset = GetDrawOffset();
	SetDrawOffset(Point(0x1309 - mMuseum->mScroll, 0));
	if (mSteps < 4)
		mPicCrack->Draw(0xf5 - (mPicCrack->mWidth >> 1), 0xd2 - (mPicCrack->mHeight >> 1));
	if (mSteps < 3)
		mPicCrack->Draw(0xd7 - (mPicCrack->mWidth >> 1), 0x10e - (mPicCrack->mHeight >> 1));
	if (mSteps < 2)
		mPicCrack->Draw(0x11d - (mPicCrack->mWidth >> 1), 0xfa - (mPicCrack->mHeight >> 1));
	g->SetColorizeImages(false);
	if (mSteps < 1)
	{
		mPicHole->Draw(0xeb - (mPicHole->mWidth >> 1), 0xf0 - (mPicHole->mHeight >> 1));
		Cell* aCell = mCells[(int) mFrame];
		aCell->Draw(0xf5 - (aCell->mWidth >> 1), 0xf0 - (aCell->mHeight >> 1));
	}
	SetDrawOffset(anOffset);
	if (mDone == true)
	{
		FossilizedApp* anApp = mMuseum->mApp;
		int aWidth = anApp->mAnnounceFont1928->Width(anApp->GetString("CONGRATULATIONS").c_str());
		g->SetColorizeImages(true);
		g->SetColor(Color(0, 0, 0, 0xff));
		// port: the original draws the shadow from a text of its own (the same words), measured as the string's
		anApp->mAnnounceFont1928->Draw(GTC(0x4fc3a0), 0xf7 - aWidth / 2, 0x3e);
		g->SetFont(anApp->mWhiteFont);
		aWidth = anApp->mWhiteFont->StringWidth(anApp->GetString("YOU_HAVE_BEATEN_THE_FOSSIL_CHALLENGE1"));
		g->DrawString(anApp->GetString("YOU_HAVE_BEATEN_THE_FOSSIL_CHALLENGE1"), 0xf7 - aWidth / 2, 0x70);
		aWidth = anApp->mWhiteFont->StringWidth(anApp->GetString("YOU_HAVE_BEATEN_THE_FOSSIL_CHALLENGE2"));
		g->DrawString(anApp->GetString("YOU_HAVE_BEATEN_THE_FOSSIL_CHALLENGE2"), 0xf7 - aWidth / 2, 0x86);
		aWidth = anApp->mWhiteFont->StringWidth(anApp->GetString("SCORE_TENK_BONUS_POINTS"));
		g->DrawString(anApp->GetString("SCORE_TENK_BONUS_POINTS"), 0xf7 - aWidth / 2, 0x174);
		g->SetColor(Color(0xff, 0xff, 0x96, 0xff));
		aWidth = anApp->mAnnounceFont1928->Width(anApp->GetString("CONGRATULATIONS").c_str());
		anApp->mAnnounceFont1928->Draw(anApp->GetString("CONGRATULATIONS").c_str(), 0xf5 - aWidth / 2, 0x3c);
		g->SetFont(anApp->mWhiteFont);
		aWidth = anApp->mWhiteFont->StringWidth(anApp->GetString("YOU_HAVE_BEATEN_THE_FOSSIL_CHALLENGE1"));
		g->DrawString(anApp->GetString("YOU_HAVE_BEATEN_THE_FOSSIL_CHALLENGE1"), 0xf5 - aWidth / 2, 0x6e);
		aWidth = anApp->mWhiteFont->StringWidth(anApp->GetString("YOU_HAVE_BEATEN_THE_FOSSIL_CHALLENGE2"));
		g->DrawString(anApp->GetString("YOU_HAVE_BEATEN_THE_FOSSIL_CHALLENGE2"), 0xf5 - aWidth / 2, 0x84);
		aWidth = anApp->mWhiteFont->StringWidth(anApp->GetString("SCORE_TENK_BONUS_POINTS"));
		g->DrawString(anApp->GetString("SCORE_TENK_BONUS_POINTS"), 0xf5 - aWidth / 2, 0x172);
		g->SetColorizeImages(false);
	}
}

// 0x42cae0
Museum::Museum(FossilizedApp* theApp, DynomiteFossil* theBoard)
{
	mApp = theApp;
	mBoard = theBoard;
	mScroll = 0;
	mRoundEnd = false;
	mKeepMusic = false;
	mY = 0x1e0;
	// port: the original leaves these to Start
	mFragmentAlpha = 0;
	mGlowAlpha = 0;
	mWait = 0;
	if ((theBoard->mGameOver == false) && (theApp->mInTutorial == false))
	{
		theApp->mMusicInterface->FadeOut(theBoard->mSong, false, 0.004);
		mApp->mMusicInterface->FadeIn(4, -1, 0.002, false);
	}
	mCloseButton = new ButtonWidget(1, this);
	mCloseButton->mButtonImage = mApp->mPicLargedbutton->mImage;
	mCloseButton->mDoFinger = true;
	mCloseButton->mLabel = mApp->GetString("CLOSE_MUSEUM");
	mCloseButton->SetFont(mApp->mWhiteFont);
	mCloseButton->SetColor(ButtonWidget::COLOR_LABEL, Color(0xff, 0xff, 0xff));
	mCloseButton->SetColor(ButtonWidget::COLOR_LABEL_HILITE, Color(0xff, 0xff, 0xff));
	mApp->mWidgetManager->AddWidget(mCloseButton);
	Pic* aLarge = mApp->mPicLargedbutton;
	mCloseButton->Resize(400 - aLarge->mWidth / 2, 0x19a, aLarge->mWidth, aLarge->mHeight);

	mLeftButton = new CellButton(2, this);
	mLeftButton->mBtnNoDraw = false;
	mLeftButton->mDoFinger = true;
	mLeftButton->SetVisible(true);
	mLeftButton->mButtonCell = mApp->mPicScrollbuttons_c0;
	mApp->mWidgetManager->AddWidget(mLeftButton);
	Cell* anArrow = mApp->mPicScrollbuttons_c0;
	mLeftButton->Resize((400 - mApp->mPicLargedbutton->mWidth / 2) - anArrow->mWidth, 0x1a1, anArrow->mWidth,
		anArrow->mHeight);

	mRightButton = new CellButton(2, this);
	mRightButton->mBtnNoDraw = false;
	mRightButton->mDoFinger = true;
	mRightButton->SetVisible(true);
	mRightButton->mButtonCell = mApp->mPicScrollbuttons_c1;
	mApp->mWidgetManager->AddWidget(mRightButton);
	anArrow = mApp->mPicScrollbuttons_c0;
	mRightButton->Resize(mApp->mPicLargedbutton->mWidth / 2 + 400, 0x1a1, anArrow->mWidth, anArrow->mHeight);
	if (mBoard->mGameOverShown == true)
		mApp->mWidgetManager->BringToBack(mBoard->mReplayButton);
	mRex = NULL;
}

// 0x42c520
Museum::~Museum()
{
	if (mBoard->mGameOverShown == true)
		mApp->mWidgetManager->BringToFront(mBoard->mReplayButton);
	if (mCloseButton != NULL)
	{
		mApp->mWidgetManager->RemoveWidget(mCloseButton);
		mApp->SafeDeleteWidget(mCloseButton);
		mCloseButton = NULL;
	}
	if (mLeftButton != NULL)
	{
		mApp->mWidgetManager->RemoveWidget(mLeftButton);
		mApp->SafeDeleteWidget(mLeftButton);
		mLeftButton = NULL;
	}
	if (mRightButton != NULL)
	{
		mApp->mWidgetManager->RemoveWidget(mRightButton);
		mApp->SafeDeleteWidget(mRightButton);
		mRightButton = NULL;
	}
	if ((mBoard->mGameOver == false) && (mApp->mInTutorial == false))
	{
		if (mKeepMusic == false)
			mApp->mMusicInterface->FadeIn(mBoard->mSong, -1, 0.002, false);
		mApp->mMusicInterface->FadeOut(4, false, 0.004);
	}
	if (mRex != NULL)
	{
		delete mRex;
		mRex = NULL;
	}
}

// 0x42c030
void Museum::ButtonPress(int theId)
{
	if (theId == 1)
		mApp->mSndClick->Play();
}

// 0x42ce70
void Museum::ButtonDepress(int theId)
{
	if ((mBoard->mLevel == 0xf) && (mBoard->mRound == 3) && (mRex == NULL) &&
		(mCloseButton->mLabel == mApp->GetString("BEWARE")))
	{
		mCloseButton->SetDisabled(true);
		if (mApp->mInTutorial == false)
			mApp->mMusicInterface->FadeOut(4, true, 0.004);
		mRex = new RexAttack(this);
		return;
	}
	if (theId == 1)
		mBoard->NextRound();
}

// 0x42c050
void Museum::SetScroll(int theScroll)
{
	mScroll = theScroll;
	if (theScroll < 0)
		mScroll = 0;
}

// 0x42d000
void Museum::Start()
{
	DynomiteFossil* aBoard = mBoard;
	mScroll = gMuseumPos[aBoard->mLevel][0] - 0xf5;
	mScroll = mApp->mFossils[aBoard->mLevel - 1]->mWidth / 2 + gMuseumPos[aBoard->mLevel][0] - 0xf5;
	if (mScroll < 0)
		mScroll = 0;
	mRoundEnd = true;
	mFragmentAlpha = 0;
	mGlowAlpha = 0;
	mWait = 0x40;
	if (aBoard->mRound < 3)
		mCloseButton->mLabel = mApp->GetString("CONTINUE_TO_NEXT_FRAGMENT");
	else
		mCloseButton->mLabel = mApp->GetString("CONTINUE_TO_NEXT_FOSSIL");
	if ((mBoard->mRound == 3) && (mBoard->mLevel == 0xf))
	{
		mCloseButton->mLabel = mApp->GetString("BEWARE");
		mCloseButton->SetDisabled(true);
	}
	mCloseButton->MarkDirty();
}

// 0x42e460: the gallery comes up a tenth of the way a frame; once up, the new fragment fades in and its glow out
void Museum::Update()
{
	if (mBoard->mPauseCount == 0)
	{
		if (mRex != NULL)
			mRex->Update();
		mY = (int) ((double) mY * 0.9);
		if (mRoundEnd == true)
		{
			if (mWait < 1)
			{
				mFragmentAlpha = mFragmentAlpha + 3;
				if (mFragmentAlpha == 9)
					mApp->mSndFossil->Play();
				if ((mFragmentAlpha == 0xfc) && (mBoard->mRound == 3))
				{
					mApp->mSndChime->Play();
					mCloseButton->SetDisabled(false);
				}
				if (mFragmentAlpha > 0xff)
					mFragmentAlpha = 0xff;
				mGlowAlpha = mGlowAlpha - 3;
				if (mGlowAlpha < 0)
					mGlowAlpha = 0;
			}
			else
			{
				mGlowAlpha = mGlowAlpha + 4;
				if (mGlowAlpha > 0xff)
					mGlowAlpha = 0xff;
			}
			mBoard->MarkDirty();
		}
		if (mY == 0)
		{
			mWait--;
			if (mWait < 0)
				mWait = 0;
		}
	}
	if (mLeftButton->mIsDown != false)
	{
		mScroll = mScroll - 4;
		mBoard->MarkDirty();
	}
	if (mRightButton->mIsDown != false)
	{
		mScroll = mScroll + 4;
		mBoard->MarkDirty();
	}
	if (mScroll < 0)
		mScroll = 0;
	if (mScroll > 0x133c)
		mScroll = 0x133c;
}

// 0x42e300: a brown wall darkening downwards, a frame of the mini dialog's edges along its top and bottom, the fossils
void Museum::Draw()
{
	Graphics* g = GetDrawGraphics();
	float aRed = 90.0f;
	float aGreen = 66.0f;
	float aBlue = 49.0f;
	int i = 0;
	do
	{
		g->SetColor(Color((int) aRed, (int) aGreen, (int) aBlue, 0xff));
		g->FillRect(0, mY + 10 + i, 0x1ea, 5);
		i = i + 5;
		aRed = (float) ((double) aRed * (double) 0.99f);
		aGreen = (float) ((double) aGreen * (double) 0.99f);
		aBlue = (float) ((double) aBlue * (double) 0.99f);
	} while (i < 0x1db);
	i = 0;
	do
	{
		mApp->mMiniDialogCells[1]->Draw(i, mY);
		mApp->mMiniDialogCells[1]->Draw(i, mY + 6);
		Cell* aCell = mApp->mMiniDialogCells[7];
		aCell->Draw(i, (mY - aCell->mHeight) + 0x1e0);
		aCell = mApp->mMiniDialogCells[7];
		aCell->Draw(i, (mY - aCell->mHeight) + 0x1da);
		i = i + 10;
	} while (i < 0x1ea);
	i = 0;
	do
	{
		i++;
		DrawFossil(i);
	} while (i < 0x14);
	if (mRex != NULL)
		mRex->Draw();
}

// 0x42c730: a fossil not yet found (or from before the game's first) is a black shape, with the new fragment fading
// in over it; the found ones are drawn whole, with the name of their period over them and, once complete, their own
// name under them
void Museum::DrawFossil(int theFossil)
{
	Graphics* g = GetDrawGraphics();
	if (mApp->mFossils[theFossil - 1] == NULL)
		return;
	int x = gMuseumPos[theFossil][0] - mScroll;
	int y = gMuseumPos[theFossil][1] + 0x14 + mY;
	int aWidth = mApp->mFossilCopies[theFossil - 1]->mWidth;
	int aHeight = mApp->mFossilCopies[theFossil - 1]->mHeight;
	if ((theFossil >= mBoard->mLevel) || (theFossil < mBoard->mFirstLevel))
	{
		g->SetColorizeImages(true);
		g->SetColor(Color(0, 0, 0));
		mApp->mFossilCopies[theFossil - 1]->Draw(x, y);
		if ((mRoundEnd == true) && (theFossil == mBoard->mLevel))
		{
			g->SetColor(Color(0xff, 0xff, 0xff, mFragmentAlpha));
			mBoard->mFossil->mFragment->Draw(x, y);
		}
		g->SetColorizeImages(false);
	}
	if ((mBoard->mFirstLevel <= theFossil) && (theFossil <= mBoard->mLevel))
		mApp->mFossils[theFossil - 1]->Draw(x, y);
	int aNameWidth = mApp->mWhiteFont2->StringWidth(gFossilRoundNames[theFossil]);
	g->SetFont(mApp->mWhiteFont2);
	g->SetColor(Color(0, 0, 0));
	int aNameX = (aWidth / 2 - aNameWidth / 2) + 2 + x;
	g->DrawString(gFossilRoundNames[theFossil], aNameX, y + 2);
	g->SetColor(Color(0xe1, 0xe1, 0xff));
	g->DrawString(gFossilRoundNames[theFossil], aNameX, y + 2);
	DynomiteFossil* aBoard = mBoard;
	if (((aBoard->mFirstLevel <= theFossil) && (theFossil < aBoard->mLevel)) ||
		((mRoundEnd == true) && (theFossil == aBoard->mLevel) && (aBoard->mRound == 3) && (mFragmentAlpha >= 0xfc)))
	{
		aNameWidth = mApp->mWhiteFont->StringWidth(gFossilNames[theFossil]);
		int aNameY = mApp->mWhiteFont->GetAscent() + aHeight + y - 3;
		if (aNameY > 400)
			aNameY = 400;
		g->SetFont(mApp->mWhiteFont);
		g->SetColor(Color(0, 0, 0));
		aNameX = (aWidth / 2 - aNameWidth / 2) + x;
		g->DrawString(gFossilNames[theFossil], aNameX + 2, aNameY + 2);
		g->SetColor(Color(0xff, 0xff, 0xff));
		g->DrawString(gFossilNames[theFossil], aNameX, aNameY);
	}
	if ((theFossil >= mBoard->mLevel) || (theFossil < mBoard->mFirstLevel))
	{
		g->SetColorizeImages(true);
		if ((mRoundEnd == true) && (theFossil == mBoard->mLevel))
		{
			g->SetColor(Color(0xff, 0xff, 0xff, mGlowAlpha));
			mBoard->mFossil->mGlow->Draw(x, y);
		}
		g->SetColorizeImages(false);
	}
}

// 0x42c490
void RexAttack::SaveState(BinFile* theFile)
{
	theFile->WriteInt(mDelay);
	theFile->WriteInt(mStepTimer);
	theFile->WriteInt(mSteps);
	theFile->WriteFloat(mFrame);
	theFile->WriteBool(mDone);
}

// 0x42c4e0
void RexAttack::LoadState(BinFile* theFile)
{
	mDelay = theFile->ReadInt();
	mStepTimer = theFile->ReadInt();
	mSteps = theFile->ReadInt();
	mFrame = theFile->ReadFloat();
	mDone = theFile->ReadBool();
}

// 0x42c630
void Museum::SaveState(BinFile* theFile)
{
	theFile->WriteInt(mScroll);
	theFile->WriteBool(mRoundEnd);
	theFile->WriteInt(mFragmentAlpha);
	theFile->WriteInt(mGlowAlpha);
	theFile->WriteInt(mWait);
	theFile->WriteInt(mY);
	theFile->WriteBool(mCloseButton->mDisabled);
	theFile->WriteString(mCloseButton->mLabel.c_str());
	if (mRex == NULL)
	{
		theFile->WriteBool(false);
		mKeepMusic = true;
		return;
	}
	theFile->WriteBool(true);
	mRex->SaveState(theFile);
	mKeepMusic = true;
}

// 0x42d2c0
void Museum::LoadState(BinFile* theFile)
{
	char aLabel[0x3c];
	mScroll = theFile->ReadInt();
	mRoundEnd = theFile->ReadBool();
	mFragmentAlpha = theFile->ReadInt();
	mGlowAlpha = theFile->ReadInt();
	mWait = theFile->ReadInt();
	mY = theFile->ReadInt();
	bool isDisabled = theFile->ReadBool();
	theFile->ReadString(aLabel);
	mCloseButton->SetDisabled(isDisabled);
	mCloseButton->mLabel = aLabel;
	if (theFile->ReadBool() == true)
	{
		if (mApp->mInTutorial == false)
			mApp->mMusicInterface->FadeOut(4, true, 0.004);
		mRex = new RexAttack(this);
		mRex->LoadState(theFile);
	}
}
