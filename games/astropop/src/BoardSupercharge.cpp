// Board: the supercharged colours of the second game type (0x45ac80). The Board keeps up to two marks (mSuperchargeMarks): the
// colours of brick that are charged. The collected fuel cells of the first type and the check that runs every 300 bricks
// of the second type call UpdateSupercharge to keep a mark, change it to the colour of the brick above the cell, or pick
// a new one; each change says so in a text that floats on the field (0x454730).

#include "Board.h"
#include "GameText.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "Font.h"
#include "Image.h"
#include "Particle.h"
#include "SmallObjects.h"
#include "Res.h"
#include <utility>

using namespace Sexy;

// 0x445870: the name of a colour of brick, in capitals when asked; no name for a kind that is no colour
static std::string ColourName(int theColour, bool theCapitals)
{
	static const char* const aNames[6] = {"Red", "Green", "Blue", "Yellow", "Purple", "Teal"};
	if (theColour < 0 || theColour > 5)
		return std::string();

	std::string aName = aNames[theColour];
	if (theCapitals)
	{
		for (size_t i = 0; i < aName.size(); i++)
		{
			if (aName[i] > '`' && aName[i] < '{')
				aName[i] = (char) (aName[i] - 0x20);
		}
	}
	return aName;
}

// 0x410b80: the letters of a text go to capitals
static std::string Capitals(std::string theText)
{
	for (size_t i = 0; i < theText.size(); i++)
	{
		if (theText[i] > '`' && theText[i] < '{')
			theText[i] = (char) (theText[i] - 0x20);
	}
	return theText;
}

// 0x454730: a text in the middle of the field that floats up. It starts at y 0x15e, or lower when the text before it
// has not moved far enough; it holds still for 200 updates and does not fade.
void Board::AnnounceText(const std::string& theText)
{
	Font* aFont = GetFontById(0xb1);
	int aX = 0x182 - aFont->StringWidth(theText) / 2;
	int aY = 0x15e;

	float aLastY = 500000000.0f;
	if (!mFloatTexts.empty())
		aLastY = mFloatTexts.back()->mY;

	int aHeight = GetFontById(0xb1)->GetHeight();
	if ((int) aLastY - 0x15e <= aHeight * 2)
		aY = GetFontById(0xb1)->GetHeight() * 2 + 0x15e;

	FloatText* aText = new FloatText(theText, GetFontById(0xb1), (float) aX, (float) aY, false);
	mFloatTexts.push_back(aText);
	aText->mNoFade = true;
	aText->mAlphaDelay = 200;
}

