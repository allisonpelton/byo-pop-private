// The board's fill: new gems for the gaps (0x6c25a4), the matches they would make and the search for a move
#include "Board.h"

#include "EffectsManager.h"
#include "Piece.h"
#include "Profile.h"
#include "Res.h"
#include "TwistDialog.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "X86Math.h"

#include <cmath>
#include <utility>

using namespace Sexy;

int Sexy::gMatchInfoCount = 0;

MatchInfo::MatchInfo()
{
	mIds.clear();
	mSerial = gMatchInfoCount++;
	Reset();
}

void MatchInfo::Reset()
{
	mCol = -1;
	mRow = -1;
	mColor = -1;
	mKind = 0;
	mCount = 0;
	mUnknown10 = true;
	mUnknown2C = 0;
	mUnknown30 = false;
	mIds.clear();
}

bool LevelValue::Roll()
{
	int aCount = GetCount();
	mValue--;
	if (mChancePow == 0.0) {
		mUnknown50 = mValue < 1;
	} else {
		double aDistPct = ((double)(aCount - mValue) / (double)aCount) * mUnknown48;
		double aChance = pow(aDistPct, mChancePow);
		int aRoll = Rand85B970() % 1000000;
		// The original also writes the roll to the debugger ("timing info expired")
		if ((double)aRoll / 1000000.0 < mUnknown40 * aChance)
			mUnknown50 = true;
	}
	return mUnknown50;
}

bool LevelValue::ShouldAppear()
{
	if (mFirstLevel <= gBoard->mUnknown158 && -1 < mFirstLevel) {
		if (mUnknown50)
			return true;
		if (!mTurnBased)
			Roll();
		return mUnknown50;
	}
	return false;
}

bool Board::CanAddCounter()
{
	return mGameMode != MODE_ZEN && mUnknown21A1 && !IsSparkBlocked() && mUnknown1508 < 2;
}

bool Board::HasChallengeFlag(int theFlag)
{
	if (mGameMode != MODE_CHALLENGE || mUnknown2630 == 0)
		return false;
	// port: the challenge at +0x2630 and its flags (+0x80) come with the challenges
	(void)theFlag;
	NotPorted("Board: the challenge's flags");
	return false;
}

void Board::MakeCounter(Piece* thePiece)
{
	thePiece->mBombCount = mCounter.GetValue();
	if (thePiece->mBombCount < 1)
		thePiece->mBombCount = 10;
	thePiece->SetFlag(Piece::FLAG_BOMB, true);
	AddStat(0x1a, 1);
}

void Board::MakeCoal(Piece* thePiece)
{
	thePiece->ClearSpecial();
	thePiece->SetFlag(Piece::FLAG_COAL, true);
	thePiece->mCanMove = false;
	thePiece->mOrigColor = thePiece->mColor;
	thePiece->mColor = 7;
}

void Board::SaveGrid()
{
	memcpy(mUnknown135C, mGrid, mCols * mRows * sizeof(Piece*));
}

void Board::RestoreGrid()
{
	memcpy(mGrid, mUnknown135C, mCols * mRows * sizeof(Piece*));
}

void Board::ClearCellHints()
{
	int aCount = mCols * mRows;
	for (int i = 0; i < aCount; i++)
		mCells[i].mUnknown34 = false;
}

