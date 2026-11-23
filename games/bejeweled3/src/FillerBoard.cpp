// Sexy::FillerBoard: see the header
#include "FillerBoard.h"

#include "Font.h"
#include "Graphics.h"
#include "Res.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "Bej3App.h"

namespace Sexy {

namespace {

// the exe's measures are in 1200ths of the art size in use
int Scaled(int theMeasure) { return gApp->mArtRes * theMeasure / 1200; }

}  // namespace

// slot 135 (0x21c), 0x6e3420 (bejeweled3): the settings NumStartGems (the credit), DropPerMove and AssistCurve are read (the curve is set from the text, in its own
// letters), the drop is allowed, and the quest's new game (0x6ddc70) follows, whose first fill drops the credit's gems. port: a quest with no AssistCurve gets a curve
// that is 0
void FillerBoard::NewGame()
{
	mCredits = SettingInt("NumStartGems");
	mDropPerMove = SettingInt("DropPerMove");
	auto aCurveIter = mSettings.find("AssistCurve");
	if (aCurveIter == mSettings.end())
		mAssist.SetConstant(0.0);
	else {
		std::string aCurve = aCurveIter->second;
		if (aCurve.size() >= 2 && aCurve.front() == '"' && aCurve.back() == '"')
			aCurve = aCurve.substr(1, aCurve.size() - 2);
		mAssist.SetCurve(aCurve);
	}
	mGemsAtMoveStart = mCredits;
	mDropAllowed = true;
	mWasSettled = false;
	mGemsDropped = 0;
	mGemsRemoved = 0;
	mFullWarning = 0.0f;
	QuestBoard::NewGame();
}

// slot 22 (0x58), 0x6e3130 (bejeweled3): with the level full the credit is 0; the quest's update; the gems that are on the screen are counted (their y with the grid's
// is above 0), and when the board has left its rest the count is kept as where the move started and the dropped and removed counts are 0. port: the effects of the
// gems that dropped (0x3264, 0x326c, 0x3278 to 0x3280, drawn only) are left out
void FillerBoard::Update()
{
	if (IsLevelFull())
		mCredits = 0;
	QuestBoard::Update();
	int aOnScreen = 0;
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			const Piece* aPiece = mGrid[aRow][aCol];
			if (aPiece != nullptr && (float)GetGridY() + aPiece->mY > 0.0f)
				aOnScreen++;
		}
	}
	const bool isSettled = IsSettled();
	if (mWasSettled && !isSettled) {
		mGemsAtMoveStart = aOnScreen;
		mGemsDropped = 0;
		mGemsRemoved = 0;
	}
	mWasSettled = isSettled;
}

// slot 176 (0x2c0), 0x6d7c20 (bejeweled3): a gem is taken off: the removed count goes up, then the goal's slot 18 (QuestBoard's)
bool FillerBoard::OnPieceRemoving(Piece* thePiece)
{
	mGemsRemoved++;
	return QuestBoard::OnPieceRemoving(thePiece);
}

// slot 180 (0x2d0), 0x6cff00 (bejeweled3): a hypercube's group is made: the credit is 2 more, then QuestBoard's
void FillerBoard::MakeHypercubeGroup(Piece* theCube, int theColor)
{
	mCredits += 2;
	QuestBoard::MakeHypercubeGroup(theCube, theColor);
}

// slot 191 (0x2fc), 0x6c52a0 (bejeweled3): a swap found a match: with the warning up the drop is not allowed (no hint); the credit grows by DropPerMove
void FillerBoard::OnSwapMatched(const SwapRecord& theSwap)
{
	(void)theSwap;
	if (mFullWarning > 0.0f)
		mDropAllowed = false;
	mCredits += mDropPerMove;
}

// slot 161 (0x284), 0x6c50f0 (bejeweled3): no move is left: the game does not end, three more gems are dropped in. The game ends by the warning (UpdateWarnings)
void FillerBoard::NoMoreMoves(bool theAnnounce)
{
	(void)theAnnounce;
	mCredits += 3;
}

// slot 98 (0x188), 0x6c6b10 (bejeweled3): a hint only while the drop is allowed, with the goal's slot 69 (0x114, true in the base, which every goal made so far has) and Board's
bool FillerBoard::HintAllowed() const
{
	return mDropAllowed && Board::HintAllowed();
}

