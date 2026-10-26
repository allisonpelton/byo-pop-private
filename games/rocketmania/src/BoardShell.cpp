// The Board's widget shell: construction and the parts of the original that are not translated yet (Update 0x426f40,
// Draw 0x41f540, the input handlers 0x4282a0-0x428780, saves 0x41b430-0x41c830). See docs/GAME_NOTES.md for the map.
// What is here makes the translated rules (BoardLogic.cpp, BoardLogic2.cpp) build and run on a Board; each stub says
// which original function it stands for.

#include "Board.h"
#include "RocketApp.h"
#include "Compat2004.h"
#include "Graphics.h"
#include "Image.h"
#include "Font.h"
#include "ImageFont.h"
#include "MTRand.h"
#include "WidgetManager.h"

#include <SDL.h>
#include <cstring>
#include <cstdio>

using namespace Sexy;

static int Ftol(double theValue)
{
	return (int) theValue;
}

// 0x41ad90's initialisation of a new board (the pieces are made through Piece(int))
Board::Board(RocketApp* theApp, bool theContinuing)
{
	mApp = theApp;
	mCols = 6;
	mRows = 9;
	mRand = new MTRand((unsigned long) Rand());
	mGen = new PieceGenerator(theApp, this, mRand);
	mEffects = new EffectManager();
	// every plain field of the original starts as zero; the lists and the widget stay as they are
	memset(&mState, 0, (char*) &mObjects - (char*) &mState);
	for (int aRow = 0; aRow < 9; aRow++)
		for (int aColumn = 0; aColumn < 6; aColumn++)
			mGrid[aRow][aColumn] = NULL;
	for (int i = 0; i < 6; i++)
		mTop[i] = NULL;
	mSpare = NULL;
	for (int i = 0; i < 9; i++)
	{
		mLeft[i] = NULL;
		mRight[i] = NULL;
	}
	m514 = NULL;
	m68 = 100.0;
	mLevel = 0;
	m74 = 10;
	m78 = 10;
	m7c = 3;
	m80 = 3;
	mState = 0;
	ResetState(theContinuing);
	SetupBoard(true, false);
	SetupBoard(false, false);
}

Board::~Board()
{
	ClearBoard(false);
	for (std::list<Points*>::iterator anItr = mTexts.begin(); anItr != mTexts.end(); ++anItr)
		(*anItr)->mWidgetManager->RemoveWidget(*anItr);
	mTexts.clear();
	delete mGen;
	delete mEffects;
	delete mRand;
}

