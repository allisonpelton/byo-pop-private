// BoardPuzzle.cpp: the puzzle files and the solutions of Sexy::Board.
//   FindPuzzleValidMoves 0x599d97, UpdateSolvability 0x599dfe, UpdateSolutionValidity 0x599e3c,
//   SolutionCommitMove 0x599e5b, ShowNextPuzzleMove 0x599f3c, LoadPuzzleThumb 0x59af4d, UpdateBonusPenalty 0x59b0bb,
//   LoadSolution 0x5a5d6e, LoadPuzzle 0x5a5f29.
//
// A solution (<puzzle>.sol) is a graph: each node lists the moves that are valid in that position, and each move leads to
// a node. A move is a byte: the column in bits 0 to 2, the row in bits 3 to 5 and the direction in bits 6 and 7 (0 left,
// 1 up, 2 right, 3 down); the gem swaps with its neighbour that way. A move word keeps the move in its low byte and the
// node that it leads to in the other three.

#include "Board.h"
#include "WinDMApp.h"
#include "Common.h"
#include <string.h>

using namespace Sexy;

static const int HINT_TIME = 290;

// the step of a move for each direction (dx, dy)
static const int gMoveStep[4][2] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};

// port: the move that makes the same swap seen from the other gem
static unsigned char OppositeMove(unsigned char theMove)
{
	int aDir = theMove >> 6;
	int aCol = theMove & 7;
	int aRow = theMove >> 3 & 7;
	return (unsigned char)(((aRow + gMoveStep[aDir][1]) * 8) | (aCol + gMoveStep[aDir][0]) | ((aDir ^ 2) << 6));
}

// 0x599d97: the moves (words) that are valid in the current node, up to theMax; returns how many
int Board::FindPuzzleValidMoves(unsigned int* theMoves, int theMax)
{
	if (mSolutionNode == -1)
		return 0;

	SolutionNode& aNode = mSolutionNodes.at(mSolutionNode);
	int aCount = theMax;
	if ((int)aNode.mCount <= aCount)
		aCount = aNode.mCount;
	for (int i = 0; i < aCount; i++)
		theMoves[i] = mSolutionMoveList.at(aNode.mStart + i);
	return aCount;
}

// 0x599dfe
void Board::UpdateSolvability()
{
	if (!mSolutionValid || mSolutionNode == -1)
	{
		mSolutionMoves = 0;
	}
	else
	{
		unsigned int aMoves[64];
		mSolutionMoves = FindPuzzleValidMoves(aMoves, 0x40);
	}
}

// 0x599e3c: the solution fits when the board is the one that it was made for
void Board::UpdateSolutionValidity()
{
	mSolutionValid = GetBoardHash() == mSolutionHash;
	UpdateSolvability();
}

// 0x599e5b: the player made a move: the solution follows it if it is one of the node's moves (in either direction)
void Board::SolutionCommitMove(unsigned char theMove)
{
	if (mSolutionNode == -1)
		return;

	unsigned char anOther = OppositeMove(theMove);
	SolutionNode& aNode = mSolutionNodes.at(mSolutionNode);
	int aCount = aNode.mCount;
	for (int i = 0; i < aCount; i++)
	{
		unsigned int aWord = mSolutionMoveList.at(aNode.mStart + i);
		unsigned int aMove = aWord & 0xff;
		if (theMove == aMove || anOther == aMove)
		{
			mSolutionNode = (int)(aWord >> 8);
			UpdateSolvability();
			mSolutionHintIndex = 0;
			return;
		}
	}
	mSolutionNode = -1;
	mSolutionMoves = 0;
	mSolutionHintIndex = 0;
}

