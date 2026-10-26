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

#include <cstring>
#include <cstdio>

using namespace Sexy;

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
	memset(&mUpdateCnt, 0, (char*) &mObjects - (char*) &mUpdateCnt);
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

// 0x426f40 (not translated yet): the rules' per-update order is Connections, Timers, UpdatePieces and the effects
void Board::Update()
{
	mUpdateCnt++;
	mEffects->Update();
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
