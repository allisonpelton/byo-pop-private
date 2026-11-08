// Board: the supercharged colours of the second game type (0x45ac80). The Board keeps up to two marks (m200): the
// colours of brick that are charged. The collected fuel cells of the first type and the check that runs every 300 bricks
// of the second type call UpdateSupercharge to keep a mark, change it to the colour of the brick above the cell, or pick
// a new one; each change says so in a text that floats on the field (0x454730).

#include "Board.h"
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
	if (!m140.empty())
		aLastY = m140.back()->mY;

	int aHeight = GetFontById(0xb1)->GetHeight();
	if ((int) aLastY - 0x15e <= aHeight * 2)
		aY = GetFontById(0xb1)->GetHeight() * 2 + 0x15e;

	FloatText* aText = new FloatText(theText, GetFontById(0xb1), (float) aX, (float) aY, false);
	m140.push_back(aText);
	aText->mNoFade = true;
	aText->m18 = 200;
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
		for (unsigned int aRow = 1; aRow < m314.size(); aRow++)
		{
			Brick* aBrick = m314[aRow][aCol];
			if (aBrick != NULL && aBrick->mMutated != 1 && aBrick->mType >= 0 && aBrick->mType < 6 && !aBrick->mSpecial &&
				aBrick->mState != 3)
			{
				aNoColourBrick = false;
				if ((!m200.empty() && m200[0].mType == aBrick->mType && !m200[0].mActive && m200[0].mCount > 0) ||
					(m200.size() == 2 && m200[1].mType == aBrick->mType && !m200[1].mActive && m200[1].mCount > 0))
					aMarkedOnField = true;
			}
		}
	}

	if (!theCheck)
	{
		if (m200.size() > 1)
		{
			// both marks stay charged; a colour from a cell replaces the marks that are not the check's
			std::string aText;
			for (unsigned int i = 0; i < m200.size(); i++)
			{
				m200[i].mCount = 0x5dc;
				if (theColour != -1 && !m200[i].mActive && m200[i].mType != theColour)
				{
					int anOld = -m200[i].mType;
					if (anOld == 0)
						anOld = -500000000;
					Fire(0x20, anOld);
					Fire(0x20, theColour);
					m200[i].mType = theColour;
				}

				if (aText.empty())
					aText = ColourName(m200[i].mType, false);
				else
					aText += " and " + ColourName(m200[i].mType, false);
			}

			if (m200[0].mType == m200[1].mType)
			{
				if (!m200[0].mActive)
					m200.erase(m200.begin());
				else
					m200.erase(m200.begin() + 1);
				aText = ColourName(m200[0].mType, false);
			}

			Fire(0x25, 0xff);
			aText += " bricks supercharged!";
			aText = Capitals(aText);
			if (!m910 || !aNoColourBrick)
				AnnounceText(aText);
			return;
		}

		if (m200.size() == 1 && (!m200[0].mActive || (theColour != -1 && theColour == m200[0].mType)))
		{
			m200[0].mCount = 0x5dc;
			int anOld = m200[0].mType;
			bool aPickNew = false;
			if (theColour == -1)
			{
				if (m910 && !aMarkedOnField)
				{
					// nothing of the colour is left: the mark goes
					std::vector<MarkRec>().swap(m200);
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
				m200[0].mType = theColour;
			}
			else
				Fire(0x25, 0xff);

			if (!aPickNew)
			{
				if ((!m910 || !aNoColourBrick) && m200[0].mType != anOld)
					AnnounceText(ColourName(m200[0].mType, true) + " BRICKS SUPERCHARGED!");
				return;
			}
		}
	}
	else if (!m200.empty())
	{
		// the check: the marks stay charged, and when one of them is the check's own it is announced again
		std::string aText;
		for (unsigned int i = 0; i < m200.size(); i++)
		{
			m200[i].mCount = 0x5dc;
			if (aText.empty())
				aText = ColourName(m200[i].mType, true);
			else
				aText += " AND " + ColourName(m200[i].mType, true);
		}

		Fire(0x25, 0xff);
		for (unsigned int i = 0; i < m200.size(); i++)
		{
			if (m200[i].mActive)
			{
				if (!m910 || !aNoColourBrick)
					AnnounceText(aText + " BRICKS SUPERCHARGED!");
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
				Brick* aBrick = m314[aRow][aCol];
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
			Brick* aBrick = m314[aPickedRow][aPickedCol];
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
			for (unsigned int aRow = 1; aRow < m314.size(); aRow++)
			{
				Brick* aBrick = m314[aRow][aCol];
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
	m200.push_back(aMark);
	Fire(0x20, aMark.mType);

	if (theCheck)
	{
		mAnim620.Reset();
		mAnim620.SetDelay(7);
		mAnim620.SetNumFrames(IMAGE_FUELCELL_DEATH->mNumCols);
		mAnim620.mHoldLast = true;
		mAnim620.SetStopped(false);
		m1b40 = 200;
	}

	std::string aText;
	for (unsigned int i = 0; i < m200.size(); i++)
	{
		m200[i].mCount = 0x5dc;
		if (aText.empty())
			aText = ColourName(m200[i].mType, false);
		else
			aText += " and " + ColourName(m200[i].mType, false);
	}
	aText += " bricks supercharged!";
	aText = Capitals(aText);
	if (!m910 || !aNoColourBrick)
		AnnounceText(aText);
}