bool Board::MatchAt(int theCol, int theRow, MatchInfo* theInfo)
{
	Piece* aPiece = GetPieceAt(theRow, theCol);
	if (aPiece == nullptr)
		return false;
	if (!aPiece->CanMove())
		return false;
	int aColor = aPiece->mColor;
	// Across
	int aCount = 1;
	int aFirst = theCol;
	int aLast = theCol;
	Piece* aNext;
	while (-1 < aFirst - 1 && (aNext = GetPieceAt(theRow, aFirst - 1)) != nullptr && aNext->mColor == aColor &&
		   aNext->CanMove()) {
		aCount++;
		aFirst--;
	}
	while (aLast + 1 < mCols && (aNext = GetPieceAt(theRow, aLast + 1)) != nullptr && aNext->mColor == aColor &&
		   aNext->CanMove()) {
		aCount++;
		aLast++;
	}
	if (mUnknown1C68 <= aCount) {
		if (theInfo != nullptr) {
			theInfo->mCol = theCol;
			theInfo->mRow = theRow;
			theInfo->mCount = aCount;
			for (int aCol = aFirst; aCol <= aLast; aCol++)
				theInfo->mIds.push_back(GetPieceAt(theRow, aCol)->mId);
		}
		return true;
	}
	// Down
	aCount = 1;
	aFirst = theRow;
	while (-1 < aFirst - 1 && (aNext = GetPieceAt(aFirst - 1, theCol)) != nullptr && aNext->mColor == aColor &&
		   aNext->CanMove()) {
		aCount++;
		aFirst--;
	}
	aLast = theRow;
	int aRow = theRow + 1;
	if (theRow + 1 < mRows) {
		do {
			aNext = GetPieceAt(aRow, theCol);
			if (aNext == nullptr || aNext->mColor != aColor || !aNext->CanMove())
				break;
			aCount++;
			aLast = aRow;
			aRow++;
		} while (aRow < mRows);
	}
	if (mUnknown1C68 <= aCount) {
		if (theInfo != nullptr) {
			theInfo->mCol = theCol;
			theInfo->mRow = theRow;
			theInfo->mCount = aCount;
			for (int aRow2 = aFirst; aRow2 <= aLast; aRow2++)
				theInfo->mIds.push_back(GetPieceAt(aRow2, theCol)->mId);
		}
		return true;
	}
	// The square: the four gems at its corners the colour of the one in its middle (with the profile's bit 6 and
	// +0x1c98)
	if ((mUnknown3918 >> 6 & 1) != 0 && mUnknown1C98 && theCol != 0 && theRow != 0) {
		bool aSquare = true;
		for (int aDRow = -1; aDRow < 2; aDRow += 2) {
			for (int aDCol = -1; aDCol < 2; aDCol += 2) {
				aNext = GetPieceAt(theRow + aDRow, theCol + aDCol);
				if (aNext == nullptr || aNext->mColor != aColor || !aNext->CanMove()) {
					aSquare = false;
					break;
				}
			}
		}
		if (aSquare) {
			if (theInfo == nullptr)
				return true;
			theInfo->mKind = 4;
			theInfo->mIds.push_back(aPiece->mId);
			theInfo->mCount = 5;
			for (int aDRow = -1; aDRow < 2; aDRow += 2) {
				for (int aDCol = -1; aDCol < 2; aDCol += 2)
					theInfo->mIds.push_back(GetPieceAt(theRow + aDRow, theCol + aDCol)->mId);
			}
			return true;
		}
	}
	return false;
}

bool Board::MatchAny(MatchInfo* theInfo)
{
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			if (MatchAt(aCol, aRow, theInfo))
				return true;
		}
	}
	return false;
}

bool Board::TestTwist(bool theTest, MatchInfo* theInfo)
{
	std::vector<Piece*> aPieces;
	bool aMatch = false;
	if (theTest) {
		SaveGrid();
		// The 2x2 under the cursor, found by its top left gem
		bool aFound = false;
		for (int aRow = 0; aRow < mRows - 1; aRow++) {
			for (int aCol = 0; aCol < mCols - 1; aCol++) {
				if (aFound)
					break;
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece == mUnknown1430) {
					aPieces.push_back(aPiece);
					aPieces.push_back(GetPieceAt(aRow, aCol + 1));
					aPieces.push_back(GetPieceAt(aRow + 1, aCol));
					aPieces.push_back(GetPieceAt(aRow + 1, aCol + 1));
					aFound = true;
					break;
				}
			}
		}
		// Each gem's move in the clockwise turn, as column and row
		static const int aMoves[4][2] = {{1, 0}, {0, 1}, {0, -1}, {-1, 0}};
		if (!aPieces.empty()) {
			bool aCanTurn = true;
			for (unsigned int i = 0; i < aPieces.size(); i++) {
				Piece* aPiece = aPieces[i];
				// The original reads a missing gem's flags here (a crash) when +0x2054 or +0x1c71 lets a gap turn
				if ((aPiece == nullptr && !mUnknown2054 && !mUnknown1C71) ||
					(!mUnknown1C6C && aPiece != nullptr && aPiece->IsFlagSet(Piece::FLAG_BOMB))) {
					aCanTurn = false;
					break;
				}
				if (aPiece != nullptr)
					mGrid[(aMoves[i][1] + aPiece->mRow) * mCols + aMoves[i][0] + aPiece->mCol] = aPiece;
			}
			if (aCanTurn)
				aMatch = MatchAny(theInfo);
		}
		RestoreGrid();
	}
	return aMatch;
}

