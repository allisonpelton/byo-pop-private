// Sexy::TimeBombBoard: see the header
#include "TimeBombBoard.h"

#include "Font.h"
#include "Graphics.h"
#include "Res.h"

#include <cstdio>

#include "Bej3App.h"

namespace Sexy {

namespace {

// the exe's measures are in 1200ths of the art size in use
int Scaled(int theMeasure) { return gApp->mArtRes * theMeasure / 1200; }

}  // namespace

// slot 135 (0x21c), 0x6e52e0 (bejeweled3): the settings BombCountdown, DropCountdown and MaxBombs are read; the moves until the drop, the bomb count, the drop's flags and the
// started flag are 0; the quest's new game (0x6ddc70) follows. (The constructor 0x6dd440 puts the real-time flag, +0x3262, which Bej3App::StartQuest sets before this)
void TimeBombBoard::NewGame()
{
	mBombCountdown = SettingInt("BombCountdown");
	mDropCountdown = SettingInt("DropCountdown");
	mMaxBombs = SettingInt("MaxBombs");
	mMovesUntilDrop = 0;
	mBombCount = 0;
	mDropPending = false;
	mStarted = false;
	QuestBoard::NewGame();
}

// slot 143 (0x23c), 0x6d0930 (bejeweled3): a bomb (the bits 0x60) that is cleared is one bomb less; then the goal's slot 40 and Board's
void TimeBombBoard::OnPieceCleared(Piece* thePiece)
{
	if ((thePiece->mFlags & FLAGS_BOMB_KIND) != 0)
		mBombCount--;
	QuestBoard::OnPieceCleared(thePiece);
}

// slot 191 (0x2fc), 0x6c61c0 (bejeweled3): a swap found a match: one move less until the drop, then the goal's slot 24 (0x60)
void TimeBombBoard::OnSwapMatched(const SwapRecord& theSwap)
{
	(void)theSwap;
	mMovesUntilDrop--;
}

// slot 204 (0x330), 0x6c53d0 (bejeweled3), the end of the fill: if a bomb was made in it, the bomb is counted and the moves until the next one are DropCountdown (a sound plays
// when the game's sound is on: left out)
void TimeBombBoard::OnFillDone(const std::vector<Piece*>& theNewPieces)
{
	(void)theNewPieces;
	if (mDropPending) {
		mBombCount++;
		mMovesUntilDrop = mDropCountdown;
	}
}

// 0x73ccc0 (bejeweled3): the piece is a bomb: the bit 0x200 with 0x40 (real time) or 0x20 (moves), the countdown, and 25 for the special countdown. port: the exe returns when the
// piece's bit 9 of +0x22c is set (a flag of the goals that are not made) and puts +0x22c to 0
void TimeBombBoard::MakeBomb(Piece* thePiece, bool theRealTime, int theCountdown)
{
	thePiece->mFlags = FLAG_BOMB | (theRealTime ? FLAG_REAL_TIME_BOMB : FLAG_MOVE_BOMB);
	thePiece->mTimeGemSeconds = theCountdown;
	thePiece->mSpecialCountdown = BOMB_SPECIAL_COUNTDOWN;
}

// slot 185 (0x2e4), 0x6c60b0 (bejeweled3), the fill's accept: the drop is not pending. When the moves until the drop have run out or there is no bomb, the grid has no
// match, no clock of a finished level runs, the level is not full and fewer than MaxBombs bombs are on, a new gem is drawn (the app's Rand()) up to 100 times: one in the
// first five rows, plain, and (with a score above 0) anywhere in them or else in the middle columns (3 and 4) becomes a bomb and the drop is pending. The goal's slot 49 (0xc4)
// is asked last (true in the base)
bool TimeBombBoard::OnFillAccepted(std::vector<Piece*>& theNewPieces)
{
	mDropPending = false;
	if ((mMovesUntilDrop < 1 || mBombCount == 0) && !HasMatches() && mQuestClock == 0 && GetLevelProgress() < GetLevelMax() && mBombCount < mMaxBombs && !theNewPieces.empty()) {
		for (int aTry = 0; aTry < BOMB_PICK_TRIES; aTry++) {
			Piece* aPiece = theNewPieces[(size_t)(Rand() % (int)theNewPieces.size())];
			if (aPiece->mRow < BOMB_MAX_ROW && (mScorePoints > 0 || (aPiece->mCol > BOMB_CENTRE_FIRST_COLUMN && aPiece->mCol < BOMB_CENTRE_LAST_COLUMN)) && aPiece->mFlags == 0) {
				mDropPending = true;
				MakeBomb(aPiece, mRealTime, mBombCountdown);
				break;
			}
		}
	}
	return true;
}

// slot 212 (0x350), 0x6c6090 over 0x73c710 (bejeweled3): nothing before the quest's play has begun. A piece with a count left: for a move (not a tick) a bomb's shake starts (the
// count goes down when the shake has run to its window, UpdateWarnings) and a butterfly's count goes down; for a tick the count goes down, and a bomb that reaches 0 while no
// clock of a finished level runs ends the game
void TimeBombBoard::CountDownPiece(Piece* thePiece, bool theTick)
{
	if (!mStarted || thePiece->mTimeGemSeconds <= 0)
		return;
	if (!theTick) {
		if ((thePiece->mFlags & FLAG_BOMB) != 0) {
			thePiece->mShakePhase = 1.0f;
			thePiece->mShakeSpeed = 0.33000001311302185f;
		}
		else if ((thePiece->mFlags & Piece::FLAG_BUTTERFLY) != 0)
			thePiece->mTimeGemSeconds--;
		return;
	}
	thePiece->mTimeGemSeconds--;
	if (thePiece->mTimeGemSeconds == 0 && (thePiece->mFlags & FLAG_BOMB) != 0 && mQuestClock == 0)
		NoMoreMoves(true);
}

// 0x767e20 (bejeweled3), the loop over the pieces (about lines 190 to 230): a bomb or a goal's piece (0x60 or 0x100) with no count left and the board at rest ends the game
// (slot 163, 0x755e50: with no clock of a finished level and the fill not held; the fill is then held); a piece that shakes steps the shake (a bomb: past 20 it stops, and
// between 5 and 10 on its way up it takes one count off and jumps to 16); a real-time bomb with the game on (no end, no clock, slot 101 false) steps its clock and ticks
// when it comes round to 0
void TimeBombBoard::UpdateWarnings()
{
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece == nullptr)
				continue;
			if ((aPiece->mFlags & (FLAGS_BOMB_KIND | FLAG_GOAL_BOMB)) != 0 && aPiece->mTimeGemSeconds == 0 && IsSettled()) {
				if (mQuestClock == 0 && mFillHeld == 0) {
					NoMoreMoves(true);
					mFillHeld = 1;
				}
			}
			if (aPiece->mShakeSpeed != 0.0f) {
				aPiece->mShakePhase += aPiece->mShakeSpeed;
				if (aPiece->mShakePhase < 0.0f)
					aPiece->mShakePhase += (float)SHAKE_END;
				if ((aPiece->mFlags & FLAG_BOMB) != 0) {
					if (aPiece->mShakePhase > (float)SHAKE_END) {
						aPiece->mShakePhase = 0.0f;
						aPiece->mShakeSpeed = 0.0f;
					}
					if (aPiece->mShakeSpeed != 0.0f && aPiece->mShakePhase > (float)SHAKE_COUNT_FROM && aPiece->mShakePhase <= (float)SHAKE_COUNT_TO) {
						aPiece->mTimeGemSeconds--;
						aPiece->mShakePhase = (float)SHAKE_AFTER_COUNT;
					}
				}
			}
			if ((aPiece->mFlags & FLAG_REAL_TIME_BOMB) != 0 && mGameOverTicks == 0 && mQuestClock == 0) {
				aPiece->mTickCounter = (aPiece->mTickCounter + 1) % aPiece->mTickPeriod;
				if (aPiece->mTickCounter == 0)
					CountDownPiece(aPiece, true);
			}
		}
	}
}

