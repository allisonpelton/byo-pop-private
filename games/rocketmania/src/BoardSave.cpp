// The Board's saved games: the writer 0x41c830 (with the piece writer 0x41c5a0), the loader 0x41b880 (with the piece
// reader 0x41b430, 0x41b6f0 and the lookup 0x41b660/0x41b690). The file is the Buffer of the 2003 framework: every field
// is written as its raw bytes (int 4, bool 1, double 8, little endian), in the order the original's writer and loader share.
// Here one visitor (Fields) lists the plain fields once, for the writer and the loader both, so they cannot drift apart.
// A piece is written with its own address as its number (a pointer has 32 bits in every build of this port, as in the
// original's), and the special and host pieces it refers to by the same numbers; the loader matches them up again.

#include "Board.h"
#include "RocketApp.h"
#include "TalkingHead.h"
#include "Buffer.h"
#include "MTRand.h"

#include <stdint.h>

using namespace Sexy;

static_assert(sizeof(int) == 4 && sizeof(double) == 8 && sizeof(bool) == 1, "the save format has fixed field sizes");

// writes and reads a field's raw bytes
struct SaveWriter
{
	Buffer*		mBuffer;
	template<class T> void	operator()(T& theValue) { mBuffer->WriteBytes((const unsigned char*) &theValue, (int) sizeof(T)); }
	template<class T, int N> void	operator()(T (&theArray)[N]) { for (int i = 0; i < N; i++) (*this)(theArray[i]); }
};

struct SaveReader
{
	const Buffer*	mBuffer;
	template<class T> void	operator()(T& theValue) { mBuffer->ReadBytes((unsigned char*) &theValue, (int) sizeof(T)); }
	template<class T, int N> void	operator()(T (&theArray)[N]) { for (int i = 0; i < N; i++) (*this)(theArray[i]); }
};

// the number a piece is saved under (and the number that a reference to it holds)
static int PieceNumber(Piece* thePiece)
{
	return (int) (uintptr_t) thePiece;
}

// 0x41c5a0: one piece
void Board::WritePiece(Buffer* theBuffer, Piece* thePiece)
{
	SaveWriter aOut = { theBuffer };
	int aNumber = PieceNumber(thePiece);
	aOut(aNumber);
	aOut(thePiece->mX);
	aOut(thePiece->mY);
	aOut(thePiece->m18);
	aOut(thePiece->m20);
	aOut(thePiece->mMoving);
	aOut(thePiece->m30);
	aOut(thePiece->m38);
	aOut(thePiece->m40);
	aOut(thePiece->m44);
	aOut(thePiece->m48);
	aOut(thePiece->m4c);
	aOut(thePiece->mRow);
	aOut(thePiece->mColumn);
	aOut(thePiece->mFromRow);
	aOut(thePiece->mFromColumn);
	aOut(thePiece->m60);
	aOut(thePiece->m61);
	aOut(thePiece->mType);
	aOut(thePiece->mKind);
	aOut(thePiece->mRotation);
	int anAge = mUpdateCnt - thePiece->mBorn;
	aOut(anAge);
	aOut(thePiece->mPath);
	aOut(thePiece->mPathFrom);
	aOut(thePiece->m7c);
	aOut(thePiece->m80);
	aOut(thePiece->m88);
	int aSpecial = PieceNumber(thePiece->mSpecial);
	int aHost = PieceNumber(thePiece->mHost);
	aOut(aSpecial);
	aOut(aHost);
	aOut(thePiece->m94);
	aOut(thePiece->m98);
	aOut(thePiece->mIndex);
	aOut(thePiece->mIndex2);
	aOut(thePiece->ma8);
	aOut(thePiece->mac);
	aOut(thePiece->mb0);
	aOut(thePiece->mb4);
}

// 0x41b430: one piece (the references to other pieces are kept as their numbers until 0x41b690)
void Board::ReadPiece(const Buffer* theBuffer, Piece* thePiece)
{
	SaveReader aIn = { theBuffer };
	aIn(thePiece->mId);
	aIn(thePiece->mX);
	aIn(thePiece->mY);
	aIn(thePiece->m18);
	aIn(thePiece->m20);
	aIn(thePiece->mMoving);
	aIn(thePiece->m30);
	aIn(thePiece->m38);
	aIn(thePiece->m40);
	aIn(thePiece->m44);
	aIn(thePiece->m48);
	aIn(thePiece->m4c);
	aIn(thePiece->mRow);
	aIn(thePiece->mColumn);
	aIn(thePiece->mFromRow);
	aIn(thePiece->mFromColumn);
	aIn(thePiece->m60);
	aIn(thePiece->m61);
	aIn(thePiece->mType);
	aIn(thePiece->mKind);
	aIn(thePiece->mRotation);
	int anAge;
	aIn(anAge);
	thePiece->mBorn = mUpdateCnt - anAge;
	aIn(thePiece->mPath);
	aIn(thePiece->mPathFrom);
	aIn(thePiece->m7c);
	aIn(thePiece->m80);
	aIn(thePiece->m88);
	aIn(thePiece->mLoadedSpecial);
	aIn(thePiece->mLoadedHost);
	aIn(thePiece->m94);
	aIn(thePiece->m98);
	aIn(thePiece->mIndex);
	aIn(thePiece->mIndex2);
	aIn(thePiece->ma8);
	aIn(thePiece->mac);
	aIn(thePiece->mb0);
	aIn(thePiece->mb4);
}