bool Board::CannotTwist(bool theCheck)
{
	mUnknown2120.clear();
	if (!theCheck)
		return true;
	int aRow = GetRowAt(mUnknown1C04);
	int aCol = GetColAt(mUnknown1C00);
	if (aRow < 0 || mRows - 1 <= aRow || aCol < 0 || aCol == mCols - 1 ||
		(GetUnknown23F5() && ((-1 < mUnknown2404 && (mUnknown2404 != aRow || mUnknown2400 != aCol)) ||
							  ((mUnknown2208 != nullptr || mUnknown23DC.size() != 0) && mUnknown2208 != mUnknown2584))))
		return true;
	std::pair<int, int> aCells[4] = {std::make_pair(aRow, aCol), std::make_pair(aRow, aCol + 1),
		std::make_pair(aRow + 1, aCol), std::make_pair(aRow + 1, aCol + 1)};
	Piece* aPieces[4];
	for (int i = 0; i < 4; i++)
		aPieces[i] = GetPieceAt(aCells[i].first, aCells[i].second);
	bool aBlocked = false;
	for (int i = 0; i < 4; i++) {
		bool aCanMove;
		if (aPieces[i] == nullptr) {
			// A gap turns only with +0x2054 and +0x1c71
			aCanMove = mUnknown2054 && mUnknown1C71;
		} else {
			aCanMove = CanMove(aPieces[i]);
		}
		if (!aCanMove) {
			mUnknown2120.push_back(aCells[i]);
			aBlocked = true;
		}
	}
	return aBlocked;
}

void Board::EvaluateCursor(MatchInfo* theInfo)
{
	ClearCellHints();
	MatchInfo anInfo;
	if (theInfo == nullptr)
		theInfo = &anInfo;
	mUnknown1C40 = TestTwist(true, theInfo);
	mUnknown1C41 = CannotTwist(true);
	if (!mUnknown1559 || 2 <= mUnknown158 || !mUnknown25A4 || mUnknown21D8 || !mApp->mShowHints)
		return;
	if (mUnknown2584 != nullptr) {
		int aState = mUnknown2584->mState;
		if (aState != Tutorial::STATE_NONE && aState != Tutorial::STATE_MATCHES && aState != Tutorial::STATE_LEVELUP)
			return;
	}
	if (-1 < mUnknown2400) {
		if (mUnknown2400 != GetColAt(mUnknown1C00))
			return;
		if (mUnknown2404 != GetRowAt(mUnknown1C04))
			return;
	}
	// The gems of the match the turn would make are marked
	for (int i = 0; i < (int)theInfo->mIds.size(); i++) {
		Piece* aPiece = GetPieceById(theInfo->mIds[i]);
		if (aPiece != nullptr)
			mCells[mCols * aPiece->mRow + aPiece->mCol].mUnknown34 = true;
	}
}

void Board::MoveCursor(int theRow, int theCol, bool theQuiet, MatchInfo* theInfo)
{
	int anOldX = mUnknown1C00;
	int anOldY = mUnknown1C04;
	SetCursor(theRow, theCol);
	if ((mUnknown1C00 != anOldX || mUnknown1C04 != anOldY) && !theQuiet && mUnknown1C00 == mUnknown2400 &&
		mUnknown1C04 == mUnknown2404)
		mApp->PlaySample(SOUND_TUTORIAL_SNAP);
	Piece* aPiece = GetPieceAt(theRow, theCol);
	if (aPiece != mUnknown1430)
		mUnknown1430 = aPiece;
	EvaluateCursor(theInfo);
	if (mUnknown20D8 && !theQuiet && mUnknown20D0 && (mUnknown2584 == nullptr || !mUnknown2584->mMouseVisible)) {
		bool aSameCell = GetColAt(Ftol((double)mUnknown1C2C)) == GetColAt(mUnknown1C00) &&
						 GetRowAt(Ftol((double)mUnknown1C30)) == GetRowAt(mUnknown1C04);
		if (!aSameCell && 2 < mUpdateCnt - mUnknown1C3C) {
			// A trail of rings from where the cursor is drawn to where it now is
			int i = 0;
			do {
				i++;
				float aPos = (float)((double)i * 0.25);
				float aX = (float)((double)mUnknown1C00 * (double)aPos + (double)mUnknown1C2C * (1.0 - (double)aPos) +
								   (double)gGemWidth);
				float aY = (float)((double)mUnknown1C04 * (double)aPos + (1.0 - (double)aPos) * (double)mUnknown1C30 +
								   (double)gGemHeight);
				Effect* anEffect = mFXManager->AllocEffect(0x29);
				anEffect->mX = aX;
				anEffect->mY = aY;
				anEffect->mAlpha = (float)((double)aPos * (double)0.8f);
				anEffect->mImage = IMAGE_CURSOR_RING;
				mFXManager->AddEffect(anEffect);
			} while (i < 3);
			mUnknown1C3C = mUpdateCnt;
		}
	}
	mUnknown1C38 = 1.0f;
	mUnknown20D8 = GetColX(0) <= mUnknown1C00 && mUnknown1C00 < GetColX(mCols - 1) && GetRowY(0) <= mUnknown1C04 &&
				   mUnknown1C04 < GetRowY(mRows - 1);
	EaseCursor();
}

