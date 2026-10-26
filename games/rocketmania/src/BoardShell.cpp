// The Board's widget shell: construction and the parts of the original that are not translated yet (Update 0x426f40,
// the input handlers 0x4282a0-0x428780, the saved games are in BoardSave.cpp, the drawing in BoardDraw.cpp). See docs/GAME_NOTES.md for the map.
// What is here makes the translated rules (BoardLogic.cpp, BoardLogic2.cpp) build and run on a Board; each stub says
// which original function it stands for.

#include "Board.h"
#include "RocketApp.h"
#include "MusicInterface.h"
#include "Compat2004.h"
#include "Dialogs.h"
#include "GameText.h"
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
	SetupBoard(true, theContinuing);
	// 0x41ae80: the game's song fades in (the head's first lines when the app's hint flag is clear are not translated yet)
	mApp->mMusicInterface->FadeIn(SONG_GAME, -1, 0.666, false);
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
// +0x51c blinks between (m520 counts to 300).
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
				Points* aPoints = new Points(mApp, mApp->mFonts[FONT_DOMO22_BOLD], GT(0x004e66f0),
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
			ShowLevelUp(LevelUpDialog::KIND_LEVEL);
			m360 = m360 + ((int) SDL_GetTicks() - m368) / 1000;
			F428de0(1000);
			// (the head is told to go on, 0x4164f0, and made a child of the board, 0x42fdf3: the head is not ported yet)
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

// (not translated yet)
void Board::ButtonDepress(int theId)
{
	(void) theId;
}

// ---------------------------------------------------------------------------------------------------------
// the originals' small helpers (their addresses are their names; what is left out is said at each)

// 0x4282a0: a burning tile is moved: the burn is cancelled and its tiles are plain again
void Board::F4282a0(int thePath)
{
	m4da = false;
	for (int aRow = mRows - 1; aRow >= 0; aRow--)
	{
		for (int aColumn = 0; aColumn < mCols; aColumn++)
		{
			Piece* aPiece = mGrid[aRow][aColumn];
			if (aPiece != NULL && aPiece->mType == 4 && aPiece->mPath == thePath)
			{
				aPiece->mType = 1;
				aPiece->mPath = 0;
				aPiece->mPathFrom = -1;
			}
		}
	}
	m2e8 = false;
	m2f0 = -1;
}

// 0x428090: takes the first special of kind 9 (type 6) out of the moving pieces
void Board::F428090()
{
	for (std::list<Piece*>::iterator anItr = mObjects.begin(); anItr != mObjects.end(); ++anItr)
	{
		Piece* aPiece = *anItr;
		if (aPiece != NULL && aPiece->mType == 6 && aPiece->mKind == 9)
		{
			EraseMover(aPiece);
			RemovePiece(aPiece, false);
			return;
		}
	}
}

// 0x421de0: the game is over (theKind 0 the night ran out, 1 the top line is full, 2 strategy mode ran out of moves).
// Left out: the talking head's lines (0x416aa0, 0x416360) and the
// buy-rockets screen (BuyRocketsWidget 0x401010, which the original adds over the board).
void Board::F421de0(int theKind)
{
	m460 = theKind;
	mState = 1;
	m440 = 300;
	mApp->mMusicInterface->FadeOut(SONG_GAME, true, 0.02);
	mApp->mMusicInterface->FadeIn(SONG_GAMEOVER, -1, 0.02, false);
	mApp->PlaySample(0x24);
}

// 0x429050, 0x429060, 0x429080: the count of the flames that were lit for one launch; when the launch is done
// MakeMarker puts that many on the board
void Board::F429050()
{
	m49c++;
}

void Board::F429060(int theRow)
{
	m49c = 0;
	m4a0 = theRow;
}

void Board::F429080()
{
	MakeMarker(m4a0, m49c + 1);
	m4a0 = -1;
}

// 0x41f280: the effect of the meter i filling up
void Board::F41f280(int theIndex)
{
	Effect* anEffect = mEffects->Add(0xe, true);
	anEffect->mLife = 1;
	anEffect->mc0 = 0;
	anEffect->mY = 44.0;
	anEffect->mX = (double) (theIndex * 0x16 + 0x108);
	mApp->PlaySample(4);
}

// 0x428620: the buy-rockets screen goes up over the board (not translated yet)
void Board::ShowBuyRockets()
{
}

// 0x4286b0: after a level's screen. The widget at +0x51c goes; the first levels bring the hint of an item (level 1 the coins,
// level 3 the gems, level 5 the bombs and, in the strategy mode, level 7 the clocks) and then the head goes on (0x416520
// with 1: the head is not ported yet); otherwise, or when the hint is turned off, the buy-rockets screen follows. The
// next level's launches are counted and the app's timer (0x42f770: the head's countdown) is looked at.
void Board::F4286b0()
{
	if (m51cWidget != NULL)
	{
		mWidgetManager->RemoveWidget(m51cWidget);
		mApp->SafeDeleteWidget(m51cWidget);
		m51cWidget = NULL;
	}
	int aHint = -1;
	if (mLevel == 0)
		aHint = HintDialog::HINT_COIN;
	else if (mLevel == 2)
		aHint = HintDialog::HINT_GEM;
	else if (mLevel == 4)
		aHint = HintDialog::HINT_BOMB;
	else if (mLevel == 6)
	{
		if (mApp->mMode == 1)
			aHint = HintDialog::HINT_CLOCK;
	}
	if (aHint == -1 || !mApp->ShowModeHint(aHint, false))
		ShowBuyRockets();
	m318 = 0.0;
	m4bc = 0;
	m74 = mGen->RocketsForLevel(mLevel + 2);
	m78 = m74;
	m374 = 0;
}

// 0x428ca0: the rank of the score (0 to 26) by the table at 0x4c7848, its name, and how many points the next rank is away
// (0 at the top rank)
static const int sRankScores[27] =
{
	0, 6000, 12000, 18000, 24000, 30000, 36000, 42000, 48000, 54000, 60000, 69000, 78000, 90000, 120000, 180000, 240000,
	300000, 360000, 420000, 480000, 600000, 750000, 900000, 1200000, 1800000, 3000000
};

// the names, by rank: the table at 0x4e62e8
static const unsigned int sRankNames[27] =
{
	0x004e6560, 0x004e6550, 0x004e653c, 0x004e6528, 0x004e6514, 0x004e6508, 0x004e6500, 0x004e64f4, 0x004e64dc, 0x004e64d0,
	0x004e64c8, 0x004e64bc, 0x004e64b0, 0x004e64a4, 0x004e6494, 0x004e6480, 0x004e6468, 0x004e6450, 0x004e6438, 0x004e6420,
	0x004e6408, 0x004e63f8, 0x004e63e0, 0x004e63cc, 0x004e63bc, 0x004e63b0, 0x004e639c
};

SexyString Board::RankName(int* theRank, int* thePointsToNext)
{
	int anIndex = 0x1a;
	int aNext = 0;
	int aRank;
	do
	{
		int aScore = sRankScores[anIndex];
		aRank = anIndex;
		if (aScore <= mScore)
			break;
		anIndex--;
		aNext = aScore;
		aRank = 0;
	} while (-1 < anIndex);
	*theRank = aRank;
	int aToNext = aNext - mScore;
	*thePointsToNext = (aToNext < 0) ? 0 : aToNext;
	return GT(sRankNames[aRank]);
}

// the level-up dialog as the board opens it (0x426860 at the end of a game with kind 5, 0x426f40 after a level with kind 4):
// white, green and green for the dialog's colours 0, 1 and 3, the whole board's size
void Board::ShowLevelUp(int theKind)
{
	LevelUpDialog* aDialog = new LevelUpDialog(mApp, theKind, mRand);
	aDialog->SetColor(0, Color(0xff, 0xff, 0xff));
	aDialog->SetColor(1, Color(0, 0xff, 0));
	aDialog->SetColor(3, Color(0, 0xff, 0));
	aDialog->mContentInsets = Insets(0x24, 0x24, 0x24, 0x24);
	aDialog->Resize(0, 0, mWidth, mHeight);
	mApp->AddDialog(DIALOG_LEVEL_UP, aDialog);
}

// 0x428d10: the specials that the launches have earned are put on the board
// (the first matches special that is attached opens the matches hint)
void Board::F428d10()
{
	if (mState != 0)
		return;
	if (mApp->mMode == 2)
	{
		while (0 < mGen->m70)
		{
			bool aWasAttached = m4d8;
			AttachSpecial10();
			if (m4d8 != aWasAttached)
				mApp->ShowModeHint(HintDialog::HINT_MATCHES, false);
			mGen->m70--;
		}
	}
	else
	{
		int aLeft = mGen->m70;
		while (0 < aLeft)
		{
			int aIndex;
			if (aLeft < 10)
			{
				if (4 < aLeft)
				{
					aLeft -= 5;
					aIndex = 2;
				}
				else if (1 < aLeft)
				{
					aLeft -= 2;
					aIndex = 1;
				}
				else
				{
					aLeft -= 1;
					aIndex = 0;
				}
			}
			else
			{
				aLeft -= 10;
				aIndex = 3;
			}
			mGen->m70 = aLeft;
			AttachBomb(aIndex);
			aLeft = mGen->m70;
		}
	}
}

// 0x428de0: the rows above the height theValue (pixels, from the top of the board) flash and go
void Board::F428de0(int theValue)
{
	int aRows = (theValue - 30) / 42;
	if (aRows < 0)
		aRows = 0;
	if (mRows < aRows)
		aRows = mRows;
	for (int aRow = 0; aRow < aRows; aRow++)
	{
		for (int aColumn = 0; aColumn < mCols; aColumn++)
		{
			Piece* aPiece = mGrid[aRow][aColumn];
			if (aPiece != NULL)
			{
				AddFlash(aPiece, 2);
				RemovePiece(aPiece, false);
			}
		}
	}
}
