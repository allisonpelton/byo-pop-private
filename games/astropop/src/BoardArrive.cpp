// Board: the handler of event 5 (0x473b20). A brick that was fired up the field has reached the place under the stack.
// The bricks that the player fired in one go are the groups of m7c; when the first of them arrives all of them are put
// into the field, the sets that they make (the same kind, at least m1a54 together) are taken out, and the Board looks
// at what the arrival did: the sound, the score's bonus, the stack's height, the hint boxes and a field that has no move
// left.

#include "Board.h"
#include "Brick.h"
#include "BrickKinds.h"
#include "AstroPopApp.h"
#include "GameText.h"
#include "Font.h"
#include "SmallObjects.h"
#include "Res.h"

using namespace Sexy;

// the bricks that flash as targets stop (shared by the placing of a power brick and the end of the arrival)
static void StopTargetFlashes(std::vector<Brick*>& theTargeted)
{
	for (unsigned int i = 0; i < theTargeted.size(); i++)
		theTargeted[i]->StopFlash();
	theTargeted.clear();
}

// 0x473b20 event 5: a brick has arrived (it has stopped in the field). When nothing else moves the groups that were
// waiting are put in their columns, the sets they make are found and taken, power bricks are placed, and the field is
// looked at again (targets, the stack's height, a move left).
void Board::OnBrickArrived(Brick* theBrick)
{
	std::vector<CellRef> aPlaced;       // the cells the bricks of the groups were put in, in the order of the groups
	bool aFlag = false;                 // "a set was made" (the byte [EBP+0xb] of the original, which also holds the first value of mFlag188)

	theBrick->StopShake();
	theBrick->StopFade();

	if (m8e4)
	{
		aFlag = theBrick->mFlag188;
		theBrick->SetTargetY((int) theBrick->mTargetY);
		if (!aFlag && theBrick->mFlag188 && theBrick->mSupport == NULL)
		{
			int aRow = 0;
			if ((int) theBrick->mY >= 0x22)
				aRow = ((int) theBrick->mY - 0x22) / 0x19 + 1;
			int aCol = CellColumn(theBrick->mX);
			aRow--;
			while (aRow > 0)
			{
				if (m314[aRow][aCol] != NULL)
				{
					theBrick->mSupport = NULL;
					break;
				}
				aRow--;
			}
		}
	}

	if (!m8e4 && CountMovingBricks(true) == 0)
	{
		if (m1994 > 0)
			return;

		// -- 1: a power brick in a group is put into the field and goes off at once
		std::list<std::list<Brick*> >::iterator aGroup = m7c.begin();
		while (aGroup != m7c.end())
		{
			if (aGroup->empty())
			{
				aGroup = m7c.erase(aGroup);
				continue;
			}
			int anX = (int) aGroup->front()->mX;
			int aCol = (anX - 0xa6) / 0x2c;
			int aBottom = FindBottomRow(anX, true);
			for (std::list<Brick*>::iterator aBrickIt = aGroup->begin(); aBrickIt != aGroup->end(); ++aBrickIt)
			{
				Brick* aBrick = *aBrickIt;
				int aType = aBrick->mType;
				if (aBrick->mMutated != 1 && (aType == 6 || aType == 8 || aType == 10 || aType == 0xe || aType == 0xb || aType == 0xd) &&
					aBrick->mFlag18A)
				{
					m314[aBottom + 1][aCol] = aBrick;
					FireTo(aBrick, 0);
					StopTargetFlashes(mTargeted);
					HighlightTargets();
					CheckStackHeight();
					CheckShipColumn();
					if (!m8fc)
						Fire(0x2a, 0);
					m8fc = true;
					m922 = false;
					RemoveBrick(aBrick, aCol, aBottom + 1, false);
					DropUnsupported(ma8);
					aGroup->erase(aBrickIt);
					if (aGroup->empty())
						m7c.erase(aGroup);
					return;
				}
			}
			if (aGroup->empty())
				aGroup = m7c.erase(aGroup);
			else
				++aGroup;
		}

		// -- 2: the bricks of the groups are put in the field, each group on top of its column
		for (aGroup = m7c.begin(); aGroup != m7c.end(); ++aGroup)
		{
			if (aGroup->empty())
				continue;
			int anX = (int) aGroup->front()->mX;
			int aCol = (anX - 0xa6) / 0x2c;
			int aRow = FindBottomRow(anX, true) + 1;
			for (std::list<Brick*>::iterator aBrickIt = aGroup->begin(); aBrickIt != aGroup->end(); ++aBrickIt)
			{
				m314[aRow][aCol] = *aBrickIt;
				CellRef aCell;
				aCell.mCol = aCol;
				aCell.mRow = aRow;
				aPlaced.push_back(aCell);
				FireTo(*aBrickIt, 0);
				aRow++;
			}
			for (int aRowToWobble = 0x12; aRowToWobble > 0; aRowToWobble--)
			{
				if (m314[aRowToWobble][aCol] != NULL)
					m314[aRowToWobble][aCol]->StartWobble();
			}
		}

		// -- 3: the sets: from each brick that was put down, the bricks of its kind that touch it are gathered
		aFlag = false;
		unsigned int aPlacedIndex = 0;
		for (aGroup = m7c.begin(); aGroup != m7c.end(); ++aGroup)
		{
			for (std::list<Brick*>::iterator aBrickIt = aGroup->begin(); aBrickIt != aGroup->end(); ++aBrickIt)
			{
				int aKind = (*aBrickIt)->GetEffectiveType();
				CellRef aCell = aPlaced[aPlacedIndex];
				if (!HasCell(aCell.mCol, aCell.mRow, mb8))
				{
					std::vector<CellRef> aBefore = mb8;
					mb8.clear();
					CollectConnected(aKind, aCell.mCol, aCell.mRow, 0xffffffff, false, NULL, -1, -1);
					if ((int) mb8.size() < m1a54)
						mb8 = aBefore;
					else
					{
						AstroRand();
						aFlag = true;
						AdvanceChain();
						for (unsigned int k = 0; k < mb8.size(); k++)
						{
							Brick* aBrick = m314[mb8[k].mRow][mb8[k].mCol];
							if (aBrick->mSpecial && aBrick->mMutated != 1)
								static_cast<KillAllBrick*>(aBrick)->mField238 = true;
						}
						mApp->PlaySampleAt(SOUND_EXPLODE1, mb8[0].mCol);
						for (unsigned int k = 0; k < aBefore.size(); k++)
							mb8.push_back(aBefore[k]);
					}
				}
				aPlacedIndex++;
			}
		}

		// the tutorial's first hint is dropped when it is the last but one
		if ((mField193C == 1 || m90a) && !m8dc && !aFlag)
		{
			StageBg* aBg = m220[1];
			if (aBg->mNext != NULL && aBg->mNext->mNext == NULL)
			{
				StageBg* aNext = aBg->mNext;
				delete aBg;
				m220[1] = aNext;
				m210[0] = aNext;
			}
		}

		// -- 4: the bricks of the sets leave the field; they die (event 3), one after the other
		std::vector<Brick*> aTaken;
		for (unsigned int k = 0; k < mb8.size(); k++)
		{
			aTaken.push_back(m314[mb8[k].mRow][mb8[k].mCol]);
			m314[mb8[k].mRow][mb8[k].mCol] = NULL;
		}
		int aLastKind = -1;
		for (unsigned int k = 0; k < mb8.size(); k++)
		{
			Brick* aBrick = aTaken[k];
			int aKind = aBrick->GetEffectiveType();
			ma0.push_back(aBrick);
			ma8.push_back(mb8[k]);
			FireTo(aBrick, 3);
			m1994++;
			aLastKind = aKind;
		}

		if ((mField193C == 1 || m90a) && !m8dc)
		{
			if (mb8.size() >= (unsigned int) m1a54)
			{
				StageBg* aBg = m220[1];
				if (aBg->mNext != NULL)
				{
					delete aBg->mNext;
					aBg->mNext = NULL;
				}
				aBg = m220[1];
				aBg->m118 = -8;
				aBg->m10c = -8;
				if (!m8fc)
					Fire(0x2a, 0);
				m8fc = true;
				m922 = false;
			}
		}

		// -- 5: the sound of what happened (and the bonus of a field that is nearly empty after a failed set)
		if (mb8.size() < 7)
		{
			if (mb8.size() < (unsigned int) m1a54)
			{
				if (!m7c.empty())
				{
					unsigned int aGroupIndex = (unsigned int) ((int) AstroRand() % 500000000) % (unsigned int) m7c.size();
					std::list<std::list<Brick*> >::iterator aPick = m7c.begin();
					while (aGroupIndex > 0)
					{
						aGroupIndex--;
						++aPick;
					}
					unsigned int aBrickIndex = (unsigned int) ((int) AstroRand() % 500000000) % (unsigned int) aPick->size();
					std::list<Brick*>::iterator aPickBrick = aPick->begin();
					while (aBrickIndex > 0)
					{
						aBrickIndex--;
						++aPickBrick;
					}
					if (!m911)
						mApp->PlaySampleAt(SOUND_BLOCK_HIT_BLOCK, ((int) (*aPickBrick)->mX - 0xa6) / 0x2c);
				}
				if (m1980 > 1 && mf8.empty() && m1980 > 2 && (mField193C > 1 || m1a60 == 2))
					m8ee = true;
				if (mf8.empty() && !AnyMoveLeft(NULL))
				{
					if (m19fc < 0x3d)
						m19d8 = (int) ((float) m19cc + m930 * 20.0f);
					else
						m19fc = 0x3c;
				}
			}
			else
			{
				m8fe = true;
				unsigned int anIndex = mb8.empty() ? 0 : (unsigned int) ((int) AstroRand() % 500000000) % (unsigned int) mb8.size();
				if (!HasMarkOfType(aLastKind))
					mApp->PlaySampleAt(SOUND_MADE_SET, mb8[anIndex].mCol);
				else
					mApp->PlaySampleAt(SOUND_BUBBLY, mb8[anIndex].mCol);
			}
		}
		else
		{
			m8fe = true;
			unsigned int anIndex = (unsigned int) ((int) AstroRand() % 500000000) % (unsigned int) mb8.size();
			if (!HasMarkOfType(aLastKind))
				mApp->PlaySampleAt(SOUND_MADE_BIG_SET, mb8[anIndex].mCol);
			else
				mApp->PlaySampleAt(SOUND_BUBBLY, mb8[anIndex].mCol);
		}

		// -- 6: the bricks next to the set are hit, what has lost its support falls, the groups are done
		if (mb8.size() >= (unsigned int) m1a54)
			DamageNeighbours(mb8);
		DropUnsupported(ma8);
		mb8.clear();
		m7c.clear();
		StopTargetFlashes(mTargeted);
		HighlightTargets();
		CheckStackHeight();
		CheckShipColumn();
		if (mMode == 3 && m1a34 == 1 && m1958 > 0xa5 && m1958 < 0x25e)
			HoverCell((m1958 - 0xa6) / 0x2c, CellRow((float) m195c), false);
		if (mMode == 2 && !m907 && m1a34 == 0 && m1994 == 0 && m7c.empty() && CountMovingBricks(false) == 0)
			Fn4335e0();
		CheckSparseField();

		// a field with no move left (the out flag of AnyMoveLeft is the byte that held "a set was made")
		if (m910 && mf8.empty() && !m912 && !m911 && m1b7c == 0 && m1994 == 0 && m7c.empty() && m94.empty() &&
			!AnyMoveLeft(&aFlag))
		{
			if (!aFlag)
			{
				m1b7c = 400;
				BuildBanner(GT(0x56568c), m1b7c, FONT_NEURO30);
				mApp->PlaySample(SOUND_NO_MORE_MOVES);
			}
			StartClearCount();
		}

		// the columns that are as high as the ship shake, the others stop
		for (int aCol = 0; aCol < 10; aCol++)
		{
			if (ColumnDepth(aCol) < 0x10)
			{
				Brick* aTop = m314[0][aCol];
				if (aTop != NULL && aTop->mShake)
				{
					for (int aRow = 0; aRow < 0x13; aRow++)
					{
						Brick* aBrick = m314[aRow][aCol];
						if (aBrick == NULL)
							break;
						aBrick->StopShake();
					}
				}
			}
			else
			{
				for (int aRow = 0; aRow < 0x13; aRow++)
				{
					Brick* aBrick = m314[aRow][aCol];
					if (aBrick == NULL)
						break;
					aBrick->StartShake();
				}
			}
		}

		if (mf8.empty() && !AnyMoveLeft(NULL))
		{
			if (m19fc < 0x3d)
				m19d8 = (int) ((float) m19cc + m930 * 20.0f);
			else
				m19fc = 0x3c;
		}
	}

	if (m1980 > 1 && m1980 != m1984 && m7c.empty() && m94.empty())
		m1984 = m1980;
}