bool Board::FindMove(int* theRow, int* theCol, bool theAny)
{
	int aRow = 0;
	bool aFound = false;
	bool aBombFound = false;
	int aCursorX = mUnknown1C00;
	int aCursorY = mUnknown1C04;
	*theCol = -1;
	*theRow = -1;
	// (below the tutorial's dialog)
	if (mUnknown2584 != nullptr && mUnknown2584->mDialog != nullptr)
		aRow = GetRowAt(mUnknown2584->mDialog->mY + mUnknown2584->mDialog->mHeight) + 2;
	for (;;) {
		if (mRows - 1 <= aRow || aBombFound) {
			MoveCursor(GetRowAt(aCursorY), GetColAt(aCursorX), true, nullptr);
			return aFound;
		}
		int aCol = 0;
		if (mCols != 1 && -1 < mCols - 1) {
			do {
				mUnknown25E0.Reset();
				MoveCursor(aRow, aCol, true, &mUnknown25E0);
				if (!mUnknown1C41) {
					if (!mUnknown1C40) {
						// A turn that makes no match
						if (theAny) {
							*theRow = aRow;
							aFound = true;
							*theCol = aCol;
						}
					} else if (!theAny) {
						// A turn that makes a match; one with a bomb in it wins at once
						bool aBomb = false;
						for (unsigned int i = 0; i < mUnknown25E0.mIds.size(); i++) {
							Piece* aPiece = GetPieceById(mUnknown25E0.mIds[i]);
							if (aPiece != nullptr && aPiece->IsFlagSet(Piece::FLAG_BOMB))
								aBomb = true;
						}
						if (!aFound || aBomb) {
							*theRow = aRow;
							*theCol = aCol;
							aFound = true;
							mUnknown25B8.clear();
							for (unsigned int i = 0; i < mUnknown25E0.mIds.size(); i++)
								mUnknown25B8.push_back(mUnknown25E0.mIds[i]);
							if (aBomb) {
								aBombFound = true;
								break;
							}
						}
					}
				}
				aCol++;
			} while (aCol < mCols - 1);
		}
		aRow++;
	}
}