// slot 214 (0x358), 0x6c6e90 (bejeweled3): once the game is over every gem speeds up by 0.1 and falls off the grid; before that Board's gravity (0x73dd70)
void FillerBoard::UpdateFalling()
{
	if (mGameOverTicks > 0) {
		for (int aRow = 0; aRow < ROWS; aRow++) {
			for (int aCol = 0; aCol < COLS; aCol++) {
				Piece* aPiece = mGrid[aRow][aCol];
				if (aPiece == nullptr)
					continue;
				aPiece->mFallSpeed = (float)((double)aPiece->mFallSpeed + 0.10000000149011612);
				aPiece->mY += aPiece->mFallSpeed;
			}
		}
		return;
	}
	Board::UpdateFalling();
}

// slot 181 (0x2d4), 0x6dc770 (bejeweled3): while the game is on and the board is at rest (slot 178): with the top row full and no clock of a finished level running the
// warning rises by 0.0075, and at 1 the gems are given a random toss (four draws of the app's Rand() for each, drawn only) and the game ends (QuestBoard's 0x6d8e30);
// with the top row not full the drop is allowed again and the warning is 0. The goal's slot 21 (0x54) follows (a return). port: the warning's effect and the gems' glow
// (+0x264) are left out
void FillerBoard::UpdateWarnings()
{
	if (mGameOverTicks >= 1 || !IsAtRest())
		return;
	bool aTopRowFull = true;
	for (int aCol = 0; aCol < COLS; aCol++) {
		if (mGrid[0][aCol] == nullptr) {
			aTopRowFull = false;
			break;
		}
	}
	if (aTopRowFull && mQuestClock == 0) {
		mFullWarning = std::min(1.0f, (float)((double)mFullWarning + 0.0075));
		if (mFullWarning == 1.0f) {
			mFullWarning = 0.0f;
			for (int aRow = 0; aRow < ROWS; aRow++) {
				for (int aCol = 0; aCol < COLS; aCol++) {
					if (mGrid[aRow][aCol] == nullptr)
						continue;
					for (int aDraw = 0; aDraw < 4; aDraw++)
						Rand();
				}
			}
			QuestBoard::NoMoreMoves(true);
		}
	}
	else {
		mDropAllowed = true;
		mFullWarning = 0.0f;
	}
}

// slot 199 (0x6c5100), bejeweled3
bool FillerBoard::FillMadeRun() const
{
	for (int aLine = 0; aLine < ROWS; aLine++) {
		for (int aDirection = 0; aDirection < 2; aDirection++) {
			int aRunLength = 0;
			int aColor = -1;
			bool isNew = false;
			for (int aStep = 0; aStep <= ROWS; aStep++) {
				const Piece* aPiece = nullptr;
				if (aStep < ROWS)
					aPiece = aDirection == 0 ? mGrid[aLine][aStep] : mGrid[aStep][aLine];
				const int aPieceColor = aPiece != nullptr ? aPiece->mColor : -1;
				if (aPieceColor != -1 && aPieceColor == aColor) {
					aRunLength++;
					isNew = isNew || aPiece->mCreatedUpdate == mUpdatesPlayed;
					continue;
				}
				if (aRunLength >= 3 && isNew)
					return true;
				aRunLength = aPieceColor != -1 ? 1 : 0;
				aColor = aPieceColor;
				isNew = aPieceColor != -1 && aPiece->mCreatedUpdate == mUpdatesPlayed;
			}
		}
	}
	return false;
}