// 0x426f40: the board's update. Left out (not translated yet): the talking head's calls (0x413b40, 0x416aa0, 0x416360,
// 0x416470, 0x4163a0, 0x4164f0), the app's countdown at app+0x860 that makes the head speak, the colour that the widget at
// +0x51c blinks between (m520 counts to 300), and the level-up dialog of state 5 (0x40c2c0).
void Board::Update()
{
	if (0 < m4c4)
		m4c4--;
	if (mState == 3)
		return;
	if (m350 < 1)
		m358 = 0;
	else if (m358 != 1.0)
	{
		m358 = m358 + 0.002;
		if (1.0 < m358)
			m358 = 1.0;
		MarkDirty();
	}
	if (1 <= m350)
		return;
	Widget::Update();
	m364 = (int) SDL_GetTicks() - m368;
	if (!m438)
		m438 = true;
	MarkDirty();
	if (m2e8)
	{
		m2ec--;
		if (m2ec < 1)
		{
			if (m2ec == 0)
				FinishBurn();
			else if (m2f4 == 0)
				Launch();
		}
	}
	if (mApp->mMode == 2 && mState == 0)
	{
		if (m108 == 0)
		{
			m380--;
			if (m380 < 1 && CountBurning() == 0)
				F421de0(2);
		}
		else
			m380 = 0xfa;
	}
	SmoothFraction();
	Timers();
	UpdatePieces();
	if (m4a8 && AllTiles())
	{
		m4a8 = false;
		m4a9 = true;
	}
	Connections();
	m4a9 = false;
	for (int i = 0; i < 10; i++)
	{
		if (m468[i] == 1)
		{
			F41f280(i);
			if (i == 7)
			{
				Points* aPoints = new Points(mApp, mApp->mFonts[FONT_DOMO22_BOLD], "Time is running out!",
					CellX(3) + mX, mY + 0x1e, 400, 0);
				aPoints->mStatic = true;
				mTexts.push_back(aPoints);
				mWidgetManager->AddWidget(aPoints);
				m4c0 = true;
			}
			m468[i] = 2;
		}
	}
	if (m2e4 != mScore)
	{
		MarkDirty();
		int aDiff = mScore - m2e4;
		if (m2e4 < mScore)
		{
			m2e4 = m2e4 + Ftol((double) aDiff * 0.02 + 2.0);
			if (mScore < m2e4)
				m2e4 = mScore;
		}
		else if (mScore < m2e4)
		{
			m2e4 = m2e4 + Ftol((double) aDiff * 0.025 - 2.0);
			if (m2e4 < mScore)
				m2e4 = mScore;
		}
	}
	for (std::list<Points*>::iterator anItr = mTexts.begin(); anItr != mTexts.end(); )
	{
		Points* aPoints = *anItr;
		aPoints->MarkDirty();
		aPoints->mPhase = (aPoints->mPhase + 7) % 0xff;
		if (mUpdateCnt % 3 == 0 && 0 < aPoints->mTop && !aPoints->mStatic)
		{
			aPoints->mTop--;
			aPoints->Position();
		}
		aPoints->mLife--;
		if (aPoints->mLife < 1)
		{
			anItr = mTexts.erase(anItr);
			mWidgetManager->RemoveWidget(aPoints);
			mApp->SafeDeleteWidget(aPoints);
		}
		else
			++anItr;
	}
	if (mState != 2)
	{
		if (mState == 7)
			F428ef0();
		else if (mState == 4)
		{
			if (m36c < 1)
				EndLevelB();
			else
				m36c--;
		}
		else if (mState == 5)
		{
			mState = 6;
			m360 = m360 + ((int) SDL_GetTicks() - m368) / 1000;
			F428de0(1000);
		}
	}
	if (mState == 0 && 0x32 < mUpdateCnt - m458 && !m45c)
		SettleAll(true);
	mEffects->Update();
	if (m4a4 != 0 && --m4a4 == 0)
		F428d10();
	if (mState == 0 && mApp->mMode == 0 && mUpdateCnt % 100 == 0 && 0 < mCols)
	{
		for (int aColumn = 0; aColumn < mCols; aColumn++)
		{
			if (mGrid[0][aColumn] != NULL && mGrid[0][aColumn]->mType == 1)
			{
				mApp->PlaySample(5);
				break;
			}
		}
	}
	if (mState == 1 && m460 == 1)
	{
		for (int aRow = 0; aRow < mRows; aRow++)
			for (int aColumn = 0; aColumn < mCols; aColumn++)
				Bounce(mGrid[aRow][aColumn]);
		for (int aColumn = 0; aColumn < mCols; aColumn++)
			Bounce(mTop[aColumn]);
		Bounce(mSpare);
	}
}

// 0x41f540 (not translated yet)
void Board::Draw(Graphics* g)
{
	(void) g;
}

// (not translated yet)
void Board::ButtonDepress(int theId)
{
	(void) theId;
}

// 0x41b690 (the saved games' loader is not translated yet)
void Board::ResolveLoadedPieces()
{
}

// ---------------------------------------------------------------------------------------------------------
// the originals' small helpers that are not read yet; each only records its call so a test can see it happen

void Board::F428780()
{
	// 0x428780: the first drop of the timed modes (not read yet)
}

void Board::F4282a0(int thePath)
{
	(void) thePath;     // 0x4282a0 (not read yet): called when a burning tile is moved
}

void Board::F428090()
{
	// 0x428090 (not read yet): called by ResetState
}

void Board::F421de0(int theKind)
{
	// 0x421de0 (not read yet): the end of a game (theKind 0 the night ran out, 1 the top line is full)
	(void) theKind;
	if (mState == 0)
		mState = 1;
}

void Board::F429050()
{
	// 0x429050 (not read yet)
}

void Board::F429060(int theRow)
{
	(void) theRow;      // 0x429060 (not read yet)
}

void Board::F429080()
{
	// 0x429080 (not read yet): called by ResetState and Launch
}

void Board::F41f280(int theIndex)
{
	(void) theIndex;    // 0x41f280 (not read yet): called when a meter m468[i] reaches 1
}

void Board::F428ef0()
{
	// 0x428ef0 (not read yet): state 7
}

void Board::F428d10()
{
	// 0x428d10 (not read yet): when the m4a4 countdown ends
}

void Board::F428de0(int theValue)
{
	(void) theValue;    // 0x428de0 (not read yet)
}