int Board::FillPieces(bool theAtOnce)
{
	int aCounterTries = 3;
	bool aTutorialGem = false;
	if (IsBusy() && mGameMode != MODE_CHALLENGE)
		aCounterTries = 0;
	int aNewCount = 0;
	int aCoalLimit = 0;
	mUnknown21A1 = mUnknown1E8 - mUnknown1EC < mAmnestyGemCount;
	int aCoalCount = 0;
	bool aHasDevil = false;
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece != nullptr) {
				if (aPiece->IsFlagSet(Piece::FLAG_COAL))
					aCoalCount++;
				if (aPiece->IsFlagSet(Piece::FLAG_DEVIL) || aPiece->IsFlagSet(Piece::FLAG_12))
					aHasDevil = true;
			}
		}
	}
	if (mCoalMax != 0)
		aCoalLimit = mUnknown1F64 + 1;
	if (HasChallengeFlag(0x100)) {
		// port: a challenge whose bombs come one at a time puts the next one on a random empty cell (mRand); it comes
		// with the challenges
	}
	int aColors = mUnknown1BF4;
	int aTry = 0;
	int aMaxTries = 30;
	bool aCounterMade = false;
	bool aCounterNow = false;
	bool aCoal = false;
	bool aNoRecolor = true;
	bool aCounters = aTutorialGem || (CanAddCounter() && 0 < aCounterTries);
	aTutorialGem = false;
	// The cells filled in this pass, as row and column
	std::vector<std::pair<int, int>> aFilled;
	bool aDone;
	bool aCounterPlaced;
	do {
		aDone = true;
		aCounterPlaced = false;
		int aNewIndex = 0;
		int aCounterIndex = 0;
		if (!aFilled.empty()) {
			// Try again: the gems of the last pass go, and the counter goes on one of the new ones
			aCounterIndex = Rand85B970() % (int)aFilled.size();
			for (unsigned int i = 0; !aFilled.empty() && i < aFilled.size(); i++) {
				int aRow = aFilled[i].first;
				int aCol = aFilled[i].second;
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece != nullptr)
					delete aPiece;
				mGrid[mCols * aRow + aCol] = nullptr;
			}
			aFilled.clear();
		}
		for (int aCol = 0; aCol < mCols; aCol++) {
			std::vector<int>& aTutorialGems = mUnknown2408[aCol];
			int aRow = mRows;
			while (--aRow > -1) {
				Piece* aPiece = GetPieceAt(aRow, aCol);
				if (aPiece == nullptr && mUnknown2108 == 0.0f) {
					if (aTry == 0) {
						if (mGameMode == MODE_CHALLENGE || mGameMode == MODE_BLITZ || (mUnknown3914 & 1) != 0)
							aCoal = aCoal | mCoalGem.ShouldAppear();
						if (aCounters && mUnknown1C6E && mCounter.ShouldAppear() && !aCounterMade) {
							aCounterMade = true;
							if ((mUnknown3914 >> 9 & 1) == 0) {
								aCounterNow = true;
								mUnknown25D0 = 2.0f;
								mUnknown2004 = true;
								aMaxTries = 100;
							}
							aCounterIndex = aNewIndex;
						}
					}
					aFilled.push_back(std::make_pair(aRow, aCol));
					aPiece = new Piece();
					aNewCount++;
					mGrid[mCols * aRow + aCol] = aPiece;
					aPiece->mX = (float)GetColX(aCol);
					if (mUnknown3924 == 0.0f)
						aPiece->mUnknown535 = false;
					// Above the gems still falling in its column, a random gap apart
					int anAbove = mRows;
					while (--anAbove > -1) {
						Piece* anOther = GetPieceAt(anAbove, aCol);
						if (anOther != nullptr && anOther != aPiece && anOther->mUnknown536) {
							int aGap = Ftol((double)gApp->mScale * (double)(Rand85B970() % 30 + 50));
							float aY;
							if ((double)anOther->mY - (double)gGemHeight - (double)aGap <= (double)aPiece->mY) {
								aGap = Ftol((double)gApp->mScale * (double)(Rand85B970() % 30 + 50));
								aY = (float)((double)anOther->mY - (double)gGemHeight - (double)aGap);
							} else {
								aY = aPiece->mY;
							}
							aPiece->mY = aY;
							if (aY < anOther->mY)
								aPiece->mUnknown34 = anOther->mUnknown34;
						}
					}
					float aTop = (float)-gGemHeight;
					if (aTop < aPiece->mY)
						aPiece->mY = aTop;
					aPiece->mCol = aCol;
					aPiece->mRow = aRow;
					if (mUnknown210C)
						aPiece->mY = (float)GetRowY(aRow);
					aPiece->mUnknown34 = (float)((double)gApp->mScale + (double)mUnknown133C[aCol]);
					int aColor = Rand85B970() % aColors;
					// (the original reads the devil gems' count here and drops it)
					bool aCounter = aCounters && mUnknown1C6E;
					bool aMakeCoal = aCoal;
					if (aCoal && aCoalLimit != 0 && aCoalLimit <= aCoalCount) {
						aMakeCoal = false;
						mCoalGem.Restart();
					}
					bool aTutorialBomb = false;
					if (GetUnknown23F5()) {
						// The tutorial's gems, set by its script, from the back of each column's list
						if (aTutorialGems.size() != 0) {
							aColor = aTutorialGems.back();
							if (aTutorialGems.size() != 0)
								aTutorialGems.pop_back();
							aMaxTries = 0;
							aTutorialGem = true;
						}
						if ((aColor & 0xf0) != 0) {
							int aKind = aColor >> 4 & 0xf;
							aColor = aColor & 0xf;
							if (aKind == 9) {
								aTutorialBomb = true;
								aCounters = true;
								aCounter = true;
							} else if (aKind == 11) {
								aPiece->SetFlag(11, true);
							}
						}
					}
					if (aCounter && (aTutorialBomb || aCounterMade) && aCounterIndex == aNewIndex) {
						MakeCounter(aPiece);
						if (GetUnknown23F5() && -1 < mUnknown2548)
							aPiece->mBombCount = mUnknown2548;
						aCounterPlaced = true;
						aMakeCoal = false;
					}
					if (aMakeCoal) {
						MakeCoal(aPiece);
						aColor = aPiece->mColor;
						mCoalGem.Restart();
						aCoal = false;
					}
					aNewIndex++;
					aPiece->mColor = aColor;
					aPiece->mOrigColor = aColor;
				}
				if (!theAtOnce) {
					mUnknown21D8 = true;
				} else if (aPiece != nullptr) {
					// (the original has no check: with +0x2108 set a gap stays and it reads through null)
					aPiece->mY = (float)GetRowY(aPiece->mRow);
				}
			}
		}
		if (HasChallengeFlag(0x800)) {
			aNoRecolor = false;
			aMaxTries = 50;
		}
		if (!aFilled.empty()) {
			if ((mCols * mRows) / 2 < (int)aFilled.size() && !aTutorialGem) {
				aNoRecolor = false;
				aMaxTries = 50;
			}
			for (unsigned int i = 0; !aFilled.empty() && i < aFilled.size(); i++) {
				Piece* aPiece = GetPieceAt(aFilled[i].first, aFilled[i].second);
				if (aPiece == nullptr)
					continue;
				if (!aPiece->IsFlagSet(Piece::FLAG_BOMB) && !aCounterNow) {
					if (!aNoRecolor && aDone && !aTutorialGem) {
						// A new gem that would match takes the next colours in turn until it does not
						int aFirstColor = aPiece->mColor;
						int aStep = 1;
						do {
							if (!MatchAt(aPiece->mCol, aPiece->mRow, nullptr)) {
								aDone = true;
								break;
							}
							int aNext = aFirstColor + aStep;
							aStep++;
							aDone = false;
							aPiece->mColor = aNext % 7;
						} while (aStep < 7);
					}
				} else if (MatchAt(aPiece->mCol, aPiece->mRow, nullptr)) {
					// A new bomb, or a counter placed this pass, must not match at once
					aDone = false;
				}
			}
		}
		int aMoveRow;
		int aMoveCol;
		if (!FindMove(&aMoveRow, &aMoveCol, false)) {
			// No move: the new gems still above the board take new colours
			for (int aRow = 0; aRow < mRows; aRow++) {
				for (int aCol = 0; aCol < mCols; aCol++) {
					Piece* aPiece = GetPieceAt(aRow, aCol);
					if (aPiece != nullptr && aPiece->mFlags == 0 && aPiece->mY <= (float)-gGemHeight)
						aPiece->mColor = Rand85B970() % aColors;
				}
			}
			if (!FindMove(&aMoveRow, &aMoveCol, false))
				aDone = false;
		}
	} while (!aDone && ++aTry < aMaxTries);
	if (HasChallengeFlag(0x800)) {
		// port: such a challenge recolours the new gems above the board that match, up to 100 times; it comes with
		// the challenges
	}
	if (aCounterPlaced)
		mCounter.Restart();
	if (HasChallengeFlag(0x1000) && !aHasDevil) {
		// port: 0x6bde87 comes with the challenges
	}
	return aNewCount;
}

int Board::FillInBlanks(bool theAtOnce)
{
	if (mGameMode == MODE_PUZZLE)
		return 0;
	mUnknown3918 = mApp->mProfile->mFlags1E0;
	mUnknown3914 = mApp->mProfile->mFlags1E4;
	int aResult = FillPieces(theAtOnce);
	if (mGameMode == MODE_CHALLENGE && mUnknown1390 && mUnknown2630 != 0) {
		// port: a challenge that rejects the board (0x6db273) clears it (0x6afe3a) and fills it again; it comes with
		// the challenges
		NotPorted("Board::FillInBlanks: the challenge's board check");
	}
	if (!mUnknown2629 && !mUnknown37C4 && mSaveBuffer.mDataBitSize == 0)
		SaveGame(&mSaveBuffer, false);
	return aResult;
}