// 0x41b6f0: reads a piece; one that was read before (the same number, a piece that stands in two of the lists) is the
// one that is returned
Piece* Board::LoadPiece(const Buffer* theBuffer)
{
	Piece* aPiece = new Piece(0);
	ReadPiece(theBuffer, aPiece);
	for (std::list<Piece*>::iterator anItr = m504.begin(); anItr != m504.end(); ++anItr)
	{
		if ((*anItr)->mId == aPiece->mId)
		{
			delete aPiece;
			return *anItr;
		}
	}
	m504.push_back(aPiece);
	return aPiece;
}

// 0x41b690: the pieces read from the save get the pieces their numbers stand for
void Board::ResolveLoadedPieces()
{
	for (std::list<Piece*>::iterator anItr = m504.begin(); anItr != m504.end(); ++anItr)
	{
		Piece* aPiece = *anItr;
		if (!aPiece->m04)
		{
			aPiece->mSpecial = FindLoadedPiece(aPiece->mLoadedSpecial);
			aPiece->mHost = FindLoadedPiece(aPiece->mLoadedHost);
			aPiece->m04 = true;
		}
	}
}

// the plain fields of the board, in the order of 0x41c830 and 0x41b880 (what comes before them is the header, the random
// numbers and the pieces of the grid; after them come the pieces that move about)
template<class Visitor>
void Board::Fields(Visitor& theVisitor)
{
	theVisitor(m250);
	theVisitor(m68);
	theVisitor(m74);
	theVisitor(m78);
	theVisitor(m7c);
	theVisitor(m80);
	theVisitor(m84);
	theVisitor(m88);
	theVisitor(m8c);
	for (int i = 0; i < 9; i++)
	{
		theVisitor(m280[i]);
		theVisitor(m2a4[i]);
		theVisitor(m2c8[i]);
		theVisitor(m2d1[i]);
	}
	theVisitor(m2dc);
	theVisitor(m2e0);
	theVisitor(m2e4);
	theVisitor(m2e8);
	theVisitor(m2ec);
	theVisitor(m2f0);
	theVisitor(m2f4);
	theVisitor(m2f8);
	theVisitor(m300);
	theVisitor(m308);
	theVisitor(m310);
	theVisitor(m318);
	theVisitor(m320);
	for (int i = 0; i < 6; i++)
	{
		theVisitor(m328[i]);
		theVisitor(m340[i]);
	}
	theVisitor(m348);
	theVisitor(m34c);
	theVisitor(m350);
	theVisitor(m358);
	theVisitor(m360);
	theVisitor(m364);
	theVisitor(m368);
	theVisitor(m36c);
	theVisitor(m370);
	theVisitor(m374);
	theVisitor(m108);
	theVisitor(m10c);
	theVisitor(m380);
	theVisitor(m378);
	theVisitor(m37c);
	for (int i = 0; i < 30; i++)
	{
		theVisitor(m384[i]);
		theVisitor(m3fc[i]);
		theVisitor(m41a[i]);
	}
	theVisitor(m438);
	theVisitor(m439);
	theVisitor(m43c);
	theVisitor(m440);
	theVisitor(m444);
	theVisitor(m448);
	theVisitor(m44c);
	theVisitor(m450);
	theVisitor(m454);
	theVisitor(m458);
	theVisitor(m45c);
	theVisitor(m460);
	theVisitor(m464);
	theVisitor(m468);
	theVisitor(m490);
	theVisitor(m494);
	theVisitor(m498);
	theVisitor(m499);
	theVisitor(m49c);
	theVisitor(m4a0);
	theVisitor(m4a4);
	theVisitor(m4a8);
	theVisitor(m4a9);
	theVisitor(m4ac);
	theVisitor(m4b0);
	theVisitor(m4b4);
	theVisitor(m4b8);
	theVisitor(m4bc);
	theVisitor(m4c0);
	theVisitor(m4c4);
	theVisitor(m4c8);
	theVisitor(m4c9);
	theVisitor(m4d0);
	theVisitor(m4d8);
	theVisitor(m4d9);
	theVisitor(m4da);
	theVisitor(mFc);
	theVisitor(m100);
	theVisitor(m104);
	theVisitor(mC4);
	theVisitor(mC8);
	theVisitor(mCc);
	theVisitor(mD0);
	theVisitor(mD4);
	theVisitor(mD8);
	theVisitor(mDc);
	theVisitor(mE0);
	theVisitor(mE4);
	theVisitor(mE8);
	theVisitor(mEc);
	theVisitor(mF0);
	theVisitor(mF4);
	theVisitor(mF8);
	// the piece generator's counters (+0x00 to +0x63)
	theVisitor(mGen->mLaunched);
	theVisitor(mGen->mCoins);
	theVisitor(mGen->mTotal);
	theVisitor(mGen->m3c);
	theVisitor(mGen->mMeter);
}