// 0x599f3c: the hint of the puzzle: the next valid move, one after another (a swap that is listed both ways counts once)
void Board::ShowNextPuzzleMove()
{
	ClearPieceHints();
	mHintsUsed++;
	mHintAccepted = true;
	unsigned int aMoves[64];
	int aMoveCount = FindPuzzleValidMoves(aMoves, 0x40);
	if (aMoveCount == 0)
		return;

	char aSeen[256];
	memset(aSeen, 0, sizeof(aSeen));
	unsigned char aList[64];
	int aListCount = 0;
	for (int i = 0; i < aMoveCount; i++)
	{
		unsigned char aMove = TranslateMoveByte((unsigned char)aMoves[i]);
		if (aSeen[aMove] == 0 && aSeen[OppositeMove(aMove)] == 0)
		{
			aSeen[aMove] = 1;
			aList[aListCount++] = aMove;
		}
	}
	if (aListCount <= 0)
		return;

	int anIndex = mSolutionHintIndex++;
	unsigned char aMove = aList[anIndex % aListCount];
	int aDX = gMoveStep[aMove >> 6][0];
	int aDY = gMoveStep[aMove >> 6][1];
	int aCol = aMove & 7;
	int aRow = aMove >> 3 & 7;
	int aCol2 = aCol + aDX;
	int aRow2 = aRow + aDY;
	Piece* aFirst = mBoard[aRow][aCol];
	if (aFirst != NULL)
	{
		aFirst->mHintFrame = 0;
		aFirst->mHintTime = HINT_TIME;
		aFirst->mGemFrame = 1;
	}
	Piece* aSecond = mBoard[aRow2][aCol2];
	if (aSecond == NULL)
	{
		aFirst->mHintMoveX = aDX;
		aFirst->mHintMoveY = aDY;
	}
	else
	{
		aFirst->mHintMoveX = 0;
		aFirst->mHintMoveY = 0;
		aSecond->mHintTime = HINT_TIME;
		aSecond->mHintFrame = 0;
		aSecond->mGemFrame = 1;
		aSecond->mHintMoveX = 0;
		aSecond->mHintMoveY = 0;
	}
	mHintFromRow = aRow;
	mHintFromCol = aCol;
	mHintToCol = aCol2;
	mHintToRow = aRow2;
}

// 0x5a5d6e: reads <puzzle>.sol: the hash of the board that it fits, the first node, and the nodes with their moves
bool Board::LoadSolution(const std::string& theFileName)
{
	mSolutionFile = theFileName;
	mSolutionValid = false;
	mSolutionHash = 0;
	mSolutionNodes.clear();

	Buffer aBuffer;
	if (!mApp->ReadBufferFromFile(theFileName, &aBuffer, true))
		return false;
	if (aBuffer.ReadLong() != 0x1337b002)
		return false;
	if ((unsigned short)aBuffer.ReadShort() != 2)
		return false;

	mSolutionMoveList.resize(aBuffer.GetDataLen() / 4, 0);
	mSolutionHash = aBuffer.ReadLong();
	mSolutionFirstNode = (int)aBuffer.ReadShort();
	int aNodeCount = (unsigned short)aBuffer.ReadShort();
	mSolutionNodes.resize(aNodeCount);

	const unsigned char* aData = aBuffer.GetDataPtr();
	const unsigned char* aEnd = aData + aBuffer.GetDataLen();
	const unsigned char* aPtr = aData + aBuffer.mReadBitPos / 8;
	int aPos = 0;
	for (int aNodeNum = 0; aNodeNum < aNodeCount; aNodeNum++)
	{
		if (aPtr >= aEnd)
			return false;
		unsigned char aCount = *aPtr++;
		SolutionNode& aNode = mSolutionNodes.at(aNodeNum);
		aNode.mStart = aPos;
		aNode.mCount = aCount;
		for (int i = 0; i < aCount; i++)
		{
			if (aPtr + 4 > aEnd || aPos >= (int)mSolutionMoveList.size())
				return false;
			unsigned int aWord;
			memcpy(&aWord, aPtr, 4);
			aPtr += 4;
			mSolutionMoveList[aPos++] = aWord;
		}
	}
	mSolutionMoveList.resize(aPos, 0);
	mSolutionNode = mSolutionFirstNode;
	UpdateSolutionValidity();
	return true;
}

// port: the type of a gem in a puzzle file: version 1 swaps the numbers 7 and 8
static int PuzzleFileType(int theVersion, int theType)
{
	if (theVersion == 1)
	{
		if (theType == 7)
			return 8;
		if (theType == 8)
			return 7;
	}
	return theType;
}

