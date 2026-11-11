// Board: the bricks that flash under the cursor and the bricks the piece in hand would hit

#include "Board.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "AstroPopApp.h"

using namespace Sexy;

// the brick flashes, once (it is put in the list of the flashing bricks)
void Board::FlashOnce(Brick* theBrick)
{
	if (theBrick != NULL && !IsTargeted(theBrick))
	{
		theBrick->StartFlash();
		mTargeted.push_back(theBrick);
	}
}

// 0x452750: the bricks the brick in hand (m354) or the first brick of the falling piece would hit start to flash: for
// a column nuke (0xd) the column from the pointer down, for a row nuke (0xb) the row under the pointer's, for a radial
// bomb (6) the 3 x 3 cells round it, else the bricks of the same colour that the piece would join
void Board::HighlightTargets()
{
	if (mMode == 3 && m1a34 == 1)
		return;

	int aType = -1;
	if (m354 != NULL)
		aType = m354->GetEffectiveType();
	int aRow = FindBottomRow(m198c, true);

	if (FirstPieceIsType(0xd))
	{
		if (aRow == -1)
			return;
		int aCol = (m198c - 0xa6) / 0x2c;
		for (int i = aRow; i > 0; i--)
			FlashOnce(m314[i][aCol]);
		return;
	}

	if (FirstPieceIsType(0xb))
	{
		int aTarget = aRow + 1;
		if (aTarget >= 0x13)
			aTarget = 0x12;
		for (int aCol = 0; aCol < 10; aCol++)
			FlashOnce(m314[aTarget][aCol]);
		return;
	}

	if (FirstPieceIsType(6))
	{
		int aCol = (m198c - 0xa6) / 0x2c;
		int aFirstCol = aCol - 1;
		if (aFirstCol < 0)
			aFirstCol = 0;
		int aLastCol = aCol + 1;
		if (aLastCol >= 10)
			aLastCol = aCol;
		int aFirstRow = aRow;
		if (aFirstRow <= 0)
			aFirstRow = 1;
		int aLastRow = aRow + 2;
		if (aLastRow >= 0x13)
			aLastRow = 0x12;
		for (int r = aFirstRow; r <= aLastRow; r++)
		{
			for (int c = aFirstCol; c <= aLastCol; c++)
				FlashOnce(m314[r][c]);
		}
		return;
	}

	if (aRow == -1)
		return;
	int aRoom = 5 - (int) m70.size();
	int aCol = (m198c - 0xa6) / 0x2c;
	for (int r = aRow; r > 0; r--)
	{
		if (aRoom < 0)
			return;
		Brick* aBrick = m314[r][aCol];
		if (aBrick != NULL && aBrick->IsIdle())
		{
			if (aType == -1)
				aType = aBrick->GetEffectiveType();
			else
			{
				if (aType == 6 || aType == 8 || aType == 10 || aType == 0xe || aType == 0xb || aType == 0xd)
					return;
			}
			if (!TypesMatch(aBrick, aType))
				return;
			if (IsTargeted(aBrick))
				return;
			aBrick->StartFlash();
			mTargeted.push_back(aBrick);
		}
		aRoom--;
	}
}

// 0x453630: the cell under the pointer (and the cells round it, as far as the power of the pointer goes: m1a8c 2 adds
// the cells above and below, 3 those on the sides) lights up; the cells that were lit before go dark. theForce lights
// the cell again even if it is the one that was lit.
void Board::HoverCell(int theCol, unsigned int theRow, bool theForce)
{
	if (m314.empty() || theRow >= m314.size() || theCol >= 10 || m1a34 != 1)
		return;

	if (theRow == 0)
		theRow = 1;
	int aOldCol = -1;
	unsigned int aOldRow = 0xffffffff;
	if (!m324.empty())
	{
		aOldCol = m324[0].mCol;
		aOldRow = m324[0].mRow;
	}
	if (theCol == aOldCol && theRow == aOldRow && !theForce)
		return;

	for (unsigned int i = 0; i < m324.size(); i++)
	{
		Brick* aBrick = m314[m324[i].mRow + (theForce ? 1 : 0)][m324[i].mCol];
		if (aBrick != NULL)
			aBrick->StopFlash();
	}
	m324.clear();

	int aRow = theRow;
	Brick* aBrick = m314[aRow][theCol];
	if (aBrick == NULL)
		return;
	CellRef aCell;
	if (aBrick->IsIdle())
	{
		aCell.mCol = theCol;
		aCell.mRow = aRow;
		m324.push_back(aCell);
		aBrick->StartFlash();
	}
	if (m1a8c > 1)
	{
		if (aRow - 1 > 0)
		{
			aBrick = m314[aRow - 1][theCol];
			if (aBrick != NULL && aBrick->IsIdle())
			{
				aCell.mCol = theCol;
				aCell.mRow = aRow - 1;
				m324.push_back(aCell);
				aBrick->StartFlash();
			}
		}
		if (aRow + 1 > 0 && aRow + 1 <= 0x13)
		{
			aBrick = m314[aRow + 1][theCol];
			if (aBrick != NULL && aBrick->IsIdle())
			{
				aBrick->StartFlash();
				aCell.mCol = theCol;
				aCell.mRow = aRow + 1;
				m324.push_back(aCell);
			}
		}
	}
	if (m1a8c > 2)
	{
		if (theCol - 1 >= 0)
		{
			aBrick = m314[aRow][theCol - 1];
			if (aBrick != NULL && aBrick->IsIdle())
			{
				aBrick->StartFlash();
				aCell.mCol = theCol - 1;
				aCell.mRow = aRow;
				m324.push_back(aCell);
			}
		}
		if (theCol + 1 >= 0 && theCol + 1 < 10)
		{
			aBrick = m314[aRow][theCol + 1];
			if (aBrick != NULL && aBrick->IsIdle())
			{
				aBrick->StartFlash();
				aCell.mCol = theCol + 1;
				aCell.mRow = aRow;
				m324.push_back(aCell);
			}
		}
	}
}
