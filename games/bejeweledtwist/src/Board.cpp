// Sexy::Board (0x6a3d7b-0x6d6899): being ported, see Board.h.
#include "Board.h"

#include "Piece.h"
#include "TwistUtil.h"
#include "WinTwistApp.h"
#include "X86Math.h"

using namespace Sexy;

Board* Sexy::gBoard = nullptr;
// port: the constructor's copy comes with it; until then the table is set here
Color Sexy::gGemColors[7] = {Color(255, 32, 32), Color(255, 255, 255), Color(0, 255, 0), Color(255, 255, 0),
	Color(255, 0, 255), Color(255, 128, 0), Color(0, 128, 255)};

Board::Board()
{
	// port: the constructor (0x6c4df9) comes with the board's update
	NotPorted("Board::Board (0x6c4df9)");
}

Board::~Board()
{
	if (gBoard == this)
		gBoard = nullptr;
}

Piece* Board::GetPieceAt(int theRow, int theCol)
{
	if (theRow < 0 || mRows <= theRow || theCol < 0 || mCols <= theCol)
		return nullptr;
	return mGrid[mCols * theRow + theCol];
}

Piece* Board::GetPieceById(int theId)
{
	std::map<int, Piece*>::iterator anItr = mPieceMap.find(theId);
	if (anItr == mPieceMap.end())
		return nullptr;
	return anItr->second;
}

void Board::AddPieceId(int theId, Piece* thePiece)
{
	mPieceMap.insert(std::make_pair(theId, thePiece));
}

void Board::RemovePieceId(int theId)
{
	std::map<int, Piece*>::iterator anItr = mPieceMap.find(theId);
	if (anItr != mPieceMap.end())
		mPieceMap.erase(anItr);
}

int LevelValue::GetValue()
{
	int aLevel = gBoard->mUnknown158;
	int aValue;
	if (aLevel < mFirstLevel || mFirstLevel < 0) {
		aValue = -1;
	} else {
		aValue = mBase;
		if (mRampLevel <= aLevel)
			aValue = aValue + ((aLevel - mRampLevel) + 1) / mPeriod * mStep;
		if (0 < mPerGames)
			aValue = aValue + (int)(gApp->mUnknownCF8 / (unsigned int)mPerGames) * mStep;
		if (-1 < mMin && aValue < mMin)
			aValue = mMin;
		if (-1 < mMax && mMax < aValue)
			aValue = mMax;
	}
	return aValue;
}

Piece* Board::GetPieceAtXY(int theX, int theY)
{
	for (int aRow = 0; aRow < mRows; aRow++) {
		for (int aCol = 0; aCol < mCols; aCol++) {
			Piece* aPiece = GetPieceAt(aRow, aCol);
			if (aPiece != nullptr) {
				float aX = (float)theX;
				if (aPiece->mX <= aX && aX < (float)gGemWidth + aPiece->mX) {
					float aY = (float)theY;
					if (aPiece->mY <= aY && aY < (float)gGemHeight + aPiece->mY)
						return aPiece;
				}
			}
		}
	}
	return nullptr;
}

void Board::PieceArrived(Piece* thePiece)
{
	(void)thePiece;
	NotPorted("Board::PieceArrived (0x6c0ccf)");
}

int Board::GetPan(int theX)
{
	int aWidth = GetColX(mCols - 1) + (gGemWidth - GetColX(0));
	int aLeft = GetColX(0);
	return Ftol((double)(theX - aWidth / 2 - aLeft) / aWidth * 2 * 800.0);
}

void Board::PlayPieceSound(Piece* thePiece, int theSoundId, float thePitch, float theVolume, bool theTrack)
{
	if (!mUnknown37F8)
		mApp->PlaySoundEx(theSoundId, GetPan(Ftol(thePiece->mX)), thePitch, theVolume, theTrack, true);
}
