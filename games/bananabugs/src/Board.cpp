#include "Board.h"

#include "BackDrop.h"
#include "Pending.h"
#include "FillObj.h"
#include "LevelObj.h"
#include "MainMenu.h"
#include "Player.h"

using namespace Sexy;

// 0x405770: the board starts clear and fades in; the level's fills, backdrop, score board and level are made, and the
// player's machine joins the scene again on top. The original also names the object "GameBoard" in a field nothing
// reads.
Board::Board()
{
	SetParent(nullptr, nullptr);
	BugObj::ResetCounters();
	SetStateInt(STATE_ALPHA, 0);
	SetStateInt(STATE_Z, 0);
	mLeaveTrack = BuildLeaveTrack(this);
	mFadeTrack = BuildFadeTrack(this);
	StartFading();
	ObjBase::Activate(this, false);
	FillObj::LoadSheet();
	mBackDrop = BackDrop::Create(this);
	mScoreBoard = new ScoreBoard();
	mScoreBoard->SetParent(this, nullptr);
	Player::SetScoreBoard(mScoreBoard);
	mLevel = new LevelObj();
	mLevel->SetParent(mBackDrop, nullptr);
	mBackDrop->BuildGround();
	mUnused790 = 0;
	mUnused794 = 0;
	mUnused798 = 0;
	mUnused79c = 0;
	Player::Reactivate();
}

// 0x405930: the level's fills go with the board
Board::~Board()
{
	sLevel = nullptr;
	FillObj::FreeSheet();
}