// 0x45ac80 (the original returns the cell of the brick that was picked, which no caller reads). theCheck is the
// check of the second type; theColour is the colour of the brick above a fuel cell that was collected, -1 for none.
void Board::UpdateSupercharge(bool theCheck, int theColour)
{
	// does any colour brick wait in the field, and does one of them match a mark that has pieces to drop?
	bool aNoColourBrick = true;
	bool aMarkedOnField = false;
	for (int aCol = 0; aCol < 10; aCol++)
	{
		for (unsigned int aRow = 1; aRow < mFieldBricks.size(); aRow++)
		{
			Brick* aBrick = mFieldBricks[aRow][aCol];
			if (aBrick != NULL && aBrick->mMutated != 1 && aBrick->mType >= 0 && aBrick->mType < 6 && !aBrick->mSpecial &&
				aBrick->mState != 3)
			{
				aNoColourBrick = false;
				if ((!mSuperchargeMarks.empty() && mSuperchargeMarks[0].mType == aBrick->mType && !mSuperchargeMarks[0].mActive && mSuperchargeMarks[0].mCount > 0) ||
					(mSuperchargeMarks.size() == 2 && mSuperchargeMarks[1].mType == aBrick->mType && !mSuperchargeMarks[1].mActive && mSuperchargeMarks[1].mCount > 0))
					aMarkedOnField = true;
			}
		}
	}

	if (!theCheck)
	{
		if (mSuperchargeMarks.size() > 1)
		{
			// both marks stay charged; a colour from a cell replaces the marks that are not the check's
			std::string aText;
			for (unsigned int i = 0; i < mSuperchargeMarks.size(); i++)
			{
				mSuperchargeMarks[i].mCount = 0x5dc;
				if (theColour != -1 && !mSuperchargeMarks[i].mActive && mSuperchargeMarks[i].mType != theColour)
				{
					int anOld = -mSuperchargeMarks[i].mType;
					if (anOld == 0)
						anOld = -500000000;
					Fire(0x20, anOld);
					Fire(0x20, theColour);
					mSuperchargeMarks[i].mType = theColour;
				}

				if (aText.empty())
					aText = ColourName(mSuperchargeMarks[i].mType, false);
				else
					aText += GT(0x00565560) + ColourName(mSuperchargeMarks[i].mType, false);
			}

			if (mSuperchargeMarks[0].mType == mSuperchargeMarks[1].mType)
			{
				if (!mSuperchargeMarks[0].mActive)
					mSuperchargeMarks.erase(mSuperchargeMarks.begin());
				else
					mSuperchargeMarks.erase(mSuperchargeMarks.begin() + 1);
				aText = ColourName(mSuperchargeMarks[0].mType, false);
			}

			Fire(0x25, 0xff);
			aText += GT(0x00565548);
			aText = Capitals(aText);
			if (!m910 || !aNoColourBrick)
				AnnounceText(aText);
			return;
		}

		if (mSuperchargeMarks.size() == 1 && (!mSuperchargeMarks[0].mActive || (theColour != -1 && theColour == mSuperchargeMarks[0].mType)))
		{
			mSuperchargeMarks[0].mCount = 0x5dc;
			int anOld = mSuperchargeMarks[0].mType;
			bool aPickNew = false;
			if (theColour == -1)
			{
				if (m910 && !aMarkedOnField)
				{
					// nothing of the colour is left: the mark goes
					std::vector<MarkRec>().swap(mSuperchargeMarks);
					aPickNew = true;
				}
				else
					Fire(0x25, 0xff);
			}
			else if (anOld != theColour)
			{
				int anOldArg = -anOld;
				if (anOldArg == 0)
					anOldArg = -500000000;
				Fire(0x20, anOldArg);
				Fire(0x20, theColour);
				mSuperchargeMarks[0].mType = theColour;
			}
			else
				Fire(0x25, 0xff);

			if (!aPickNew)
			{
				if ((!m910 || !aNoColourBrick) && mSuperchargeMarks[0].mType != anOld)
					AnnounceText(ColourName(mSuperchargeMarks[0].mType, true) + GT(0x00565530));
				return;
			}
		}
	}
	else if (!mSuperchargeMarks.empty())
	{
		// the check: the marks stay charged, and when one of them is the check's own it is announced again
		std::string aText;
		for (unsigned int i = 0; i < mSuperchargeMarks.size(); i++)
		{
			mSuperchargeMarks[i].mCount = 0x5dc;
			if (aText.empty())
				aText = ColourName(mSuperchargeMarks[i].mType, true);
			else
				aText += GT(0x00565528) + ColourName(mSuperchargeMarks[i].mType, true);
		}

		Fire(0x25, 0xff);
		for (unsigned int i = 0; i < mSuperchargeMarks.size(); i++)
		{
			if (mSuperchargeMarks[i].mActive)
			{
				if (!m910 || !aNoColourBrick)
					AnnounceText(aText + GT(0x00565530));
				return;
			}
		}
	}

	// a new mark: a brick of the field gives its colour
	int aPickedCol = -1;
	int aPickedRow = -1;
	MarkRec aMark;
	aMark.mType = -1;
	aMark.mCount = -1;
	aMark.mActive = false;

	if (theColour == -1)
	{
		// a colour brick of the top two rows (the first of them that has any) whose colour is not marked yet
		std::vector<std::pair<int, int> > aCandidates;  // column, row
		for (int aRow = 1; aRow <= 2 && aCandidates.empty(); aRow++)
		{
			for (int aCol = 0; aCol < 10; aCol++)
			{
				Brick* aBrick = mFieldBricks[aRow][aCol];
				if (aBrick != NULL && aBrick->mMutated != 1 && aBrick->mType >= 0 && aBrick->mType < 6 && !aBrick->mSpecial)
				{
					int aKind = (aBrick->mMutated == 1) ? 0xc : aBrick->mType;
					if (!HasMarkOfType(aKind))
					{
						aCandidates.push_back(std::make_pair(aCol, aRow));
					}
				}
			}
		}

		if (!aCandidates.empty())
		{
			unsigned int anIndex = (unsigned int) ((int) AstroRand() % 500000000) % (unsigned int) aCandidates.size();
			aPickedCol = aCandidates[anIndex].first;
			aPickedRow = aCandidates[anIndex].second;
			Brick* aBrick = mFieldBricks[aPickedRow][aPickedCol];
			aMark.mType = (aBrick->mMutated == 1) ? 0xc : aBrick->mType;
			aMark.mActive = theCheck;
		}
		else
		{
			// no colour brick in the top two rows: any colour that is not marked (the original also writes a note to the
			// demo recording of the framework, which the port does not have)
			int aColour = ((int) AstroRand() % 500000000) % (m1a28 + 4);
			while (HasMarkOfType(aColour))
				aColour = ((int) AstroRand() % 500000000) % (m1a28 + 4);
			aPickedCol = 0;
			aPickedRow = 0;
			aMark.mType = aColour;
			aMark.mActive = theCheck;
		}
		ShowHint(-1, aPickedCol, aPickedRow, true);
	}
	else
	{
		// the first brick of that colour, column by column from the top
		for (int aCol = 0; aCol < 10 && aPickedCol == -1; aCol++)
		{
			for (unsigned int aRow = 1; aRow < mFieldBricks.size(); aRow++)
			{
				Brick* aBrick = mFieldBricks[aRow][aCol];
				if (aBrick != NULL && (!aBrick->mSpecial || aBrick->mMutated == 1))
				{
					int aKind = (aBrick->mMutated == 1) ? 0xc : aBrick->mType;
					if (aKind == theColour)
					{
						aMark.mType = theColour;
						aMark.mActive = false;
						aPickedCol = aCol;
						aPickedRow = (int) aRow;
						break;
					}
				}
			}
		}
		if (aPickedCol != -1 && aPickedRow != -1)
			ShowHint(-1, aPickedCol, aPickedRow, true);
	}

	aMark.mCount = 0x5dc;
	mSuperchargeMarks.push_back(aMark);
	Fire(0x20, aMark.mType);

	if (theCheck)
	{
		mSuperchargeBlastAnim.Reset();
		mSuperchargeBlastAnim.SetDelay(7);
		mSuperchargeBlastAnim.SetNumFrames(IMAGE_FUELCELL_DEATH->mNumCols);
		mSuperchargeBlastAnim.mHoldLast = true;
		mSuperchargeBlastAnim.SetStopped(false);
		m1b40 = 200;
	}

	std::string aText;
	for (unsigned int i = 0; i < mSuperchargeMarks.size(); i++)
	{
		mSuperchargeMarks[i].mCount = 0x5dc;
		if (aText.empty())
			aText = ColourName(mSuperchargeMarks[i].mType, false);
		else
			aText += GT(0x00565560) + ColourName(mSuperchargeMarks[i].mType, false);
	}
	aText += GT(0x00565548);
	aText = Capitals(aText);
	if (!m910 || !aNoColourBrick)
		AnnounceText(aText);
}