// slot 203 (0x34c), 0x6db550 (bejeweled3): the fill. It waits for a piece that is being worked on. The gems fall into the gaps of their columns (as Board's fill does,
// but a gem that a swap holds or whose scale curve is a pulse holds the gems above it) and the column's top row is noted (8 for none); the fill goes no further while a
// gem pulses or the grid has a match. The gems to make are as many as the credit and the empty cells allow (the credit less the cells is the input of the assist curve: at
// most 64); they carry the update they are made in. Then the colours and places are tried until an arrangement is found: each gem gets a colour from the list; its
// column is drawn from those that have room, a column with f cells free as often as ceil(f to the fourth) times, each column once before any twice (the bag); its
// speed is that of the gem at the top of the column (or the column's) less 0.55 for each row it has to fall, and it starts above the screen a little higher than the one under it.
// An arrangement is taken when it has a move (one that makes a bonus when the assist curve is above the number of tries and under 10 gems are made), no match, and no run
// that holds a new gem; the credit goes down by the gems made. After 500 tries one more gem is made when fewer were made than there are empty cells. port: the drop-in effect
// lists (the strings "b+0.25,1,0.05,..." and the curve at +0x3288) are left out; slots 182 and 183 (the goal's, false) are not asked; the extra colours (the flag at
// +0x1040 and the vector at +0x1050, empty in the modes made) are not kept; a limit of tries is added
void FillerBoard::FillInBlanks(bool theNoMatchCheck)
{
	if (mFillHeld != 0 || IsFillStopped())
		return;
	for (int aRow = mLastRow; aRow >= 0; aRow--) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			if (mGrid[aRow][aCol] != nullptr && mGrid[aRow][aCol]->mBusyCount > 0)
				return;
		}
	}
	int aMinRow[COLS];
	bool aChanged;
	do {
		aChanged = false;
		for (int aCol = 0; aCol < COLS; aCol++) {
			aMinRow[aCol] = ROWS;
			bool aGapSeen = false;
			for (int aRow = mLastRow; aRow >= 0; aRow--) {
				Piece* aPiece = mGrid[aRow][aCol];
				if (aPiece == nullptr) {
					if (aGapSeen)
						mGrid[aRow + 1][aCol] = nullptr;
					aGapSeen = true;
					continue;
				}
				if (aPiece->mRow < aMinRow[aCol])
					aMinRow[aCol] = aPiece->mRow;
				if (!aGapSeen)
					continue;
				if (!IsHeldBySwap(aPiece) && aPiece->mScaleCurve.mRamp != CurvedVal::RAMP_CURVEDATA && aPiece->mSpecialState != -1) {
					aChanged = true;
					if (aPiece->mFallSpeed == 0.0f) {
						aPiece->mFallSpeed = (float)(mColumnSpeed[aCol] + 1.0 + aPiece->mFallSpeed);
						aPiece->mMovedUpdate = mUpdatesPlayed;
					}
					aPiece->mRow++;
					mGrid[aRow][aCol] = nullptr;
					mGrid[aRow + 1][aCol] = aPiece;
				}
				else {
					mGrid[aRow + 1][aCol] = nullptr;
					aGapSeen = false;
				}
			}
			mColumnTag[aCol] = -1;
		}
	} while (aChanged);

	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			if (mGrid[aRow][aCol] != nullptr && mGrid[aRow][aCol]->mScaleCurve.mRamp == CurvedVal::RAMP_CURVEDATA)
				return;
		}
	}
	if (HasMatches())
		return;

	int anEmpty = 0;
	for (int aRow = 0; aRow < ROWS; aRow++) {
		for (int aCol = 0; aCol < COLS; aCol++) {
			if (mGrid[aRow][aCol] == nullptr)
				anEmpty++;
		}
	}
	const int aMade = std::min(mCredits, anEmpty);
	mGemsDropped += aMade;
	mAssist.SetInVal((double)std::min(mCredits - anEmpty + 64, 64), false);
	if (aMade <= 0)
		return;
	std::vector<Piece*> aNewPieces;
	for (int i = 0; i < aMade; i++) {
		Piece* aPiece = new Piece(this);
		aPiece->mCreatedUpdate = mUpdatesPlayed;
		aPiece->mMovedUpdate = mUpdatesPlayed;
		aNewPieces.push_back(aPiece);
	}
	int aTries = 0;
	for (;;) {
		int aFree[COLS];
		for (int aCol = 0; aCol < COLS; aCol++)
			aFree[aCol] = aMinRow[aCol];
		for (Piece* aPiece : aNewPieces) {
			aPiece->mFlags = 0;
			aPiece->mColor = Piece::NO_COLOR;
			aPiece->mClearable = true;
			aPiece->mCanFall = true;
			aPiece->mTimeGemSeconds = 0;
		}
		std::vector<int> aBag;
		for (Piece* aPiece : aNewPieces) {
			if (aPiece->mFlags == 0)
				aPiece->mColor = mColorList[mRand.Next((unsigned long)mColorList.size())];
			std::vector<int> aList;
			for (int aPass = 0; aPass < 2; aPass++) {
				if (aBag.empty()) {
					for (int aCol = 0; aCol < COLS; aCol++)
						aBag.push_back(aCol);
				}
				aList.clear();
				for (int aCol : aBag) {
					if (aFree[aCol] <= 0)
						continue;
					const int aWeight = (int)std::ceil(std::pow((double)aFree[aCol], (double)GRID_COLUMN_WEIGHT_POWER));
					int aCount = 0;
					do {
						aList.push_back(aCol);
						aCount++;
					} while (aCount < aWeight);
				}
				if (!aList.empty())
					break;
				if (aBag.size() == (size_t)COLS)
					break;   // every column is full
				aBag.clear();
			}
			if (aList.empty())
				continue;
			const int aCol = aList[mRand.Next((unsigned long)aList.size())];
			const int aTopRow = aMinRow[aCol];
			float aSpeed;
			int aTag;
			if (aTopRow < ROWS) {
				aSpeed = mGrid[aTopRow][aCol]->mFallSpeed;
				aTag = mGrid[aTopRow][aCol]->mColumnTag;
			}
			else {
				aSpeed = mColumnSpeed[aCol];
				aTag = mColumnTag[aCol];
			}
			const int aNewRow = aFree[aCol] - 1;
			aFree[aCol] = aNewRow;
			aPiece->mRow = aNewRow;
			aPiece->mCol = aCol;
			aPiece->mColumnTag = aTag;
			aPiece->mFallSpeed = (float)((double)aSpeed - (double)(aTopRow - aNewRow) * 0.55000001192092896);
			aPiece->mX = (float)(aCol * CELL);
			float aY = (aNewRow == ROWS - 1 || mGrid[aNewRow + 1][aCol] == nullptr) ? -256.0f : (float)((double)mGrid[aNewRow + 1][aCol]->mY - 128.0);
			aY = (float)((double)aY - (double)(int)(mRand.Next() % 20UL + 10UL));
			aPiece->mY = aY;
			mGrid[aNewRow][aCol] = aPiece;
			aBag.erase(std::find(aBag.begin(), aBag.end(), aCol));
		}
		const bool isBonusWanted = mAssist.GetOutVal() > (double)aTries && aMade < MIN_FILL_BONUS_GEMS;
		bool isOk = FindMove(nullptr, 0, false, nullptr, isBonusWanted);
		if (!theNoMatchCheck)
			isOk = isOk && !HasMatches();
		isOk = isOk && !FillMadeRun();
		if (isOk || aTries >= TRY_LIMIT)
			break;
		aTries++;
		if (aTries == TRIES_BEFORE_EXTRA_GEM && (int)aNewPieces.size() < anEmpty) {
			Piece* anExtra = new Piece(this);
			anExtra->mCreatedUpdate = mUpdatesPlayed;
			anExtra->mMovedUpdate = mUpdatesPlayed;
			aNewPieces.push_back(anExtra);
		}
		for (Piece* aPiece : aNewPieces) {
			if (aPiece->mRow < aMinRow[aPiece->mCol] && mGrid[aPiece->mRow][aPiece->mCol] == aPiece)
				mGrid[aPiece->mRow][aPiece->mCol] = nullptr;
		}
	}
	mCredits -= aMade;
	OnFillDone(aNewPieces);
}

// slots 218, 220, 115 and 26 draw the warning's bar and the credit's meter. port: the credit and the warning are written plainly beside the score
void FillerBoard::DrawModeOverlay(Graphics* g)
{
	g->SetColor(Color(255, 255, 255, 255));
	g->SetFont(FONT_SCORE);
	char aText[64];
	snprintf(aText, sizeof(aText), "Gems to drop: %d", mCredits);
	g->WriteString(aText, Scaled(250), Scaled(330), 0, 0, true, 0, -1, -1);
	snprintf(aText, sizeof(aText), "%d / %d", mLevelProgress, GetLevelMax());
	g->WriteString(aText, Scaled(250), Scaled(390), 0, 0, true, 0, -1, -1);
	if (mFullWarning > 0.0f) {
		snprintf(aText, sizeof(aText), "Full: %d%%", (int)(mFullWarning * 100.0f));
		g->WriteString(aText, Scaled(250), Scaled(450), 0, 0, true, 0, -1, -1);
	}
}

}  // namespace Sexy
