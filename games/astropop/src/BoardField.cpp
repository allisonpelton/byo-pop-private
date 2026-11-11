// Board: the field of bricks (19 rows of 10 in m314) and the lists of bricks that hang off the Board: emptying them,
// filling the field from a level's layout, and the questions about the falling piece.

#include "Board.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "AstroPopApp.h"
#include "UserProfile.h"
#include "Res.h"

using namespace Sexy;

// 0x433030: do two bricks' types count as the same colour? A mutator counts as 0xc; stone (7), mutators (0xc) and
// steel (9) never match anything.
bool Board::TypesMatch(Brick* theBrick, int theType)
{
	int aType;
	if (theBrick->mMutated == 1)
		aType = 0xc;
	else
		aType = theBrick->mType;
	if (aType == theType && aType != 7 && aType != 0xc && aType != 9 && theType != 7 && theType != 0xc && theType != 9)
		return true;
	return false;
}

// 0x436b30: is the first brick of the falling piece of this type (a mutator counts as 0xc)?
bool Board::FirstPieceIsType(int theType)
{
	if (m70.empty())
		return false;
	Brick* aBrick = m70.front();
	if (aBrick->mMutated == 1)
		return theType == 0xc;
	return aBrick->mType == theType;
}

// 0x439510: does the brick flash already?
bool Board::IsTargeted(Brick* theBrick)
{
	for (unsigned int i = 0; i < mTargeted.size(); i++)
	{
		if (mTargeted[i] == theBrick)
			return true;
	}
	return false;
}

// 0x44e010: every brick the Board holds (the field, the falling piece, the groups, the lists at +0x88, +0xa0, +0x108
// and the one at +0x354) goes into the set of the bricks to delete, the lists are emptied, and the set is deleted
void Board::ClearField()
{
	for (unsigned int aRow = 0; aRow < m314.size(); aRow++)
	{
		for (int aCol = 0; aCol < 10; aCol++)
		{
			Brick* aBrick = m314[aRow][aCol];
			if (aBrick != NULL)
				m128.insert(aBrick);
			m314[aRow][aCol] = NULL;
		}
	}

	for (std::list<Brick*>::iterator i = m70.begin(); i != m70.end(); ++i)
		m128.insert(*i);
	m70.clear();

	for (std::list<Brick*>::iterator i = m88.begin(); i != m88.end(); ++i)
		m128.insert(*i);
	m88.clear();

	for (std::list<Brick*>::iterator i = ma0.begin(); i != ma0.end(); ++i)
		m128.insert(*i);
	ma0.clear();

	ma8.clear();

	for (std::list<std::list<Brick*> >::iterator i = m7c.begin(); i != m7c.end(); ++i)
	{
		for (std::list<Brick*>::iterator j = i->begin(); j != i->end(); ++j)
			m128.insert(*j);
	}
	m7c.clear();

	for (std::list<std::list<Brick*> >::iterator i = m94.begin(); i != m94.end(); ++i)
	{
		for (std::list<Brick*>::iterator j = i->begin(); j != i->end(); ++j)
			m128.insert(*j);
	}
	m94.clear();

	for (unsigned int i = 0; i < m108.size(); i++)
		m128.insert(m108[i]);
	m108.clear();

	mTargeted.clear();

	if (m354 != NULL)
		m128.insert(m354);
	m354 = NULL;

	DeleteDoomedBricks();
}

// 0x44e350: the field is cleared and filled from the layout at +0x334 (a brick type for each cell, -1 for none); a
// steel brick is cracked at once when the saved game says so (the profile's score table holds the saved field's flags)
void Board::BuildField()
{
	unsigned int aRow = 0;
	m95c = 0;
	m954 = 0;
	m8dd = false;
	m958 = 0;
	m8fc = true;
	ClearField();
	int aY = 4;
	while (aRow < m334.size())
	{
		int aCol = 0;
		int aX = 0xa6;
		do
		{
			Brick* aBrick = NULL;
			switch (m334[aRow][aCol])
			{
			case 0:
				aBrick = CreateBrick(0);
				break;
			case 1:
				aBrick = CreateBrick(1);
				break;
			case 2:
				aBrick = CreateBrick(2);
				break;
			case 3:
				aBrick = CreateBrick(3);
				break;
			case 4:
				aBrick = CreateBrick(4);
				break;
			case 5:
				aBrick = CreateBrick(5);
				break;
			case 6:
				aBrick = CreateSpecialBrick(6);
				break;
			case 7:
				aBrick = CreateSpecialBrick(7);
				break;
			case 8:
				aBrick = CreateFuelCell(500, 5);
				break;
			case 9:
			{
				SpecialBrick* aSteel = CreateSpecialBrick(9);
				aBrick = aSteel;
				if (mApp->mUser->mScores[aRow][aCol].mAux == 1)
					aSteel->Crack(false);
				break;
			}
			case 10:
				aBrick = CreatePowerBrick(10);
				break;
			case 0xb:
				aBrick = CreateSpecialBrick(0xb);
				break;
			case 0xc:
				aBrick = CreateMutator();
				if (aBrick != NULL)
					FireTo(aBrick, 0x28, 1);
				break;
			case 0xd:
				aBrick = CreatePowerBrick(0xd);
				break;
			case 0xe:
				aBrick = CreatePowerBrick(0xe);
				break;
			default:
				aBrick = NULL;
				break;
			}

			if (aBrick != NULL)
			{
				aBrick->SetParticleX((float)aX);
				int aBrickY = aY;
				if (4 < aY)
					aBrickY = aY + 5;
				aBrick->SetParticleY((float)aBrickY);
			}
			m314[aRow][aCol] = aBrick;
			aX = aX + 0x2c;
			aCol++;
		} while (aX < 0x25e);
		aRow++;
		aY = aY + 0x19;
	}
}