// the bombs' counts are written over their gems; the bombs on the board, the moves until the next drop and the level's progress beside the score. port: the exe draws the
// bomb as a picture with the count in it (the bomb's effects and the text at +0xe34), which is left out
void TimeBombBoard::DrawModeOverlay(Graphics* g)
{
	g->SetColor(Color(255, 255, 255, 255));
	g->SetFont(FONT_SCORE);
	char aText[64];
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			const Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece == nullptr || (aPiece->mFlags & FLAG_BOMB) == 0)
				continue;
			snprintf(aText, sizeof(aText), "%d", aPiece->mTimeGemSeconds);
			g->WriteString(aText, Scaled(GetGridX() + (int)aPiece->mX + 64), Scaled(GetGridY() + (int)aPiece->mY + 64), 0, 0, true, 0, -1, -1);
		}
	}
	snprintf(aText, sizeof(aText), "Bombs: %d", mBombCount);
	g->WriteString(aText, Scaled(250), Scaled(330), 0, 0, true, 0, -1, -1);
	snprintf(aText, sizeof(aText), "Next in: %d", mMovesUntilDrop < 0 ? 0 : mMovesUntilDrop);
	g->WriteString(aText, Scaled(250), Scaled(390), 0, 0, true, 0, -1, -1);
	snprintf(aText, sizeof(aText), "%d / %d", mLevelProgress, GetLevelMax());
	g->WriteString(aText, Scaled(250), Scaled(450), 0, 0, true, 0, -1, -1);
}

}  // namespace Sexy