// 0x5a5f29: a puzzle: the file (version 1 or 2) holds three texts (the name, the hint and one more) and the 8 by 8 gems
bool Board::LoadPuzzle(const std::string& theFileName)
{
	mPuzzleFile = theFileName;
	ClearBoard();

	Buffer aBuffer;
	if (!mApp->ReadBufferFromFile(theFileName, &aBuffer, true))
		return false;
	if (aBuffer.ReadLong() != 0x33f13bf)
		return false;
	unsigned short aVersion = (unsigned short)aBuffer.ReadShort();
	if ((short)aVersion >= 3)
		return false;

	aBuffer.ReadLong();
	std::string aName = aBuffer.ReadString();
	std::string aHint = aBuffer.ReadString();
	std::string aThird = aBuffer.ReadString();
	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			int aType = aBuffer.ReadByte();
			aType = PuzzleFileType(aVersion, aType);
			if (aType != 0xff)
			{
				uchar aFlags = aBuffer.ReadByte();
				Piece* aPiece = new Piece();
				aPiece->mType = aType;
				aPiece->mCol = aCol;
				aPiece->mRow = aRow;
				aPiece->mX = (float)GetColX(aCol);
				aPiece->mY = (float)GetRowY(aRow);
				if (aType == 8)
					aPiece->mCountdown = aBuffer.ReadByte();
				if ((aFlags & 1) != 0)
					aPiece->mFlame = true;
				mBoard[aRow][aCol] = aPiece;
			}
		}
	}
	mSolutionNode = -1;
	mFreeMode = false;
	mIsPuzzle = true;
	mIsAction = false;
	mHintAccepted = true;
	mState = STATE_IDLE;
	mPodArrowShown = false;
	mSolutionHash = 0;
	mSolutionNodes.clear();
	mHintsUsed = 0;
	mScoreSnapshot = mScore;
	ClearUndoBuffers();
	std::string aSolutionFile = GetFileDir(theFileName, false) + "\\" + GetFileName(theFileName, true) + ".sol";
	LoadSolution(aSolutionFile);
	SaveState(mUndoBuffer);
	return true;
}

// 0x59af4d: the gems of a puzzle file for the thumbnail of the pod: the type of each square (-1 for none)
bool Board::LoadPuzzleThumb(const std::string& theFileName, int (*theThumb)[8])
{
	Buffer aBuffer;
	if (!gApp->ReadBufferFromFile(theFileName, &aBuffer, true))
		return false;
	if (aBuffer.ReadLong() != 0x33f13bf)
		return false;
	unsigned short aVersion = (unsigned short)aBuffer.ReadShort();
	if ((short)aVersion >= 3)
		return false;

	aBuffer.ReadLong();
	std::string aName = aBuffer.ReadString();
	std::string aHint = aBuffer.ReadString();
	std::string aThird = aBuffer.ReadString();
	for (int aRow = 0; aRow < 8; aRow++)
	{
		for (int aCol = 0; aCol < 8; aCol++)
		{
			int aType = aBuffer.ReadByte();
			aType = PuzzleFileType(aVersion, aType);
			if (aType != 0xff)
			{
				aBuffer.ReadByte();
				theThumb[aRow][aCol] = aType;
				if (aType == 8)
					aBuffer.ReadByte();
			}
			else
			{
				theThumb[aRow][aCol] = -1;
			}
		}
	}
	return true;
}

// 0x59b0bb: the penalty of the timed games (the bonus points of the level) runs down as the level goes on; the
// original also prints the percentage to the debugger every 100 updates
void Board::UpdateBonusPenalty()
{
	double aValue = (double)mBarShown / 500.0 + 0.65;
	float aPercent = (float)aValue;
	if (1.0 <= aValue)
		aPercent = 1.0f;

	mDrain = (double)aPercent * mDrainSpeed / 100.0 + mDrain;
	mDrainSpeed = (double)aPercent * mDrainAccel / 100.0 + mDrainSpeed;
	mDrainAccel = (double)aPercent * mDrainJerk / 100.0 + mDrainAccel;
}