// 0x41c830: the game in progress is written to the mode's save file. The pause that comes first (0x421f30 true, then
// false) adds the time played so far to the seconds count.
bool Board::Save()
{
	Buffer aBuffer;
	F421f30(true);
	F421f30(false);
	SaveWriter aOut = { &aBuffer };
	unsigned int aMagic = 0xFADA0002;
	aOut(aMagic);
	aOut(mApp->mMode);
	aOut(mApp->mDifficulty);
	aOut(mScore);
	aOut(mLevel);
	aOut(mState);
	aBuffer.WriteString(mRand->Serialize());

	Piece** aGroups[] = { &mGrid[0][0], mTop, &mSpare, mLeft, mRight };
	int aCounts[] = { 54, 6, 1, 9, 9 };
	for (int aGroup = 0; aGroup < 5; aGroup++)
	{
		for (int i = 0; i < aCounts[aGroup]; i++)
		{
			Piece* aPiece = aGroups[aGroup][i];
			bool aPresent = aPiece != NULL;
			aOut(aPresent);
			if (aPresent)
				WritePiece(&aBuffer, aPiece);
		}
	}

	Fields(aOut);

	int aCount = (int) mObjects.size();
	aOut(aCount);
	for (std::list<Piece*>::iterator anItr = mObjects.begin(); anItr != mObjects.end(); ++anItr)
		WritePiece(&aBuffer, *anItr);

	return mApp->WriteBufferToFile(RocketApp::SavePath(mApp->mMode), &aBuffer);
}

// 0x41b880: the mode's saved game is read into this board (which was made new for it). False when the file is missing
// or is not a save.
bool Board::Load()
{
	Buffer aBuffer;
	m504.clear();
	if (!mApp->ReadBufferFromFile(RocketApp::SavePath(mApp->mMode), &aBuffer))
		return false;
	SaveReader aIn = { &aBuffer };
	unsigned int aMagic;
	aIn(aMagic);
	if (aMagic != 0xFADA0002)
		return false;
	aIn(mApp->mMode);
	aIn(mApp->mDifficulty);
	aIn(mScore);
	aIn(mLevel);
	aIn(mState);
	mRand->SRand(aBuffer.ReadString());

	Piece** aGroups[] = { &mGrid[0][0], mTop, &mSpare, mLeft, mRight };
	int aCounts[] = { 54, 6, 1, 9, 9 };
	for (int aGroup = 0; aGroup < 5; aGroup++)
	{
		for (int i = 0; i < aCounts[aGroup]; i++)
		{
			Piece* aPiece = NULL;
			bool aPresent;
			aIn(aPresent);
			if (aPresent)
				aPiece = LoadPiece(&aBuffer);
			aGroups[aGroup][i] = aPiece;
		}
	}

	Fields(aIn);
	m350 = 0;

	int aCount;
	aIn(aCount);
	// (the pieces that moved about, which the constructor's list had none of: the list is cleared first as the original's is)
	mObjects.clear();
	for (int i = 0; i < aCount; i++)
		mObjects.push_back(LoadPiece(&aBuffer));
	ResolveLoadedPieces();
	AfterLoad();
	return true;
}

// 0x41b7e0: what a loaded game was in the middle of (a screen of the end of a level, the buying of rockets) is flagged in
// mFc, m100 and m104 and put up again here: the hint of a bonus item (m104 2) goes on to the buy-rockets screen, the level-up
// screen (mFc) to the high scores after a game and to the question after a level, and the buy-rockets screen (m100) to
// the next level.
void Board::AfterLoad()
{
	m36c = 0;
	if (m104 == 2)
		ShowBuyRockets();
	if (mFc == 1)
	{
		if (mState == 2)
			mApp->ShowHighScores(0);
		else
			F4286b0();
	}
	if (m100 == 1)
		NextLevel();
	mFc = 0;
	m100 = 0;
	m104 = 0;
	if (mState != 4)
		mApp->mHead->GoToRest(true);
}
